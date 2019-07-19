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

#include "TestableCapabilityProvider.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> TestCapabilityProvider::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void TestCapabilityProvider::addCapabilityConfiguration(
    const std::string& interfaceType,
    const std::string& interfaceName,
    const std::string& interfaceVersion,
    const std::string& interfaceConfig) {
    std::unordered_map<std::string, std::string> capabilityConfigurationMap;
    capabilityConfigurationMap.insert(std::make_pair<std::string, std::string>(
        std::string{avsCommon::avs::CAPABILITY_INTERFACE_TYPE_KEY}, std::string{interfaceType}));
    capabilityConfigurationMap.insert(std::make_pair<std::string, std::string>(
        std::string{avsCommon::avs::CAPABILITY_INTERFACE_NAME_KEY}, std::string{interfaceName}));
    capabilityConfigurationMap.insert(std::make_pair<std::string, std::string>(
        std::string{avsCommon::avs::CAPABILITY_INTERFACE_VERSION_KEY}, std::string{interfaceVersion}));
    if (!interfaceConfig.empty()) {
        capabilityConfigurationMap.insert(std::make_pair<std::string, std::string>(
            std::string{avsCommon::avs::CAPABILITY_INTERFACE_CONFIGURATIONS_KEY}, std::string{interfaceConfig}));
    }

    auto capabilityConfiguration =
        std::make_shared<avsCommon::avs::CapabilityConfiguration>(capabilityConfigurationMap);
    addCapabilityConfiguration(capabilityConfiguration);
}

void TestCapabilityProvider::addCapabilityConfiguration(
    const std::shared_ptr<avsCommon::avs::CapabilityConfiguration>& capabilityConfiguration) {
    m_capabilityConfigurations.insert(capabilityConfiguration);
}

void TestCapabilityProvider::clearCapabilityConfigurations() {
    m_capabilityConfigurations.clear();
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
