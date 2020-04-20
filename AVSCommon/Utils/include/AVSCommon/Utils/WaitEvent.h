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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_WAITEVENT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_WAITEVENT_H_

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Class that can be used to wait for an event.
 *
 * @note After the first @c wakeUp call, any call to @c wait will no longer block. Use @c reset() to reset the internal
 * state and allow wait to block again.
 */
class WaitEvent {
public:
    /**
     * Constructor.
     */
    WaitEvent();

    /**
     * Notify all threads that are waiting for this event.
     *
     * This method will also set the @c m_wakeUpTriggered to @c true to ensure that the waiting thread will not wait
     * forever in case there is a race condition and @c wakeUp() is triggered before the @c wait().
     */
    void wakeUp();

    /**
     * Wait for wake up event.
     *
     * @param timeout The maximum amount of time to wait for the event.
     * @return @c true if wakeUp has been called within the timeout; @c false otherwise.
     */
    bool wait(const std::chrono::milliseconds& timeout);

    /**
     * Reset the event occurrence flag.
     */
    void reset();

private:
    /// The condition variable used to wake up the thread that is waiting.
    std::condition_variable m_condition;

    /// The mutex used to lock the condition.
    std::mutex m_mutex;

    /// The boolean condition to check if wakeUp has been called or not.
    bool m_wakeUpTriggered;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_WAITEVENT_H_
