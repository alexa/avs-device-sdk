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

#include "SynchronizeStateSender/PostConnectSynchronizeStateSender.h"

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/RetryTimer.h>

namespace alexaClientSDK {
namespace synchronizeStateSender {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectSynchronizeStateSender");

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for the SynchronizeState event.
static const std::string SYNCHRONIZE_STATE_NAMESPACE = "System";

/// The name of the SynchronizeState event.
static const std::string SYNCHRONIZE_STATE_NAME = "SynchronizeState";

/// Table with the retry times on subsequent retries.
static const std::vector<int> RETRY_TABLE = {
    250,    // Retry 1:  0.25s
    1000,   // Retry 2:  1.00s
    3000,   // Retry 3:  3.00s
    5000,   // Retry 4:  5.00s
    10000,  // Retry 5: 10.00s
    20000,  // Retry 6: 20.00s
};

/// The instance of the @c RetryTimer used to calculate retry backoff times.
static avsCommon::utils::RetryTimer RETRY_TIMER{RETRY_TABLE};

/// Timeout value for ContextManager to return the context.
std::chrono::milliseconds CONTEXT_FETCH_TIMEOUT = std::chrono::milliseconds(2000);

std::shared_ptr<PostConnectSynchronizeStateSender> PostConnectSynchronizeStateSender::create(
    std::shared_ptr<ContextManagerInterface> contextManager) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
    } else {
        return std::shared_ptr<PostConnectSynchronizeStateSender>(
            new PostConnectSynchronizeStateSender(contextManager));
    }
    return nullptr;
}

PostConnectSynchronizeStateSender::PostConnectSynchronizeStateSender(
    std::shared_ptr<ContextManagerInterface> contextManager) :
        m_contextManager{contextManager},
        m_isStopping{false} {
}

unsigned int PostConnectSynchronizeStateSender::getOperationPriority() {
    return SYNCHRONIZE_STATE_PRIORITY;
}

void PostConnectSynchronizeStateSender::onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    ACSDK_ERROR(LX(__func__).d("reason", error));
    m_wakeTrigger.notify_all();
}

void PostConnectSynchronizeStateSender::onContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG5(LX(__func__));
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_contextString = jsonContext;
    }
    m_wakeTrigger.notify_all();
}

bool PostConnectSynchronizeStateSender::fetchContext() {
    ACSDK_DEBUG5(LX(__func__));
    std::unique_lock<std::mutex> lock{m_mutex};
    m_contextString = "";
    m_contextManager->getContext(shared_from_this());

    auto pred = [this] { return !m_contextString.empty() || m_isStopping; };

    if (!m_wakeTrigger.wait_for(lock, CONTEXT_FETCH_TIMEOUT, pred)) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "context fetch timeout"));
        return false;
    }

    if (m_contextString.empty()) {
        ACSDK_ERROR(LX(__func__).m("invalid context received."));
        return false;
    }

    if (m_isStopping) {
        ACSDK_DEBUG5(LX(__func__).m("Stopped while context fetch in progress"));
        return false;
    }

    return true;
}

bool PostConnectSynchronizeStateSender::performOperation(const std::shared_ptr<MessageSenderInterface>& messageSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!messageSender) {
        ACSDK_ERROR(LX("performOperationFailed").d("reason", "nullPostConnectSender"));
        return false;
    }

    int retryAttempt = 0;
    while (!isStopping()) {
        if (fetchContext()) {
            /// Context fetch successful, proceed to send SynchronizeState event.
            std::unique_lock<std::mutex> lock{m_mutex};
            if (m_isStopping) {
                return false;
            }

            auto event =
                buildJsonEventString(SYNCHRONIZE_STATE_NAMESPACE, SYNCHRONIZE_STATE_NAME, "", "{}", m_contextString);
            m_postConnectRequest = std::make_shared<WaitableMessageRequest>(event.second);
            lock.unlock();

            messageSender->sendMessage(m_postConnectRequest);

            auto status = m_postConnectRequest->waitForCompletion();
            ACSDK_DEBUG5(LX(__func__).d("SynchronizeState event status", status));

            if (status == MessageRequestObserverInterface::Status::SUCCESS ||
                status == MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT) {
                return true;
            } else if (status == MessageRequestObserverInterface::Status::CANCELED) {
                return false;
            }
        }

        /// Retry with backoff.
        std::unique_lock<std::mutex> lock{m_mutex};
        auto timeout = RETRY_TIMER.calculateTimeToRetry(retryAttempt++);
        if (m_wakeTrigger.wait_for(lock, timeout, [this] { return m_isStopping; })) {
            return false;
        }
    }

    return false;
}

bool PostConnectSynchronizeStateSender::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

void PostConnectSynchronizeStateSender::abortOperation() {
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<WaitableMessageRequest> requestCopy;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isStopping) {
            /// Already stopping, return.
            return;
        }
        m_isStopping = true;
        requestCopy = m_postConnectRequest;
    }

    /// Call shutdown outside the lock.
    if (requestCopy) {
        requestCopy->shutdown();
    }

    m_wakeTrigger.notify_all();
}

}  // namespace synchronizeStateSender
}  // namespace alexaClientSDK
