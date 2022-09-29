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

#include "IPCServerSampleApp/AlexaPresentation/APLRuntimePresentationAdapter.h"

#include <memory>

#include <acsdk/APLCapabilityCommonInterfaces/APLTimeoutType.h>

#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSession.h"
#include "IPCServerSampleApp/AlexaPresentation/APLPayloadParser.h"
#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSessionManager.h"
#include "IPCServerSampleApp/AlexaPresentation/APLRuntimeInterfaceImpl.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
using namespace alexaPresentationInterfaces;
using namespace aplCapabilityCommonInterfaces;
using namespace aplCapabilityCommonInterfaces::aplEventPayload;
using namespace avsCommon::sdkInterfaces;
using namespace presentationOrchestratorInterfaces;

#define TAG "APLRuntimePresentationAdapter"
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<APLRuntimePresentationAdapter> APLRuntimePresentationAdapter::create(
    const std::shared_ptr<APLRuntimeInterfaceImpl>& runtimeInterface,
    std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier) {
    if (!runtimeInterface) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null runtimeInterface"));
        return nullptr;
    }

    if (!activityEventNotifier) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null activityEventNotifier"));
        return nullptr;
    }

    auto sessionManager = APLDocumentSessionManager::create();
    if (!sessionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null sessionManager"));
        return nullptr;
    }

    return std::shared_ptr<APLRuntimePresentationAdapter>(new APLRuntimePresentationAdapter(
        runtimeInterface, std::move(sessionManager), std::move(activityEventNotifier)));
}

APLRuntimePresentationAdapter::APLRuntimePresentationAdapter(
    const std::shared_ptr<APLRuntimeInterfaceImpl>& runtimeInterface,
    std::unique_ptr<APLDocumentSessionManager> sessionManager,
    std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier) :
        m_runtimeInterface(runtimeInterface),
        m_sessionManager(std::move(sessionManager)),
        m_activityEventNotifier(std::move(activityEventNotifier)),
        m_isSpeakingOrListening(false),
        m_executor(std::make_shared<avsCommon::utils::threading::Executor>()) {
}

void APLRuntimePresentationAdapter::setAlexaPresentationCA(
    std::shared_ptr<AlexaPresentationCapabilityAgentInterface> alexaPresentationCA) {
    m_executor->submit([this, alexaPresentationCA]() { m_alexaPresentationCA = alexaPresentationCA; });
}

void APLRuntimePresentationAdapter::setDefaultWindowId(const std::string& windowId) {
    m_executor->submit([this, windowId]() { m_runtimeInterface->setDefaultWindowId(windowId); });
}

std::shared_ptr<APLRuntimePresentationAdapter::PresentationAgentDocumentTracker> APLRuntimePresentationAdapter::
    executeGetPresentationAgentTrackerFromToken(const std::string& token) {
    auto agentTracker = m_presentationAgentTrackers.find(token);
    if (agentTracker != m_presentationAgentTrackers.end()) {
        return agentTracker->second;
    }
    return nullptr;
}

std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> APLRuntimePresentationAdapter::
    executeGetPresentationAgentFromToken(const std::string& token) {
    if (auto agentTracker = executeGetPresentationAgentTrackerFromToken(token)) {
        return agentTracker->presentationAgent.lock();
    }
    return nullptr;
}

void APLRuntimePresentationAdapter::executeRemoveTokenFromPresentationAgentTracker(const std::string& token) {
    if (auto agentTracker = executeGetPresentationAgentTrackerFromToken(token)) {
        agentTracker->docCount--;
        if (agentTracker->docCount == 0) {
            if (auto ca = agentTracker->presentationAgent.lock()) {
                auto trackerIt = m_presentationTrackersPerCA.find(ca);
                if (trackerIt != m_presentationTrackersPerCA.end() && --trackerIt->second == 0) {
                    // Notify the CA that no document is being displayed
                    ca->onActiveDocumentChanged("", {});
                    m_presentationTrackersPerCA.erase(trackerIt);
                }
            }
            m_presentationAgentTrackers.erase(token);
        }
    }
}

