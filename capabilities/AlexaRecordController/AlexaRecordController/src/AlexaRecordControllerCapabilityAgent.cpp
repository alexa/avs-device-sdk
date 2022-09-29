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

#include <acsdk/AlexaRecordController/private/AlexaRecordControllerCapabilityAgent.h>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace alexaRecordController {

using namespace avsCommon::avs;
using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

using namespace alexaRecordControllerInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AlexaRecordControllerCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.RecordController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for StartRecording directive
static const std::string NAME_START_RECORDING{"StartRecording"};

/// The name for StopRecording directive
static const std::string NAME_STOP_RECORDING{"StopRecording"};

/// The name for the context of the response event
static const std::string NAME_RECORDING_STATE{"recordingState"};

/// The namespace for Alexa.Video.ErrorResponse.
static const std::string NAMESPACE_ALEXA_VIDEO_ERRORRESPONSE{"Alexa.Video"};

/// Proactive state reporting is currently disabled for the Record Controller
static const bool IS_PROACTIVELY_REPORTED{false};

/// State where the endpoint is currently recording
static const std::string RECORDING{"RECORDING"};

/// State where the endpoint is not currently recording
static const std::string NOT_RECORDING{"NOT_RECORDING"};

std::shared_ptr<AlexaRecordControllerCapabilityAgent> AlexaRecordControllerCapabilityAgent::create(
    EndpointIdentifier endpointId,
    std::shared_ptr<RecordControllerInterface> recordController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!recordController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullRecordController"));
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

    auto recordControllerCapabilityAgent =
        std::shared_ptr<AlexaRecordControllerCapabilityAgent>(new AlexaRecordControllerCapabilityAgent(
            std::move(endpointId),
            std::move(recordController),
            std::move(contextManager),
            std::move(responseSender),
            std::move(exceptionSender),
            isRetrievable));

    if (!recordControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (!recordControllerCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return recordControllerCapabilityAgent;
}

AlexaRecordControllerCapabilityAgent::AlexaRecordControllerCapabilityAgent(
    EndpointIdentifier endpointId,
    std::shared_ptr<RecordControllerInterface> recordController,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<AlexaInterfaceMessageSenderInterface> responseSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaRecordControllerCapabilityAgent"},
        m_endpointId{std::move(endpointId)},
        m_recordController{std::move(recordController)},
        m_contextManager{std::move(contextManager)},
        m_responseSender{std::move(responseSender)},
        m_isRetrievable{isRetrievable} {
}

bool AlexaRecordControllerCapabilityAgent::initialize() {
    ACSDK_DEBUG9(LX(__func__));

    if (m_isRetrievable) {
        m_contextManager->addStateProvider({NAMESPACE, NAME_RECORDING_STATE, m_endpointId}, shared_from_this());
    }
    return true;
}

void AlexaRecordControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG9(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaRecordControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX("preHandleDirective"));
    // do nothing.
}

void AlexaRecordControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.execute([this, info] {
        ACSDK_DEBUG9(LX("handleDirectiveInExecutor"));
        const std::string directiveName = info->directive->getName();
        if (!info->directive->getEndpoint().hasValue() ||
            (info->directive->getEndpoint().value().endpointId != m_endpointId)) {
            executeUnknownDirective(info, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }
        RecordControllerInterface::Response response;
        if (directiveName == NAME_START_RECORDING) {
            response = m_recordController->startRecording();
        } else if (directiveName == NAME_STOP_RECORDING) {
            response = m_recordController->stopRecording();
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        std::string recordingStateString{NOT_RECORDING};
        if (response.type == RecordControllerInterface::Response::Type::SUCCESS) {
            auto recording = m_recordController->isRecording();
            if (recording) {
                recordingStateString = RECORDING;
            }
        }
        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, response, recordingStateString);
    });
}

void AlexaRecordControllerCapabilityAgent::provideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG9(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.execute([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG9(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
    return;
}

bool AlexaRecordControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG9(LX(__func__));
    return m_isRetrievable;
}

bool AlexaRecordControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG9(LX(__func__));
    return m_isRetrievable;
}

void AlexaRecordControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG9(LX(__func__));
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

DirectiveHandlerConfiguration AlexaRecordControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG9(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_START_RECORDING, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_STOP_RECORDING, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaRecordControllerCapabilityAgent::
    getCapabilityConfigurations() {
    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(
                                              {m_isRetrievable, IS_PROACTIVELY_REPORTED, {NAME_RECORDING_STATE}})};
    return {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};
}

void AlexaRecordControllerCapabilityAgent::doShutdown() {
    ACSDK_DEBUG9(LX(__func__));
    if (!m_executor.isShutdown()) {
        m_executor.shutdown();
    }

    m_recordController.reset();
    m_responseSender.reset();
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider({NAMESPACE, NAME_RECORDING_STATE, m_endpointId});
    }
    m_contextManager.reset();
}

void AlexaRecordControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaRecordControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaRecordControllerCapabilityAgent::executeUnknownDirective(
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

void AlexaRecordControllerCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG9(LX("executeProvideState"));
    auto isError = false;
    if (stateProviderName.endpointId != m_endpointId) {
        ACSDK_ERROR(LX("provideStateFailed")
                        .d("reason", "notExpectedEndpointId")
                        .sensitive("endpointId", stateProviderName.endpointId));
        isError = true;
    }
    if (stateProviderName.name != NAME_RECORDING_STATE) {
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

    auto recording = m_recordController->isRecording() ? RECORDING : NOT_RECORDING;
    m_contextManager->provideStateResponse(
        stateProviderName, CapabilityState(R"(")" + recording + R"(")"), contextRequestToken);
}

void AlexaRecordControllerCapabilityAgent::sendAlexaErrorResponse(
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

void AlexaRecordControllerCapabilityAgent::sendAlexaVideoErrorResponse(
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

void AlexaRecordControllerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const RecordControllerInterface::Response& result,
    const std::string& recordingStateString) {
    switch (result.type) {
        case RecordControllerInterface::Response::Type::SUCCESS: {
            std::string responsePayload = R"({"recording state":")" + recordingStateString + R"("})";
            m_responseSender->sendResponseEvent(
                info->directive->getInstance(),
                info->directive->getCorrelationToken(),
                AVSMessageEndpoint(m_endpointId),
                responsePayload);
            break;
        }
        case RecordControllerInterface::Response::Type::FAILED_TOO_MANY_FAILED_ATTEMPTS:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
        case RecordControllerInterface::Response::Type::FAILED_CONFIRMATION_REQUIRED:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::CONFIRMATION_REQUIRED,
                result.errorMessage);
            break;
        case RecordControllerInterface::Response::Type::FAILED_CONTENT_NOT_RECORDABLE:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::CONTENT_NOT_RECORDABLE,
                result.errorMessage);
            break;
        case RecordControllerInterface::Response::Type::FAILED_STORAGE_FULL:
            sendAlexaVideoErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::AlexaVideoErrorResponseType::STORAGE_FULL,
                result.errorMessage);
            break;
        case RecordControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE:
            sendAlexaErrorResponse(
                info,
                AlexaInterfaceMessageSenderInterface::ErrorResponseType::ENDPOINT_UNREACHABLE,
                result.errorMessage);
            break;
        case RecordControllerInterface::Response::Type::FAILED_INTERNAL_ERROR:
            sendAlexaErrorResponse(
                info, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, result.errorMessage);
            break;
    }
}

}  // namespace alexaRecordController
}  // namespace alexaClientSDK
