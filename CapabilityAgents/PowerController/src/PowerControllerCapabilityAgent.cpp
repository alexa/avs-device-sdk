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

#include "PowerController/PowerControllerCapabilityAgent.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace powerController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::powerController;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG{"PowerControllerCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.PowerController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for TurnOn directive
static const std::string NAME_TURNON{"TurnOn"};

/// The name for TurnOff directive
static const std::string NAME_TURNOFF{"TurnOff"};

/// The name of powerState property
static const std::string POWERSTATE_PROPERTY_NAME{"powerState"};

/// Json value for ON power state
static const std::string POWERSTATE_ON{R"("ON")"};

/// Json value for OFF power state
static const std::string POWERSTATE_OFF{R"("OFF")"};

std::shared_ptr<PowerControllerCapabilityAgent> PowerControllerCapabilityAgent::create(
    const EndpointIdentifier& endpointId,
    std::shared_ptr<PowerControllerInterface> powerController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!powerController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPowerContoller"));
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

    auto powerContollerCapabilityAgent =
        std::shared_ptr<PowerControllerCapabilityAgent>(new PowerControllerCapabilityAgent(
            endpointId,
            powerController,
            contextManager,
            responseSender,
            exceptionSender,
            isProactivelyReported,
            isRetrievable));

    if (!powerContollerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (!powerContollerCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return powerContollerCapabilityAgent;
}

PowerControllerCapabilityAgent::PowerControllerCapabilityAgent(
    const EndpointIdentifier& endpointId,
    std::shared_ptr<PowerControllerInterface> powerController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"PowerControllerCapabilityAgent"},
        m_endpointId{endpointId},
        m_isProactivelyReported{isProactivelyReported},
        m_isRetrievable{isRetrievable},
        m_powerController{powerController},
        m_contextManager{contextManager},
        m_responseSender{responseSender} {
}

bool PowerControllerCapabilityAgent::initialize() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_isRetrievable) {
        m_contextManager->addStateProvider({NAMESPACE, POWERSTATE_PROPERTY_NAME, m_endpointId}, shared_from_this());
    }
    if (m_isProactivelyReported) {
        if (!m_powerController->addObserver(shared_from_this())) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "addObserverFailed"));
            return false;
        }
    }

    return true;
}

void PowerControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void PowerControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void PowerControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        ACSDK_DEBUG5(LX("handleDirectiveInExecutor"));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        std::pair<AlexaResponseType, std::string> result;
        if (directiveName == NAME_TURNON) {
            result = m_powerController->setPowerState(true, AlexaStateChangeCauseType::VOICE_INTERACTION);
        } else if (directiveName == NAME_TURNOFF) {
            result = m_powerController->setPowerState(false, AlexaStateChangeCauseType::VOICE_INTERACTION);
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

void PowerControllerCapabilityAgent::provideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG5(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.submit([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG5(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
    return;
}

bool PowerControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable;
}

bool PowerControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable || m_isProactivelyReported;
}

void PowerControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    if (!info->directive->getEndpoint().hasValue() ||
        info->directive->getEndpoint().value().endpointId != m_endpointId) {
        ACSDK_WARN(LX("cancelDirective").d("reason", "notExpectedEndpointId"));
    }
    removeDirective(info);
}

DirectiveHandlerConfiguration PowerControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_TURNON, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_TURNOFF, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

CapabilityConfiguration PowerControllerCapabilityAgent::getCapabilityConfiguration() {
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(
                                              {m_isRetrievable, m_isProactivelyReported, {POWERSTATE_PROPERTY_NAME}})};
    return configuration;
}

void PowerControllerCapabilityAgent::onPowerStateChanged(
    const PowerState& powerState,
    AlexaStateChangeCauseType cause) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_isProactivelyReported) {
        ACSDK_ERROR(LX("onPowerStateChangedFailed").d("reason", "invalidOnPowerStateChangedCall"));
        return;
    }

    m_executor.submit([this, powerState, cause] {
        ACSDK_DEBUG5(LX("onPowerStateChangedInExecutor"));
        m_contextManager->reportStateChange(
            CapabilityTag(NAMESPACE, POWERSTATE_PROPERTY_NAME, m_endpointId), buildCapabilityState(powerState), cause);
    });
}

void PowerControllerCapabilityAgent::doShutdown() {
    if (m_isProactivelyReported) {
        m_powerController->removeObserver(shared_from_this());
    }
    m_executor.shutdown();
    m_powerController.reset();
    m_responseSender.reset();
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider({NAMESPACE, POWERSTATE_PROPERTY_NAME, m_endpointId});
    }
    m_contextManager.reset();
}

void PowerControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void PowerControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void PowerControllerCapabilityAgent::executeUnknownDirective(
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

void PowerControllerCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG3(LX("executeProvideState"));
    bool isError = false;
    if (stateProviderName.endpointId != m_endpointId) {
        ACSDK_ERROR(LX("provideStateFailed")
                        .d("reason", "notExpectedEndpointId")
                        .sensitive("endpointId", stateProviderName.endpointId));
        isError = true;
    }
    if (stateProviderName.name != POWERSTATE_PROPERTY_NAME) {
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

    auto result = m_powerController->getPowerState();
    if (AlexaResponseType::SUCCESS != result.first) {
        ACSDK_WARN(LX("executeProvideState").m("failedToGetPropertyValue").sensitive("reason", result.first));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, true);
    } else {
        if (!result.second.hasValue()) {
            ACSDK_ERROR(LX("executeProvideStateFailed").m("emptyPowerState"));
            m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, true);
            return;
        }
        m_contextManager->provideStateResponse(
            stateProviderName, buildCapabilityState(result.second.value()), contextRequestToken);
    }
}

void PowerControllerCapabilityAgent::executeSendResponseEvent(
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

CapabilityState PowerControllerCapabilityAgent::buildCapabilityState(const PowerState& powerState) {
    return CapabilityState(
        powerState.powerState ? POWERSTATE_ON : POWERSTATE_OFF,
        powerState.timeOfSample,
        powerState.valueUncertainty.count());
}

}  // namespace powerController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
