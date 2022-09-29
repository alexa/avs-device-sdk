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

#ifndef ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLER_PRIVATEINCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLER_PRIVATE_ALEXALIVEVIEWCONTROLLERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLER_PRIVATEINCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLER_PRIVATE_ALEXALIVEVIEWCONTROLLERCAPABILITYAGENT_H_

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
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RTCSCNativeInterface/RtcscAppClientListenerInterface.h>
#include <RTCSCNativeInterface/RtcscAppClientInterface.h>
#include <RTCSCNativeInterface/Types/MediaConnectionState.h>
#include <RTCSCNativeInterface/Types/MediaSide.h>
#include <RTCSCNativeInterface/Types/MediaType.h>
#include <RTCSCNativeInterface/Types/Optional.h>
#include <RTCSCNativeInterface/Types/RtcscErrorCode.h>
#include <RTCSCNativeInterface/Types/SessionState.h>
#include <RTCSCNativeInterface/Types/VideoEffect.h>

#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerInterface.h>

namespace alexaClientSDK {
namespace alexaLiveViewController {

/**
 * The @c AlexaLiveViewControllerCapabilityAgent is responsible for handling Alexa.LiveViewController directives and
 * calls the @c LiveViewControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.LiveViewController Interface.
 */
class AlexaLiveViewControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public alexaLiveViewControllerInterfaces::LiveViewControllerObserverInterface
        , public rtc::native::RtcscAppClientListenerInterface
        , public std::enable_shared_from_this<AlexaLiveViewControllerCapabilityAgent> {
public:
    /// Alias to improve readability.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;

    /**
     * Create an instance of @c AlexaLiveViewControllerCapabilityAgent.
     *
     * @param endpointId An endpoint to which this capability is associated.
     * @param liveViewController An interface that this object will use to perform the live view controller operations.
     * @param messageSender The object to use for sending events.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaLiveViewControllerCapabilityAgent.
     */
    static std::shared_ptr<AlexaLiveViewControllerCapabilityAgent> create(
        EndpointIdentifier endpointId,
        std::shared_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerInterface> liveViewController,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
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

    /// @name LiveViewControllerObserverInterface Functions
    /// @{
    void onMicrophoneStateChanged(alexaLiveViewControllerInterfaces::AudioState microphoneState) override;
    void onLiveViewCleared() override;
    /// @}

    /// @name RtcscAppClientListenerInterface Functions
    /// @{
    void onSessionAvailable(const std::string& sessionId) override;
    void onSessionRemoved(const std::string& sessionId) override;
    void onError(
        rtc::native::RtcscErrorCode errorCode,
        const std::string& errorMessage,
        const rtc::native::Optional<std::string>& sessionId) override;
    void onSessionStateChanged(const std::string& sessionId, rtc::native::SessionState sessionState) override;
    void onMediaStatusChanged(
        const std::string& sessionId,
        rtc::native::MediaSide mediaSide,
        rtc::native::MediaType mediaType,
        bool enabled) override;
    void onVideoEffectChanged(
        const std::string& sessionId,
        rtc::native::VideoEffect currentVideoEffect,
        int videoEffectDurationMs) override;
    void onMediaConnectionStateChanged(const std::string& sessionId, rtc::native::MediaConnectionState state) override;
    void onFirstFrameRendered(const std::string& sessionId, rtc::native::MediaSide mediaSide) override;
    void onFirstFrameReceived(const std::string& sessionId, rtc::native::MediaType mediaType) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Set the rtcscAppClient used for communications.
     *
     * @param rtcscAppClient The @c RtcscAppClient to set
     * @note This function should only be used for testing purposes. No call to any other method should
     * be done prior to this call.
     */
    void setRtcscAppClient(const std::shared_ptr<rtc::native::RtcscAppClientInterface>& rtcscAppClient);

private:
    /**
     * Constructor.
     *
     * @param endpointId An endpoint to which this capability is associated.
     * @param liveViewController An interface that this object will use to perform the live view controller operations.
     * @param messageSender The object to use for sending events.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     */
    AlexaLiveViewControllerCapabilityAgent(
        EndpointIdentifier endpointId,
        std::shared_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerInterface> liveViewController,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
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
     * This function handles a @c StartLiveView directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleStartLiveView(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c StopLiveView directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleStopLiveView(std::shared_ptr<DirectiveInfo> info);

    /**
     * This is a state machine function to handle the StartLiveView directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void executeStartLiveViewDirective(const std::shared_ptr<DirectiveInfo> info);

    /**
     * This is a state machine function to handle the StopLiveView directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void executeStopLiveViewDirective(const std::shared_ptr<DirectiveInfo> info);

    /**
     * This is an internal function that is called when the state machine is ready to clear live view.
     */
    void executeClearLiveView();

    /**
     * Handles any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param type The @c ExceptionErrorType.
     */
    void handleUnknownDirective(std::shared_ptr<DirectiveInfo> info, avsCommon::avs::ExceptionErrorType type);

    /**
     * Call methods of @c AlexaInterfaceMessageSenderInterface based on the endpoint's response for a live view'
     * controller method call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response for a live view controller call.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const alexaLiveViewControllerInterfaces::LiveViewControllerInterface::Response& result);

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
     * Builds the configuration json required for the discovery.
     *
     * @param configuration The live view controller configuration object containing the supported display modes,
     * overlay types and overlay positions.
     * @return The json string corresponding to the configuration.
     */
    std::string buildLiveViewControllerConfigurationJson(
        const alexaLiveViewControllerInterfaces::Configuration& configuration);

    /**
     * Parse the payload received with Alexa.LiveViewController.StartLiveView directives.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     * @return unique pointer to @c StartLiveViewRequest if the payload is parseable, else nullptr.
     */
    std::unique_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerInterface::StartLiveViewRequest>
    parseStartLiveViewDirectivePayload(std::shared_ptr<DirectiveInfo> info);

    /**
     * Parse the payload received with Alexa.LiveViewController.StopLiveView directives.
     *
     * @param info The @c DirectiveInfo to read the payload string from.
     * @return true if the validation is successful, false otherwise.
     */
    bool validateStopLiveViewDirectivePayload(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function sends a live view event back to the cloud.
     *
     * @param eventName Name of the live view event
     * @param payload Payload of the live view event
     */
    void executeSendLiveViewEvent(const std::string& eventName, const std::string& payload);

    /**
     * If m_rtcscAppClient was previously set do nothing otherwise set a new instance of it
     */
    void executeInstantiateRtcscAppClient();

    /**
     * This is an internal function that is called when the state machine is ready to display live view.
     */
    void executeRenderLiveView();

    /**
     * Internal function for disconnecting a given RTC client session.
     *
     * @param sessionId the id of the session to disconnect.
     * @param disconnectCode the reason for disconnect code.
     */
    void executeDisconnectRtcscSession(
        const std::string& sessionId,
        rtc::native::RtcscAppDisconnectCode disconnectCode);

    /**
     * Internal function for determining if there is an active @c StartLiveView directive.
     * @return true if there is an active @c StartLiveView directive.
     */
    bool hasActiveLiveView();

    /**
     * This function handles the notification of the render live view callbacks to all the observers. This function
     * is intended to be used in the context of @c m_executor worker thread.
     *
     * @param isClearLiveView whether to clear the current displayed live view or not.
     */
    void executeRenderLiveViewCallbacks(bool isClearLiveView);

    /**
     * Set the microphone state
     *
     * @param enabled whether the microphone should be turned on or not
     */
    void setMicrophoneState(bool enabled);

    /// Endpoint the capability agent associated to.
    EndpointIdentifier m_endpointId;

    /// Reference to @c LiveViewControllerInterface
    std::shared_ptr<alexaLiveViewControllerInterfaces::LiveViewControllerInterface> m_liveViewController;

    /// The @c MessageSender used to sent event.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// RTCSC AppClient object
    std::shared_ptr<rtc::native::RtcscAppClientInterface> m_rtcscAppClient;

    /// The AppInfo object.
    rtc::native::AppInfo m_appInfo;

    /// The target endpoint ID obtained from StartLiveView directive.
    std::string m_targetEndpointId;

    /// The target type obtained from StartLiveView directive.
    std::string m_targetType;

    /// The sessionId obtained from the last StartLiveView directive.
    std::string m_lastSessionId;

    /// The sessionId obtained from the current StartLiveView directive.
    std::string m_currentSessionId;

    /// The concurrentTwoWayTalk obtained from the last StartLiveView directive.
    alexaLiveViewControllerInterfaces::ConcurrentTwoWayTalkState m_concurrentTwoWayTalk;

    /// The directive corresponding to the last StartLiveView directive.
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> m_lastDisplayedDirective;

    /// This is the worker thread for the @c AlexaLiveViewControllerCapabilityAgent.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

}  // namespace alexaLiveViewController
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLER_PRIVATEINCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLER_PRIVATE_ALEXALIVEVIEWCONTROLLERCAPABILITYAGENT_H_
