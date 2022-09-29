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

#ifndef ACSDK_VISUALCHARACTERISTICS_PRIVATE_VCCONFIGPARSER_H_
#define ACSDK_VISUALCHARACTERISTICS_PRIVATE_VCCONFIGPARSER_H_

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

#include <unordered_map>

namespace alexaClientSDK {
namespace visualCharacteristics {
class VCConfigParser {
public:
    /**
     * Parse interaction modes from visual characteristics configuration.
     * @param config  @c ConfigurationNode for InteractionMode
     * @param interactionMode [ out ] Parsed object for @c InteractionMode.
     *
     * @return True if parsing successful, false otherwise.
     */
    static bool parseInteractionMode(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        visualCharacteristicsInterfaces::InteractionMode& interactionMode);

    /**
     * Parse window template from visual characteristics configuration.
     * @param config  @c ConfigurationNode for template
     * @param windowTemplate [ out ] Parsed object for @c WindowTemplate.
     *
     * @return True if parsing successful, false otherwise.
     */
    static bool parseWindowTemplate(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        visualCharacteristicsInterfaces::WindowTemplate& windowTemplate);

    /**
     * Parse display characteristics from visual characteristics configuration.
     * @param config  @c ConfigurationNode for display characteristics
     * @param display [ out ] Parsed object for @c DisplayCharacteristics.
     *
     * @return True if parsing successful, false otherwise.
     */
    static bool parseDisplayCharacteristics(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        visualCharacteristicsInterfaces::DisplayCharacteristics& display);

    /**
     * Serialize interaction modes into reportable json format.
     * @param interactionModes  Collection of @c InteractionMode to be serialized
     * @param configJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    static bool serializeInteractionMode(
        const std::vector<visualCharacteristicsInterfaces::InteractionMode>& interactionModes,
        std::string& configJson);

    /**
     * Serialize window template into reportable json format.
     * @param windowTemplates  Collection of @c WindowTemplate to be serialized
     * @param configJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    static bool serializeWindowTemplate(
        const std::vector<visualCharacteristicsInterfaces::WindowTemplate>& windowTemplates,
        std::string& configJson);

    /**
     * Serialize display characteristics into reportable json format.
     * @param display Instance of @c DisplayCharacteristics to be serialized
     * @param configJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    static bool serializeDisplayCharacteristics(
        const visualCharacteristicsInterfaces::DisplayCharacteristics& display,
        std::string& configJson);

private:
    static bool stringFromConfig(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        const std::string& configKey,
        std::string& out);

    static bool intFromConfig(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        const std::string& configKey,
        int& out);

    static bool doubleFromConfig(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        const std::string& configKey,
        double& out);

    static bool supportedBoolFromConfig(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        const std::string& configKey,
        bool& out);

    template <typename T>
    static bool enumFromConfig(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        const std::string& configKey,
        const std::unordered_map<std::string, T>& enumMapping,
        T& out);

    template <typename T>
    static bool stringToEnum(const std::unordered_map<std::string, T>& mapping, const std::string& in, T& out);

    static bool dimensionFromConfig(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        const std::string& configKey,
        visualCharacteristicsInterfaces::Dimension& out);
};
}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
#endif  // ACSDK_VISUALCHARACTERISTICS_PRIVATE_VCCONFIGPARSER_H_
