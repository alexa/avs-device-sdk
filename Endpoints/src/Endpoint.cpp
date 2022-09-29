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
#include "Endpoints/EndpointAttributeValidation.h"

namespace alexaClientSDK {
namespace endpoints {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "Endpoint"

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
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_attributes.endpointId;
}

AVSDiscoveryEndpointAttributes Endpoint::getAttributes() const {
    std::lock_guard<std::mutex> lock{m_mutex};
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

bool Endpoint::update(const std::shared_ptr<EndpointModificationData>& endpointModificationData) {
    auto endpointId = endpointModificationData->endpointIdentifier;
    auto updatedAttributes = endpointModificationData->updatedEndpointAttributes;
    auto updatedConfigurations = endpointModificationData->updatedConfigurations;
    auto addedCapabilities = endpointModificationData->capabilitiesToAdd;
    auto deletedCapabilities = endpointModificationData->capabilitiesToRemove;
    auto capabilitiesToShutDown = endpointModificationData->capabilitiesToShutDown;

    // Update endpoint attributes
    if (updatedAttributes.hasValue()) {
        AVSDiscoveryEndpointAttributes newAttributes = updatedAttributes.value();
        if (endpointId != newAttributes.endpointId) {
            ACSDK_ERROR(LX("updateFailed").d("reason", "invalid endpointId"));
            return false;
        }
        if (!validateEndpointAttributes(newAttributes)) {
            ACSDK_ERROR(LX("updateFailed").d("reason", "invalid endpoint attributes"));
            return false;
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        m_attributes = updatedAttributes.value();
        lock.unlock();
    }

    // Update capability configurations
    for (const auto& capabilityConfiguration : updatedConfigurations) {
        auto capabilities = getCapabilities();
        for (const auto& currentCapability : capabilities) {
            if (currentCapability.first.interfaceName.compare(capabilityConfiguration.interfaceName) == 0 &&
                currentCapability.first.instanceName.valueOr("").compare(
                    capabilityConfiguration.instanceName.valueOr("")) == 0) {
                auto handler = currentCapability.second;
                if (!removeCapability(currentCapability.first)) {
                    return false;
                }

                if ((handler == nullptr && !addCapabilityConfiguration(capabilityConfiguration)) ||
                    (handler != nullptr && !addCapability(capabilityConfiguration, handler))) {
                    return false;
                }
                ACSDK_DEBUG5(LX("updateCapabilitySucceeded")
                                 .d("interface", capabilityConfiguration.interfaceName)
                                 .d("type", capabilityConfiguration.type)
                                 .d("instance", capabilityConfiguration.instanceName.valueOr("")));
                break;
            }
        }
    }

    // Add capabilities
    for (const auto& addedCapability : addedCapabilities) {
        auto handler = addedCapability.second;
        if ((handler == nullptr && !addCapabilityConfiguration(addedCapability.first)) ||
            (handler != nullptr && !addCapability(addedCapability.first, handler))) {
            return false;
        }
        ACSDK_DEBUG5(LX("addCapabilitySucceeded")
                         .d("interface", addedCapability.first.interfaceName)
                         .d("type", addedCapability.first.type)
                         .d("instance", addedCapability.first.instanceName.valueOr("")));
    }

    // Remove capabilities
    for (const auto& deletedCapability : deletedCapabilities) {
        if (!removeCapability(deletedCapability)) {
            return false;
        }
        ACSDK_DEBUG5(LX("removeCapabilitySucceeded")
                         .d("interface", deletedCapability.interfaceName)
                         .d("type", deletedCapability.type)
                         .d("instance", deletedCapability.instanceName.valueOr("")));
    }

    // Add new capabilities needed to shut down
    if (!capabilitiesToShutDown.empty()) {
        addRequireShutdownObjects(capabilitiesToShutDown);
    }

    return true;
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

bool Endpoint::removeCapability(const CapabilityConfiguration& capabilityConfiguration) {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_capabilities.find(capabilityConfiguration) == m_capabilities.end()) {
        ACSDK_ERROR(LX("removeCapabilityAgentFailed")
                        .d("reason", "capabilityNotExists")
                        .d("interface", capabilityConfiguration.interfaceName)
                        .d("type", capabilityConfiguration.type)
                        .d("instance", capabilityConfiguration.instanceName.valueOr("")));
        return false;
    }
    m_capabilities.erase(capabilityConfiguration);
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

bool Endpoint::validateEndpointAttributes(const EndpointAttributes& updatedAttributes) {
    if (!isEndpointIdValid(updatedAttributes.endpointId)) {
        ACSDK_ERROR(LX("invalidEndpointAttributes").d("reason", "Invalid EndpointId of the endpoint attributes"));
        return false;
    }

    if (!isFriendlyNameValid(updatedAttributes.friendlyName)) {
        ACSDK_ERROR(LX("invalidEndpointAttributes").d("reason", "Invalid friendly name of the endpoint attributes"));
        return false;
    }

    if (!isDescriptionValid(updatedAttributes.description)) {
        ACSDK_ERROR(LX("invalidEndpointAttributes").d("reason", "Invalid description name of the endpoint attributes"));
        return false;
    }

    if (!isManufacturerNameValid(updatedAttributes.manufacturerName)) {
        ACSDK_ERROR(
            LX("invalidEndpointAttributes").d("reason", "Invalid manufacturer name of the endpoint attributes"));
        return false;
    }

    if (updatedAttributes.additionalAttributes.hasValue() &&
        !isAdditionalAttributesValid(updatedAttributes.additionalAttributes.value())) {
        ACSDK_ERROR(
            LX("invalidEndpointAttributes").d("reason", "Invalid additional attributes of the endpoint attributes"));
        return false;
    }

    if (!areConnectionsValid(updatedAttributes.connections)) {
        ACSDK_ERROR(LX("invalidEndpointAttributes").d("reason", "Invalid connections of the endpoint attributes"));
        return false;
    }

    if (!areCookiesValid(updatedAttributes.cookies)) {
        ACSDK_ERROR(LX("invalidEndpointAttributes").d("reason", "Invalid cookies of the endpoint attributes"));
        return false;
    }

    return true;
}

}  // namespace endpoints
}  // namespace alexaClientSDK
