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

#include "ToggleController/ToggleControllerCapabilityAgent.h"

#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace toggleController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::toggleController;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
#define TAG "ToggleControllerCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.ToggleController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for TurnOn directive
static const std::string NAME_TURNON{"TurnOn"};

/// The name for TurnOff directive
static const std::string NAME_TURNOFF{"TurnOff"};

/// The name of toggleState property
static const std::string TOGGLESTATE_PROPERTY_NAME{"toggleState"};

/// Json value for ON toggle state
static const std::string TOGGLESTATE_ON{R"("ON")"};

/// Json value for OFF toggle state
static const std::string TOGGLESTATE_OFF{R"("OFF")"};

/// The capabilityResources key
static const std::string CAPABILITY_RESOURCES_KEY{"capabilityResources"};

/// The semantics key
static const std::string CAPABILITY_SEMANTICS_KEY{"semantics"};

/**
 *  Helper function to validate the toggle controller attributes.
 *
 * @param toggleControllerAttributes The toggle controller attribute to be validated.
 * @result @c true if valid, otherwise @c false.
 */
static bool isToggleControllerAttributeValid(const ToggleControllerAttributes& toggleControllerAttributes) {
    if (!toggleControllerAttributes.capabilityResources.isValid()) {
        ACSDK_ERROR(LX("isToggleControllerAttributeValidFailed").d("reason", "friendlyNamesInvalid"));
        return false;
    }
    return true;
}

std::shared_ptr<ToggleControllerCapabilityAgent> ToggleControllerCapabilityAgent::create(
    const EndpointIdentifier& endpointId,
    const std::string& instance,
    const ToggleControllerAttributes& toggleControllerAttributes,
    std::shared_ptr<ToggleControllerInterface> toggleController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable,
    bool isNonControllable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (instance.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyInstance"));
        return nullptr;
    }
    if (!toggleController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullToggleContoller"));
        return nullptr;
    }
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    if (!responseSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullResponseSender"));
        return nullptr;
    }
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    auto toggleContollerCapabilityAgent =
        std::shared_ptr<ToggleControllerCapabilityAgent>(new ToggleControllerCapabilityAgent(
            endpointId,
            instance,
            toggleControllerAttributes,
            toggleController,
            contextManager,
            responseSender,
            exceptionSender,
            isProactivelyReported,
            isRetrievable,
            isNonControllable));

    if (!toggleContollerCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return toggleContollerCapabilityAgent;
}

ToggleControllerCapabilityAgent::ToggleControllerCapabilityAgent(
    const EndpointIdentifier& endpointId,
    const std::string& instance,
    const ToggleControllerAttributes& toggleControllerAttributes,
    std::shared_ptr<ToggleControllerInterface> toggleController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable,
    bool isNonControllable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"ToggleControllerCapabilityAgent"},
        m_endpointId{endpointId},
        m_instance{instance},
        m_isProactivelyReported{isProactivelyReported},
        m_isRetrievable{isRetrievable},
        m_isNonControllable{isNonControllable},
        m_toggleControllerAttributes{toggleControllerAttributes},
        m_toggleController{toggleController},
        m_contextManager{contextManager},
        m_responseSender{responseSender} {
}

bool ToggleControllerCapabilityAgent::initialize() {
    ACSDK_DEBUG5(LX(__func__));
    if (!isToggleControllerAttributeValid(m_toggleControllerAttributes)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "invalidToggleControllerAttributes"));
        return false;
    }

    if (m_isProactivelyReported) {
        if (!m_toggleController->addObserver(shared_from_this())) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "addObserverFailed"));
            return false;
        }
    }
    if (m_isRetrievable) {
        m_contextManager->addStateProvider(
            {NAMESPACE, TOGGLESTATE_PROPERTY_NAME, m_endpointId, m_instance}, shared_from_this());
    }

    return true;
}

void ToggleControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void ToggleControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void ToggleControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.execute([this, info] {
        ACSDK_DEBUG5(LX("handleDirectiveInExecutor"));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId) ||
            (info->directive->getInstance() != m_instance)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        // Directive is not expected if property is nonControllable.
        if (m_isNonControllable) {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "propertyIsNonControllable"));
            sendExceptionEncounteredAndReportFailed(
                info, "propertyIsNonControllable", ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        std::pair<AlexaResponseType, std::string> result;
        if (directiveName == NAME_TURNON) {
            result = m_toggleController->setToggleState(true, AlexaStateChangeCauseType::VOICE_INTERACTION);
        } else if (directiveName == NAME_TURNOFF) {
            result = m_toggleController->setToggleState(false, AlexaStateChangeCauseType::VOICE_INTERACTION);
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

void ToggleControllerCapabilityAgent::provideState(
    const avsCommon::avs::CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG5(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.execute([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG5(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
    return;
}

bool ToggleControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable;
}

bool ToggleControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG5(LX(__func__));
    return (m_isRetrievable || m_isProactivelyReported);
}

void ToggleControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (!info->directive->getEndpoint().hasValue() ||
        info->directive->getEndpoint().value().endpointId != m_endpointId) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "notExpectedEndpointId"));
        return;
    }
    if (info->directive->getInstance() != m_instance) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "notExpectedInstance"));
        return;
    }
    removeDirective(info);
}

