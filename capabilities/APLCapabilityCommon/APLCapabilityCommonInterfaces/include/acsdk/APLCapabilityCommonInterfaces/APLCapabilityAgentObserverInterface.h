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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCAPABILITYAGENTOBSERVERINTERFACE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCAPABILITYAGENTOBSERVERINTERFACE_H_

#include <chrono>
#include <string>

#include "PresentationSession.h"
#include "APLCapabilityAgentInterface.h"
#include "APLTimeoutType.h"
#include "PresentationToken.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
/**
 * This @c APLCapabilityAgentObserverInterface class is used to notify observers when APL directive is received.
 */
class APLCapabilityAgentObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~APLCapabilityAgentObserverInterface() = default;

    /**
     * Used to notify the observer when an APL document is ready to be rendered, typically in response
     * to a Alexa.Presentation.APL.RenderDocument directive being received. Once called, the
     * client should render the document based on the APL specification in the payload in structured JSON format.
     *
     * @note The payload may contain customer sensitive information and should be used with utmost care.
     * Failure to do so may result in exposing or mishandling of customer data.
     *
     * @param document  JSON string containing APL document
     * @param datasource  JSON string containing data payload associated with APL
     * document
     * @param token The APL presentation token associated with this payload.
     * @param windowId The target windowId.
     * @param timeoutType The @c APLTimeoutType associated with this payload.
     * @param interfaceName The interface (namespace) which is associated with this directive.
     * @param supportedViewports string containing the json value of supported viewports.
     * @param presentationSession Configuration for presentation session
     * displaying document
     * @param receiveTime The time at which the directive was received, for more accurate telemetry.
     * @param agent Pointer to the @c APLCapabilityAgentInterface notifying the observer.
     */
    virtual void onRenderDocument(
        const std::string& document,
        const std::string& datasource,
        const PresentationToken& token,
        const std::string& windowId,
        const APLTimeoutType timeoutType,
        const std::string& interfaceName,
        const std::string& supportedViewports,
        const PresentationSession& presentationSession,
        const std::chrono::steady_clock::time_point& receiveTime,
        std::shared_ptr<APLCapabilityAgentInterface> agent) = 0;

    /**
     * Used to notify observer when @c Alexa.Presentation.APL.ExecuteCommands directive has been received.
     *
     * @param jsonPayload The payload of the Alexa.Presentation.APL.ExecuteCommands directive in structured JSON format.
     * @param token presentation token used in renderdocument
     */
    virtual void onExecuteCommands(const std::string& jsonPayload, const PresentationToken& token) = 0;

    /**
     * Used to notify observer when @c Alexa.Presentation.APL directives related to DataSource updates received.
     *
     * @param sourceType DataSource type.
     * @param jsonPayload The payload of the directive in structured JSON format.
     * @param token presentation token used in renderdocument
     */
    virtual void onDataSourceUpdate(
        const std::string& sourceType,
        const std::string& jsonPayload,
        const PresentationToken& token) = 0;

    /**
     * Used to notify observer when a show document directive has been received for an existing presentation.
     *
     * @param token presentation token used in renderdocument
     */
    virtual void onShowDocument(const PresentationToken& token) = 0;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCAPABILITYAGENTOBSERVERINTERFACE_H_
