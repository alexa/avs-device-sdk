/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_INCLUDE_INTERACTIONMODEL_INTERACTIONMODELCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_INCLUDE_INTERACTIONMODEL_INTERACTIONMODELCAPABILITYAGENT_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace interactionModel {

/**
 * The Interaction Model Capability Agent provides a way for AVS cloud initiated actions to be
 * executed by the client.
 *
 * When AVS requires the client to execute an action, it will send an InteractionModel.NewDialogRequest directive to
 * set a @c dialogRequestId on the @c DirectiveSequencer.
 *
 * Once @c dialogRequestId is set, the @c DirectiveSequencer can then expect directives with the actions
 * tagged with the dialogRequestId.
 */
class InteractionModelCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface {
public:
    /**
     * Creates an instance of the Interaction Model capability agent.
     *
     * @param directiveSequencer The Directive Sequencer responsible for processing AVS directives.
     * @param exceptionEncounteredSender The object to use for sending AVS Exception messages.
     */
    static std::shared_ptr<InteractionModelCapabilityAgent> create(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Destructor.
     */
    ~InteractionModelCapabilityAgent();

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param directiveSequencer The Directive Sequencer responsible for processing AVS directives.
     * @param exceptionEncounteredSender The object to use for sending AVS Exception messages.
     */
    InteractionModelCapabilityAgent(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Helper function to process the incoming directive
     *
     * @param info Directive to be processed.
     * @param[out] errMessage Error message associated to failure to process the directive
     * @param[out] type Error type associated to failure to process the directive
     * @return A bool indicating the success of processing the directive
     */
    bool handleDirectiveHelper(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        std::string* errMessage,
        avsCommon::avs::ExceptionErrorType* type);

    /// Set of capability configurations that will get published using Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Pointer to the Directive Sequencer responsible for processing AVS directives
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;
};
}  // namespace interactionModel
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_INCLUDE_INTERACTIONMODEL_INTERACTIONMODELCAPABILITYAGENT_H_