void APLRuntimePresentationAdapter::onRenderDocument(
    const std::string& document,
    const std::string& datasource,
    const PresentationToken& token,
    const std::string& windowId,
    const APLTimeoutType timeoutType,
    const std::string& interfaceName,
    const std::string& supportedViewports,
    const PresentationSession& presentationSession,
    const std::chrono::steady_clock::time_point& receiveTime,
    std::shared_ptr<APLCapabilityAgentInterface> agent) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this,
                        document,
                        datasource,
                        token,
                        windowId,
                        timeoutType,
                        interfaceName,
                        supportedViewports,
                        presentationSession,
                        receiveTime,
                        agent]() {
        executeRenderDocument(
            document,
            datasource,
            token,
            windowId,
            timeoutType,
            interfaceName,
            supportedViewports,
            presentationSession,
            receiveTime,
            agent,
            true);
    });
}

void APLRuntimePresentationAdapter::renderDocumentWithoutPresentation(
    const std::string& jsonPayload,
    const std::string& token,
    const std::string& windowId,
    const std::chrono::steady_clock::time_point& receiveTime,
    const std::string& interface,
    std::shared_ptr<APLCapabilityAgentInterface> agent) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, jsonPayload, token, windowId, receiveTime, interface, agent]() {
        rapidjson::Document payload;
        rapidjson::ParseResult result = payload.Parse(jsonPayload);
        if (!result) {
            ACSDK_ERROR(LX("renderDocumentWithoutPresentationFailed")
                            .d("reason", "parsingPayloadFailed")
                            .sensitive("payload", jsonPayload));
            return;
        }

        std::string document = APLPayloadParser::extractDocument(payload);
        std::string dataSources = APLPayloadParser::extractDatasources(payload);
        std::string supportedViewports = APLPayloadParser::extractSupportedViewports(payload);

        executeRenderDocument(
            document,
            dataSources,
            token,
            windowId,
            APLTimeoutType::SHORT,
            interface,
            supportedViewports,
            {interface, token + windowId},
            receiveTime,
            agent,
            false);
    });
}

void APLRuntimePresentationAdapter::executeRenderDocument(
    const std::string& document,
    const std::string& datasource,
    const PresentationToken& token,
    const std::string& windowId,
    const APLTimeoutType timeoutType,
    const std::string& interfaceName,
    const std::string& supportedViewports,
    const PresentationSession& presentationSession,
    const std::chrono::steady_clock::time_point& receiveTime,
    std::shared_ptr<APLCapabilityAgentInterface> agent,
    bool trackAsPresentation) {
    ACSDK_DEBUG5(LX(__func__));

    // Create or update agent mapping for document token
    if (auto agentTracker = executeGetPresentationAgentTrackerFromToken(token)) {
        agentTracker->docCount++;
    } else {
        m_presentationAgentTrackers[token] =
            std::make_shared<PresentationAgentDocumentTracker>(token, agent, trackAsPresentation);
        ++m_presentationTrackersPerCA[agent];
    }

    // Translate APL timeout to presentation lifespan
    PresentationLifespan lifespan = PresentationLifespan::SHORT;  // Short life spam
    switch (timeoutType) {
        case APLTimeoutType::SHORT:
            lifespan = PresentationLifespan::SHORT;
            break;
        case APLTimeoutType::TRANSIENT:
            lifespan = PresentationLifespan::TRANSIENT;
            break;
        case APLTimeoutType::LONG:
            lifespan = PresentationLifespan::LONG;
            break;
    }

    aplCapabilityCommonInterfaces::PresentationOptions presentationOptions = {
        windowId,
        PresentationInterface::getTimeoutDefault(),
        token,
        lifespan,
        supportedViewports,
        receiveTime,
        interfaceName};

    if (trackAsPresentation) {
        // TODO: Workaround to avoid problems caused by duplicate tokens being received in consecutive renderDocument
        // directives.
        if (auto session = m_sessionManager->getDocumentSessionByPresentationSession(presentationSession)) {
            ACSDK_DEBUG5(LX(__func__).d("token", token).m("Session already exists for token"));
            m_sessionManager->associateTokenWithPresentationSession(presentationSession, token);
            // This implementation of APLRuntime re-uses APLDocumentSessions so it is necessary to convert the interface
            // back into an APLDocumentSession
            auto documentSession = APLDocumentSession::getDocumentSessionFromInterface(session);
            if (!documentSession) {
                ACSDK_ERROR(LX("executeRenderDocumentFailed")
                                .d("reason", "Unable to convert APLDocumentSessionInterface to APLDocumentSession"));
                return;
            }

            documentSession->renderDocument(document, datasource, presentationSession, presentationOptions);
        } else {
            // Before we render an APL document, we clear existing APL document sessions.
            // This logic needs to be added to ensure we don't allow concurrent APL document rendering
            // until we have a full support.
            for (const auto& agentTrackerPair : m_presentationAgentTrackers) {
                // Only clear APL documents that originated from the APL CA
                if (!agentTrackerPair.second->handleOnPresentationDismissed) {
                    continue;
                }
                if (const auto& docSession = m_sessionManager->getDocumentSessionByToken(agentTrackerPair.first)) {
                    docSession->clearDocument();
                }
            }

            m_runtimeInterface->renderDocument(
                document, datasource, presentationSession, presentationOptions, shared_from_this());
        }
    } else {
        auto session = m_runtimeInterface->createDocumentSession(
            document, datasource, presentationSession, presentationOptions, shared_from_this(), false);
        session->firstRender();
    }
}

