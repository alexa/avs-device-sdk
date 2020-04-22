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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_UTILS_DISCOVERYUTILS_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_UTILS_DISCOVERYUTILS_H_

#include <map>
#include <utility>
#include <string>
#include <vector>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace utils {

/**
 * Validates if all the required fields are available for the given @c CapabilityConfiguration.
 *
 * @param capabilityConfig The @c CapabilityConfiguration to validate.
 * @return True if the capabilityConfig is valid, else false.
 */
bool validateCapabilityConfiguration(const avsCommon::avs::CapabilityConfiguration& capabilityConfig);

/**
 * Validates if the given @c EndpointAttributes contains all the required fields.
 *
 * @param endpointAttributes The input @c EndpointAttributes to validate.
 * @return True if the endpointAttributes are valid, else false.
 */
bool validateEndpointAttributes(const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes);

/**
 * Formats the given @c EndpointAttributes and @c CapabilityConfigurations into a JSON required to send in the
 * @c Discovery.AddOrUpdateReport event.
 *
 * @param endpointAttributes The @c EndpointAttributes to format.
 * @param capabilities The list of @c CapabilityConfigurations to format.
 * @return The JSON formatted Endpoint configuration.
 */
std::string getEndpointConfigJson(
    const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
    const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities);

/**
 * Formats the @c Discovery.AddOrUpdateReport event.
 *
 * @param endpointConfigurations The endpointConfiguration jsons to be included in the event.
 * @param authToken The authorization token that needs to be included in the event.
 * @return The string pair containing the JSON formatted Discovery.AddOrUpdateReport event and the
 * eventCorrelationToken.
 */
std::pair<std::string, std::string> getAddOrUpdateReportEventJson(
    const std::vector<std::string>& endpointConfigurations,
    const std::string& authToken);

/**
 * Formats the endpoint ID into endpointConfig JSON that will be sent in the payload of @c Discovery.DeleteReport
 * event.
 *
 * @param endpointId The endpointId for which the endpoint config JSON will be created.
 * @return The string representing the individual endpointConfig JSON for the @c Discovery.DeleteReport event.
 */
std::string getDeleteReportEndpointConfigJson(const std::string& endpointId);

/**
 * Formats the @c Discovery.DeleteReport event.
 *
 * @param endpointConfigurations The endpointConfiguration jsons to be included in the event.
 * @param authToken The authorization token that needs to be included in the event.
 * @return The string containing the JSON formatted Discovery.DeleteReport event.
 */
std::string getDeleteReportEventJson(
    const std::vector<std::string>& endpointConfigurations,
    const std::string& authToken);

}  // namespace utils
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_UTILS_DISCOVERYUTILS_H_
