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

#ifndef ACSDK_PROPERTIES_PRIVATE_RETRYEXECUTOR_H_
#define ACSDK_PROPERTIES_PRIVATE_RETRYEXECUTOR_H_

#include <functional>
#include <mutex>
#include <thread>

#include <acsdk/Properties/ErrorCallbackInterface.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief Tracked operation types for error callbacks.
 */
enum class OperationType {
    Open,   ///< Failed call to open properties container.
    Get,    ///< Failed call to get property.
    Put,    ///< Failed call to put property.
    Other,  ///< Failed call to other operation.
};

/**
 * @brief Operation result from a retryable operation.
 *
 * Retryable operation completes with one of three outcomes: success, failure, or retryable failure. If result is
 * retryable failure, the executor may restart the operation or fail it.
 *
 * @sa RetryExecutor
 * @sa StatusCodeWithRetry
 */
enum class RetryableOperationResult {
    Success,  ///< Operation completed with success.
    Failure,  ///< Operation has failed.
    Cleanup,  ///< Operation has failed and cleanup is requested.
};

/**
 * @brief Status code with a retry flag.
 *
 * A combination of a status code with retry flag. Retry executor uses status code to propagate to error callback, and
 * retry flag to determine if operation is actually retyable.
 *
 * @sa RetryExecutor
 * @sa RetryableOperationResult
 */
typedef std::pair<StatusCode, bool> StatusCodeWithRetry;

/**
 * @brief Helper class to execute with retries.
 *
 * This class handles operation errors and retries. Whenever operation fails, an error callback is notified, and then
 * decision is made to retry operation, fail it, or mark operation for cleanup action.
 *
 * Executor isn't aware of operation specifics, but it gets \ref OperationType and namespace URI when constructed,
 * executes given operation, and works with \ref RetryableOperationResult. For improved debugging, the classes.
 *
 * This class also provides functions to set (change) error callback interface to use whenever an operation encounters
 * an error. The number of retries can be limited, and when the retry limit is reached, the class marks operation as
 * failed even if error callback requests a retry.
 *
 * The class uses the same retry counter for all invocations, so if any of the operations fail, this reduces total
 * number of retry attempts.
 *
 * The class shall be used as follows:
 * @code
 * RetryExecutor executor(OperationType::Open, "namespaceUri");
 * auto action = execute("actionName", []() -> RetryableOperationResult {
 *    .. do something
 *    if (success) {
 *       return RetryExecutor::SUCCESS;
 *    } else {
 *       // Indicate the operation has failed, but the failure is retryable.
 *       return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
 *    }
 * }, Action::FAIL);
 *
 * // now follow up on result
 * switch (action} {
 * when Action::SUCCESS:
 *   // Handle success.
 *   ...
 *   break;
 * when Action::FAIL:
 *   // Handle failure.
 *   ...
 *   break;
 * when Action::CLEAR_DATA:
 *   // Clear data and continue.
 *   ...
 *   break;
 * @endcode
 *
 * @sa ErrorCallbackInterface
 * @sa setErrorCallback()
 *
 * @ingroup Lib_acsdkProperties
 */
class RetryExecutor {
public:
    /**
     * @brief Retryable operation.
     *
     * \ref RetryExecutor invokes retryable operation and checks the result. If the result is success, it is propagated
     * to the caller, otherwise error callback is invoked, and operation may be retried.
     *
     * sa #execute()
     */
    typedef std::function<StatusCodeWithRetry()> RetryableOperation;

    /// @brief Success result.
    static const StatusCodeWithRetry SUCCESS;
    /// @brief Retryable cryptography error.
    static const StatusCodeWithRetry RETRYABLE_CRYPTO_ERROR;
    /// @brief Non-retryable cryptography error.
    static const StatusCodeWithRetry NON_RETRYABLE_CRYPTO_ERROR;
    /// @brief Retryable HSM error.
    static const StatusCodeWithRetry RETRYABLE_HSM_ERROR;
    /// @brief Retryable inner properties interface error.
    static const StatusCodeWithRetry RETRYABLE_INNER_PROPERTIES_ERROR;
    /// @brief Non-retryable inner properties interface error.
    static const StatusCodeWithRetry NON_RETRYABLE_INNER_PROPERTIES_ERROR;

