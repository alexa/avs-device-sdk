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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_CONDITIONVARIABLEWRAPPER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_CONDITIONVARIABLEWRAPPER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <unordered_set>
#include <thread>

#include <AVSCommon/Utils/Power/PowerMonitor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

/**
 * A wrapper around the standard std::condition_variable class that supports different power levels.
 * Functionally it is the same as std::condition_variable.
 *
 * Methods not containing a predicate param are omitted.
 */
class ConditionVariableWrapper {
public:
    /**
     * Returns the tag for logging purposes.
     */
    inline static std::string getTag() {
        return "ConditionVariableWrapper";
    }

    /// Constructor.
    ConditionVariableWrapper();

    /**
     * Notifies one thread waiting on the @c ConditionVariableWrapper.
     */
    void notifyOne();

    /**
     * Notifies all threads waiting on the @c ConditionVariableWrapper.
     */
    void notifyAll();

    /**
     * This function will freeze any associated thread @c PowerResource on the thread, ensuring that
     * the system may enter a lower power state upon waiting. Upon unblocking, the thread @c PowerResource
     * will be thawed.
     *
     * Refer to documentation for std::condition_variable for more information on standard behavior.
     *
     * @tparam Predicate The predicate to evaluate.
     *
     * @param lock The locked mutex.
     * @param pred The predicate to evaluate.
     */
    template <typename Predicate>
    void wait(std::unique_lock<std::mutex>& lock, Predicate pred);

    /**
     * This function will freeze any associated thread @c PowerResource on the thread, ensuring that
     * the system may enter a lower power state upon waiting. Upon unblocking, the thread @c PowerResource
     * will be thawed.
     *
     * Refer to documentation for std::condition_variable for more information on standard behavior.
     *
     * @tparam Rep The data type used in representing the duration.
     * @tparam Period The period used in representing the duration.
     * @tparam Predicate The predicate to evaluate.
     *
     * @param lock The locked mutex.
     * @param rel_time The time to wait for.
     * @param pred The predicate to evaluate.
     * @return The evaluation of pred.
     */
    template <typename Rep, typename Period, typename Predicate>
    bool waitFor(
        std::unique_lock<std::mutex>& lock,
        const std::chrono::duration<Rep, Period>& rel_time,
        Predicate pred);

    /**
     * This function will freeze any associated thread @c PowerResource on the thread, ensuring that
     * the system may enter a lower power state upon waiting. Upon unblocking, the thread @c PowerResource
     * will be thawed.
     *
     * Refer to documentation for std::condition_variable for more information on standard behavior.
     *
     * @tparam Clock The clock.
     * @tparam Duration The duration.
     * @tparam Pred The predicate to evaluate.
     *
     * @param lock The locked mutex.
     * @param timeout_time The time to wait until.
     * @param pred The predicate to evaluate.
     *
     * @return The evaluation of pred.
     */
    template <typename Clock, typename Duration, typename Predicate>
    bool waitUntil(
        std::unique_lock<std::mutex>& lock,
        const std::chrono::time_point<Clock, Duration>& timeout_time,
        Predicate pred);

private:
    /**
     * This function removes the template parameters to reduce amount of code expanded.
     *
     * @param lock The locked mutex.
     * @param pred The transformed predicate.
     */
    void waitInner(std::unique_lock<std::mutex>& lock, std::function<bool()> pred);

    /**
     * This function removes the template parameters to reduce amount of code expanded.
     *
     * @param lock The locked mutex.
     * @param relTime The time to wait for.
     * @param pred The transformed predicate.
     */
    bool waitForInner(std::unique_lock<std::mutex>& lock, std::chrono::nanoseconds relTime, std::function<bool()> pred);

    /// Identifier for instances.
    static std::atomic<uint64_t> g_id;

    /// Identifier.
    const uint64_t m_id;

    /// Underlying std::condition_variable.
    std::condition_variable m_cv;

    /**
     * This is not the same mutex associated with m_cv, the underlying std::condition_variable.
     * This is for protecting resources in this @c ConditionVariableWrapper class.
     */
    std::mutex m_mutex;

    /**
     * A @c PowerResource used to hold the @c PowerResource for notify_one() calls. This is necessary because
     * the thread that is chosen to be unblocked by a notify_one() call is indeterministic from the SDKs perspective.
     */
    std::shared_ptr<power::PowerResource> m_notifyOnePowerResource;

    /**
     * Track refCount on m_notifyOnPowerResource. This is to ensure we always have a corresponding release for each
     * acquire.
     */
    uint64_t m_notifyOnePowerResourceRefCount;

    /// Stores the set of thread resources frozen by this @c ConditionVariableWrapper.
    std::unordered_set<std::shared_ptr<power::PowerResource>> m_frozenResources;
};

template <typename Predicate>
void ConditionVariableWrapper::wait(std::unique_lock<std::mutex>& lock, Predicate pred) {
    waitInner(lock, pred);
}

template <typename Rep, typename Period, typename Predicate>
bool ConditionVariableWrapper::waitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& rel_time,
    Predicate pred) {
    return waitForInner(lock, rel_time, pred);
};

template <typename Clock, typename Duration, typename Predicate>
bool ConditionVariableWrapper::waitUntil(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& timeout_time,
    Predicate pred) {
    return waitForInner(lock, timeout_time - Clock::now(), pred);
};

}  //  namespace threading
}  //  namespace utils
}  //  namespace avsCommon
}  //  namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_THREADING_CONDITIONVARIABLEWRAPPER_H_
