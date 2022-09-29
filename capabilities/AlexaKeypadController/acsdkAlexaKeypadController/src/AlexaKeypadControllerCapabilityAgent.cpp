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

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAlexaKeypadController/AlexaKeypadControllerCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaKeypadController {

using namespace avsCommon::utils::json;
using namespace avsCommon::avs;
using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

using namespace acsdkAlexaKeypadControllerInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"AlexaKeypadControllerCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.KeypadController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for SendKeystroke directive
static const std::string NAME_SENDKEYSTROKE{"SendKeystroke"};

/// The key in the directive payload
static const char KEYPAD_KEY[] = "keystroke";

/// The key for Keys
static const std::string KEYS{"keys"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

/**
 * Generates a json object of type array from a set of @c Keystroke values.
 *
 * @param keysSet A set of keystroke values to be generated into json format.
 * @return Returns the json object of an array of keystroke properties.
 */
static std::string generateKeystrokeJson(std::set<acsdkAlexaKeypadControllerInterfaces::Keystroke> keysSet) {
    if (keysSet.empty()) {
        return "";
    }

    std::string keyStrokeJson = "[";
    for (auto& key : keysSet) {
        auto keystrokeString = acsdkAlexaKeypadControllerInterfaces::keystrokeToString(key);
        keyStrokeJson += "\"" + keystrokeString + "\",";
    }
    keyStrokeJson.pop_back();  // remove extra comma
    keyStrokeJson += "]";

    ACSDK_DEBUG5(LX(__func__).sensitive("configuration", keyStrokeJson));
    return keyStrokeJson;
}

std::shared_ptr<AlexaKeypadControllerCapabilityAgent> AlexaKeypadControllerCapabilityAgent::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaKeypadControllerInterface>& keypadController,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!keypadController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullKeypadController"));
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

    auto alexaKeypadControllerCapabilityAgent =
        std::shared_ptr<AlexaKeypadControllerCapabilityAgent>(new AlexaKeypadControllerCapabilityAgent(
            endpointId, keypadController, contextManager, responseSender, exceptionSender));

    if (!alexaKeypadControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    return alexaKeypadControllerCapabilityAgent;
}

AlexaKeypadControllerCapabilityAgent::AlexaKeypadControllerCapabilityAgent(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaKeypadControllerInterface>& keypadController,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaKeypadControllerCapabilityAgent"},
        m_endpointId{endpointId},
        m_keypadController{keypadController},
        m_contextManager{contextManager},
        m_responseSender{responseSender} {
}

void AlexaKeypadControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaKeypadControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void AlexaKeypadControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
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

        AlexaKeypadControllerInterface::Response result;
        if (directiveName == NAME_SENDKEYSTROKE) {
            auto payload = info->directive->getPayload();
            std::string keystrokeString;
            if (!jsonUtils::retrieveValue(payload, KEYPAD_KEY, &keystrokeString)) {
                std::string errorMessage = "keypadKeyNotFound";
                ACSDK_ERROR(LX("sendKeystrokeDirectiveFailed").m(errorMessage));
                sendExceptionEncounteredAndReportFailed(info, errorMessage);
                return;
            }

            auto keystroke = acsdkAlexaKeypadControllerInterfaces::stringToKeystroke(keystrokeString);
            if (!keystroke.hasValue()) {
                std::string errorMessage = "invalidKeypadKey";
                ACSDK_ERROR(LX("sendKeystrokeDirectiveFailed").m(errorMessage));
                sendExceptionEncounteredAndReportFailed(info, errorMessage);
                return;
            }

            result = m_keypadController->handleKeystroke(keystroke.value());
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

void AlexaKeypadControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
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

DirectiveHandlerConfiguration AlexaKeypadControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_SENDKEYSTROKE, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaKeypadControllerCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    auto supportedKeys = m_keypadController->getSupportedKeys();
    auto supportedKeysJson = generateKeystrokeJson(supportedKeys);
    additionalConfigurations[KEYS] = supportedKeysJson;

    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(),
                                          additionalConfigurations};
    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaKeypadControllerCapabilityAgent::doShutdown() {
    m_executor.shutdown();
    m_keypadController.reset();
    m_responseSender.reset();
    m_contextManager.reset();
}

void AlexaKeypadControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaKeypadControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaKeypadControllerCapabilityAgent::executeUnknownDirective(
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

void AlexaKeypadControllerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const AlexaKeypadControllerInterface::Response& result) {
    switch (result.type) {
        case AlexaKeypadControllerInterface::Response::Type::SUCCESS:
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId));
            break;
        case AlexaKeypadControllerInterface::Response::Type::NO_INFORMATION_AVAILABLE:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
        case AlexaKeypadControllerInterface::Response::Type::INVALID_SELECTION:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INVALID_VALUE, result.errorMessage);
            break;
        case AlexaKeypadControllerInterface::Response::Type::KEYSTROKE_NOT_SUPPORTED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::ACTION_NOT_PERMITTED_FOR_CONTENT,
                result.errorMessage);
            break;
        case AlexaKeypadControllerInterface::Response::Type::INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

void AlexaKeypadControllerCapabilityAgent::sendAlexaErrorResponse(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    AlexaInterfaceMessageSenderInterface::ErrorResponseType alexaErrorResponseType,
    const std::string& responseMessage) {
    m_responseSender->sendErrorResponseEvent(
        info->directive->getInstance(),
        info->directive->getCorrelationToken(),
        AVSMessageEndpoint(m_endpointId),
        alexaErrorResponseType,
        responseMessage);
}

void AlexaKeypadControllerCapabilityAgent::sendAlexaVideoErrorResponse(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType alexaVideoErrorResponseType,
    const std::string& responseMessage) {
    std::string payload =
        R"({"type":")" +
        AlexaInterfaceMessageSenderInterface::alexaVideoErrorResponseToString(alexaVideoErrorResponseType) +
        R"(", "message":")" + responseMessage + R"("})";

    m_responseSender->sendErrorResponseEvent(
        info->directive->getInstance(),
        info->directive->getCorrelationToken(),
        AVSMessageEndpoint(m_endpointId),
        NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE,
        payload);
}

}  // namespace acsdkAlexaKeypadController
}  // namespace alexaClientSDK
