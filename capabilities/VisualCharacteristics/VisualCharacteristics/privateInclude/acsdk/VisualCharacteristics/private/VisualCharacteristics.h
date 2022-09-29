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

#ifndef ACSDK_VISUALCHARACTERISTICS_PRIVATE_VISUALCHARACTERISTICS_H_
#define ACSDK_VISUALCHARACTERISTICS_PRIVATE_VISUALCHARACTERISTICS_H_

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/StateProviderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateObserverInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace alexaClientSDK {
namespace visualCharacteristics {

/**
 * This class implements a @c CapabilityConfigurationInterface that publish viewport characteristic data.
 * This set of data includes all necessary information about its windows configuration. There are three APIs:
 * Alexa.Display: The display interface expresses explicitly the raw properties of a display.
 * Alexa.Display.Window: An expression of windows that may be created on a display.
 * Alexa.InteractionMode: Expression of interaction modes that the device intends to support.
 */
class VisualCharacteristics
        : public avsCommon::avs::CapabilityAgent
        , public visualCharacteristicsInterfaces::VisualCharacteristicsInterface
        , public presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public std::enable_shared_from_this<VisualCharacteristics>
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Create an instance of @c VisualCharacteristics using the given configuration.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param exceptionSender The @c ExceptionEncounteredSenderInterface used to report exceptions to AVS.
     * @param configuration The @c VisualCharacteristicsConfiguration to use
     *
     * @return Shared pointer to the instance of @VisualCharacteristics capability agent.
     */
    static std::shared_ptr<VisualCharacteristics> createWithConfiguration(
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const visualCharacteristicsInterfaces::VisualCharacteristicsConfiguration& configuration);

    /**
     * Create an instance of @c VisualCharacteristics using configuration from the configuration node
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param exceptionSender The @c ExceptionEncounteredSenderInterface used to report exceptions to AVS.
     *
     * @return Shared pointer to the instance of @VisualCharacteristics capability agent.
     */
    static std::shared_ptr<VisualCharacteristics> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender);

    /**
     * Destructor.
     */
    virtual ~VisualCharacteristics() = default;

    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override{};
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override{};
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override{};
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override{};
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, unsigned int stateRequestToken)
        override;
    /// @}

    /// @name VisualCharacteristicsInterface Functions
    /// @{
    std::vector<visualCharacteristicsInterfaces::WindowTemplate> getWindowTemplates() override;
    std::vector<visualCharacteristicsInterfaces::InteractionMode> getInteractionModes() override;
    visualCharacteristicsInterfaces::DisplayCharacteristics getDisplayCharacteristics() override;
    void setWindowInstances(
        const std::vector<visualCharacteristicsInterfaces::WindowInstance>& instances,
        const std::string& defaultWindowInstanceId) override;
    bool addWindowInstance(const visualCharacteristicsInterfaces::WindowInstance& instance) override;
    bool removeWindowInstance(const std::string& windowInstanceId) override;
    void updateWindowInstance(const visualCharacteristicsInterfaces::WindowInstance& instance) override;
    bool setDefaultWindowInstance(const std::string& windowInstanceId) override;
    /// @}

    /// @name PresentationOrchestratorStateObserverInterface Functions
    /// @{
    void onStateChanged(
        const std::string& windowId,
        const presentationOrchestratorInterfaces::PresentationMetadata& metadata) override;
    /// @}

    /**
     * Sets the executor for this module. Note that this method will be used only for test.
     *
     * @param executor Pointer to the executor instance.
     */
    void setExecutor(const std::shared_ptr<avsCommon::utils::threading::Executor>& executor);

private:
    /**
     * Constructor.
     */
    VisualCharacteristics(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionSender,
        const visualCharacteristicsInterfaces::VisualCharacteristicsConfiguration& configuration);

    /**
     * Creates the VisualCharacteristics interface configuration.
     */
    static bool getVisualCharacteristicsCapabilityConfiguration(
        visualCharacteristicsInterfaces::VisualCharacteristicsConfiguration& configuration);

    /**
     * Validate Visual Characteristics configuration.
     * @param configuration Instance of @c VisualCharacteristicsConfiguration.
     *
     * @return True if validation is successful
     */
    static bool validateConfiguration(
        const visualCharacteristicsInterfaces::VisualCharacteristicsConfiguration& configuration);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Generate window states context json.
     *
     * @param stateJson [ out ] Generated payload
     */
    void generateWindowStateContext(std::string& stateJson);

    /**
     * Initialize instance of CapabilityConfiguration with visual characteristic configuration.
     */
    void initializeCapabilityConfiguration();

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// This is the worker thread for the @c VisualCharacteristics CA.
    std::shared_ptr<avsCommon::utils::threading::Executor> m_executor;

    /// An immutable instance of @c VisualCharacteristicsConfiguration
    const visualCharacteristicsInterfaces::VisualCharacteristicsConfiguration m_visualCharacteristicsConfiguration;

    /// Collection of @c WindowInstance indexed by windowId
    std::unordered_map<std::string, visualCharacteristicsInterfaces::WindowInstance> m_windowInstances;

    /// Default Window ID
    std::string m_defaultWindowId;

    /// A map to store tokens served by every window. <Key: windowId, Value: Token>
    std::unordered_map<std::string, std::string> m_tokenPerWindow;
};

}  // namespace visualCharacteristics
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALCHARACTERISTICS_PRIVATE_VISUALCHARACTERISTICS_H_