void APLRuntimePresentationAdapter::clearDocument(const std::string& token) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, token] { executeClearDocument(token); });
}

void APLRuntimePresentationAdapter::executeClearDocument(const std::string& token) {
    auto session = m_sessionManager->getDocumentSessionByToken(token);
    if (!session) {
        ACSDK_ERROR(LX("executeClearDocumentFailed")
                        .d("reason", "No active session exists to call executeClearDocument")
                        .d("token", token));
        return;
    }
    session->clearDocument();
}

void APLRuntimePresentationAdapter::onExecuteCommands(const std::string& jsonPayload, const PresentationToken& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, jsonPayload, token]() { executeExecuteCommands(jsonPayload, token); });
}

void APLRuntimePresentationAdapter::executeExecuteCommands(
    const std::string& jsonPayload,
    const PresentationToken& token) {
    auto session = m_sessionManager->getDocumentSessionByToken(token);
    if (!session) {
        ACSDK_ERROR(LX("executeExecuteCommandsFailed")
                        .d("reason", "No active session exists to call executeCommands")
                        .d("token", token));
        if (auto ca = executeGetPresentationAgentFromToken(token)) {
            ca->processExecuteCommandsResult(token, APLCommandExecutionEvent::FAILED, "No matching APL session");
        }
        return;
    }
    session->executeCommands(jsonPayload);
    m_activityEventNotifier->notifyObserversOfGUIActivityEvent("APLCommandExecution", GUIActivityEvent::ACTIVATED);
}

void APLRuntimePresentationAdapter::onDataSourceUpdate(
    const std::string& sourceType,
    const std::string& jsonPayload,
    const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit(
        [this, sourceType, jsonPayload, token]() { executeDataSourceUpdate(sourceType, jsonPayload, token); });
}

void APLRuntimePresentationAdapter::executeDataSourceUpdate(
    const std::string& sourceType,
    const std::string& jsonPayload,
    const std::string& token) {
    auto session = m_sessionManager->getDocumentSessionByToken(token);
    if (!session) {
        ACSDK_ERROR(LX(__func__).m("No active session exists to call dataSourceUpdate"));
        return;
    }
    session->dataSourceUpdate(sourceType, jsonPayload);
}

void APLRuntimePresentationAdapter::onActivityEvent(const PresentationToken& token, const GUIActivityEvent& event) {
}

void APLRuntimePresentationAdapter::onCommandExecutionComplete(
    const aplCapabilityCommonInterfaces::PresentationToken& token,
    alexaClientSDK::aplCapabilityCommonInterfaces::APLCommandExecutionEvent event,
    const std::string& error) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, token, event, error]() { executeOnCommandExecutionComplete(token, event, error); });
}

void APLRuntimePresentationAdapter::executeOnCommandExecutionComplete(
    const std::string& token,
    APLCommandExecutionEvent event,
    const std::string& message) {
    if (auto ca = executeGetPresentationAgentFromToken(token)) {
        ca->processExecuteCommandsResult(token, event, message);
        m_activityEventNotifier->notifyObserversOfGUIActivityEvent(
            "APLCommandExecution", GUIActivityEvent::DEACTIVATED);
    }
}

void APLRuntimePresentationAdapter::onRenderDocumentComplete(
    const PresentationToken& token,
    bool result,
    const std::string& error,
    const std::chrono::steady_clock::time_point& timestamp) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, token, result, error, timestamp]() {
        executeOnRenderDocumentComplete(token, result, error, timestamp);
    });
}

void APLRuntimePresentationAdapter::onShowDocument(const PresentationToken& token) {
}

