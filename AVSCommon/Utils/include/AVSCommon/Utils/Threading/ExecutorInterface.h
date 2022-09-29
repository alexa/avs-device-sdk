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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTORINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTORINTERFACE_H_

#include <functional>
#include <system_error>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * @brief Interface for asynchronous execution of functions.
 *
 * This interface enables submission of functions for asynchronous execution. The implementations should use thread pool
 * to acquire threads for running functions, and may be single- or multi-threaded.
 *
 * Executors can be in normal mode, when they accept tasks, and in shutdown mode. In shutdown mode executors do not
 * accept new tasks for processing, and any tasks, that haven't started an execution will be dropped.
 *
 * @code
 * auto error = executor->execute([]{ ... });
 * if (error) {
 *     ACSDK_ERROR(LX(__func__).("executorError", error.message()));
 * }
 * @endcode
 *
 * @see Executor
 * @see ThreadPool
 */
class ExecutorInterface {
public:
    /**
     * @brief Destructs an Executor.
     *
     * This method awaits till all running tasks are completed, and drops all enqueued tasks that haven't started
     * execution.
     */
    virtual ~ExecutorInterface() noexcept = default;

    /**
     * @{
     * @brief Schedules a function for execution.
     *
     * Submits a function to be executed on an executor thread.
     *
     * @param[in] function Function to execute. Function must not be empty.
     *
     * @return Platform-independent error code. If successful, the error value is zero.
     * @retval std::errc::invalid_argument If @a function is empty.
     * @retval std::errc::operation_not_permitted If executor is shutdown.
     */
    virtual std::error_condition execute(std::function<void()>&& function) noexcept = 0;
    virtual std::error_condition execute(const std::function<void()>& function) noexcept = 0;
    /// @}
};

}  // namespace threading
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_EXECUTORINTERFACE_H_
