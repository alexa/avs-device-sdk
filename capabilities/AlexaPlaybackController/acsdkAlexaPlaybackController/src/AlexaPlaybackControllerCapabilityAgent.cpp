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

#include "acsdkAlexaPlaybackController/AlexaPlaybackControllerCapabilityAgent.h"

#include <acsdkAlexaPlaybackControllerInterfaces/PlaybackOperation.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acsdkAlexaPlaybackController {

using namespace avsCommon::utils::json;
using namespace avsCommon::avs;
using namespace avsCommon;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

using namespace acsdkAlexaPlaybackControllerInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"AlexaPlaybackControllerCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The namespace for this capability agent.
static const std::string NAMESPACE{"Alexa.PlaybackController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for PLAY directive
static const std::string NAME_PLAY{"Play"};

/// The name for PAUSE directive
static const std::string NAME_PAUSE{"Pause"};

/// The name for STOP directive
static const std::string NAME_STOP{"Stop"};

/// The name for STARTOVER directive
static const std::string NAME_STARTOVER{"StartOver"};

/// The name for PREVIOUS directive
static const std::string NAME_PREVIOUS{"Previous"};

/// The name for NEXT directive
static const std::string NAME_NEXT{"Next"};

/// The name for REWIND directive
static const std::string NAME_REWIND{"Rewind"};

/// The name for FASTFORWARD directive
static const std::string NAME_FASTFORWARD{"FastForward"};

/// The playbackOperation for SupportedOperations
static const std::string SUPPORTED_OPERATIONS_KEY{"supportedOperations"};

/// The namespace for this capability agent.
static const std::string NAMESPACE_PLAYBACK_STATE_REPORTER{"Alexa.PlaybackStateReporter"};

/// Property name for Alexa.PlaybackStateReporter
static const std::string PLAYBACK_STATE_REPORTER_PROPERTY{"playbackState"};

/**
 * Convert an @c AlexaPlaybackControllerInterface.Response.Type to its corresponding @c
 * AlexaInterfaceMessageSenderInterface.ErrorResponseType. Note that any @c
 * AlexaPlaybackControllerInterface.Response.Type that does not map to @c ErrorResponseType will return INTERNAL_ERROR.
 *
 * @param responseType The response type to convert.
 * @return The corresponding @c AlexaInterfaceMessageSenderInterface.ErrorResponseType.
 */
static avsCommon::sdkInterfaces::AlexaInterfaceMessageSenderInterface::ErrorResponseType
alexaPlaybackControllerResponseTypeToErrorType(const AlexaPlaybackControllerInterface::Response::Type responseType) {
    ACSDK_DEBUG9(LX(__func__));
    switch (responseType) {
        case AlexaPlaybackControllerInterface::Response::Type::SUCCESS:
            return AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
        case AlexaPlaybackControllerInterface::Response::Type::PLAYBACK_OPERATION_NOT_SUPPORTED:
            return AlexaInterfaceMessageSenderInterface::ErrorResponseType::INVALID_VALUE;
        case AlexaPlaybackControllerInterface::Response::Type::NO_CONTENT_AVAILABLE:
            return AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
        case AlexaPlaybackControllerInterface::Response::Type::NOT_SUPPORTED_IN_CURRENT_MODE:
            return AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
        case AlexaPlaybackControllerInterface::Response::Type::INTERNAL_ERROR:
            return AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
    }

    return AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR;
}

/**
 * Generates a json object of type array from a set of @c PlaybackOperation values.
 *
 * @param playbackOperations A set of playbackOperationstroke values to be generated into json format.
 * @return Returns the json object of an array of playback operation properties.
 */
static std::string generatePlaybackOperationsJson(std::set<PlaybackOperation> playbackOperations) {
    if (playbackOperations.empty()) {
        return "";
    }

    std::string playbackOperationsJson = "[";
    for (const auto& playbackOperation : playbackOperations) {
        auto playbackOperationString =
            acsdkAlexaPlaybackControllerInterfaces::playbackOperationToString(playbackOperation);
        playbackOperationsJson += "\"" + playbackOperationString + "\",";
    }
    playbackOperationsJson.pop_back();  // remove extra comma
    playbackOperationsJson += "]";

    ACSDK_DEBUG9(LX(__func__).sensitive("configuration", playbackOperationsJson));
    return playbackOperationsJson;
}

std::shared_ptr<AlexaPlaybackControllerCapabilityAgent> AlexaPlaybackControllerCapabilityAgent::create(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    std::shared_ptr<AlexaPlaybackControllerInterface> playbackController,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) {
    if (endpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyEndpointId"));
        return nullptr;
    }
    if (!playbackController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPlaybackController"));
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

    auto playbackControllerCapabilityAgent =
        std::shared_ptr<AlexaPlaybackControllerCapabilityAgent>(new AlexaPlaybackControllerCapabilityAgent(
            endpointId,
            playbackController,
            contextManager,
            responseSender,
            exceptionSender,
            isProactivelyReported,
            isRetrievable));

    if (!playbackControllerCapabilityAgent) {
        ACSDK_ERROR(LX("createFailed").d("reason", "instantiationFailed"));
        return nullptr;
    }

    if (!playbackControllerCapabilityAgent->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }

    return playbackControllerCapabilityAgent;
}

AlexaPlaybackControllerCapabilityAgent::AlexaPlaybackControllerCapabilityAgent(
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId,
    const std::shared_ptr<AlexaPlaybackControllerInterface>& playbackController,
    const std::shared_ptr<ContextManagerInterface>& contextManager,
    const std::shared_ptr<AlexaInterfaceMessageSenderInterface>& responseSender,
    const std::shared_ptr<ExceptionEncounteredSenderInterface>& exceptionSender,
    bool isProactivelyReported,
    bool isRetrievable) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        RequiresShutdown{"AlexaPlaybackControllerCapabilityAgent"},
        m_endpointId{endpointId},
        m_isProactivelyReported{isProactivelyReported},
        m_isRetrievable{isRetrievable},
        m_playbackController{playbackController},
        m_contextManager{contextManager},
        m_responseSender{responseSender} {
}

bool AlexaPlaybackControllerCapabilityAgent::initialize() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_isRetrievable) {
        m_contextManager->addStateProvider(
            {NAMESPACE_PLAYBACK_STATE_REPORTER, PLAYBACK_STATE_REPORTER_PROPERTY, m_endpointId}, shared_from_this());
    }
    if (m_isProactivelyReported) {
        if (!m_playbackController->addObserver(shared_from_this())) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "addObserverFailed"));
            return false;
        }
    }

    return true;
}

void AlexaPlaybackControllerCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void AlexaPlaybackControllerCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("preHandleDirective"));
    // do nothing.
}

void AlexaPlaybackControllerCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
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

        AlexaPlaybackControllerInterface::Response result;
        if (directiveName == NAME_PLAY) {
            result = m_playbackController->play();
        } else if (directiveName == NAME_PAUSE) {
            result = m_playbackController->pause();
        } else if (directiveName == NAME_STOP) {
            result = m_playbackController->stop();
        } else if (directiveName == NAME_STARTOVER) {
            result = m_playbackController->startOver();
        } else if (directiveName == NAME_PREVIOUS) {
            result = m_playbackController->previous();
        } else if (directiveName == NAME_NEXT) {
            result = m_playbackController->next();
        } else if (directiveName == NAME_REWIND) {
            result = m_playbackController->rewind();
        } else if (directiveName == NAME_FASTFORWARD) {
            result = m_playbackController->fastForward();
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "unexpectedDirective").d("name", directiveName));
            executeUnknownDirective(info, ExceptionErrorType::UNSUPPORTED_OPERATION);
            return;
        }

        executeSetHandlingCompleted(info);
        executeSendResponseEvent(info, result);
    });
}

void AlexaPlaybackControllerCapabilityAgent::provideState(
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

void AlexaPlaybackControllerCapabilityAgent::executeProvideState(
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
    if (stateProviderName.name != PLAYBACK_STATE_REPORTER_PROPERTY) {
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

    auto playbackState = m_playbackController->getPlaybackState();
    auto playbackStateString = acsdkAlexaPlaybackControllerInterfaces::playbackStateToString(playbackState);

    if (playbackStateString.empty()) {
        ACSDK_ERROR(LX("executeProvideStateFailed").m("emptyPlaybackState"));
        m_contextManager->provideStateUnavailableResponse(stateProviderName, contextRequestToken, false);
        return;
    }

    m_contextManager->provideStateResponse(
        stateProviderName, buildCapabilityState(playbackStateString), contextRequestToken);
}

CapabilityState AlexaPlaybackControllerCapabilityAgent::buildCapabilityState(const std::string& playbackStateString) {
    std::string state;
    state = "{ \"state\" : \"" + playbackStateString + "\"}";
    return CapabilityState(state);
}

bool AlexaPlaybackControllerCapabilityAgent::canStateBeRetrieved() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable;
}

bool AlexaPlaybackControllerCapabilityAgent::hasReportableStateProperties() {
    ACSDK_DEBUG5(LX(__func__));
    return m_isRetrievable || m_isProactivelyReported;
}

void AlexaPlaybackControllerCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
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

DirectiveHandlerConfiguration AlexaPlaybackControllerCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));
    DirectiveHandlerConfiguration configuration;
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    configuration[{NAMESPACE, NAME_PLAY, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_PAUSE, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_STOP, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_STARTOVER, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_PREVIOUS, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_NEXT, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_REWIND, m_endpointId}] = neitherNonBlockingPolicy;
    configuration[{NAMESPACE, NAME_FASTFORWARD, m_endpointId}] = neitherNonBlockingPolicy;

    return configuration;
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlexaPlaybackControllerCapabilityAgent::
    getCapabilityConfigurations() {
    auto additionalConfigurations = CapabilityConfiguration::AdditionalConfigurations();
    auto supportedOperations = m_playbackController->getSupportedOperations();
    auto supportedOperationsJson = generatePlaybackOperationsJson(supportedOperations);
    if (!supportedOperationsJson.empty()) additionalConfigurations[SUPPORTED_OPERATIONS_KEY] = supportedOperationsJson;

    CapabilityConfiguration configuration{CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
                                          NAMESPACE,
                                          INTERFACE_VERSION,
                                          Optional<std::string>(),  // instance
                                          Optional<CapabilityConfiguration::Properties>(),
                                          additionalConfigurations};
    m_capabilityConfigurations = {std::make_shared<avsCommon::avs::CapabilityConfiguration>(configuration)};

    // enabling PlaybackStateReporter if any state report is required
    if (m_isProactivelyReported || m_isRetrievable) {
        CapabilityConfiguration configuration{
            CapabilityConfiguration::ALEXA_INTERFACE_TYPE,
            NAMESPACE_PLAYBACK_STATE_REPORTER,
            INTERFACE_VERSION,
            Optional<std::string>(),  // instance
            Optional<CapabilityConfiguration::Properties>(
                {m_isRetrievable, m_isProactivelyReported, {PLAYBACK_STATE_REPORTER_PROPERTY}}),
            {}};
        m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configuration));
    }

    return m_capabilityConfigurations;
}