void APLRuntimePresentationAdapter::executeOnRenderDocumentComplete(
    const std::string& token,
    bool result,
    const std::string& error,
    const std::chrono::steady_clock::time_point& timestamp) {
    if (auto ca = executeGetPresentationAgentFromToken(token)) {
        ca->processRenderDocumentResult(token, result, error);
        ca->recordRenderComplete(timestamp);
    }

    /// TODO: The following code is pending the migration to NowPlaying
    /**
    if (m_isNowPlaying && result) {
        executeOnPlayerActivityChanged(avsCommon::avs::PlayerActivity::PLAYING);
    }
    **/
}

void APLRuntimePresentationAdapter::onVisualContextAvailable(
    const ContextRequestToken requestToken,
    const aplEventPayload::VisualContext& context) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, requestToken, context]() { executeOnVisualContextAvailable(requestToken, context); });
}

void APLRuntimePresentationAdapter::executeOnVisualContextAvailable(
    const ContextRequestToken requestToken,
    const aplEventPayload::VisualContext& context) {
    if (auto ca = executeGetPresentationAgentFromToken(context.token)) {
        ca->onVisualContextAvailable(requestToken, context);
    }
}

void APLRuntimePresentationAdapter::provideState(
    const PresentationToken& token,
    const ContextRequestToken stateRequestToken) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, token, stateRequestToken]() { executeProvideState(token, stateRequestToken); });
}

void APLRuntimePresentationAdapter::executeProvideState(
    const PresentationToken& token,
    const ContextRequestToken stateRequestToken) {
    auto session = m_sessionManager->getDocumentSessionByToken(token);
    if (!session) {
        ACSDK_WARN(LX("executeProvideStateFailed").d("reason", "No session for token"));

        VisualContext context{token, m_runtimeInterface->getMaxAPLVersion(), "", ""};
        executeOnVisualContextAvailable(stateRequestToken, context);
        return;
    }
    session->provideDocumentContext(stateRequestToken);
}

void APLRuntimePresentationAdapter::onSendEvent(const aplEventPayload::UserEvent& payload) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, payload]() { executeOnSendEvent(payload); });
}

void APLRuntimePresentationAdapter::executeOnSendEvent(const aplEventPayload::UserEvent& userEvent) {
    if (auto ca = executeGetPresentationAgentFromToken(userEvent.token)) {
        ca->sendUserEvent(userEvent);
    }
}

void APLRuntimePresentationAdapter::onDataSourceUpdateComplete(
    const std::string& token,
    bool result,
    const std::string& error) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, token, result, error]() { executeOnDataSourceUpdateComplete(token, result, error); });
}

void APLRuntimePresentationAdapter::executeOnDataSourceUpdateComplete(
    const std::string& token,
    bool result,
    const std::string& error) {
    // no op
}

void APLRuntimePresentationAdapter::onDataSourceFetch(const DataSourceFetch& dataSourceFetch) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, dataSourceFetch]() { executeOnDataSourceFetch(dataSourceFetch); });
}

void APLRuntimePresentationAdapter::executeOnDataSourceFetch(const DataSourceFetch& dataSourceFetch) {
    if (auto ca = executeGetPresentationAgentFromToken(dataSourceFetch.token)) {
        ca->sendDataSourceFetchRequestEvent(dataSourceFetch);
    }
}

void APLRuntimePresentationAdapter::onRuntimeError(const RuntimeError& runtimeError) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor->submit([this, runtimeError]() { executeOnRuntimeError(runtimeError); });
}

void APLRuntimePresentationAdapter::executeOnRuntimeError(const RuntimeError& runtimeError) {
    if (auto ca = executeGetPresentationAgentFromToken(runtimeError.token)) {
        ca->sendRuntimeErrorEvent(runtimeError);
    }
}

void APLRuntimePresentationAdapter::onDocumentFinished(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__).d("token", token));
    m_executor->submit([this, token]() { executeOnDocumentFinished(token); });
}

void APLRuntimePresentationAdapter::executeOnDocumentFinished(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__).d("token", token));

    // We only inform agent of a dismissed document if there are no other documents for the agent with the same token.
    if (auto tracker = executeGetPresentationAgentTrackerFromToken(token)) {
        if (tracker->handleOnPresentationDismissed && m_alexaPresentationCA && tracker->docCount <= 1) {
            m_alexaPresentationCA->onPresentationDismissed(token);
        }
    }

    executeRemoveTokenFromPresentationAgentTracker(token);
}

