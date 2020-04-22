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

#include "CapabilitiesDelegate/Utils/DiscoveryUtils.h"

#include <AVSCommon/AVS/AVSMessageEndpoint.h>
#include <AVSCommon/AVS/AVSMessageHeader.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Optional.h>
#include <Endpoints/EndpointAttributeValidation.h>

/// String to identify log entries originating from this file.
static const std::string TAG("DiscoveryUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace utils {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils;

/// CapabilityConfiguration keys
/// The instance name key
static const std::string CAPABILITY_INTERFACE_INSTANCE_NAME_KEY = "instance";
/// The properties key
static const std::string CAPABILITY_INTERFACE_PROPERTIES_KEY = "properties";
/// The interface name key
static const std::string CAPABILITY_INTERFACE_PROPERTIES_NAME_KEY = "name";
/// The supported key
static const std::string CAPABILITY_INTERFACE_PROPERTIES_SUPPORTED_KEY = "supported";
/// The proactively report key
static const std::string CAPABILITY_INTERFACE_PROPERTIES_PROACTIVELY_REPORTED_KEY = "proactivelyReported";
/// The retrievable key
static const std::string CAPABILITY_INTERFACE_PROPERTIES_RETRIEVABLE_KEY = "retrievable";
/// The nonControllable key
static const std::string CAPABILITY_INTERFACE_PROPERTIES_NON_CONTROLLABLE_KEY = "nonControllable";

/// Endpoint Configuration keys
/// Endpoint ID key
const static std::string ENDPOINT_ID_KEY = "endpointId";
/// Friendly name key
const static std::string FRIENDLY_NAME_KEY = "friendlyName";
/// Description key
const static std::string DESCRIPTION_KEY = "description";
/// Manufacturer name key
const static std::string MANUFACTURER_NAME_KEY = "manufacturerName";
/// Display Categories key
const static std::string DISPLAY_CATEGORIES_KEY = "displayCategories";
/// Additional Attributes key
const static std::string ADDITIONAL_ATTRIBUTES_KEY = "additionalAttributes";
/// DSN Key
const static std::string DEVICE_SERIAL_NUMBER_KEY = "deviceSerialNumber";
/// Connections Key
const static std::string CONNECTIONS_KEY = "connections";
/// Cookie key
const static std::string COOKIE_KEY = "cookie";
/// Manufacturer name key
static const std::string ADDITIONAL_MANUFACTURER_KEY = "manufacturer";
/// model key
static const std::string ADDITIONAL_MODEL_KEY = "model";
/// serial number key
static const std::string ADDITIONAL_SERIAL_NUMBER_KEY = "serialNumber";
/// firmware version key
static const std::string ADDITIONAL_FIRMWARE_VERSION_KEY = "firmwareVersion";
/// software version key
static const std::string ADDITIONAL_SOFTWARE_VERSION_KEY = "softwareVersion";
/// custom identifier key
static const std::string ADDITIONAL_CUSTOM_IDENTIFIER_KEY = "customIdentifier";
/// Capabilities key in message body
static const std::string CAPABILITIES_KEY = "capabilities";

/// Discovery Keys
/// Discovery Namespace
static const std::string DISCOVERY_NAMESPACE = "Alexa.Discovery";
/// Payload version
static const std::string PAYLOAD_VERSION = "3";
/// AddOrUpdateReport event name
static const std::string ADD_OR_UPDATE_REPORT_NAME = "AddOrUpdateReport";
/// DeleteReport event name
static const std::string DELETE_REPORT_NAME = "DeleteReport";
/// Endpoints Key
static const std::string ENDPOINTS_KEY = "endpoints";
/// Scope Key
static const std::string SCOPE_KEY = "scope";
/// Scope Type Key
static const std::string SCOPE_TYPE_KEY = "type";
/// Scope Type Value Key
static const std::string SCOPE_TYPE_BEARER_TOKEN = "BearerToken";
/// Scope Token key
static const std::string SCOPE_TOKEN_KEY = "token";

/**
 * Helper struct to build json objects
 */
struct JsonObjectScope {
    JsonObjectScope(JsonGenerator* generator, std::string key) {
        m_generator = generator;
        m_generator->startObject(key);
    }
    ~JsonObjectScope() {
        m_generator->finishObject();
    }

    JsonGenerator* m_generator;
};

/**
 * Formats the scope object that needs to be included in the @c AddOrUpdateReport and @c DeleteReport events.
 *
 * @param authToken The authorization token to be included in the event.
 * @return The JSON formatted scope object.
 */
static std::string getScopeJson(const std::string& authToken) {
    JsonGenerator scopeGenerator;

    scopeGenerator.addMember(SCOPE_TYPE_KEY, SCOPE_TYPE_BEARER_TOKEN);
    scopeGenerator.addMember(SCOPE_TOKEN_KEY, authToken);

    return scopeGenerator.toString();
}

/**
 * Formats the given @c CapabilityConfiguration into a JSON required to send in @c Discovery.AddOrUpdateReport event.
 * Note: The caller should validate the CapabilityConfiguration before calling this method.
 *
 * @param capabilityConfig The input @c CapabilityConfiguration.
 * @return The JSON formatted CapabilityConfiguration.
 */
static std::string getCapabilityConfigJson(const CapabilityConfiguration& capabilityConfig) {
    JsonGenerator generator;

    generator.addMember(CAPABILITY_INTERFACE_TYPE_KEY, capabilityConfig.type);
    generator.addMember(CAPABILITY_INTERFACE_NAME_KEY, capabilityConfig.interfaceName);
    generator.addMember(CAPABILITY_INTERFACE_VERSION_KEY, capabilityConfig.version);
    if (capabilityConfig.instanceName.hasValue()) {
        generator.addMember(CAPABILITY_INTERFACE_INSTANCE_NAME_KEY, capabilityConfig.instanceName.value());
    }
    if (capabilityConfig.properties.hasValue()) {
        JsonObjectScope scope(&generator, CAPABILITY_INTERFACE_PROPERTIES_KEY);
        std::vector<std::string> supportedJson;
        for (auto supported : capabilityConfig.properties.value().supportedList) {
            JsonGenerator generator2;
            generator2.addMember(CAPABILITY_INTERFACE_PROPERTIES_NAME_KEY, supported);
            supportedJson.push_back(generator2.toString());
        }
        generator.addMembersArray(CAPABILITY_INTERFACE_PROPERTIES_SUPPORTED_KEY, supportedJson);
        generator.addMember(
            CAPABILITY_INTERFACE_PROPERTIES_PROACTIVELY_REPORTED_KEY,
            capabilityConfig.properties.value().isProactivelyReported);
        generator.addMember(
            CAPABILITY_INTERFACE_PROPERTIES_RETRIEVABLE_KEY, capabilityConfig.properties.value().isRetrievable);
        if (capabilityConfig.properties.value().isNonControllable.hasValue()) {
            generator.addMember(
                CAPABILITY_INTERFACE_PROPERTIES_NON_CONTROLLABLE_KEY,
                capabilityConfig.properties.value().isNonControllable.value());
        }
    }

    for (const auto& customConfigPair : capabilityConfig.additionalConfigurations) {
        generator.addRawJsonMember(customConfigPair.first, customConfigPair.second);
    }

    return generator.toString();
}

bool validateCapabilityConfiguration(const CapabilityConfiguration& capabilityConfig) {
    if (capabilityConfig.type.empty() || capabilityConfig.interfaceName.empty() || capabilityConfig.version.empty()) {
        ACSDK_ERROR(LX("validateCapabilityConfigurationFailed")
                        .d("reason", "requiredFieldEmpty")
                        .d("missingType", capabilityConfig.type.empty())
                        .d("capabilityConfig", capabilityConfig.interfaceName.empty())
                        .d("version", capabilityConfig.version.empty()));
        return false;
    }

    if (capabilityConfig.instanceName.hasValue() && capabilityConfig.instanceName.value().empty()) {
        ACSDK_ERROR(LX("validateCapabilityConfigurationFailed").d("reason", "emptyInstanceName"));
        return false;
    }

    if (capabilityConfig.properties.hasValue() && capabilityConfig.properties.value().supportedList.empty()) {
        ACSDK_ERROR(LX("validateCapabilityConfigurationFailed").d("reason", "emptySupportedList"));
        return false;
    }

    // Validate if the custom configuration values string is a valid json
    for (auto it = capabilityConfig.additionalConfigurations.begin();
         it != capabilityConfig.additionalConfigurations.end();
         ++it) {
        rapidjson::Document configJson;
        if (configJson.Parse(it->second).HasParseError()) {
            ACSDK_ERROR(LX("validateCapabilityConfigurationFailed").d("reason", "invalidCustomConfiguration"));
            return false;
        }
    }

    return true;
}

bool validateEndpointAttributes(const AVSDiscoveryEndpointAttributes& endpointAttributes) {
    if (!endpoints::isEndpointIdValid(endpointAttributes.endpointId)) {
        ACSDK_ERROR(LX("validateEndpointAttributesFailed").d("reason", "invalidEndpointId"));
        return false;
    }

    if (!endpoints::isDescriptionValid(endpointAttributes.description)) {
        ACSDK_ERROR(LX("validateEndpointAttributesFailed").d("reason", "invalidDescription"));
        return false;
    }

    if (!endpoints::isManufacturerNameValid(endpointAttributes.manufacturerName)) {
        ACSDK_ERROR(LX("validateEndpointAttributesFailed").d("reason", "invalidManufacturerName"));
        return false;
    }

    if (endpointAttributes.displayCategories.empty()) {
        ACSDK_ERROR(LX("validateEndpointAttributesFailed").d("reason", "invalidDisplayCategories"));
        return false;
    }

    return true;
}

std::string getEndpointConfigJson(
    const AVSDiscoveryEndpointAttributes& endpointAttributes,
    const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) {
    JsonGenerator generator;

    generator.addMember(ENDPOINT_ID_KEY, endpointAttributes.endpointId);
    generator.addMember(FRIENDLY_NAME_KEY, endpointAttributes.friendlyName);
    generator.addMember(DESCRIPTION_KEY, endpointAttributes.description);
    generator.addMember(MANUFACTURER_NAME_KEY, endpointAttributes.manufacturerName);

    generator.addStringArray(DISPLAY_CATEGORIES_KEY, endpointAttributes.displayCategories);

    /// Additional Attributes Object.
    if (endpointAttributes.additionalAttributes.hasValue()) {
        JsonObjectScope scope(&generator, ADDITIONAL_ATTRIBUTES_KEY);

        AVSDiscoveryEndpointAttributes::AdditionalAttributes additionalAttributes =
            endpointAttributes.additionalAttributes.value();
        if (!additionalAttributes.manufacturer.empty()) {
            generator.addMember(ADDITIONAL_MANUFACTURER_KEY, additionalAttributes.manufacturer);
        }
        if (!additionalAttributes.model.empty()) {
            generator.addMember(ADDITIONAL_MODEL_KEY, additionalAttributes.model);
        }
        if (!additionalAttributes.serialNumber.empty()) {
            generator.addMember(ADDITIONAL_SERIAL_NUMBER_KEY, additionalAttributes.serialNumber);
        }
        if (!additionalAttributes.firmwareVersion.empty()) {
            generator.addMember(ADDITIONAL_FIRMWARE_VERSION_KEY, additionalAttributes.firmwareVersion);
        }
        if (!additionalAttributes.softwareVersion.empty()) {
            generator.addMember(ADDITIONAL_SOFTWARE_VERSION_KEY, additionalAttributes.softwareVersion);
        }
        if (!additionalAttributes.customIdentifier.empty()) {
            generator.addMember(ADDITIONAL_CUSTOM_IDENTIFIER_KEY, additionalAttributes.customIdentifier);
        }
    }

    /// Registration Object.
    if (endpointAttributes.registration.hasValue()) {
        JsonObjectScope registration(&generator, endpointAttributes.registration.value().registrationKey);
        generator.addMember(
            endpointAttributes.registration.value().productIdKey, endpointAttributes.registration.value().productId);
        generator.addMember(DEVICE_SERIAL_NUMBER_KEY, endpointAttributes.registration.value().serialNumber);
    }

    /// Connections Object.
    if (!endpointAttributes.connections.empty()) {
        std::vector<std::string> connectionsJsons;
        for (size_t i = 0; i < endpointAttributes.connections.size(); ++i) {
            JsonGenerator connectionJsonGenerator;
            for (auto it = endpointAttributes.connections[i].begin(); it != endpointAttributes.connections[i].end();
                 ++it) {
                connectionJsonGenerator.addMember(it->first, it->second);
            }
            connectionsJsons.push_back(connectionJsonGenerator.toString());
        }
        generator.addMembersArray(CONNECTIONS_KEY, connectionsJsons);
    }

    /// Cookie Object.
    if (!endpointAttributes.cookies.empty()) {
        JsonObjectScope cookies(&generator, COOKIE_KEY);
        for (auto it = endpointAttributes.cookies.begin(); it != endpointAttributes.cookies.end(); ++it) {
            generator.addMember(it->first, it->second);
        }
    }

    /// Capabilities Object.
    std::vector<std::string> capabilityConfigJsons;
    for (auto capabilityConfig : capabilities) {
        if (validateCapabilityConfiguration(capabilityConfig)) {
            std::string capabilityConfigJson = getCapabilityConfigJson(capabilityConfig);
            capabilityConfigJsons.push_back(capabilityConfigJson);
        }
    }
    generator.addMembersArray(CAPABILITIES_KEY, capabilityConfigJsons);

    return generator.toString();
}

std::string getDeleteReportEndpointConfigJson(const std::string& endpointId) {
    JsonGenerator deleteReportEndpointConfigGenerator;
    { deleteReportEndpointConfigGenerator.addMember(ENDPOINT_ID_KEY, endpointId); }
    return deleteReportEndpointConfigGenerator.toString();
}

std::pair<std::string, std::string> getAddOrUpdateReportEventJson(
    const std::vector<std::string>& endpointConfigurations,
    const std::string& authToken) {
    ACSDK_DEBUG5(LX(__func__));

    auto header = AVSMessageHeader::createAVSEventHeader(
        DISCOVERY_NAMESPACE, ADD_OR_UPDATE_REPORT_NAME, "", "", PAYLOAD_VERSION, "");
    JsonGenerator payloadGenerator;
    {
        payloadGenerator.addRawJsonMember(SCOPE_KEY, getScopeJson(authToken));
        payloadGenerator.addMembersArray(ENDPOINTS_KEY, endpointConfigurations);
    }

    std::string addOrUpdateEvent =
        buildJsonEventString(header, Optional<AVSMessageEndpoint>(), payloadGenerator.toString());
    return {addOrUpdateEvent, header.getEventCorrelationToken()};
}

std::string getDeleteReportEventJson(
    const std::vector<std::string>& endpointConfigurations,
    const std::string& authToken) {
    ACSDK_DEBUG5(LX(__func__));

    auto header =
        AVSMessageHeader::createAVSEventHeader(DISCOVERY_NAMESPACE, DELETE_REPORT_NAME, "", "", PAYLOAD_VERSION, "");

    JsonGenerator payloadGenerator;
    {
        payloadGenerator.addRawJsonMember(SCOPE_KEY, getScopeJson(authToken));
        payloadGenerator.addMembersArray(ENDPOINTS_KEY, endpointConfigurations);
    }

    return buildJsonEventString(header, Optional<AVSMessageEndpoint>(), payloadGenerator.toString());
}

}  // namespace utils
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
