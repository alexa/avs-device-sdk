/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTCAPABILITYPROVIDER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTCAPABILITYPROVIDER_H_

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * A test capability provider that can provide the capabilities that you want to test with.
 */
class TestCapabilityProvider : public avsCommon::sdkInterfaces::CapabilityConfigurationInterface {
public:
    /// @name CapabilityConfigurationInterface method overrides.
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /**
     * Constructor
     */
    TestCapabilityProvider() = default;

    /**
     * Adds a capability configuration.
     *
     * @param interfaceType The interface type.
     * @param interfaceName The interface name.
     * @param interfaceVersion The interface version.
     * @param interfaceConfig The interface config as a json string.
     */
    void addCapabilityConfiguration(
        const std::string& interfaceType,
        const std::string& interfaceName,
        const std::string& interfaceVersion,
        const std::string& interfaceConfig = "");

    /**
     * Adds a capability configuration.
     *
     * @param capabilityConfiguration The CapabilityConfiguration object.
     */
    void addCapabilityConfiguration(
        const std::shared_ptr<avsCommon::avs::CapabilityConfiguration>& capabilityConfiguration);

    /**
     * Clears all capability configurations.
     */
    void clearCapabilityConfigurations();

private:
    /// The set of capability configurations
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTCAPABILITYPROVIDER_H_ */
