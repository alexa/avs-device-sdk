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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERRESOURCEMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERRESOURCEMANAGERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface provides APIs for components of AVS-SDK to manage power resources. These components,
 * such as AudioInputProcessor and SpeechSynthesizer, can claim the level of power resource they need
 * when active (listening or speacking) by invoking the acquirePowerResource API, such that the power
 * management system keeps the hardware devices works in the claimed power levels. When the component
 * is inactive, it invokes the releasePowerResource API to release the acquired power resource level.
 * This interface defined 6 power resource levels. To implement this interface, the user need define
 * a mapping from them to the real power resource levels of the power management system.
 */
class PowerResourceManagerInterface {
public:
    /**
     * Power resource levels. Each hardware device may have multiple STANDBY and ACTIVE power modes.
     * For example, CPU can work in different frequency and number of cores. It has a latency to
     * switch from low power mode to high power mode, and the latency increases with the gaps of
     * the power levels. Power management system defines a group of power polices; each policy specifies
     * the power modes of hardware components. To implement this interface, the user needs to map
     * the power polices (that allow applications to proactively claim) to the 6 power resource
     * levels we defined in this enumaration.
     */
    enum class PowerResourceLevel {
        // A STANDBY level means hardware components are in standby mode, they are suitable
        // for background activities.
        // STANDBY_LOW usually means all the hardware components work in the lowest standby power levels
        STANDBY_LOW = 0,
        // STANDBY_MED usually means all the hardware components work in the medium standby power levels
        STANDBY_MED,
        // STANDBY_HIGH usually means all the hardware components work in the highest standby power levels
        STANDBY_HIGH,
        // An ACTIVE level means hardware components are in active mode, they are suitable
        // for foreground activities.
        // ACTIVE_LOW usually means all the hardware components work in the lowest active power levels
        ACTIVE_LOW,
        // ACTIVE_LOW usually means all the hardware components work in the medium active power levels
        ACTIVE_MED,
        // ACTIVE_HIGH usually means all the hardware components work in the highest active power levels
        ACTIVE_HIGH
    };

    /**
     * Destructor
     */
    virtual ~PowerResourceManagerInterface() = default;

    /**
     * Acquire a power resource for the component.
     * @param component component name.
     * @param level power resource level.
     */
    virtual void acquirePowerResource(
        const std::string& component,
        const PowerResourceLevel level = PowerResourceLevel::ACTIVE_HIGH) = 0;

    /**
     * Release the acquired power resource of the specified component.
     * @param component component name.
     */
    virtual void releasePowerResource(const std::string& component) = 0;

    /**
     * Checks whether a power resource had been acquired or not.
     * @param component component name.
     * @return true if the power resource had been acquired, otherwise return false.
     */
    virtual bool isPowerResourceAcquired(const std::string& component) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERRESOURCEMANAGERINTERFACE_H_
