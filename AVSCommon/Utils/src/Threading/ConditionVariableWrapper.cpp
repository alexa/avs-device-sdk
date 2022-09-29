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

#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Threading/ConditionVariableWrapper.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace threading {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG ConditionVariableWrapper::getTag()

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::atomic<uint64_t> ConditionVariableWrapper::g_id{0};

ConditionVariableWrapper::ConditionVariableWrapper() : m_id{g_id++} {
    ACSDK_DEBUG9(LX(__func__).d("id", m_id));
    m_notifyOnePowerResourceRefCount = 0;

#ifdef ENABLE_LPM
    std::stringstream ss;
    ss << TAG << "_" << m_id;

    m_notifyOnePowerResource = power::PowerMonitor::getInstance()->createLocalPowerResource(ss.str());
#endif
}

void ConditionVariableWrapper::notifyOne() {
    ACSDK_DEBUG5(LX(__func__));

#ifdef ENABLE_LPM
    std::lock_guard<std::mutex> lock(m_mutex);
    // Only acquire if we have a guaranteed thread to awake and release the resource.
    if (m_notifyOnePowerResource && m_notifyOnePowerResourceRefCount < m_frozenResources.size()) {
        m_notifyOnePowerResource->acquire();
        m_notifyOnePowerResourceRefCount++;
    }
#endif

    m_cv.notify_one();
};

void ConditionVariableWrapper::notifyAll() {
    ACSDK_DEBUG5(LX(__func__));

#ifdef ENABLE_LPM
    /*
     * If no threads are blocked, we won't thaw any resources.
     * Therefore we won't thaw a resource whose associated thread is stays blocked.
     */
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto powerResource : m_frozenResources) {
        ACSDK_DEBUG9(LX(__func__).d("reason", "thawingResource").d("id", powerResource->getId()));
        powerResource->thaw();
    }
    m_frozenResources.clear();
#endif

    m_cv.notify_all();
};

void ConditionVariableWrapper::waitInner(std::unique_lock<std::mutex>& lock, std::function<bool()> pred) {
    ACSDK_DEBUG5(LX(__func__));
#ifdef ENABLE_LPM
    auto threadPowerResource = power::PowerMonitor::getInstance()->getThreadPowerResource();

    // PowerMonitor is not active or no associated thread PowerResource, do a standard wait.
    if (!threadPowerResource) {
        ACSDK_DEBUG9(LX(__func__).d("reason", "powerManagementNotNeeded").d("action", "rawWait"));
        m_cv.wait(lock, pred);
        return;
    }

    auto predicateWithPowerLogic = [this, pred, threadPowerResource]() {
        // Could be due to spurious wakeup or notifyOne().
        if (threadPowerResource->isFrozen()) {
            ACSDK_DEBUG9(LX("waitPredicate")
                             .d("reason", "spuriousOrNotifyOne")
                             .d("thawingResource", threadPowerResource->getId())
                             .d("threadId", std::this_thread::get_id()));
            threadPowerResource->thaw();

            std::lock_guard<std::mutex> innerLock(m_mutex);
            m_frozenResources.erase(threadPowerResource);
        }

        {
            std::lock_guard<std::mutex> innerLock(m_mutex);

            // In case we were unblocked due to notifyOne().
            if (m_notifyOnePowerResource && m_notifyOnePowerResourceRefCount > 0) {
                m_notifyOnePowerResource->release();
                m_notifyOnePowerResourceRefCount--;
            }
        }

        bool exit = pred();
        if (!exit) {
            {
                ACSDK_DEBUG9(LX("waitPredicate")
                                 .d("reason", "notExiting")
                                 .d("freezingResource", threadPowerResource->getId())
                                 .d("threadId", std::this_thread::get_id()));

                std::lock_guard<std::mutex> innerLock(m_mutex);
                m_frozenResources.insert(threadPowerResource);
            }

            /*
             * It is acceptable if system transitions to low power mode after freeze().
             * The notifying thread will need to take the system out of low power mode.
             */
            threadPowerResource->freeze();
        }

        return exit;
    };

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_frozenResources.insert(threadPowerResource);
    }

    threadPowerResource->freeze();
    /*
     * It is acceptable if system transitions to low power mode before the wait().
     * The notifying thread will need to take the system out of low power mode.
     * If pred is satisfied, then wait() will exit immediately. Otherwise, we will continue
     * waiting as per usual.
     */
    m_cv.wait(lock, predicateWithPowerLogic);
#else
    m_cv.wait(lock, pred);
#endif
}

bool ConditionVariableWrapper::waitForInner(
    std::unique_lock<std::mutex>& lock,
    std::chrono::nanoseconds relTime,
    std::function<bool()> pred) {
    ACSDK_DEBUG9(LX(__func__).d("duration", relTime.count()));

#ifdef ENABLE_LPM
    if (!power::PowerMonitor::getInstance()->getThreadPowerResource()) {
        return m_cv.wait_for(lock, relTime, pred);
    }

    timing::Timer timer;
    /*
     * The timer task may execute even after the original predicate (pred()) has returned true.
     * This is because:
     *
     * 1. The timer may expire before stop() is called but after wait(...) has returned.
     * 2. Calling Timer::stop() does not halt any currently executing task.
     *
     * Reusing the outer lock (lock) in the timer task will result in deadlock in the above scenarios.
     * This is becase wait(...) will have returned, thus lock will be locked.
     *
     * We use a separate lock to ensure that timedOut is synchronized.
     * Using an atomic is insufficient because it's possible that
     * timedOut and notifyAll() are called after the predicateOrTimedout logic is evaluated,
     * which would cause this notification to be missed.
     *
     * 1. predicateOrTimedout is called and timedOut is evaluated to be false. Before thread waits, Step 2 happens.
     * 2. Timer task fires.
     * 3. timedOut is set to true.
     * 4. notifyAll() is called. Thread in Step 1 is still running, thus no threads are blocked.
     * 5. Timer task exits.
     * 6. Thread in step 1 goes to wait, misses notification.
     */
    std::mutex timerMutex;
    bool timedOut = false;

    timer.start(relTime, [this, &timedOut, &timerMutex]() {
        ACSDK_DEBUG9(LX("timeoutTask").d("reason", "timerExpired"));
        std::lock_guard<std::mutex> timerLock(timerMutex);

        timedOut = true;

        // Wake up all threads to re-evaluate predicates.
        notifyAll();
    });

    auto predicateOrTimedout = [pred, &timedOut, &timerMutex]() {
        std::lock_guard<std::mutex> timerLock(timerMutex);
        return pred() || timedOut;
    };

    wait(lock, predicateOrTimedout);

    // If timer is still running, stop.
    if (timer.isActive()) {
        ACSDK_DEBUG9(LX("timer").d("reason", "stopBeforeExpiration"));
        timer.stop();
    }

    return pred();
#else
    return m_cv.wait_for(lock, relTime, pred);
#endif
}

}  //  namespace threading
}  //  namespace utils
}  //  namespace avsCommon
}  //  namespace alexaClientSDK