DirectiveHandlerConfiguration ToggleControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_TURNON, m_endpointId, m_instance}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_TURNOFF, m_endpointId, m_instance}] = neitherNonBlockingPolicy;

    return configuration;
}

avsCommon::avs::CapabilityConfiguration ToggleControllerCapabilityAgent::getCapabilityConfiguration() {
    ACSDK_DEBUG5(LX(__func__));
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    additionalConfigurations[CAPABILITY_RESOURCES_KEY] = m_toggleControllerAttributes.capabilityResources.toJson();
    if (m_toggleControllerAttributes.semantics.hasValue()) {
        additionalConfigurations[CAPABILITY_SEMANTICS_KEY] = m_toggleControllerAttributes.semantics.value().toJson();
    }
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          avsCommon::utils::Optional<std::string>(m_instance),
                                          avsCommon::utils::Optional<CapabilityConfiguration::Properties>(
                                              {m_isRetrievable,
                                               m_isProactivelyReported,
                                               {TOGGLESTATE_PROPERTY_NAME},
                                               avsCommon::utils::Optional<bool>(m_isNonControllable)}),
                                          additionalConfigurations};
    return configuration;
}

void ToggleControllerCapabilityAgent::onToggleStateChanged(
    const ToggleState& toggleState,
    const AlexaStateChangeCauseType cause) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_isProactivelyReported) {
        ACSDK_ERROR(LX("onToggleStateChangedFailed").d("reason", "invalidOnToggleStateChangedCall"));
        return;
    }

    m_executor.execute([this, toggleState, cause] {
        m_contextManager->reportStateChange(
            CapabilityTag(NAMESPACE, TOGGLESTATE_PROPERTY_NAME, m_endpointId, m_instance),
            buildCapabilityState(toggleState),
            cause);
    });
}

void ToggleControllerCapabilityAgent::doShutdown() {
    if (m_isProactivelyReported) {
        m_toggleController->removeObserver(shared_from_this());
    }
    m_executor.shutdown();
    m_toggleController.reset();
    m_responseSender.reset();
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider({NAMESPACE, TOGGLESTATE_PROPERTY_NAME, m_endpointId, m_instance});
    }
    m_contextManager.reset();
}

void ToggleControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info && info->directive) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void ToggleControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void ToggleControllerCapabilityAgent::executeUnknownDirective(
    std::shared_ptr<DirectiveInfo> info,
    ExceptionErrorType type) {
    ACSDK_ERROR(LX("executeUnknownDirectiveFailed")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    const std::string exceptionMessage =
        "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

    sendExceptionEncounteredAndReportFailed(info, exceptionMessage, type);
}

void ToggleControllerCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    bool isError = false;
    if (stateProviderName.endpointId != m_endpointId) {
        ACSDK_ERROR(LX("provideStateFailed")
                        .d("reason", "notExpectedEndpointId")
                        .sensitive("endpointId", stateProviderName.endpointId));
        isError = true;
    }
    if (stateProviderName.name != TOGGLESTATE_PROPERTY_NAME) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "notExpectedName").d("name", stateProviderName.name));
        isError = true;
    }
    if (!m_isRetrievable) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "provideStateOnNotRetrievableProperty"));
        isError = true;
    }

    if (isError) {
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }

    auto result = m_toggleController->getToggleState();
    if (AlexaResponseType::SUCCESS != result.first) {
        ACSDK_WARN(LX("executeProvideState").m("failedToGetPropertyValue").sensitive("reason", result.first));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, true);
    } else {
        if (!result.second.hasValue()) {
            ACSDK_ERROR(LX("executeProvideStateFailed").m("emptyToggleState"));
            m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, true);
            return;
        }
        m_contextManager->provideStateResponse(
            stateProviderName, buildCapabilityState(result.second.value()), contextRequestToken);
    }
}

void ToggleControllerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const std::pair<AlexaResponseType, std::string> result) {
    if (AlexaResponseType::SUCCESS == result.first) {
        m_responseSender->sendResponseEvent(
            info->directive->getInstance(), info->directive->getCorrelationToken(), AVSMessageEndpoint(m_endpointId));
    } else {
        m_responseSender->sendErrorResponseEvent(
            info->directive->getInstance(),
            info->directive->getCorrelationToken(),
            AVSMessageEndpoint(m_endpointId),
            m_responseSender->alexaResponseTypeToErrorType(result.first),
            result.second);
    }
}

CapabilityState ToggleControllerCapabilityAgent::buildCapabilityState(const ToggleState& toggleState) {
    return CapabilityState(
        toggleState.toggleState ? TOGGLESTATE_ON : TOGGLESTATE_OFF,
        toggleState.timeOfSample,
        toggleState.valueUncertainty.count());
}

}  // namespace toggleController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
