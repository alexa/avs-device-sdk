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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACEMESSAGESENDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACEMESSAGESENDER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {

/**
 * Implementation of AlexaInterfaceMessageSenderInterface and AlexaInterfaceMessageSenderInternalInterface.  This
 * class implementes centralized messaging for any AlexaInterface capability that needs to send AlexaInterface events.
 */
class AlexaInterfaceMessageSender
        : public AlexaInterfaceMessageSenderInternalInterface
        , public avsCommon::sdkInterfaces::ContextManagerObserverInterface
        , public avsCommon::sdkInterfaces::ContextRequesterInterface
        , public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaInterfaceMessageSender> {
public:
    /**
     * Destructor.
     */
    ~AlexaInterfaceMessageSender();

    /**
     * Factory method to create a @c AlexaInterfaceMessageSender.
     *
     * @param contextManager Interface to access AVS state.
     * @param connectionManager Interface to send events to AVS.
     * @param shutdownNotifier Interface to notify of shutdown.
     * @return A new instance of @c AlexaInterfaceMessageSender on success, @c nullptr otherwise.
     */
    static std::shared_ptr<AlexaInterfaceMessageSender> createAlexaInterfaceMessageSender(
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier);

    /**
     * Factory method to forward an instance of @c AlexaInterfaceMessageSender to @c
     * AlexaInterfaceMessageSenderInternalInterface.
     *
     * @param messageSender The instance to forward.
     * @return A forwarded instance of @c AlexaInterfaceMessageSenderInternalInterface.
     */
    static std::shared_ptr<AlexaInterfaceMessageSenderInternalInterface>
    createAlexaInterfaceMessageSenderInternalInterface(
        const std::shared_ptr<AlexaInterfaceMessageSender>& messageSender);

    /**
     * Factory method to create a @c AlexaInterfaceMessageSender.
     *
     * @deprecated Use createAlexaInterfaceMessageSender.
     * @param contextManager Interface to access AVS state.
     * @param messageSender Interface to send events to AVS.
     * @return A new instance of @c AlexaInterfaceMessageSender on success, @c nullptr otherwise.
     */
    static std::shared_ptr<AlexaInterfaceMessageSender> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// @name ContextManagerObserverInterface methods
    /// @{
    void onStateChanged(
        const avsCommon::avs::CapabilityTag& identifier,
        const avsCommon::avs::CapabilityState& state,
        const avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    /// @}

    /// @name ContextRequesterInterface methods
    /// @{
    void onContextAvailable(
        const std::string& endpointId,
        const avsCommon::avs::AVSContext& endpointContext,
        avsCommon::sdkInterfaces::ContextRequestToken token) override;
    void onContextFailure(
        const avsCommon::sdkInterfaces::ContextRequestError error,
        avsCommon::sdkInterfaces::ContextRequestToken token) override;
    /// @}

    /// @name MessageRequestObserverInterface methods
    /// @{
    void onSendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    void onExceptionReceived(const std::string& exceptionMessage) override;
    /// @}

    /// @name AlexaInterfaceMessageSenderInternalInterface methods
    /// @{
    virtual bool sendStateReportEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint) override;
    virtual bool sendResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const std::string& jsonPayload = "{}") override;
    virtual bool sendResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const std::string& responseNamespace,
        const std::string& responseName,
        const std::string& jsonPayload = "{}") override;
    virtual bool sendErrorResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const ErrorResponseType errorType,
        const std::string& errorMessage = "") override;
    bool sendErrorResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const std::string& responseNamespace,
        const std::string& jsonPayload = "{}") override;
    virtual bool sendDeferredResponseEvent(
        const std::string& instance,
        const std::string& correlationToken,
        const int estimatedDeferralInSeconds = 0) override;
    /// @}

