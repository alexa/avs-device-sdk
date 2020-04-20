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

#include "Endpoints/EndpointAttributeValidation.h"

namespace alexaClientSDK {
namespace endpoints {

// Used to calculate the cookies size. Each cookie encoding will have 6 bytes (4x'"', 1x':' 1x',') extra.
constexpr size_t EXTRA_BYTES_PER_COOKIE = 6;

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces::endpoints;

bool isEndpointIdValid(const EndpointIdentifier& identifier) {
    auto length = identifier.length();
    return (length > 0) && (length <= AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_IDENTIFIER_LENGTH);
}

bool isFriendlyNameValid(const std::string& name) {
    auto length = name.length();
    return (length > 0) && (length <= AVSDiscoveryEndpointAttributes::MAX_FRIENDLY_NAME_LENGTH);
}

bool isDescriptionValid(const std::string& description) {
    auto length = description.length();
    return (length > 0) && (length <= AVSDiscoveryEndpointAttributes::MAX_DESCRIPTION_LENGTH);
}

bool isManufacturerNameValid(const std::string& manufacturerName) {
    auto length = manufacturerName.length();
    return (length > 0) && (length <= AVSDiscoveryEndpointAttributes::MAX_MANUFACTURER_NAME_LENGTH);
}

bool isAdditionalAttributesValid(const AVSDiscoveryEndpointAttributes::AdditionalAttributes& attributes) {
    return (
        (attributes.manufacturer.length() <=
         AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH) &&
        (attributes.model.length() <= AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH) &&
        (attributes.serialNumber.length() <=
         AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH) &&
        (attributes.firmwareVersion.length() <=
         AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH) &&
        (attributes.softwareVersion.length() <=
         AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH) &&
        (attributes.customIdentifier.length() <=
         AVSDiscoveryEndpointAttributes::MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH));
}

bool areConnectionsValid(const std::vector<std::map<std::string, std::string>>& connections) {
    for (auto& connection : connections) {
        for (auto& keyValuePair : connection) {
            if (keyValuePair.second.length() == 0 ||
                keyValuePair.second.length() > AVSDiscoveryEndpointAttributes::MAX_CONNECTIONS_VALUE_LENGTH) {
                return false;
            }
        }
    }
    return true;
}

bool areCookiesValid(const std::map<std::string, std::string>& cookies) {
    size_t totalSizeBytes = 0;
    for (auto& cookie : cookies) {
        totalSizeBytes += cookie.first.size() + cookie.second.size() + EXTRA_BYTES_PER_COOKIE;
    }
    return totalSizeBytes < AVSDiscoveryEndpointAttributes::MAX_COOKIES_SIZE_BYTES;
}

}  // namespace endpoints
}  // namespace alexaClientSDK
