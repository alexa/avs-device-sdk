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

#ifndef ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_DEFAULTENDPOINTBUILDER_H_
#define ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_DEFAULTENDPOINTBUILDER_H_

#include <memory>
#include <string>

#include <acsdkManufactory/Annotated.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <Alexa/AlexaInterfaceMessageSenderInternalInterface.h>
#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/SDKInterfaces/AlexaInterfaceMessageSenderInterface.h>
#include "AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h"
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

#include "Endpoints/EndpointBuilder.h"

namespace alexaClientSDK {
namespace endpoints {

/**
 * Interface for the default endpoint builder. This class wraps the @c EndpointBuilder with additional
 * functionality specific to the default endpoint, e.g. blocking further attribute modifications after construction.
 *
 * The builder is responsible for configuring and building the default endpoint object. Once built,
 * @c EndpointRegistrationManagerInterface should be used to register the endpoint with AVS
 * for it to be ready to use.
 */
class DefaultEndpointBuilder : public avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface {
public:
    /// Alias to improve readability.
    /// @{
    using EndpointAttributes = avsCommon::avs::AVSDiscoveryEndpointAttributes;
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;
    using EndpointInterface = avsCommon::sdkInterfaces::endpoints::EndpointInterface;
    using CapabilityConfiguration = avsCommon::avs::CapabilityConfiguration;
    using DirectiveHandlerInterface = avsCommon::sdkInterfaces::DirectiveHandlerInterface;
    using DefaultEndpointAnnotation = avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation;
    /// @}

    /**
     * Forwards the @c annotated EndpointBuilderInterface as an annotated @c EndpointCapabilitiesRegistrarInterface
     * instance to build and register capabilities for the default endpoint.
     *
     * @param builder The @c EndpointBuilderInterface to forward.
     * @return The forwarded @c EndpointCapabilitiesRegistrarInterface instance.
     */
    static acsdkManufactory::Annotated<DefaultEndpointAnnotation, EndpointCapabilitiesRegistrarInterface>
    createDefaultEndpointCapabilitiesRegistrarInterface(
        acsdkManufactory::Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface> builder);

    /**
     * Factory that creates the annotated @c EndpointBuilderInterface for the default endpoint.
     *
     * @param deviceInfo Structure with information about the Alexa client device.
     * @param contextManager Object used to retrieve the current state of an endpoint.
     * @param exceptionSender Object used to send exceptions.
     * @param alexaInterfaceMessageSender Object used to send AlexaInterface events.
     * @return A pointer to the annotated builder if it succeeds; otherwise, return @c nullptr.
     */
    static acsdkManufactory::Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface>
    createDefaultEndpointBuilderInterface(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        const std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSenderInternalInterface>&
            alexaMessageSender);

    /**
     * Destructor.
     */
    virtual ~DefaultEndpointBuilder();

    /// @name @c EndpointBuilderInterface methods.
    /// @{
    DefaultEndpointBuilder& withDerivedEndpointId(const std::string& suffix) override;
    DefaultEndpointBuilder& withDeviceRegistration() override;
    DefaultEndpointBuilder& withEndpointId(const EndpointIdentifier& endpointId) override;
    DefaultEndpointBuilder& withFriendlyName(const std::string& friendlyName) override;
    DefaultEndpointBuilder& withDescription(const std::string& description) override;
    DefaultEndpointBuilder& withManufacturerName(const std::string& manufacturerName) override;
    DefaultEndpointBuilder& withDisplayCategory(const std::vector<std::string>& displayCategories) override;
    DefaultEndpointBuilder& withAdditionalAttributes(
        const std::string& manufacturer,
        const std::string& model,
        const std::string& serialNumber,
        const std::string& firmwareVersion,
        const std::string& softwareVersion,
        const std::string& customIdentifier) override;
    DefaultEndpointBuilder& withConnections(
        const std::vector<std::map<std::string, std::string>>& connections) override;
    DefaultEndpointBuilder& withCookies(const std::map<std::string, std::string>& cookies) override;
    DefaultEndpointBuilder& withPowerController(
        std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerInterface> powerController,
        bool isProactivelyReported,
        bool isRetrievable) override;
    DefaultEndpointBuilder& withToggleController(
        std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface> toggleController,
        const std::string& instance,
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false) override;
    DefaultEndpointBuilder& withEndpointCapabilitiesBuilder(
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesBuilderInterface>&
            endpointCapabilitiesBuilder) override;
    DefaultEndpointBuilder& withModeController(
        std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerInterface> modeController,
        const std::string& instance,
        const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false) override;
    DefaultEndpointBuilder& withRangeController(
        std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface> rangeController,
        const std::string& instance,
        const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
        bool isProactivelyReported,
        bool isRetrievable,
        bool isNonControllable = false) override;
    DefaultEndpointBuilder& withCapability(
        const CapabilityConfiguration& configuration,
        std::shared_ptr<DirectiveHandlerInterface> directiveHandler) override;
    DefaultEndpointBuilder& withCapability(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface,
        std::shared_ptr<DirectiveHandlerInterface> directiveHandler) override;
    DefaultEndpointBuilder& withCapabilityConfiguration(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface)
        override;
    std::unique_ptr<EndpointInterface> build() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param builder The configured @c EndpointBuilder to wrap.
     */
    explicit DefaultEndpointBuilder(std::unique_ptr<EndpointBuilder> builder);

    /// The internal @c EndpointBuilder implementation.
    std::unique_ptr<EndpointBuilder> m_builder;
};

}  // namespace endpoints
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ENDPOINTS_INCLUDE_ENDPOINTS_DEFAULTENDPOINTBUILDER_H_
