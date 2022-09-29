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

#ifndef ACSDK_SAMPLE_TEMPLATERUNTIME_TEMPLATERUNTIMEPRESENTATIONADAPTEROBSERVERINTERFACE_H_
#define ACSDK_SAMPLE_TEMPLATERUNTIME_TEMPLATERUNTIMEPRESENTATIONADAPTEROBSERVERINTERFACE_H_

#include <string>

#include <acsdk/TemplateRuntimeInterfaces/TemplateRuntimeObserverInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * An contract fulfilled by user interface to present the template runtime cards.
 */
class TemplateRuntimePresentationAdapterObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~TemplateRuntimePresentationAdapterObserverInterface() = default;

    /**
     * Used to notify when a RenderTemplate presentation is ready to display. Once called, the client should
     * render the Template display card based on the metadata provided in the payload in structured JSON format.
     *
     * @note The payload may contain customer sensitive information and should be used with utmost care.
     * Failure to do so may result in exposing or mishandling of customer data.
     *
     * @param jsonPayload The payload of the RenderTemplate directive in structured JSON format.
     */
    virtual void renderTemplateCard(const std::string& jsonPayload) = 0;

    /**
     * Used to notify when a RenderPlayerInfo presentation is ready to display. Once called, the client should
     * render the PlayerInfo display card based on the metadata provided in the payload in structured JSON
     * format.
     *
     * @param jsonPayload The payload of the RenderPlayerInfo directive in structured JSON format.
     * @param audioPlayerInfo Information on the @c AudioPlayer.
     */
    virtual void renderPlayerInfoCard(
        const std::string& jsonPayload,
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) = 0;

    /**
     * Used to notify when the render template card should be cleared.
     */
    virtual void clearRenderTemplateCard() = 0;

    /**
     * Used to notify when the player info card should be cleared.
     */
    virtual void clearPlayerInfoCard() = 0;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_TEMPLATERUNTIME_TEMPLATERUNTIMEPRESENTATIONADAPTEROBSERVERINTERFACE_H_
