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

#include "acsdkAlexaSeekController/AlexaSeekControllerCapabilityAgent.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acsdkAlexaSeekController {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;

using namespace acsdkAlexaSeekControllerInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"AlexaSeekControllerCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.SeekController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for AdjustSeekPosition directive
static const std::string NAME_ADJUST_SEEK_POSITION{"AdjustSeekPosition"};

/// The key for deltaPositionMilliseconds in the directive
static const std::string KEY_POSITION_MILLISECONDS{"deltaPositionMilliseconds"};

/// Property name for Alexa.SeekController positionMilleseconds property
static const std::string POSITION_MILLESECONDS_PROPERTY{"positionMilliseconds"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

std::shared_ptr<AlexaSeekControllerCapabilityAgent> AlexaSeekControllerCapabilityAgent::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaSeekControllerInterface>& seekController,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!seekController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSeekController"));
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

    auto alexaSeekControllerCapabilityAgent =
        std::shared_ptr<AlexaSeekControllerCapabilityAgent>(new AlexaSeekControllerCapabilityAgent(
            endpointId, seekController, contextManager, responseSender, exceptionSender, isRetrievable));

    if (!alexaSeekControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (isRetrievable) {
        contextManager->addStateProvider(
            {NAMESPACE, POSITION_MILLESECONDS_PROPERTY, endpointId}, alexaSeekControllerCapabilityAgent);
    }

    return alexaSeekControllerCapabilityAgent;
}

AlexaSeekControllerCapabilityAgent::AlexaSeekControllerCapabilityAgent(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaSeekControllerInterface>& seekController,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaSeekControllerCapabilityAgent"},
        m_endpointId{endpointId},
        m_seekController{seekController},
        m_lastSeekedPosition{std::chrono::milliseconds(0)},
        m_contextManager{contextManager},
        m_responseSender{responseSender},
        m_isRetrievable{isRetrievable},
        m_isProactivelyReported{false} {
}

void AlexaSeekControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaSeekControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void AlexaSeekControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        ACSDK_DEBUG9(LX("handleDirectiveInExecutor"));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (directiveName == NAME_ADJUST_SEEK_POSITION) {
            auto payload = info->directive->getPayload();
            int64_t deltaPosition;
            if (!jsonUtils::retrieveValue(payload, KEY_POSITION_MILLISECONDS, &deltaPosition)) {
                std::string errorMessage = "deltaPositionMillisecondsNotFound";
                ACSDK_ERROR(LX("executeAdjustSeekPositionDirectiveFailed").m(errorMessage));
                sendExceptionEncounteredAndReportFailed(info, errorMessage);
                return;
            }

            auto result = m_seekController->adjustSeekPosition(std::chrono::milliseconds(deltaPosition));
            m_lastSeekedPosition = result.currentMediaPosition;

            executeSetHandlingCompleted(info);
            executeSendResponseEvent(info, result);
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }
    });
}

void AlexaSeekControllerCapabilityAgent::provideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG5(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.submit([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG9(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
    return;
}

void AlexaSeekControllerCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG5(LX("executeProvideState"));
    bool isError = false;

    if (stateProviderName.endpointId != m_endpointId) {
        ACSDK_ERROR(LX("provideStateFailed")
                        .d("reason", "notExpectedEndpointId")
                        .sensitive("endpointId", stateProviderName.endpointId));
        isError = true;
    }

    if (stateProviderName.name != POSITION_MILLESECONDS_PROPERTY) {
        ACSDK_ERROR(LX("provideStateFailed").d("reason", "notExpectedName").d("name", stateProviderName.name));
        isError = true;
    }

    if (isError) {
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }

    auto lastSeekedPositionString = std::to_string(m_lastSeekedPosition.count());

    m_contextManager->provideStateResponse(
        stateProviderName, CapabilityState(lastSeekedPositionString), contextRequestToken);
}

bool AlexaSeekControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable;
}

bool AlexaSeekControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG5(LX(__func__));
    return false;
}

void AlexaSeekControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
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

DirectiveHandlerConfiguration AlexaSeekControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_ADJUST_SEEK_POSITION, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaSeekControllerCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    CapabilityConfiguration configuration{
        CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
        NAMESPACE,
        INTERFACE_VERSION,
        Optional<std::string>(),  // instance
        Optional<CapabilityConfiguration::Properties>(
            {m_isRetrievable, m_isProactivelyReported, {POSITION_MILLESECONDS_PROPERTY}}),
        additionalConfigurations};
    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaSeekControllerCapabilityAgent::doShutdown() {
    m_executor.shutdown();
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider({NAMESPACE, POSITION_MILLESECONDS_PROPERTY, m_endpointId});
    }
    m_seekController.reset();
    m_responseSender.reset();
    m_contextManager.reset();
}

void AlexaSeekControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaSeekControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaSeekControllerCapabilityAgent::executeUnknownDirective(
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

void AlexaSeekControllerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo>& info,
    const acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface::Response& result) {
    switch (result.type) {
        case AlexaSeekControllerInterface::Response::Type::SUCCESS:
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId));
            break;
        case AlexaSeekControllerInterface::Response::Type::ALREADY_IN_OPERATION:
            sendAlexaErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::ALREADY_IN_OPERATION,
                result.errorMessage);
            break;
        case AlexaSeekControllerInterface::Response::Type::CANCELED_BY_NEW_REQUEST:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_BUSY, result.errorMessage);
            break;
        case AlexaSeekControllerInterface::Response::Type::NO_CONTENT_AVAILABLE:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
        case AlexaSeekControllerInterface::Response::Type::CONTENT_CANNOT_BE_SEEKED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::ACTION_NOT_PERMITTED_FOR_CONTENT,
                result.errorMessage);
            break;
        case AlexaSeekControllerInterface::Response::Type::INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

void AlexaSeekControllerCapabilityAgent::sendAlexaErrorResponse(
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

void AlexaSeekControllerCapabilityAgent::sendAlexaVideoErrorResponse(
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

}  // namespace acsdkAlexaSeekController
}  // namespace alexaClientSDK
