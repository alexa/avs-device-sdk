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

#include <Alexa/AlexaInterfaceCapabilityAgent.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "Endpoints/DefaultEndpointBuilder.h"
#include "Endpoints/Endpoint.h"
#include "Endpoints/EndpointAttributeValidation.h"
#include "Endpoints/EndpointBuilder.h"

namespace alexaClientSDK {
namespace endpoints {

using namespace acsdkManufactory;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "DefaultEndpointBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

Annotated<DefaultEndpointAnnotation, EndpointCapabilitiesRegistrarInterface> DefaultEndpointBuilder::
    createDefaultEndpointCapabilitiesRegistrarInterface(
        Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface> builder) {
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<EndpointBuilderInterface> tmp = builder;
    return Annotated<DefaultEndpointAnnotation, EndpointCapabilitiesRegistrarInterface>(tmp);
}

Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface> DefaultEndpointBuilder::
    createDefaultEndpointBuilderInterface(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
        const std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSenderInternalInterface>&
            alexaMessageSender) {
    ACSDK_DEBUG5(LX(__func__));

    auto builder = EndpointBuilder::create(deviceInfo, contextManager, exceptionSender, alexaMessageSender);
    if (!builder) {
        ACSDK_ERROR(LX("createDefaultEndpointBuilderFailed"));
        return nullptr;
    }

    builder->configureDefaultEndpoint();
    builder->finalizeAttributes();

    return Annotated<DefaultEndpointAnnotation, EndpointBuilderInterface>(
        new DefaultEndpointBuilder(std::move(builder)));
}

DefaultEndpointBuilder::DefaultEndpointBuilder(std::unique_ptr<EndpointBuilder> builder) :
        m_builder{std::move(builder)} {
}

DefaultEndpointBuilder::~DefaultEndpointBuilder() {
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withDerivedEndpointId(const std::string& suffix) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withDerivedEndpointId(suffix);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withDeviceRegistration() {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withDeviceRegistration();
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withEndpointId(const EndpointIdentifier& endpointId) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withEndpointId(endpointId);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withFriendlyName(const std::string& friendlyName) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withFriendlyName(friendlyName);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withDescription(const std::string& description) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withDescription(description);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withManufacturerName(const std::string& manufacturerName) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withManufacturerName(manufacturerName);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withDisplayCategory(const std::vector<std::string>& displayCategories) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withDisplayCategory(displayCategories);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withAdditionalAttributes(
    const std::string& manufacturer,
    const std::string& model,
    const std::string& serialNumber,
    const std::string& firmwareVersion,
    const std::string& softwareVersion,
    const std::string& customIdentifier) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withAdditionalAttributes(
        manufacturer, model, serialNumber, firmwareVersion, softwareVersion, customIdentifier);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withConnections(
    const std::vector<std::map<std::string, std::string>>& connections) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withConnections(connections);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withCookies(const std::map<std::string, std::string>& cookies) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withCookies(cookies);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withPowerController(
    std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerInterface> powerController,
    bool isProactivelyReported,
    bool isRetrievable) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withPowerController(powerController, isProactivelyReported, isRetrievable);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withToggleController(
    std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface> toggleController,
    const std::string& instance,
    const avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes& toggleControllerAttributes,
    bool isProactivelyReported,
    bool isRetrievable,
    bool isNonControllable) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withToggleController(
        toggleController,
        instance,
        toggleControllerAttributes,
        isProactivelyReported,
        isRetrievable,
        isNonControllable);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withModeController(
    std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerInterface> modeController,
    const std::string& instance,
    const avsCommon::sdkInterfaces::modeController::ModeControllerAttributes& modeControllerAttributes,
    bool isProactivelyReported,
    bool isRetrievable,
    bool isNonControllable) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withModeController(
        modeController, instance, modeControllerAttributes, isProactivelyReported, isRetrievable, isNonControllable);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withRangeController(
    std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface> rangeController,
    const std::string& instance,
    const avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes& rangeControllerAttributes,
    bool isProactivelyReported,
    bool isRetrievable,
    bool isNonControllable) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withRangeController(
        rangeController, instance, rangeControllerAttributes, isProactivelyReported, isRetrievable, isNonControllable);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withEndpointCapabilitiesBuilder(
    const std::shared_ptr<EndpointCapabilitiesBuilderInterface>& endpointCapabilitiesBuilder) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withEndpointCapabilitiesBuilder(endpointCapabilitiesBuilder);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withCapability(
    const EndpointBuilder::CapabilityConfiguration& configuration,
    std::shared_ptr<DirectiveHandlerInterface> directiveHandler) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withCapability(configuration, directiveHandler);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withCapability(
    const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface,
    std::shared_ptr<DirectiveHandlerInterface> directiveHandler) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withCapability(configurationInterface, directiveHandler);
    return *this;
}

DefaultEndpointBuilder& DefaultEndpointBuilder::withCapabilityConfiguration(
    const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& configurationInterface) {
    ACSDK_DEBUG5(LX(__func__));
    m_builder->withCapabilityConfiguration(configurationInterface);
    return *this;
}

std::unique_ptr<EndpointInterface> DefaultEndpointBuilder::build() {
    return m_builder->build();
}

}  // namespace endpoints
}  // namespace alexaClientSDK