private:
    // Hold Alexa.Response event details until we have context.
    struct ResponseData {
        /**
         * Constructor.
         *
         * @param type The type of response event.
         * @param instance The instance ID of the responding capability.
         * @param correlationToken The correlation token from the directive to which we are responding.
         * @param endpoint The @c AVSMessageEndpoint to identify the endpoint related to this event.
         * @param jsonPayload a JSON string representing the payload for the response event.
         */
        ResponseData(
            const std::string& type,
            const std::string& instance,
            const std::string& correlationToken,
            const avsCommon::avs::AVSMessageEndpoint& endpoint,
            const std::string& jsonPayload,
            const std::string& responseNamespace);

        /// The type of response event.
        const std::string type;

        /// The instance ID of the responding capability.
        const std::string instance;

        /// The correlation token from the directive to which we are responding.
        const std::string correlationToken;

        /// The endpoint related to this event.
        const avsCommon::avs::AVSMessageEndpoint endpoint;

        /// JSON string representing the payload for the response event.
        const std::string jsonPayload;

        /// The namespace of response event.
        const std::string responseNamespace;
    };

    // Hold Alexa.ChangeReport event details until we have context.
    struct ChangeReportData {
        /**
         * Constructor.
         *
         * @param tag Identifies the source of the state change.
         * @param state The new state.
         * @param cause The reason for the state change.
         */
        ChangeReportData(
            const avsCommon::avs::CapabilityTag& tag,
            const avsCommon::avs::CapabilityState& state,
            const avsCommon::sdkInterfaces::AlexaStateChangeCauseType& cause);

        /// Tag that identifies the source of the state change.
        const avsCommon::avs::CapabilityTag tag;

        /// The new state of the capability.
        const avsCommon::avs::CapabilityState state;

        /// The reason for the state change.
        const avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause;
    };

    /**
     * Constructor.
     *
     * @param contextManager Interface to access AVS state.
     * @param messageSender Interface to send events to AVS.
     */
    AlexaInterfaceMessageSender(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /**
     * Initialize the message sender.  Must be called before calling any other interfaces.
     *
     * @return true on success, false on failure.
     */
    bool initialize();

    // @name RequiresShutdown method
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Alexa.Response and Alexa.StateReport events have nearly identical formats.
     *
     * @param type The type of response event.
     * @param instance The instance ID of the responding capability.
     * @param correlationToken The correlation token from the directive to which we are responding.
     * @param endpoint The @c AVSMessageEndpoint to identify the endpoint related to this event.
     * @param jsonPayload a JSON string representing the payload for the response event (optional).
     * @return true if the event was successfuly enqueued, false otherwise.
     */
    bool sendCommonResponseEvent(
        const std::string& type,
        const std::string& instance,
        const std::string& correlationToken,
        const avsCommon::avs::AVSMessageEndpoint& endpoint,
        const std::string& jsonPayload = "{}",
        const std::string& responseNamespace = "");

    /**
     * Complete sending of an Alexa.Response event.  Called from the executor context.  If there was a fatal error
     * retreiving the context, this will be called with no context at all.
     *
     * @param event Pointer to the event to complete.
     * @param context Optional context for the event.
     */
    void completeResponseEvent(
        const std::shared_ptr<ResponseData>& event,
        const avsCommon::utils::Optional<avsCommon::avs::AVSContext>& context =
            avsCommon::utils::Optional<avsCommon::avs::AVSContext>());

    /**
     * Called by the base class to complete sending of the event.  Called from the executor context.  If there was a
     * fatal error retreiving the context, this will be called with no context at all.
     *
     * @param event Pointer to the event to complete.
     * @param context Context for the event.
     */
    void completeChangeReportEvent(
        const std::shared_ptr<ChangeReportData>& event,
        const avsCommon::avs::AVSContext& context);

    /**
     * Utility function to send events.
     *
     * @param eventJson the event JSON to send.
     */
    void sendEvent(const std::string& eventJson);

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// Map of in-flight response events that are waiting on the ContextManager.
    std::map<avsCommon::sdkInterfaces::ContextRequestToken, std::shared_ptr<ResponseData>> m_pendingResponses;

    /// Map of in-flight change report events that are waiting on the ContextManager.
    std::map<avsCommon::sdkInterfaces::ContextRequestToken, std::shared_ptr<ChangeReportData>> m_pendingChangeReports;

    /// An executor used for serializing requests on a standalone thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACEMESSAGESENDER_H_
