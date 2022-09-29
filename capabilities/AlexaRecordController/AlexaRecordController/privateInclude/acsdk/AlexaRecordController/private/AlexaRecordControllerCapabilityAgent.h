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

#ifndef ACSDK_ALEXARECORDCONTROLLER_PRIVATE_ALEXARECORDCONTROLLERCAPABILITYAGENT_H_
#define ACSDK_ALEXARECORDCONTROLLER_PRIVATE_ALEXARECORDCONTROLLERCAPABILITYAGENT_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/CapabilityState.h>
#include <AVSCommon/AVS/CapabilityTag.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequestToken.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/AlexaRecordControllerInterfaces/RecordControllerInterface.h>

namespace alexaClientSDK {
namespace alexaRecordController {

/**
 * The @c AlexaRecordControllerCapabilityAgent is responsible for handling Alexa.RecordController directives and
 * calls the @c RecordControllerInterface APIs.
 *
 * This class implements a @c CapabilityAgent that handles the @c Alexa.RecordController Interface.
 */
class AlexaRecordControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AlexaRecordControllerCapabilityAgent> {
public:
    /**
     * Create an instance of @c AlexaRecordControllerCapabilityAgent.
     *
     * @param endpointId A endpoint to which this capability is associated.
     * @param recordController An interface that this object will use to perform the record controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isRetrievable Whether properties can be retrieved when AVS sends a state report request or the endpoint
     * responds with an Alexa response to a directive.
     * @return @c nullptr if the inputs are invalid, else a new instance of @c AlexaRecordControllerCapabilityAgent.
     */
    static std::shared_ptr<AlexaRecordControllerCapabilityAgent> create(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<alexaRecordControllerInterfaces::RecordControllerInterface> recordController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isRetrievable);

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken) override;
    bool canStateBeRetrieved() override;
    bool hasReportableStateProperties() override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
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
     * @param recordController An interface that this object will use to perform the record controller operations.
     * @param contextManager An interface to which this object will send property state updates.
     * @param responseSender An interface that this object will use to send the response to AVS.
     * @param exceptionSender An interface to report exceptions to AVS.
     * @param isRetrievable Whether properties can be retrieved when AVS sends a state report request or the endpoint
     * responds with an Alexa response to a directive.
     */

    AlexaRecordControllerCapabilityAgent(
        avsCommon::sdkInterfaces::endpoints::EndpointIdentifier endpointId,
        std::shared_ptr<alexaRecordControllerInterfaces::RecordControllerInterface> recordController,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> responseSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        bool isRetrievable);

    /**
     * Initializes the object.
     */
    bool initialize();

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
     * Gets the current recording state from endpoint and notifies @c ContextManager
     *
     * @param stateProviderName Provides the property name and used in the @c ContextManager methods.
     * @param contextRequestToken The token to be used when providing the response to @c ContextManager
     */
    void executeProvideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken);

    /**
     * Send event based the endpoint's response for a controller method call.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param result This contains the endpoint's response of a controller call.
     * @param recordingStateString This contains the stringified recording state (RECORDING or NOT_RECORDING) if the
     * result type is SUCCESS.
     */
    void executeSendResponseEvent(
        const std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        const alexaRecordControllerInterfaces::RecordControllerInterface::Response& result,
        const std::string& recordingStateString);

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

    /// Endpoint the capability agent is associated to.
    avsCommon::sdkInterfaces::endpoints::EndpointIdentifier m_endpointId;

    /// Reference to @c AlexaRecordControllerInterface
    std::shared_ptr<alexaRecordControllerInterfaces::RecordControllerInterface> m_recordController;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c AlexaInterfaceMessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface> m_responseSender;

    /// Whether the recording state properties can be retrieved when Alexa sends a state report request to the endpoint.
    bool m_isRetrievable;

    /// This is the worker thread for the @c AlexaRecordControllerCapabilityAgent.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alexaRecordController
}  // namespace alexaClientSDK
#endif  // ACSDK_ALEXARECORDCONTROLLER_PRIVATE_ALEXARECORDCONTROLLERCAPABILITYAGENT_H_