std::string APLRuntimePresentationAdapter::getAPLRuntimeVersion() {
    ACSDK_DEBUG9(LX(__func__));
    // Execute on executor thread but block this thread
    // until completed to ensure APL version is reported before continuing.
    return m_executor->submit([this]() { return m_runtimeInterface->getMaxAPLVersion(); }).get();
}

void APLRuntimePresentationAdapter::setExecutor(std::shared_ptr<avsCommon::utils::threading::Executor> executor) {
    m_executor = executor;
}

void APLRuntimePresentationAdapter::onPlayerActivityChanged(avsCommon::avs::PlayerActivity state) {
    ACSDK_DEBUG9(LX(__func__).d("newState", avsCommon::avs::playerActivityToString(state).c_str()));
    m_executor->submit([this, state]() { executeOnPlayerActivityChanged(state); });
}

void APLRuntimePresentationAdapter::executeOnPlayerActivityChanged(avsCommon::avs::PlayerActivity state) {
    // TODO: Once NowPlaying is available this code needs to be implemented
}

void APLRuntimePresentationAdapter::onAPLDocumentSessionAvailable(
    const PresentationSession& presentationSession,
    const PresentationToken& token,
    std::unique_ptr<APLDocumentSessionInterface>&& session) {
    ACSDK_DEBUG9(LX(__func__));
    std::shared_ptr<APLDocumentSessionInterface> sessionShared = std::move(session);
    m_executor->submit([this, presentationSession, token, sessionShared] {
        m_sessionManager->addDocumentSession(presentationSession, token, sessionShared);
    });
}

void APLRuntimePresentationAdapter::executeUpdateTimeoutType(
    const std::string& aplToken,
    const aplCapabilityCommonInterfaces::APLTimeoutType& timeoutType) {
    auto session = m_sessionManager->getDocumentSessionByToken(aplToken);
    if (!session) {
        ACSDK_ERROR(LX(__func__).m("No active session exists to call dataSourceUpdate"));
        return;
    }

    switch (timeoutType) {
        case APLTimeoutType::SHORT:
            session->updateLifespan(PresentationLifespan::SHORT);
            break;
        case APLTimeoutType::TRANSIENT:
            session->updateLifespan(PresentationLifespan::TRANSIENT);
            break;
        case APLTimeoutType::LONG:
            session->updateLifespan(PresentationLifespan::LONG);
            break;
    }
}

void APLRuntimePresentationAdapter::onDialogUXStateChanged(DialogUXStateObserverInterface::DialogUXState newState) {
    m_executor->submit([this, newState] {
        if (newState == DialogUXState::SPEAKING || newState == DialogUXState::LISTENING) {
            m_isSpeakingOrListening = true;
        } else {
            m_isSpeakingOrListening = false;
        }
    });
}

void APLRuntimePresentationAdapter::onGUIActivityEventReceived(
    const std::string& source,
    const GUIActivityEvent& activityEvent) {
    auto func = [this, source, activityEvent](const std::shared_ptr<APLDocumentSessionInterface>& session) {
        if (session->isForegroundFocused()) {
            ACSDK_DEBUG9(LX("onActivityEventReceivedBySession").d("reason", "APL session foreground focused"));
            if (auto ca = executeGetPresentationAgentFromToken(session->getToken())) {
                if (GUIActivityEvent::INTERRUPT == activityEvent) {
                    session->interruptCommandSequence();

                    if (m_isSpeakingOrListening) {
                        ca->clearExecuteCommands();
                    }
                }
            }
        }
    };

    m_executor->submit([this, func] { m_sessionManager->invokeFunctionPerDocumentSession(func); });
}

void APLRuntimePresentationAdapter::onActiveDocumentChanged(
    const std::string& token,
    const PresentationSession& session) {
    m_executor->submit([this, token, session] {
        if (auto ca = executeGetPresentationAgentFromToken(token)) {
            ca->onActiveDocumentChanged(token, session);
        }
    });
}

void APLRuntimePresentationAdapter::onSessionEnded(const PresentationSession& presentationSession) {
    m_executor->submit([this, presentationSession] {
        if (auto documentSession = m_sessionManager->getDocumentSessionByPresentationSession(presentationSession)) {
            executeRemoveTokenFromPresentationAgentTracker(documentSession->getToken());
        }
        m_sessionManager->clearDocumentSession(presentationSession);
    });
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
