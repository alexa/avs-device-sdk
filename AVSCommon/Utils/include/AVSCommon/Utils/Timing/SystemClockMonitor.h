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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_SYSTEMCLOCKMONITOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_SYSTEMCLOCKMONITOR_H_

#include <memory>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/SystemClockMonitorObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * This class monitors the system clock. When the system clock is synchronized, this class notifies
 * its observers of the synchronization.
 */
class SystemClockMonitor {
public:
    /**
     * Should be called when the device clock has synchronized (ex. ntp time sync)
     */
    void notifySystemClockSynchronized();

    /**
     * Add an observer to the system clock monitor
     * @param observer The observer to add to our set
     */
    void addSystemClockMonitorObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::SystemClockMonitorObserverInterface>& observer);

    /**
     * Remove an observer to the system clock monitor
     * @param observer The observer to remove from our set
     */
    void removeSystemClockMonitorObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::SystemClockMonitorObserverInterface>& observer);

private:
    /**
     * Notify observers that device clock has synchronized
     */
    void notifyObservers();

    /// Mutex for system clock observers.
    std::mutex m_systemClockObserverMutex;

    // The set of SystemClockMonitor observers
    std::unordered_set<std::shared_ptr<sdkInterfaces::SystemClockMonitorObserverInterface>> m_observers;
};

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_SYSTEMCLOCKMONITOR_H_