void AlexaPlaybackControllerCapabilityAgent::doShutdown() {
    m_executor.shutdown();
    if (m_isProactivelyReported) {
        m_playbackController->removeObserver(shared_from_this());
    }
    if (m_isRetrievable) {
        m_contextManager->removeStateProvider(
            {NAMESPACE_PLAYBACK_STATE_REPORTER, PLAYBACK_STATE_REPORTER_PROPERTY, m_endpointId});
    }
    m_playbackController.reset();
    m_responseSender.reset();
    m_contextManager.reset();
}

void AlexaPlaybackControllerCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void AlexaPlaybackControllerCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void AlexaPlaybackControllerCapabilityAgent::executeUnknownDirective(
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

void AlexaPlaybackControllerCapabilityAgent::executeSendResponseEvent(
    const std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const AlexaPlaybackControllerInterface::Response& result) {
    if (AlexaPlaybackControllerInterface::Response::Type::SUCCESS == result.responseType) {
        m_responseSender->sendResponseEvent(
            info->directive->getInstance(), info->directive->getCorrelationToken(), AVSMessageEndpoint(m_endpointId));
    } else {
        m_responseSender->sendErrorResponseEvent(
            info->directive->getInstance(),
            info->directive->getCorrelationToken(),
            AVSMessageEndpoint(m_endpointId),
            alexaPlaybackControllerResponseTypeToErrorType(result.responseType),
            result.errorMessage);
    }
}

void AlexaPlaybackControllerCapabilityAgent::onPlaybackStateChanged(const PlaybackState& playbackState) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_isProactivelyReported) {
        ACSDK_ERROR(LX("onPlaybackStateChangeFailed").d("reason", "invalidOnPlaybackStateChangedCall"));
        return;
    }

    auto playbackStateString = acsdkAlexaPlaybackControllerInterfaces::playbackStateToString(playbackState);

    if (playbackStateString.empty()) {
        ACSDK_ERROR(LX("onPlaybackStateChangedFailed").m("emptyPlaybackState"));
        return;
    }

    m_executor.submit([this, playbackStateString] {
        ACSDK_DEBUG9(LX("onPlaybackStateChangedInExecutor"));
        m_contextManager->reportStateChange(
            CapabilityTag(NAMESPACE_PLAYBACK_STATE_REPORTER, PLAYBACK_STATE_REPORTER_PROPERTY, m_endpointId),
            buildCapabilityState(playbackStateString),
            AlexaStateChangeCauseType::VOICE_INTERACTION);
    });
}
}  // namespace acsdkAlexaPlaybackController
}  // namespace alexaClientSDK
