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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EQUALIZER_INCLUDE_EQUALIZER_EQUALIZERCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EQUALIZER_INCLUDE_EQUALIZER_EQUALIZERCAPABILITYAGENT_H_

#include <memory>

#include <rapidjson/document.h>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerModeControllerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerControllerListenerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerStorageInterface.h>
#include <EqualizerImplementations/EqualizerController.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace equalizer {

/**
 * Capability Agent to handle EqualizerController AVS Interface.
 */
class EqualizerCapabilityAgent
        : public std::enable_shared_from_this<EqualizerCapabilityAgent>
        , public avsCommon::sdkInterfaces::audio::EqualizerControllerListenerInterface
        , public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler {
public:
    /**
     * Destructor.
     */
    ~EqualizerCapabilityAgent() override = default;

    /**
     * Factory method to create a capability agent instance.
     *
     * @param equalizerController An SDK's component to control equalizer state and listen to its changes.
     * @param capabilitiesDelegate Interface to control DCF configuration.
     * @param equalizerStorage Interface to store equalizer state between runs.
     * @param customerDataManager Component to register Capability Agent as customer data container (equalizer state).
     * @param exceptionEncounteredSender Interface to report exceptions to AVS.
     * @param contextManager Interface to provide equalizer state to AVS.
     * @param messageSender Interface to send events to AVS.
     * @return A new instance of @c EqualizerCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<EqualizerCapabilityAgent> create(
        std::shared_ptr<alexaClientSDK::equalizer::EqualizerController> equalizerController,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> equalizerStorage,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

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

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

    /// @name EqualizerControllerListenerInterface Functions
    /// @{
    void onEqualizerStateChanged(const avsCommon::sdkInterfaces::audio::EqualizerState& state) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param equalizerController An SDK's component to control equalizer state and listen to its changes.
     * @param capabilitiesDelegate Interface to control DCF configuration.
     * @param equalizerStorage Interface to store equalizer state between runs.
     * @param customerDataManager Component to register Capability Agent as customer data container (equalizer state).
     * @param exceptionEncounteredSender Interface to report exceptions to AVS.
     * @param contextManager Interface to provide equalizer state to AVS.
     * @param messageSender Interface to send events to AVS.
     */
    EqualizerCapabilityAgent(
        std::shared_ptr<alexaClientSDK::equalizer::EqualizerController> equalizerController,
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
        std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> equalizerStorage,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /**
     * Prepares EqualizerController Interface DCF configuration and keeps it internally.
     */
    void generateCapabilityConfiguration();

    /**
     * Performs an attempt to fix the DCF configuration desynchronization. This is introduced to handle the case when
     * AVS EqualizerController API directive contains a band or mode that is unsupported according to current
     * configuration. We assume two options:
     * - AVS bug
     * - AVS and SDK has different versions of the DCF configuration for whatever reason.
     * Here we tray to fix the second case.
     */
    void fixConfigurationDesynchronization();

    /**
     * Handles the EqualizerController.SetBands AVS Directive.
     *
     * @param info Directive details.
     * @param JSON document containing the parsed directive payload.
     * @return true if operation succeeds and could be reported as such to AVS, false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool handleSetBandsDirective(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& document);

    /**
     * Handles the EqualizerController.AdjustBands AVS Directive.
     *
     * @param info Directive details.
     * @param JSON document containing the parsed directive payload.
     * @return true if operation succeeds and could be reported as such to AVS, false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool handleAdjustBandsDirective(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& document);

    /**
     * Handles the EqualizerController.ResetBands AVS Directive.
     *
     * @param info Directive details.
     * @param JSON document containing the parsed directive payload.
     * @return true if operation succeeds and could be reported as such to AVS, false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool handleResetBandsDirective(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& document);

    /**
     * Handles the EqualizerController.SetMode AVS Directive.
     *
     * @param info Directive details.
     * @param JSON document containing the parsed directive payload.
     * @return true if operation succeeds and could be reported as such to AVS, false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool handleSetModeDirective(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        rapidjson::Document& document);

    /// An SDK's component to control equalizer state and listen to its changes.
    std::shared_ptr<alexaClientSDK::equalizer::EqualizerController> m_equalizerController;

    /// An instance of @c CapabilitiesDelegate to invalidate DCF configuration if required.
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> m_capabilitiesDelegate;

    /// Interface to store equalizer state between runs.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> m_equalizerStorage;

    /// Set of capability configurations that will get published using DCF
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// An executor used for serializing requests on agent's own thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace equalizer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EQUALIZER_INCLUDE_EQUALIZER_EQUALIZERCAPABILITYAGENT_H_
