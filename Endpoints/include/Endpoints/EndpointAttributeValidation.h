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

#ifndef ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTATTRIBUTEVALIDATION_H_
#define ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTATTRIBUTEVALIDATION_H_

#include <map>
#include <string>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>

namespace alexaClientSDK {
namespace endpoints {

/**
 * Returns whether the given identifier follows AVS specification.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param identifier The identifier to be validated.
 * @return @c true if valid; otherwise, return @c false.
 */
bool isEndpointIdValid(const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& identifier);

/**
 * Returns whether the given name follows AVS specification.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param name The friendly name to be validated.
 * @return @c true if valid; otherwise, return @c false.
 */
bool isFriendlyNameValid(const std::string& name);

/**
 * Returns whether the given description follows AVS specification.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param description The description to be validated.
 * @return @c true if valid; otherwise, return @c false.
 */
bool isDescriptionValid(const std::string& description);

/**
 * Returns whether the given manufacturer name follows AVS specification.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param manufacturerName The manufacturer name to be validated.
 * @return @c true if valid; otherwise, return @c false.
 */
bool isManufacturerNameValid(const std::string& manufacturerName);

/**
 * Returns whether the given attributes follows AVS specification.
 *
 * See format specification here:
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param attributes The attributes to be validated.
 * @return @c true if all attributes are valid; otherwise, return @c false.
 */
bool isAdditionalAttributesValid(
    const avsCommon::avs::AVSDiscoveryEndpointAttributes::AdditionalAttributes& attributes);

/**
 * Returns whether the given connections values follows AVS specification.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param connections The list of maps of connections objects
 * @return @c true if valid; otherwise, return @c false.
 */
bool areConnectionsValid(const std::vector<std::map<std::string, std::string>>& connections);

/**
 * Returns whether the given cookies follow the AVS specification.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
 *
 * @param cookies The map of cookies name and values.
 * @return @c true if valid; otherwise, return @c false.
 */
bool areCookiesValid(const std::map<std::string, std::string>& cookies);

}  // namespace endpoints
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTATTRIBUTEVALIDATION_H_
