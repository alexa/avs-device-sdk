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

#include "System/SystemCapabilityProvider.h"

#include <AVSCommon/AVS/CapabilityConfiguration.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;

/// System capability constants
/// System interface type
static const std::string SYSTEM_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// System interface name
static const std::string SYSTEM_CAPABILITY_INTERFACE_NAME = "System";
/// System interface version
static const std::string SYSTEM_CAPABILITY_INTERFACE_VERSION = "1.2";

/**
 * Creates the System capability configuration.
 *
 * @return The System capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSystemCapabilityConfiguration();

std::shared_ptr<SystemCapabilityProvider> SystemCapabilityProvider::create() {
    return std::shared_ptr<SystemCapabilityProvider>(new SystemCapabilityProvider());
}

SystemCapabilityProvider::SystemCapabilityProvider() {
    m_capabilityConfigurations.insert(getSystemCapabilityConfiguration());
}

std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSystemCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SYSTEM_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SYSTEM_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SYSTEM_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<avsCommon::avs::CapabilityConfiguration>(configMap);
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SystemCapabilityProvider::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
