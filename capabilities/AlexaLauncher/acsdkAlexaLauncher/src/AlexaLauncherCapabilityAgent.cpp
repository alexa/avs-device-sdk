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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAlexaLauncher/AlexaLauncherCapabilityAgent.h"

namespace alexaClientSDK {
namespace acsdkAlexaLauncher {

using namespace avsCommon::utils::json;
using namespace avsCommon::avs;
using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace rapidjson;

using namespace acsdkAlexaLauncherInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"AlexaLauncherCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.Launcher"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for LaunchTarget directive
static const std::string NAME_LAUNCHTARGET{"LaunchTarget"};

/// The property name for the Launcher
static const std::string LAUNCHER_PROPERTY_NAME{"Target"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

std::shared_ptr<AlexaLauncherCapabilityAgent> AlexaLauncherCapabilityAgent::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaLauncherInterface>& launcher,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!launcher) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLauncher"));
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

    auto alexaLauncherCapabilityAgent = std::shared_ptr<AlexaLauncherCapabilityAgent>(new AlexaLauncherCapabilityAgent(
        endpointId, launcher, contextManager, responseSender, exceptionSender, isProactivelyReported, isRetrievable));

    if (!alexaLauncherCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (!alexaLauncherCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return alexaLauncherCapabilityAgent;
}

bool AlexaLauncherCapabilityAgent::initialize() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_isProactivelyReported) {
        if (!m_launcher->addObserver(shared_from_this())) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "addObserverFailed"));
            return false;
        }
    }

    m_contextManager->addStateProvider({NAMESPACE, LAUNCHER_PROPERTY_NAME, m_endpointId}, shared_from_this());

    return true;
}

AlexaLauncherCapabilityAgent::AlexaLauncherCapabilityAgent(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaLauncherInterface>& launcher,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"alexaLauncherCapabilityAgent"},
        m_endpointId{endpointId},
        m_isProactivelyReported{isProactivelyReported},
        m_isRetrievable{isRetrievable},
        m_launcher{launcher},
        m_contextManager{contextManager},
        m_responseSender{responseSender} {
}

void AlexaLauncherCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaLauncherCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

/**
 * Parses a directive payload JSON and returns a parsed document object.
 *
 * @param payload JSON string to parse.
 * @param[out] document Pointer to a parsed document.
 * @return @c true if parsing was successful, @c false otherwise.
 */
static bool parseDirectivePayload(const std::string& payload, Document* document) {
    ACSDK_DEBUG5(LX(__func__));
    if (!document) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed").d("reason", "nullDocument"));
        return false;
    }

    ParseResult result = document->Parse(payload);
    if (!result) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return false;
    }

    return true;
}

void AlexaLauncherCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
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

        Document payloadDocument(kObjectType);
        if (!parseDirectivePayload(info->directive->getPayload(), &payloadDocument)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        AlexaLauncherInterface::Response result;
        if (directiveName == NAME_LAUNCHTARGET) {
            acsdkAlexaLauncherInterfaces::TargetState target;
            readLaunchTargetPayload(info, payloadDocument, target);

            result = m_launcher->launchTarget(target);
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

void AlexaLauncherCapabilityAgent::readLaunchTargetPayload(
    const std::shared_ptr<DirectiveInfo> info,
    const rapidjson::Document& payloadDocument,
    acsdkAlexaLauncher::TargetState& targetState) {
    ACSDK_DEBUG5(LX(__func__));

    if (!jsonUtils::retrieveValue(payloadDocument, "identifier", &targetState.identifier)) {
        std::string errorMessage = "readLaunchTargetIdentifierNotFound";
        ACSDK_ERROR(LX("readLaunchTargetPayloadFailed").m(errorMessage));
        sendExceptionEncounteredAndReportFailed(info, errorMessage);
        return;
    }

    if (!jsonUtils::retrieveValue(payloadDocument, "name", &targetState.name)) {
        std::string errorMessage = "readLaunchTargetNameNotFound";
        ACSDK_ERROR(LX("readLaunchTargetPayloadFailed").m(errorMessage));
        sendExceptionEncounteredAndReportFailed(info, errorMessage);
        return;
    }
}

void AlexaLauncherCapabilityAgent::provideState(
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

bool AlexaLauncherCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable;
}

bool AlexaLauncherCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG5(LX(__func__));
    return (m_isRetrievable || m_isProactivelyReported);
}

void AlexaLauncherCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
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

DirectiveHandlerConfiguration AlexaLauncherCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_LAUNCHTARGET, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaLauncherCapabilityAgent::
    getCapabilityConfigurations() {
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>()};
    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaLauncherCapabilityAgent::doShutdown() {
    m_executor.shutdown();
    m_launcher.reset();
    m_responseSender.reset();
    m_contextManager->removeStateProvider({NAMESPACE, LAUNCHER_PROPERTY_NAME, m_endpointId});

    m_contextManager.reset();
}

void AlexaLauncherCapabilityAgent::removeDirective(const std::shared_ptr<DirectiveInfo>& info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaLauncherCapabilityAgent::executeSetHandlingCompleted(const std::shared_ptr<DirectiveInfo>& info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaLauncherCapabilityAgent::executeUnknownDirective(
    const std::shared_ptr<DirectiveInfo>& info,
    ExceptionErrorType type) {
    ACSDK_ERROR(LX("executeUnknownDirectiveFailed")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));

    const std::string exceptionMessage =
        "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

    sendExceptionEncounteredAndReportFailed(info, exceptionMessage, type);
}

void AlexaLauncherCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const AlexaLauncherInterface::Response result) {
    auto responseType = result.type;

    if (AlexaLauncherInterface::Response::Type::SUCCESS == responseType) {
        m_responseSender->sendResponseEvent(
            info->directive->getInstance(), info->directive->getCorrelationToken(), AVSMessageEndpoint(m_endpointId));
    } else if (isVideoErrorResponseType(result.type)) {
        auto videoErrorResponseType = alexaLauncherResponseTypeToVideoErrorType(responseType);
        std::string payload = R"({"message:")" + result.errorMessage + R"(",
                                    "type:")" +
                              m_responseSender->alexaVideoErrorResponseToString(videoErrorResponseType) + R"("})";
        m_responseSender->sendErrorResponseEvent(
            info->directive->getInstance(),
            info->directive->getCorrelationToken(),
            AVSMessageEndpoint(m_endpointId),
            NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE,
            payload);
    } else {
        m_responseSender->sendErrorResponseEvent(
            info->directive->getInstance(),
            info->directive->getCorrelationToken(),
            AVSMessageEndpoint(m_endpointId),
            alexaLauncherResponseTypeToErrorType(result.type),
            result.errorMessage);
    }
}

avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType
AlexaLauncherCapabilityAgent::alexaLauncherResponseTypeToVideoErrorType(
    const AlexaLauncherInterface::Response::Type responseType) {
    ACSDK_DEBUG5(LX(__func__));
    switch (responseType) {
        case AlexaLauncherInterface::Response::Type::CONFIRMATION_REQUIRED:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::
                CONFIRMATION_REQUIRED;
        default:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::NONE;
    }
    return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::NONE;
}

avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType AlexaLauncherCapabilityAgent::
    alexaLauncherResponseTypeToErrorType(const AlexaLauncherInterface::Response::Type responseType) {
    ACSDK_DEBUG5(LX(__func__));
    switch (responseType) {
        case AlexaLauncherInterface::Response::Type::ALREADY_IN_OPERATION:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::
                ALREADY_IN_OPERATION;
        case AlexaLauncherInterface::Response::Type::INSUFFICIENT_PERMISSIONS:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::
                INSUFFICIENT_PERMISSIONS;
        case AlexaLauncherInterface::Response::Type::INTERNAL_ERROR:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
        case AlexaLauncherInterface::Response::Type::INVALID_VALUE:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::INVALID_VALUE;
        case AlexaLauncherInterface::Response::Type::NOT_SUPPORTED_IN_CURRENT_MODE:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::
                NOT_SUPPORTED_IN_CURRENT_MODE;
        default:
            return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
    }
    return avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
}

bool AlexaLauncherCapabilityAgent::isVideoErrorResponseType(const AlexaLauncherInterface::Response::Type responseType) {
    ACSDK_DEBUG5(LX(__func__));
    switch (responseType) {
        case AlexaLauncherInterface::Response::Type::CONFIRMATION_REQUIRED:
            return true;
        case AlexaLauncherInterface::Response::Type::SUCCESS:
        case AlexaLauncherInterface::Response::Type::ALREADY_IN_OPERATION:
        case AlexaLauncherInterface::Response::Type::INSUFFICIENT_PERMISSIONS:
        case AlexaLauncherInterface::Response::Type::INTERNAL_ERROR:
        case AlexaLauncherInterface::Response::Type::INVALID_VALUE:
        case AlexaLauncherInterface::Response::Type::NOT_SUPPORTED_IN_CURRENT_MODE:
            return false;
    }
    return false;
}

void AlexaLauncherCapabilityAgent::executeProvideState(
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
    if (stateProviderName.name != LAUNCHER_PROPERTY_NAME) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "notExpectedName").d("name", stateProviderName.name));
        isError = true;
    }

    if (isError) {
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }

    auto targetState = m_launcher->getLauncherTargetState();
    m_contextManager->provideStateResponse(stateProviderName, buildCapabilityState(targetState), contextRequestToken);
}

void AlexaLauncherCapabilityAgent::onLauncherTargetChanged(
    const acsdkAlexaLauncherInterfaces::TargetState& targetState) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_isProactivelyReported) {
        ACSDK_ERROR(LX("onLauncherTargetChangedFailed").d("reason", "invalidOnLauncherTargetChangedCall"));
        return;
    }

    m_executor.submit([this, targetState] {
        m_contextManager->reportStateChange(
            CapabilityTag(NAMESPACE, LAUNCHER_PROPERTY_NAME, m_endpointId),
            buildCapabilityState(targetState),
            AlexaStateChangeCauseType::VOICE_INTERACTION);
    });
}

/**
 * Helper function to add Comma Separated Values.
 *
 * @param value Launcher target value.
 * @param[out] out Output string with value added.
 */
static void AddCsv(const std::string& value, std::string& out) {
    if (!out.empty()) out += ",";
    out += value;
}

CapabilityState AlexaLauncherCapabilityAgent::buildCapabilityState(const acsdkAlexaLauncher::TargetState& targetState) {
    std::string targetJson = "";
    if (!targetState.identifier.empty()) {
        AddCsv("\"identifier\":\"" + targetState.identifier + "\"", targetJson);
    }
    if (!targetState.name.empty()) {
        AddCsv("\"name\":\"" + targetState.name + "\"", targetJson);
    }

    targetJson = "{" + targetJson + "}";

    return CapabilityState(targetJson);
}

}  // namespace acsdkAlexaLauncher
}  // namespace alexaClientSDK