    /**
     * @brief Sets an error callback.
     *
     * This method can both set a new callback or clear existing one if \a callback is nullptr. Changing callback
     * affects error handling of Property API methods that are called after the callback is changed.
     *
     * @param[in] callback New callback reference or nullptr to remove callback.
     * @param[in] maxRetries Maximum number of retries to use with this callback. If implementation encounters more
     * errors, than number of \a maxRetries plus one, the operation fails. If \ref UNLIMITED_RETRIES value is specified,
     * the implementation executes unlimited number of retries until operation succeeds or \a callback indicates that
     * operation must stop.
     * @param[out] previous Optional pointer to store previous callback.
     *
     * @return Boolean indicating operation success. On failure, contents of *previous is undefined and false is
     * returned.
     *
     * @ingroup Lib_acsdkProperties
     */
    static bool setErrorCallback(
        const std::weak_ptr<ErrorCallbackInterface>& callback,
        uint32_t maxRetries,
        std::weak_ptr<ErrorCallbackInterface>* previous = nullptr) noexcept;

    /**
     * @brief Constructs helper object.
     *
     * This method atomically captures configured callback interface and maximum retry count, so that all retries will
     * user the same callback interface and retry limit parameters.
     *
     * @param[in] operationType Operation type.
     * @param[in] configUri Configuration URI.
     */
    RetryExecutor(OperationType operationType, const std::string& configUri);

    /**
     * @brief Execute retryable operation.
     *
     * This method executes operation until it returns OperationResult::Success, OperationResult::Failure, or there
     * are no more retry attempts left.
     *
     * If operation execution fails with error, an error callback is called. If callback returns Action::CONTINUE, then
     * value of \a continueAction is used. If operation result was retryable, and desired action is Action::RETRY,
     * executor retries operation unless there were too many execution attempts.
     *
     * This method doesn't rest retry counter, so when this method is called for the same instance, the number of
     * retries left decreases.
     *
     * @param[in] actionName Operation name for logging.
     * @param[in] operation Operation to execute. The operation must return status code, and flag, if the operation
     * may be retried.
     * @param[in] continueAction Default action to use if error callback returns Action::CONTINUE. This parameter must
     * not be Action::CONTINUE.
     *
     * @return Status code from the last attempted execution of \a operation.
     */
    RetryableOperationResult execute(
        const std::string& actionName,
        const RetryableOperation& operation,
        Action continueAction) noexcept;

private:
    /**
     * @brief Helper to invoke error callback for failed operation.
     *
     * This method increments retry counter, and then invokes error callback. If retry counter exceed maximum value,
     * the method returns Action::FAIL.
     *
     * @param[in] status Error type.
     *
     * @return Error callback invocation result or Action::DEFAULT_ACTION if error callback is not configured. If the
     * number of retries exceed the predefined limit, the method returns Action::FAIL.
     *
     * @sa ErrorCallbackInterface#onOpenPropertiesError
     */
    Action invokeErrorCallback(StatusCode status) noexcept;

    /**
     * @brief Check if action value is valid as a default one.
     *
     * The method checks if action is one of Action::FAIL, Action::RETRY, or Action::CLEAR_DATA.
     *
     * @param[in] continueAction Action value to test.
     * @return True, if \a continueAction is one of Action::FAIL, Action::RETRY, or Action::CLEAR_DATA. Returns false
     * otherwise.
     */
    bool isValidContinueAction(Action continueAction) noexcept;

    /// Mutex for controlling access to error callback reference.
    static std::mutex c_stateMutex;

    /// Maximum number of retries.
    static volatile uint32_t c_maxRetries;

    /// Error callback reference.
    static std::weak_ptr<ErrorCallbackInterface> c_callback;

    /// Operation type for selecting callback method.
    const OperationType m_operationType;

    /// Config URI for callbacks.
    const std::string m_configUri;

    /// Retry counter to prevent infinite loops.
    uint32_t m_retryCounter;

    /// Instance-specific callback reference.
    std::shared_ptr<ErrorCallbackInterface> m_callback;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_RETRYEXECUTOR_H_
