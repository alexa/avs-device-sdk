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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIMEINTERFACES_INCLUDE_ACSDK_TEMPLATERUNTIMEINTERFACES_TEMPLATERUNTIMEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIMEINTERFACES_INCLUDE_ACSDK_TEMPLATERUNTIMEINTERFACES_TEMPLATERUNTIMEOBSERVERINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/SDKInterfaces/MediaPropertiesInterface.h>

namespace alexaClientSDK {
namespace templateRuntimeInterfaces {

/**
 * This @c TemplateRuntimeObserverInterface class is used to notify observers when a @c RenderTemplate or
 * @c RenderPlayerInfo directive is received. These two directives contains metadata for rendering display
 * cards for devices with GUI support.
 */
class TemplateRuntimeObserverInterface {
public:
    /**
     * The @c AudioPlayerInfo contains information that is useful for rendering a PlayerInfo display card.
     * @c AudioPlayerInfo is passed to the observers as a parameter in the @c renderPlayerInfoCard callback.
     */
    struct AudioPlayerInfo {
        /**
         * Default constructor.
         */
        AudioPlayerInfo() : audioPlayerState{avsCommon::avs::PlayerActivity::IDLE} {};

        /**
         * The state of the @c AudioPlayer. This information is useful for implementing the progress bar
         * in the display card.  It is assumed that the client is responsible for progressing the progress bar
         * when the @c AudioPlayer is in PLAYING state.
         */
        avsCommon::avs::PlayerActivity audioPlayerState;

        /**
         * The properties for the media that @c AudioPlayer is handling.  This information is
         * useful for implementation of the progress bar.
         */
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MediaPropertiesInterface> mediaProperties;
    };

    /**
     * Destructor
     */
    virtual ~TemplateRuntimeObserverInterface() = default;

    /**
     * Used to notify the observer when a RenderTemplate directive is received. Once called, the client should
     * render the Template display card based on the metadata provided in the payload in structured JSON format.
     *
     * @note The payload may contain customer sensitive information and should be used with utmost care.
     * Failure to do so may result in exposing or mishandling of customer data.
     *
     * @param jsonPayload The payload of the RenderTemplate directive in structured JSON format.
     */
    virtual void renderTemplateCard(const std::string& jsonPayload) = 0;

    /**
     * Used to notify the observer when a RenderPlayerInfo directive is received. Once called, the client should
     * render the PlayerInfo display card based on the metadata provided in the payload in structured JSON
     * format.
     *
     * @param jsonPayload The payload of the RenderPlayerInfo directive in structured JSON format.
     * @param audioPlayerInfo Information on the @c AudioPlayer.
     */
    virtual void renderPlayerInfoCard(
        const std::string& jsonPayload,
        TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) = 0;
};

}  // namespace templateRuntimeInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIMEINTERFACES_INCLUDE_ACSDK_TEMPLATERUNTIMEINTERFACES_TEMPLATERUNTIMEOBSERVERINTERFACE_H_
