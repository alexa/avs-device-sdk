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

#include <bitset>
#include <chrono>
#include <memory>
#include <ostream>
#include <string>

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
        /// A STANDBY level means hardware components are in standby mode, they are suitable
        /// for background activities.
        /// STANDBY_LOW usually means all the hardware components work in the lowest standby power levels
        STANDBY_LOW = 0,
        /// STANDBY_MED usually means all the hardware components work in the medium standby power levels
        STANDBY_MED,
        /// STANDBY_HIGH usually means all the hardware components work in the highest standby power levels
        STANDBY_HIGH,
        /// An ACTIVE level means hardware components are in active mode, they are suitable
        /// for foreground activities.
        /// ACTIVE_LOW usually means all the hardware components work in the lowest active power levels
        ACTIVE_LOW,
        /// ACTIVE_LOW usually means all the hardware components work in the medium active power levels
        ACTIVE_MED,
        /// ACTIVE_HIGH usually means all the hardware components work in the highest active power levels
        ACTIVE_HIGH
    };

    /**
     * Power resource type index. These indices represent a bit position for each
     * of the different power resource types that are likely to be used on a given
     * system. These can be used in combination with std::bitset by using the
     * [] operator.
     */
    enum PowerResourceTypeIndex {
        /// CPU capacity is limited or improved under the CPU resource type.
        TYPE_CPU = 0,
        ///  DDR latency/bandwidth are limited or improved under the DDR resource type.
        TYPE_DDR,
        /// Disk I/O performance is limited or improved under the storage resource type.
        TYPE_STORAGE,
        /// Network latency is limited or improved under the network resource type.
        TYPE_NETWORK,
        /// A reserved type that allows platforms to reserve custom PowerResourceTypeFlags.
        TYPE_RESERVED_ONE,
        /// A reserved type that allows platforms to reserve custom PowerResourceTypeFlags.
        TYPE_RESERVED_TWO,
        /// Number of power resource types. This must always be the last enum member.
        NUM_OF_TYPES
    };

    /**
     * Alias for the bitset of partial states in low power mode.
     */
    using PartialStateBitSet = std::bitset<PowerResourceTypeIndex::NUM_OF_TYPES>;

    /**
     * Power resource type flags. Each hardware device might have multiple
     * power resource types that they must support. This interface describes some of
     * the commonly-used resource types that consumers of this interface may choose to implement.
     * These resource types are presented in a bitwise format to allow combinations of resource
     * types to be added as well. The resource types can be used to set a particular value
     * or to perform a bitwise comparison.
     */
    enum PowerResourceTypeFlag {
        /// The default flag value which corresponds to all power resource types.
        TYPE_ALL_FLAG = 0,
        /// Flag value which represents the CPU resource type.
        TYPE_CPU_FLAG = 1 << PowerResourceTypeIndex::TYPE_CPU,
        /// Flag value which represents the DDR resource type.
        TYPE_DDR_FLAG = 1 << PowerResourceTypeIndex::TYPE_DDR,
        /// Flag value which represents the storage resource type.
        TYPE_STORAGE_FLAG = 1 << PowerResourceTypeIndex::TYPE_STORAGE,
        /// Flag value which represents the network resource type.
        TYPE_NETWORK_FLAG = 1 << PowerResourceTypeIndex::TYPE_NETWORK,
        /// Flag value which represents the first reserved resource type.
        TYPE_RESERVED_ONE_FLAG = 1 << PowerResourceTypeIndex::TYPE_RESERVED_ONE,
        /// Flag value which represents the second reserved resource type.
        TYPE_RESERVED_TWO_FLAG = 1 << PowerResourceTypeIndex::TYPE_RESERVED_TWO
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
        const PowerResourceLevel level = PowerResourceLevel::STANDBY_MED) = 0;

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

    /**
     * Acquires the time since latest system resume.
     * @return time since last system resume, if implemented by power manager, zero otherwise.
     */
    virtual std::chrono::milliseconds getTimeSinceLastResumeMS();

    /**
     * Acquires the time since latest partial low power mode state change.
     * This API should only be called after a power resource has been acquired.
     * @param component component name.
     * @param resourceFlags in/out parameter to get resource flags which exit lpm state last.
     * This parameter is passed by the caller of this API and it gets updated with the bit
     * pattern which represents one bit position for each resource type.Ref:enum PowerResourceTypeIndex
     * @param partialState the partial low power mode state (PowerResourceTypeFlags) to check.
     * The state type is determined based on the bits that are passed in. For example, if
     * TYPE_CPU is passed in then the time since the most recent CPU low power mode state will
     * be returned. The default behavior of this partial state, TYPE_ALL, returns the most recent low
     * power mode state value. If more than one partial state is passed in, e.g. TYPE_CPU|TYPE_DDR,
     * this API will return the time from the most recent change of the types given.
     * @return time since last partial system resume - zero indicates that either the platform doesn't
     * support the partial state or that the device was never in partial low power mode.
     */
    virtual std::chrono::milliseconds getTimeSinceLastPartialMS(
        const std::string& component,
        PartialStateBitSet& resourceFlags,
        PartialStateBitSet partialState = PowerResourceTypeFlag::TYPE_ALL_FLAG);

    /**
     * New APIs to support refcount and acquire with timeout.
     * Use the below new APIs - create(), acquire(), release() and close()
     * if you need refcounting or autorelease timeout support for your component.
     * @warning Do not mix and match new and legacy APIs
     */

    /**
     * Class PowerResourceId used to represent a power resource
     */

    class PowerResourceId {
    public:
        /// Getter for the resourceId string member
        /// @return resourceId string member
        std::string getResourceId() const {
            return m_resourceId;
        }
        /// Constructor
        PowerResourceId(const std::string& resourceId) : m_resourceId(resourceId) {
        }

    private:
        /// string member denoting resourceId used to key this object
        const std::string m_resourceId;
    };

    /**
     * Create a power resource keyed by the unique string resourceId.
     * @param resourceId mentions what the resource is for.
     * @param isRefCounted whether refcounting is enabled for this resource
     * @param level power resource level.
     * @return shared pointer of type PowerResourceId representing the resource
     */
    virtual std::shared_ptr<PowerResourceId> create(
        const std::string& resourceId,
        bool isRefCounted = true,
        const PowerResourceLevel level = PowerResourceLevel::STANDBY_MED) = 0;

    /**
     * Acquire a power resource.
     * @param id shared pointer of type PowerResourceId representing the resource.
     * @param autoReleaseTimeout auto release timeout value. Zero denotes auto release disabled.
     * @return true if acquire was successful, false if it failed.
     */
    virtual bool acquire(
        const std::shared_ptr<PowerResourceId>& id,
        const std::chrono::milliseconds autoReleaseTimeout = std::chrono::milliseconds::zero()) = 0;

    /**
     * Release a power resource.
     * @param id shared pointer of type PowerResourceId representing the resource.
     * @return true if release was successful, false if it failed.
     */
    virtual bool release(const std::shared_ptr<PowerResourceId>& id) = 0;

    /**
     * Close a power resource.
     * @param id shared pointer of type PowerResourceId representing the resource.
     * @return true if close was successful, false if it failed.
     */
    virtual bool close(const std::shared_ptr<PowerResourceId>& id) = 0;
};

