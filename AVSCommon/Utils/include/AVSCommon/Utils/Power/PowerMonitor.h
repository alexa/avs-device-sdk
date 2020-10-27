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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_POWERMONITOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_POWERMONITOR_H_

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include "AVSCommon/Utils/Power/PowerResource.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

/**
 * A class providing power monitoring capabilities for the SDK.
 * To use this object, activate() must first be called.
 *
 * This must be activated before any components are created that may have power management logic.
 * For applications that wish to use any of the Power related primitives, this may mean calling activate()
 * outside the SDK.
 */
class PowerMonitor {
public:
    /**
     * Returns an instance of @c PowerMonitor. This is guaranteed to be non-null.
     *
     * @return The singleton instance of @c PowerMonitor.
     */
    static std::shared_ptr<PowerMonitor> getInstance();

    /**
     * Iniitalizes the @c PowerMonitor with a @c PowerResourceManagerInterface and activates.
     *
     * @param powerManager A @c PowerResourceManagerInterface instance.
     */
    void activate(std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> powerManager);

    /**
     * Whether the @c PowerMonitor has been activated with an @c PowerResourceManagerInterface.
     *
     * @return A bool indicating activation state.
     */
    bool isActive();

    /**
     * Deactivate the @c PowerMonitor. This clears the internal map and resets the
     * @c PowerManagerResourceMangerInterface instance.
     *
     * @return a PowerManagerResourceMangerInterface instance.
     */
    void deactivate();

    /**
     * Get the @c PowerResourceManagerInterface.
     *
     * @return The @c PowerResourceManagerInterface.
     */
    std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> getPowerResourceManager();

    /**
     * Gets the @c PowerResource associated with the current thread, or create one if none are associated.
     * The same identifier must be used with a particular thread. The creator of the thread PowerResource is
     * responsible for cleaning it up via removeThreadPowerResource(). This should typically be done when
     * the thread exits.
     *
     * @param identifier The identifier to create the thread @c PowerResource if none exists.
     * @param level The @c PowerResourceLevel to use on first creation. Otherwise this is ignored.
     * @return @c PowerResource associated with the current thread.
     */
    std::shared_ptr<PowerResource> getThreadPowerResourceOrCreate(
        const std::string& identifier,
        sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level =
            sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel::STANDBY_MED);

    /**
     * Get the @c PowerResource associated with the current thread.
     *
     * @return @c PowerResource associated with the current thread.
     */
    std::shared_ptr<PowerResource> getThreadPowerResource();

    /**
     * Assigns a previously created @c PowerResource to a thread. This will not affect the state of the @c
     * PowerResource. If there is a previously assigned @c PowerResource this will fail.
     *
     * @param powerResource The @c PowerResource to assign.
     * @return The assigned @c PowerResource if successful, else a nullptr.
     */
    std::shared_ptr<PowerResource> assignThreadPowerResource(std::shared_ptr<PowerResource> powerResource);

    /**
     * Convenience method for creating a local (non-thread associated) @c PowerResource using
     * getPowerManager().
     *
     * @param The identifier to create the local @c PowerResource with.
     * @param level The @c PowerResourceLevel to use.
     * @return @c PowerResource.
     */
    std::shared_ptr<PowerResource> createLocalPowerResource(
        const std::string& identifier,
        sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level =
            sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel::STANDBY_MED);

    /**
     * Remove the current @c PowerResource associated with this thread.
     */
    void removeThreadPowerResource();

private:
    /**
     * Returns m_active. m_mutex must be locked before calling this.
     *
     * @return A bool indicating activation state.
     */
    bool isActiveLocked();

    /// Mutex.
    static std::mutex m_mutex;

    /// Singleton instance to the @c PowerMonitor.
    static std::shared_ptr<PowerMonitor> m_monitor;

    /// The underlying @c PowerManagerResourceInterface.
    std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> m_powerManager;

    /// A map of thread to @c PowerResource.
    std::unordered_map<std::thread::id, std::shared_ptr<PowerResource>> m_threadPowerResources;
};

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_POWERMONITOR_H_
