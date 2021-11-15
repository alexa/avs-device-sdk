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

#ifndef ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTBUILDER_H_
#define ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTBUILDER_H_

#include <list>
#include <memory>
#include <string>

#include <Alexa/AlexaInterfaceMessageSender.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationManagerInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>

namespace alexaClientSDK {
namespace endpoints {

/**
 * Interface for an endpoint builder.
 *
 * For the default endpoint, use @c DefaultEndpointBuilder.
 *
 * The builder is responsible for configuring and building an endpoint object. Once built,
 * @c EndpointRegistrationManagerInterface should be used to register the endpoint with AVS
 * for it to be ready to use.
 *
 */
class EndpointBuilder : public avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface {
public:
    /// Alias to improve readability.
    /// @{
    using EndpointAttributes = avsCommon::avs::AVSDiscoveryEndpointAttributes;
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using EndpointInterface = avsCommon::sdkInterfaces::endpoints::EndpointInterface;
    using CapabilityConfiguration = avsCommon::avs::CapabilityConfiguration;
    using DirectiveHandlerInterface = avsCommon::sdkInterfaces::DirectiveHandlerInterface;
    /// @}

    /**
     * Creates an EndpointBuilder.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param contextManager Object used to retrieve the current state of an endpoint.
     * @param exceptionSender Object used to send exceptions.
     * @param alexaInterfaceMessageSender Object used to send AlexaInterface events.
     * @return A pointer to the new builder if it succeeds; otherwise, return @c nullptr.
     */
    static std::unique_ptr<EndpointBuilder> create(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        const std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSenderInternalInterface>&
            alexaMessageSender);

    /**
     * Creates an EndpointBuilder.
     *
     * @deprecated
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param contextManager Object used to retrieve the current state of an endpoint.
     * @param exceptionSender Object used to send exceptions.
     * @param alexaInterfaceMessageSender Object used to send AlexaInterface events.
     * @return A pointer to the new builder if it succeeds; otherwise, return @c nullptr.
     */
    static std::unique_ptr<EndpointBuilder> create(
        const avsCommon::utils::DeviceInfo& deviceInfo,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender);

    /**
     * Destructor.
     */
    virtual ~EndpointBuilder();

    /// @name @c EndpointBuilderInterface methods.
    /// @{
    EndpointBuilder& withDerivedEndpointId(const std::string& suffix) override;
    EndpointBuilder& withDeviceRegistration() override;
    EndpointBuilder& withEndpointId(const EndpointIdentifier& endpointId) override;
    EndpointBuilder& withFriendlyName(const std::string& friendlyName) override;
    EndpointBuilder& withDescription(const std::string& description) override;
    EndpointBuilder& withManufacturerName(const std::string& manufacturerName) override;
    EndpointBuilder& withDisplayCategory(const std::vector<std::string>& displayCategories) override;
    EndpointBuilder& withAdditionalAttributes(
        const std::string& manufacturer,
        const std::string& model,
        const std::string& serialNumber,
        const std::string& firmwareVersion,
        const std::string& softwareVersion,
        const std::string& customIdentifier) override;
    EndpointBuilder& withConnections(const std::vector<std::map<std::string, std::string>>& connections) override;
    EndpointBuilder& withCookies(const std::map<std::string, std::string>& cookies) override;
    EndpointBuilder& withPowerController(
        std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerInterface> powerController,
        bool isProactivelyReported,
        bool isRetrievable) override;
    EndpointBuilder& withToggleController(
        std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface> toggleController,
        const std::string& instance,
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false) override;
    EndpointBuilder& withEndpointCapabilitiesBuilder(
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface>&
            endpointCapabilitiesBuilder) override;
    EndpointBuilder& withModeController(
        std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerInterface> modeController,
        const std::string& instance,
        const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false) override;
    EndpointBuilder& withRangeController(
        std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface> rangeController,
        const std::string& instance,
        const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false) override;
    EndpointBuilder& withCapability(
        const CapabilityConfiguration& configuration,
        std::shared_ptr<DirectiveHandlerInterface> directiveHandler) override;
    EndpointBuilder& withCapability(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface,
        std::shared_ptr<DirectiveHandlerInterface> directiveHandler) override;
    EndpointBuilder& withCapabilityConfiguration(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface)
        override;
    std::unique_ptr<EndpointInterface> build() override;
    /// @}

    /**
     * Finalize attributes.
     *
     * Once called, this builder will no longer allow endpoint attribute related configurations to be set. This allows
     * applications to add more capabilities to the endpoint without changing its attributes.
     */
    void finalizeAttributes();

private:
    /// @c DefaultEndpointBuilder wraps this class to provide functionality specific to building the default endpoint.
    friend class DefaultEndpointBuilder;

    /// Defines a function that can be used to build a capability.
    using CapabilityBuilder =
        std::function<std::pair<CapabilityConfiguration, std::shared_ptr<DirectiveHandlerInterface>>()>;

    /**
     * Constructor.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param contextManager Object used to retrieve the current state of an endpoint.
     * @param exceptionSender Object used to send exceptions.
     * @param alexaInterfaceMessageSender Object used to send AlexaInterface events.
     */
    EndpointBuilder(
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSenderInternalInterface> alexaMessageSender);

    /**
     * Configures the endpoint builder for the default endpoint.
     */
    void configureDefaultEndpoint();

    /**
     * Implements the build logic used by @c build().
     *
     * @return A unique pointer to an endpoint if the build succeeds; otherwise, nullptr.
     */
    std::unique_ptr<EndpointInterface> buildImplementation();

    /// Flag used to indicate whether attribute configuration has been finalized.
    bool m_isConfigurationFinalized;

    /// Flag used to indicate whether this builder has already been used to build an endpoint.
    bool m_hasBeenBuilt;

    /// Flag used to indicate whether any unrecoverable error was found.
    bool m_invalidConfiguration;

    /// Flag used to indicate whether the builder is building the default endpoint, as some validation
    /// rules apply differently.
    bool m_isDefaultEndpoint;

    /// The client endpoint id that is used to build the default endpoint and generate derived endpoints.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// The attributes used to build the endpoint.
    EndpointAttributes m_attributes;

    /// The context manager object that is used during the create of many capability agents.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The exception sender object that is used during the create of many capability agents.
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> m_exceptionSender;

    /// The AlexaInterface message sender object that is used during the create of many capability agents.
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSenderInternalInterface> m_alexaMessageSender;

    /// List of capability builders. This is used for capabilities that depend on other endpoint attributes.
    std::list<CapabilityBuilder> m_capabilitiesBuilders;

    /// List of capabilities that were injected.
    std::list<CapabilityConfiguration> m_capabilityConfigurations;

    /// List of directive handlers that were injected.
    std::list<std::shared_ptr<DirectiveHandlerInterface>> m_directiveHandlers;

    /// List of objects that require shutdown when endpoint is destroyed.
    std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>> m_requireShutdownObjects;
};

}  // namespace endpoints
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_ENDPOINTBUILDER_H_
