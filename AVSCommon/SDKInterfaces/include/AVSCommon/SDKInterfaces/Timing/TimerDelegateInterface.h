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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TIMING_TIMERDELEGATEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TIMING_TIMERDELEGATEINTERFACE_H_

#include <chrono>
#include <functional>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace timing {

/**
 * A class which contains Timer logic that runs a task after a certain delay.
 * Implementations of this MUST be thread safe.
 */
class TimerDelegateInterface {
public:
    /// Destructor.
    virtual ~TimerDelegateInterface() = default;

    /**
     * Value for @c start()'s @c maxCount parameter which indicates that the @c TimerDelegateInterface
     * should continue firing indefinitely.
     */
    static const size_t FOREVER = 0;

    /// Static member function to get FOREVER.
    static size_t getForever() {
        return FOREVER;
    }

    /// Specifies different ways to apply the period of a recurring task.
    enum class PeriodType {
        /**
         * The period specifies the time from the start of one task call to the start of the next task call.  This
         * period type ensures task calls occur on a predictable cadence.
         *
         * @note A timer makes one task call at a time, so if a task call takes more than one period to execute,
         *     the subsequent calls which would have occured while the task was still executing will be skipped, and
         *     the next call will not occur until the next period-multiple after the original task call completes.
         */
        ABSOLUTE,

        /**
         * The period specifies the time from the end of one task call to the start of the next task call.  This period
         * type ensures a specific amount of idle time between task calls.
         */
        RELATIVE
    };

    /**
     * Waits for the @c delay, then calls @c task periodically.
     *
     * @param delay The non-negative time to wait before making the first @c task call.
     * @param period The non-negative time to wait between subsequent @c task calls.
     * @param periodType The type of period to use when making subsequent task calls.
     * @param maxCount The desired number of times to call task.
     * @c TimerDelegateInterface::getForever() means to call forever until
     *     @c stop() is called.  Note that fewer than @c maxCount calls may occur if @c periodType is
     *     @c PeriodType::ABSOLUTE and the task runtime exceeds @c period.
     * @param task A callable type representing a task.
     */
    virtual void start(
        std::chrono::nanoseconds delay,
        std::chrono::nanoseconds period,
        PeriodType periodType,
        size_t maxCount,
        std::function<void()> task) = 0;

    /**
     * Stops the @c TimerDelegateInterface (if running).  This should not interrupt an active call to the task,
     * but will prevent any subequent calls to the task.  If @c stop() is called while the task is executing,
     * this function will block until the task completes.
     *
     * @note In the special case that @c stop() is called from inside the task function, @c stop() will still
     * prevent any subsequent calls to the task, but will *not* block as described above.
     */
    virtual void stop() = 0;

    /**
     * Marks this @c TimerDelegateInterface as active for strict ordering purposes. When called, the implementation
     * must atomically set the internal state as active. Functionally this method must behave as an
     * std::atomic_exchange operation.
     *
     * @returns @c true if the @c Timer was previously inactive, else @c false.
     */
    virtual bool activate() = 0;

    /**
     * Reports whether the @c TimerDelegateInterface is active.  A timer is considered active if it is waiting
     * to start a call to the task, or if a call to the task is in progress.
     * Examples of these can be after calls to activate() or start().
     * A timer is only considered inactive if it has not been started,
     * if all requested/scheduled calls to the task have completed, or after a call to @c stop().
     *
     * @returns @c true if the @c Timer is active, else @c false.
     */
    virtual bool isActive() const = 0;
};

}  // namespace timing
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TIMING_TIMERDELEGATEINTERFACE_H_
