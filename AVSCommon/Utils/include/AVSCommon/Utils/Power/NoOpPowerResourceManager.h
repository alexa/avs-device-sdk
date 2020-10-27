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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_NOOPPOWERRESOURCEMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_NOOPPOWERRESOURCEMANAGER_H_

#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

/**
 * A no-op implemention of the @c PowerResourceManager to be used for fallback behavior.
 */
class NoOpPowerResourceManager : public avsCommon::sdkInterfaces::PowerResourceManagerInterface {
public:
    /// @name PowerResourceManagerInterface Functions
    /// @{
    void acquirePowerResource(
        const std::string& component,
        const PowerResourceLevel level = PowerResourceLevel::ACTIVE_HIGH) override{};
    void releasePowerResource(const std::string& component) override{};
    bool isPowerResourceAcquired(const std::string& component) override {
        return false;
    };
    std::shared_ptr<PowerResourceId> create(
        const std::string& resourceId,
        bool isRefCounted = true,
        const PowerResourceLevel level = PowerResourceLevel::STANDBY_MED) override {
        return nullptr;
    };
    bool acquire(
        const std::shared_ptr<PowerResourceId>& id,
        const std::chrono::milliseconds autoReleaseTimeout = std::chrono::milliseconds::zero()) override {
        return true;
    };
    bool release(const std::shared_ptr<PowerResourceId>& id) override {
        return true;
    };
    bool close(const std::shared_ptr<PowerResourceId>& id) override {
        return true;
    };
    /// @}
};

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_NOOPPOWERRESOURCEMANAGER_H_