/**
 * Provides the default @c PowerResourceManagerInterface time since last resume in MS.
 *
 * @return Return default value of 0 milliseconds in the form of std::chrono::milliseconds.
 */
inline std::chrono::milliseconds PowerResourceManagerInterface::getTimeSinceLastResumeMS() {
    return std::chrono::milliseconds::zero();
}

/**
 * Provides the default @c PowerResourceManagerInterface time since last partial in MS.
 * @param component component name.
 * @param resourceFlags in/out parameter to get resource flags which exit lpm state last.
 * @param partialState the partial low power mode state (PowerResourceTypeFlags) to check.
 * @return Return default value of 0 milliseconds in the form of std::chrono::milliseconds.
 */
inline std::chrono::milliseconds PowerResourceManagerInterface::getTimeSinceLastPartialMS(
    const std::string& component,
    PartialStateBitSet& resourceFlags,
    PartialStateBitSet partialState) {
    return std::chrono::milliseconds::zero();
}

/**
 * Converts the @c PowerResourceLevel enum to a string.
 *
 * @param The level to convert.
 * @return A string representation of the level.
 */
inline std::string powerResourceLevelToString(PowerResourceManagerInterface::PowerResourceLevel level) {
    switch (level) {
        case PowerResourceManagerInterface::PowerResourceLevel::STANDBY_LOW:
            return "STANDBY_LOW";
        case PowerResourceManagerInterface::PowerResourceLevel::STANDBY_MED:
            return "STANDBY_MED";
        case PowerResourceManagerInterface::PowerResourceLevel::STANDBY_HIGH:
            return "STANDBY_HIGH";
        case PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_LOW:
            return "ACTIVE_LOW";
        case PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_MED:
            return "ACTIVE_MED";
        case PowerResourceManagerInterface::PowerResourceLevel::ACTIVE_HIGH:
            return "ACTIVE_HIGH";
    }

    return "UNKNOWN";
}

/**
 * Overload for the @c PowerResourceLevel enum. This will write the @c PowerResourceLevel as a string to the provided
 * stream.
 *
 * @param An ostream to send the level as a string.
 * @param The level to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, PowerResourceManagerInterface::PowerResourceLevel level) {
    return stream << powerResourceLevelToString(level);
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POWERRESOURCEMANAGERINTERFACE_H_
