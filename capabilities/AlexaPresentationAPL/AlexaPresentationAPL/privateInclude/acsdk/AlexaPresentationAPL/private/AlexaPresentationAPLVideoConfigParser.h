/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ACSDK_ALEXAPRESENTATIONAPL_PRIVATE_ALEXAPRESENTATIONAPLVIDEOCONFIGPARSER_H_
#define ACSDK_ALEXAPRESENTATIONAPL_PRIVATE_ALEXAPRESENTATIONAPLVIDEOCONFIGPARSER_H_

#include <string>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include <acsdk/APLCapabilityCommonInterfaces/APLVideoConfiguration.h>

namespace alexaClientSDK {
namespace aplCapabilityAgent {

/**
 * Class to serialize and parse @c VideoSettings corresponding to Alexa.Presentation.APL.Video interface
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl-video.html
 */
class AlexaPresentationAPLVideoConfigParser {
public:
    /**
     * Serialize APL video settings into reportable json format.
     * @param videoSettings  @c VideoSettings to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    static bool serializeVideoSettings(
        const aplCapabilityCommonInterfaces::VideoSettings& videoSettings,
        std::string& serializedJson);

    /**
     * Parse video settings from configuration.
     * @param config  @c ConfigurationNode for APL video settings
     * @param videoSettings [ out ] Parsed object for @c VideoSettings.
     *
     * @return True if parsing successful, false otherwise.
     */
    static bool parseVideoSettings(
        const avsCommon::utils::configuration::ConfigurationNode& config,
        aplCapabilityCommonInterfaces::VideoSettings& videoSettings);
};

}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAPRESENTATIONAPL_PRIVATE_ALEXAPRESENTATIONAPLVIDEOCONFIGPARSER_H_