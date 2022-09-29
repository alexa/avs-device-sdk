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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_AGGREGATEDPOWERRESOURCEMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_AGGREGATEDPOWERRESOURCEMANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <AVSCommon/Utils/functional/hash.h>
#include <AVSCommon/Utils/Logger/LogStringFormatter.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

/**
 * An AVS SDK implementation of @c PowerResourceManagerInterface which aggregates calls to the application provided
 * @c PowerResourceManagerInterface. This implementation creates a @c PowerResourceManagerInterface::PowerResourceId
 * for each @c PowerResourceLevel, and maps acquire/release/etc calls down to each level-aggregated @c PowerResourceId.
 *
 * This reduces the number of resources that are created from the perspective of the application provided @c
 * PowerResourceManagerInterface. Additionally, it allows optimizations (such as deduping calls to reference counted
 * resources) to be more effective.
 *
 * To reduce latency associated with create/close, aggregated PowerResourceIds are not closed dynamically, and will
 * persist for the lifetime of the @c AggregatedPowerResourceManager.
 *
 * This class does not aggregate the acquirePowerResource/releasePowerResource related APIs.
 */
class AggregatedPowerResourceManager : public avsCommon::sdkInterfaces::PowerResourceManagerInterface {
public:
    /**
     * Create an instance of this class.
     *
     * @param powerResourceManager The application provided @c PowerResourceManagerInterface.
     * @return An instance if successful, else nullptr.
     */
    static std::shared_ptr<AggregatedPowerResourceManager> create(
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager);

    /// Destructor.
    virtual ~AggregatedPowerResourceManager();

    /// @name PowerResourceManagerInterface Legacy Methods
    /// @{
    /**********************************Deprecated Legacy APIs**********************************************/
    void acquirePowerResource(
        const std::string& component,
        const PowerResourceLevel level = PowerResourceLevel::STANDBY_MED) override;
    void releasePowerResource(const std::string& component) override;
    bool isPowerResourceAcquired(const std::string& component) override;
    /******************************************************************************************************/
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId> create(
        const std::string& resourceId,
        bool isRefCounted = true,
        const PowerResourceLevel level = PowerResourceLevel::STANDBY_MED) override;
    bool acquire(
        const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId>& id,
        const std::chrono::milliseconds autoReleaseTimeout = std::chrono::milliseconds::zero()) override;
    bool release(
        const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId>& id) override;
    bool close(
        const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId>& id) override;
    /// @}

private:
    /// This is used to key aggregated PowerResourceId to the level it is aggregated by.
    using AggregatedPowerResourceMap = std::unordered_map<
        avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel,
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceId>,
        avsCommon::utils::functional::EnumClassHash>;

    /**
     * A struct to track reference counting preference and level for a @c
     * PowerResourceManagerInterface::PowerResourceId.
     */
    struct PowerResourceInfo {
        /**
         * Constructor.
         *
         * @param isRefCounted Whether this resource is reference counted.
         * @param level The power level.
         */
        PowerResourceInfo(
            bool isRefCounted,
            avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level);

        /**
         * Update lastAcquired time point with current system clock time.
         */
        void updateLastAcquiredTimepoint();

        /// Whether this resource is reference counted.
        const bool isRefCounted;

        /// The power level.
        const avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level;

        /// The current refCount.
        uint64_t refCount;

        /// When this PowerResourceInfo was last acquired, for logging purposes.
        std::chrono::system_clock::time_point lastAcquired;
    };

    /**
     * Constructor
     *
     * @param powerResourceManager The application provided @c PowerResourceManagerInterface.
     */
    AggregatedPowerResourceManager(
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager);

    /**
     * Log all active power resource identifiers, without aggregation.
     */
    void logActivePowerResources();

    /**
     * Generates a @c PowerResourceId for the given power level. If it already exists, this function will return
     * the previously created @c PowerResourceId for the given power level. The lock must be held.
     *
     * @param level The level to create an aggregated object for.
     */
    std::shared_ptr<PowerResourceId> getAggregatedPowerResourceIdLocked(
        const PowerResourceManagerInterface::PowerResourceLevel level);

    /// A mutex for synchronization.
    std::mutex m_mutex;

    /// The underlying application provided @c PowerResourceManagerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> m_appPowerResourceManager;

    /// A map of string resource identifier to PowerResourceInfo. The string is stored from the unique id passed into
    /// the PowerResourceManagerInterface::create call.
    std::unordered_map<std::string, PowerResourceInfo> m_ids;

    /// The map of @c PowerResourceId objects that are grouped by level.
    AggregatedPowerResourceMap m_aggregatedPowerResources;

    /// Timer that periodically logs active power resources.
    avsCommon::utils::timing::Timer m_timer;

    /// Object to format log strings correctly.
    avsCommon::utils::logger::LogStringFormatter m_logFormatter;
};

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_AGGREGATEDPOWERRESOURCEMANAGER_H_
