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

#ifndef ALEXA_CLIENT_SDK_ACSDKALEXAVIDEORECORDER_PRIVATEINCLUDE_ACSDKALEXAVIDEORECORDER_ALEXAVIDEORECORDERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_ACSDKALEXAVIDEORECORDER_PRIVATEINCLUDE_ACSDKALEXAVIDEORECORDER_ALEXAVIDEORECORDERCAPABILITYAGENT_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <rapidjson/document.h>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdkAlexaVideoRecorderInterfaces/VideoRecorderInterface.h>

namespace alexaClientSDK {
namespace acsdkAlexaVideoRecorder {

/**
 * The @c AlexaVideoRecorderCapabilityAgent is responsible for handling Alexa.VideoRecorder directives and
 * calls the @c VideoRecorderHandlerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.VideoRecorder Interface.
 */
class AlexaVideoRecorderCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaVideoRecorderCapabilityAgent> {
public:
    /**
     * Create an instance of @c AlexaVideoRecorderCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param videoRecorder An interface that this object will use to perform the video recorder operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaVideoRecorderCapabilityAgent.
     */
    static std::shared_ptr<AlexaVideoRecorderCapabilityAgent> create(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface> videoRecorder,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param videoRecorder An interface that this object will use to perform the video recorder operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     */
    AlexaVideoRecorderCapabilityAgent(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface> videoRecorder,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send the handling completed notification and clean up the resources.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Called on executor to handle any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param type The @c ExceptionErrorType.
     */
    void executeUnknownDirective(std::shared_ptr<DirectiveInfo> info, avsCommon::avs::ExceptionErrorType type);

    /**
     * Call methods of @c AlexaInterfaceMessageSenderInterface based the endpoint's response for a controller method
     * call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response of a controller call.
     */
    void sendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface::Response& result);

    /**
     * Parse the payload received with Alexa.ViewRecorder directives.
     *
     * @param jsonPayload String json payload to be parsed.
     * @return shared pointer to @c VideoRecorderRequest, else nullptr
     */
    std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest> parseDirectivePayload(
        const std::string& jsonPayload);

    /**
     * Parse a video entity object.
     * @param entityJson String json entity to be parsed
     * @param [out] videoRecorderRequest Context to be loaded with the video entity object.
     * @return true if successful.
     */
    bool parseEntityJson(
        const rapidjson::Value& entityJson,
        acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest& videoRecorderRequest);

    /**
     * Send Alexa interface error response.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param alexaErrorResponseType Alexa interface error response type.
     * @param responseMessage Alexa interface error response message.
     */
    void sendAlexaErrorResponse(
        const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType alexaErrorResponseType,
        const std::string& responseMessage);

    /**
     * Send Alexa.Video interface error response.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param alexaVideoErrorResponseType Alexa.Video interface error response type.
     * @param responseMessage Alexa.Video interface error response message.
     */
    void sendAlexaVideoErrorResponse(
        const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType
            alexaVideoErrorResponseType,
        const std::string& responseMessage);

    /// Endpoint the capability agent associated to.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Reference to @c VideoRecorderHandlerInterface.
    std::shared_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface> m_videoRecorder;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// This is the worker thread for the @c AlexaVideoRecorderCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkAlexaVideoRecorder
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_ACSDKALEXAVIDEORECORDER_PRIVATEINCLUDE_ACSDKALEXAVIDEORECORDER_ALEXAVIDEORECORDERCAPABILITYAGENT_H_
