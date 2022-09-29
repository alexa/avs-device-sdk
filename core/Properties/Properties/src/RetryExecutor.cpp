/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <acsdk/Properties/ErrorCallbackSetter.h>
#include <acsdk/Properties/private/RetryExecutor.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

/// String to identify log entries originating from this file.
/// @private
#define TAG "RetryExecutor"

const StatusCodeWithRetry RetryExecutor::SUCCESS{StatusCode::SUCCESS, false};
const StatusCodeWithRetry RetryExecutor::RETRYABLE_CRYPTO_ERROR{StatusCode::CRYPTO_ERROR, true};
const StatusCodeWithRetry RetryExecutor::NON_RETRYABLE_CRYPTO_ERROR{StatusCode::CRYPTO_ERROR, false};
const StatusCodeWithRetry RetryExecutor::RETRYABLE_HSM_ERROR{StatusCode::HSM_ERROR, true};
const StatusCodeWithRetry RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR{StatusCode::INNER_PROPERTIES_ERROR, true};
const StatusCodeWithRetry RetryExecutor::NON_RETRYABLE_INNER_PROPERTIES_ERROR{StatusCode::INNER_PROPERTIES_ERROR,
                                                                              false};

std::mutex RetryExecutor::c_stateMutex;
volatile uint32_t RetryExecutor::c_maxRetries{DEFAULT_MAX_RETRIES};
std::weak_ptr<ErrorCallbackInterface> RetryExecutor::c_callback;

bool RetryExecutor::setErrorCallback(
    const std::weak_ptr<ErrorCallbackInterface>& callback,
    uint32_t maxRetries,
    std::weak_ptr<ErrorCallbackInterface>* previous) noexcept {
    auto tmp = callback;

    {
        std::unique_lock<std::mutex> lock(c_stateMutex);
        c_maxRetries = maxRetries;
        c_callback.swap(tmp);
    }
    if (previous) {
        previous->swap(tmp);
    }

    ACSDK_DEBUG0(LX("setErrorCallbackSuccess"));
    return true;
}

RetryExecutor::RetryExecutor(OperationType operationType, const std::string& configUri) :
        m_operationType{operationType},
        m_configUri{configUri},
        m_retryCounter{0} {
    std::lock_guard<std::mutex> lock(c_stateMutex);
    m_callback = c_callback.lock();
    m_retryCounter = c_maxRetries;
}

RetryableOperationResult RetryExecutor::execute(
    const std::string& actionName,
    const RetryableOperation& operation,
    Action continueAction) noexcept {
    if (!isValidContinueAction(continueAction)) {
        ACSDK_ERROR(LX("executeFailed").d("action", actionName).m("continueActionInvalid"));
        return RetryableOperationResult::Failure;
    }

    for (;;) {
        const auto result = operation();
        if (StatusCode::SUCCESS == result.first) {
            // Upon successful result, forward the result to caller.
            ACSDK_DEBUG9(LX("executeSuccess").d("action", actionName).d("retriesLeft", m_retryCounter));
            return RetryableOperationResult::Success;
        } else if (!result.second) {
            // Upon unsuccessful non-retryable result, invoke the callback, and forward the result to caller.
            invokeErrorCallback(result.first);
            ACSDK_DEBUG9(LX("executeFailed").d("action", actionName).d("retriesLeft", m_retryCounter));
            return RetryableOperationResult::Failure;
        } else {
            // Check if we can retry
            auto action = invokeErrorCallback(result.first);
            if (Action::CONTINUE == action) {
                action = continueAction;
            }
            switch (action) {
                case Action::CLEAR_DATA:
                    ACSDK_DEBUG9(LX("executeCleanup").d("action", actionName).d("retriesLeft", m_retryCounter));
                    return RetryableOperationResult::Cleanup;
                case Action::RETRY:
                    ACSDK_DEBUG9(LX("executeRetry").d("action", actionName).d("retriesLeft", m_retryCounter));
                    continue;
                case Action::FAIL:  // fall through
                default:
                    ACSDK_DEBUG9(LX("executeFailed").d("action", actionName).d("retriesLeft", m_retryCounter));
                    return RetryableOperationResult::Failure;
            }
        }
    }
}

Action RetryExecutor::invokeErrorCallback(StatusCode status) noexcept {
    Action result = Action::CONTINUE;

    if (m_callback) {
        switch (m_operationType) {
            case OperationType::Open:
                ACSDK_DEBUG0(LX("invokeErrorCallbackOnOpen").d("configUri", m_configUri));
                result = m_callback->onOpenPropertiesError(status, m_configUri);
                break;
            case OperationType::Get:
                ACSDK_DEBUG0(LX("invokeErrorCallbackOnGet").d("configUri", m_configUri));
                result = m_callback->onGetPropertyError(status, m_configUri);
                break;
            case OperationType::Put:
                ACSDK_DEBUG0(LX("invokeErrorCallbackOnPut").d("configUri", m_configUri));
                result = m_callback->onPutPropertyError(status, m_configUri);
                break;
            case OperationType::Other:  // fall through
            default:
                ACSDK_WARN(LX("invokeErrorCallbackFailed").m("otherOperation"));
                result = Action::FAIL;
                break;
        }
    }
    ACSDK_DEBUG9(LX("invokeErrorCallback").d("result", static_cast<int>(result)));

    if (Action::RETRY == result) {
        if (!m_retryCounter) {
            ACSDK_ERROR(LX("invokeErrorCallbackFailed").d("configUri", m_configUri).m("tooManyRetries"));
            return Action::FAIL;
        }
        if (m_retryCounter != UNLIMITED_RETRIES) {
            m_retryCounter--;
        }
    }

    return result;
}

bool RetryExecutor::isValidContinueAction(Action continueAction) noexcept {
    switch (continueAction) {
        case Action::FAIL:
        case Action::RETRY:
        case Action::CLEAR_DATA:
            return true;
        default:
            return false;
    }
}

}  // namespace properties
}  // namespace alexaClientSDK
