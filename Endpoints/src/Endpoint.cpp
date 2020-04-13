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

#include <algorithm>

#include "Endpoints/Endpoint.h"

namespace alexaClientSDK {
namespace endpoints {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("Endpoint");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

Endpoint::Endpoint(const EndpointAttributes& attributes) : m_attributes(attributes) {
}

Endpoint::~Endpoint() {
    for (auto& shutdownObj : m_requireShutdownObjects) {
        shutdownObj->shutdown();
    }
}

void Endpoint::addRequireShutdownObjects(const std::list<std::shared_ptr<RequiresShutdown>>& requireShutdownObjects) {
    m_requireShutdownObjects.insert(requireShutdownObjects.begin(), requireShutdownObjects.end());
}

Endpoint::EndpointIdentifier Endpoint::getEndpointId() const {
    return m_attributes.endpointId;
}

AVSDiscoveryEndpointAttributes Endpoint::getAttributes() const {
    return m_attributes;
}

std::vector<avsCommon::avs::CapabilityConfiguration> Endpoint::getCapabilityConfigurations() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    std::vector<CapabilityConfiguration> retValue;
    std::for_each(
        m_capabilities.begin(), m_capabilities.end(), [&retValue](decltype(m_capabilities)::value_type value) {
            retValue.push_back(value.first);
        });
    return retValue;
}

std::unordered_map<CapabilityConfiguration, std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>>
Endpoint::getCapabilities() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_capabilities;
}

bool Endpoint::addCapability(
    const CapabilityConfiguration& capabilityConfiguration,
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler) {
    if (!directiveHandler) {
        ACSDK_ERROR(LX("addCapabilityAgentFailed").d("reason", "nullHandler"));
        return false;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_capabilities.find(capabilityConfiguration) != m_capabilities.end()) {
        ACSDK_ERROR(LX("addCapabilityAgentFailed")
                        .d("reason", "capabilityAlreadyExists")
                        .d("interface", capabilityConfiguration.interfaceName)
                        .d("type", capabilityConfiguration.type)
                        .d("instance", capabilityConfiguration.instanceName.valueOr("")));
        return false;
    }

    m_capabilities.insert(std::make_pair(capabilityConfiguration, directiveHandler));
    return true;
}

bool Endpoint::addCapabilityConfiguration(const CapabilityConfiguration& capabilityConfiguration) {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_capabilities.insert(std::make_pair(capabilityConfiguration, nullptr)).second) {
        ACSDK_ERROR(LX("addCapabilityConfigurationFailed")
                        .d("reason", "capabilityAlreadyExists")
                        .d("interface", capabilityConfiguration.interfaceName)
                        .d("type", capabilityConfiguration.type)
                        .d("instance", capabilityConfiguration.instanceName.valueOr("")));
        return false;
    }

    return true;
}

}  // namespace endpoints
}  // namespace alexaClientSDK
