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
#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSession.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace presentationOrchestratorInterfaces;
using namespace aplCapabilityCommonInterfaces;

static const std::string TAG{"APLDocumentSession"};

#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

APLDocumentSession::APLDocumentSession(
    const std::string& document,
    const std::string& data,
    const std::string& supportedViewports,
    const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
    const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions,
    std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::APLDocumentObserverInterface> observer,
    APLViewhostInterfacePtr viewhost,
    bool hasPresentationAssociation) :
        m_viewhost{viewhost},
        m_hasPresentationAssociation{hasPresentationAssociation},
        m_presentation{nullptr},
        m_state{PresentationState::NONE},
        m_document{document},
        m_data{data},
        m_presentationSession{presentationSession},
        m_presentationOptions{presentationOptions},
        m_observer{observer},
        m_supportedViewports{supportedViewports},
        m_renderComplete{false} {
}

void APLDocumentSession::clearDocument() {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this] { executeClearDocument(); });
}

void APLDocumentSession::executeClearDocument(bool forceClear) {
    if (!forceClear && !executeIsPresentationActive()) {
        ACSDK_WARN(LX("clearDocumentFailed").d("reason", "Presentation is not active"));
        return;
    }

    if (auto viewhost = m_viewhost.lock()) {
        viewhost->clearDocument(m_presentationOptions.token);
    }
}

void APLDocumentSession::executeCommands(const std::string& commands) {
    ACSDK_DEBUG9(LX(__func__));

    m_executor.submit([this, commands] {
        if (executeIsPresentationForegrounded()) {
            if (auto viewhost = m_viewhost.lock()) {
                viewhost->executeCommands(commands, m_presentationOptions.token);
            }
        } else {
            onCommandExecutionComplete(
                m_presentationOptions.token,
                APLClient::AplCommandExecutionEvent::FAILED,
                "Presentation is not in focus");
            ACSDK_WARN(LX("executeCommandsFailed").d("reason", "Presentation is not in focus"));
        }
    });
}

void APLDocumentSession::dataSourceUpdate(const std::string& sourceType, const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, sourceType, payload] {
        if (executeIsPresentationForegrounded()) {
            if (auto viewhost = m_viewhost.lock()) {
                viewhost->dataSourceUpdate(sourceType, payload, m_presentationOptions.token);
            }
        } else {
            onDataSourceUpdateComplete(m_presentationOptions.token, false, "Presentation is not in focus");
            ACSDK_WARN(LX("dataSourceUpdateFailed").d("reason", "Presentation is not in focus"));
        }
    });
}

void APLDocumentSession::interruptCommandSequence() {
    ACSDK_DEBUG9(LX(__func__));

    m_executor.submit([this] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("interruptCommandSequenceFailed").d("reason", "Presentation is not active"));
            return;
        }

        if (auto viewhost = m_viewhost.lock()) {
            viewhost->interruptCommandSequence(m_presentationOptions.token);
        }
    });
}

void APLDocumentSession::provideDocumentContext(const unsigned int stateRequestToken) {
    ACSDK_DEBUG5(LX(__func__));

    m_executor.submit([this, stateRequestToken] {
        if (!executeIsPresentationActive()) {
            aplEventPayload::VisualContext visualContext;
            ACSDK_WARN(LX("provideDocumentContextFailed").d("reason", "Presentation is not active"));
            m_observer->onVisualContextAvailable(stateRequestToken, visualContext);
            return;
        }

        if (!m_renderComplete) {
            aplEventPayload::VisualContext visualContext;
            ACSDK_WARN(LX("provideDocumentContextFailed").d("reason", "Document has not rendered"));
            m_observer->onVisualContextAvailable(stateRequestToken, visualContext);
            return;
        }

        if (auto viewhost = m_viewhost.lock()) {
            viewhost->provideState(getToken(), stateRequestToken);
        }
    });
}

void APLDocumentSession::requestForeground() {
    m_executor.submit([this] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("requestForegroundFailed").d("reason", "No active presentation"));
            return;
        } else if (executeIsPresentationForegrounded()) {
            ACSDK_DEBUG0(LX("requestForeground").m("Presentation already foreground"));
            return;
        }

        if (m_presentation) {
            m_presentation->foreground();
        }
    });
}

void APLDocumentSession::stopTimeout() {
    m_executor.submit([this] {
        if (!m_hasPresentationAssociation) {
            ACSDK_WARN(LX("stopTimeout").m("No presentation associated with this document session."));
            return;
        }
        m_presentation->stopTimeout();
    });
}

void APLDocumentSession::resetTimeout() {
    m_executor.submit([this] {
        if (!m_hasPresentationAssociation) {
            ACSDK_WARN(LX("resetTimeout").m("No presentation associated with this document session."));
            return;
        }

        m_presentation->stopTimeout();
        m_presentation->startTimeout();
    });
}

void APLDocumentSession::updateLifespan(
    alexaClientSDK::presentationOrchestratorInterfaces::PresentationLifespan lifespan) {
    m_executor.submit([this, lifespan] {
        if (!m_hasPresentationAssociation) {
            ACSDK_WARN(LX("updateLifespan").m("No presentation associated with this document session."));
            return;
        }
        m_presentation->setLifespan(lifespan);
    });
}

void APLDocumentSession::updateTimeout(std::chrono::milliseconds timeout) {
    m_executor.submit([this, timeout] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("updateTimeoutFailed").d("reason", "Presentation is not active"));
            return;
        }

        if (m_presentation) {
            m_presentation->setTimeout(timeout);
            if (timeout != PresentationInterface::getTimeoutDisabled() &&
                m_presentation->getState() == PresentationState::FOREGROUND) {
                // APL interfaces expect any active timeouts to be reset
                m_presentation->startTimeout();
            }
        }
    });
}

void APLDocumentSession::updateTimeoutType(const APLTimeoutType& timeoutType) {
    m_executor.submit([this, timeoutType] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("updateTimeoutTypeFailed").d("reason", "Presentation is not active"));
            return;
        }

        PresentationLifespan presentationLifespan;
        switch (timeoutType) {
            case APLTimeoutType::SHORT:
                presentationLifespan = PresentationLifespan::SHORT;
                break;
            case APLTimeoutType::TRANSIENT:
                presentationLifespan = PresentationLifespan::TRANSIENT;
                break;
            case APLTimeoutType::LONG:
                presentationLifespan = PresentationLifespan::LONG;
                break;
        }

        if (m_presentation) {
            m_presentation->setLifespan(presentationLifespan);
            m_presentation->startTimeout();
        }
    });
}

std::string APLDocumentSession::getToken() const {
    return m_presentationOptions.token;
}

void APLDocumentSession::onPresentationAvailable(
    PresentationRequestToken id,
    std::shared_ptr<PresentationInterface> presentation) {
    ACSDK_DEBUG5(LX(__func__).d("id", id));
    m_executor.submit([this, presentation] {
        m_presentation = presentation;
        m_state = presentation->getState();
        firstRender();
    });
}

void APLDocumentSession::onPresentationStateChanged(PresentationRequestToken id, PresentationState newState) {
    ACSDK_DEBUG5(LX(__func__).d("id", id).d("oldState", m_state).d("newState", newState));

    m_executor.submit([this, newState] {
        if (newState == m_state) {
            return;
        }

        m_state = newState;

        switch (newState) {
            case PresentationState::FOREGROUND:
                m_observer->onActiveDocumentChanged(m_presentationOptions.token, m_presentationSession);
                // fall-through
            case PresentationState::FOREGROUND_UNFOCUSED:
                // TODO: Restore state if previous state was background
                break;
            case PresentationState::BACKGROUND:
                // TODO: We do not yet support the background state, once it is possible to suspend state then this
                // should be replaced
                if (executeIsPresentationActive()) {
                    if (m_presentation) {
                        m_presentation->dismiss();
                    }
                }
                break;
            case PresentationState::NONE:
                executeClearDocument(true);
                if (m_presentation) {
                    m_presentation.reset();
                }
                break;
        }
    });
}

void APLDocumentSession::onCommandExecutionComplete(
    const std::string& token,
    APLClient::AplCommandExecutionEvent event,
    const std::string& message) {
    if (!canHandleToken(token)) {
        return;
    }

    aplCapabilityCommonInterfaces::APLCommandExecutionEvent eventType =
        aplCapabilityCommonInterfaces::APLCommandExecutionEvent::FAILED;
    switch (event) {
        case APLClient::AplCommandExecutionEvent::FAILED:
            eventType = aplCapabilityCommonInterfaces::APLCommandExecutionEvent::FAILED;
            break;
        case APLClient::AplCommandExecutionEvent::RESOLVED:
            eventType = aplCapabilityCommonInterfaces::APLCommandExecutionEvent::RESOLVED;
            break;
        case APLClient::AplCommandExecutionEvent::TERMINATED:
            eventType = aplCapabilityCommonInterfaces::APLCommandExecutionEvent::TERMINATED;
            break;
    }

    // Presentation state not checked to avoid possibility of blocking the directive
    m_observer->onCommandExecutionComplete(token, eventType, message);
}

void APLDocumentSession::onRenderDocumentComplete(
    const std::string& token,
    bool result,
    const std::string& error,
    const std::chrono::steady_clock::time_point& timestamp) {
    if (!canHandleToken(token)) {
        return;
    }

    m_executor.submit([this, token, result, error, timestamp] {
        // Only change the renderComplete state if this belongs to the last renderDocument we received.
        if (getToken() == token) {
            m_renderComplete = result;
        }

        // Presentation state not checked to avoid possibility of blocking the directive
        m_observer->onRenderDocumentComplete(token, result, error, timestamp);
    });
}

void APLDocumentSession::onSendEvent(
    const std::string& token,
    const std::string& arguments,
    const std::string& components,
    const std::string& source) {
    if (!canHandleToken(token)) {
        return;
    }

    m_executor.submit([this, token, arguments, components, source] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("onSendEventFailed").d("reason", "Presentation is not active"));
            return;
        }

        aplEventPayload::UserEvent event;
        event.token = std::move(token);
        event.arguments = std::move(arguments);
        event.source = std::move(source);
        event.components = std::move(components);

        m_observer->onSendEvent(event);
    });
}

void APLDocumentSession::onVisualContextAvailable(
    const unsigned int requestToken,
    const std::string& token,
    const std::string& version,
    const std::string& visualContext,
    const std::string& datasourceContext) {
    if (!canHandleToken(token)) {
        return;
    }

    aplEventPayload::VisualContext context;
    context.token = std::move(token);
    context.version = std::move(version);
    context.visualContext = std::move(visualContext);
    context.datasourceContext = std::move(datasourceContext);
    m_observer->onVisualContextAvailable(requestToken, context);
}

void APLDocumentSession::onDataSourceUpdateComplete(const std::string& token, bool result, const std::string& error) {
    if (!canHandleToken(token)) {
        return;
    }

    m_observer->onDataSourceUpdateComplete(token, result, error);
}

void APLDocumentSession::onDataSourceFetch(
    const std::string& token,
    const std::string& dataSourceType,
    const std::string& payload) {
    if (!canHandleToken(token)) {
        return;
    }

    m_executor.submit([this, token, dataSourceType, payload] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("onDataSourceFetchFailed").d("reason", "Presentation is not active"));
            return;
        }

        aplEventPayload::DataSourceFetch dataSourceFetch{token, dataSourceType, payload};

        m_observer->onDataSourceFetch(dataSourceFetch);
    });
}

void APLDocumentSession::onRuntimeError(const std::string& token, const std::string& payload) {
    if (!canHandleToken(token)) {
        return;
    }

    m_executor.submit([this, token, payload] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("onRuntimeErrorFailed").d("reason", "Presentation is not active"));
            return;
        }

        aplEventPayload::RuntimeError runtimeError{token, payload};

        m_observer->onRuntimeError(runtimeError);
    });
}

void APLDocumentSession::onDocumentFinished(const std::string& token) {
    if (!canHandleToken(token)) {
        return;
    }

    ACSDK_DEBUG5(LX(__func__).d("token", token));

    m_executor.submit([this, token] {
        if (executeIsPresentationActive()) {
            // Dismissing the presentation will result in a onDocumentCleared event which will notify observers
            if (m_presentation) {
                m_presentation->dismiss();
            }
        } else {
            ACSDK_ERROR(LX("onDocumentFinishedFailed").d("reason", "No active presentation"));
        }
    });
}

void APLDocumentSession::onOpenURL(const std::string& token, const std::string& url) {
    if (!canHandleToken(token)) {
        return;
    }

    ACSDK_INFO(LX(__func__).m("Open URL not supported"));
}

void APLDocumentSession::onDocumentCleared(const std::string& token) {
    ACSDK_DEBUG5(LX(__func__).d("token", token));
    // onDocumentCleared should only sent in response to a request from us to clear the document
    if (!canHandleToken(token)) {
        return;
    }

    m_executor.submit([this, token] {
        m_renderComplete = false;
        // If there is an active presentation for a cleared APL document session, it should be dismissed.
        if (executeIsPresentationActive()) {
            if (m_presentation) {
                m_presentation->dismiss();
            }
        }

        m_observer->onDocumentFinished(token);
        m_observer->onSessionEnded(m_presentationSession);

        if (auto viewhost = m_viewhost.lock()) {
            viewhost->removeObserver(shared_from_this());
        }
    });
}

void APLDocumentSession::onSetDocumentIdleTimeout(const std::string& token, const std::chrono::milliseconds& timeout) {
    if (!canHandleToken(token)) {
        return;
    }

    ACSDK_DEBUG5(LX(__func__).d("token", token).d("timeoutMs", timeout.count()));

    m_executor.submit([this, token, timeout] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("onSetDocumentIdleTimeoutFailed").d("reason", "Presentation is not active"));
            return;
        }

        if (m_presentation) {
            m_presentation->setTimeout(timeout);
            m_presentation->startTimeout();
        }
    });
}

void APLDocumentSession::firstRender() {
    ACSDK_DEBUG5(LX(__func__));
    if (executeRenderDocument(true)) {
        ACSDK_DEBUG5(LX(__func__).d("executedRenderDocument", true));
        auto documentSessionWrapper =
            std::unique_ptr<APLDocumentSessionWrapper>(new APLDocumentSessionWrapper(shared_from_this()));
        m_observer->onAPLDocumentSessionAvailable(
            m_presentationSession, m_presentationOptions.token, std::move(documentSessionWrapper));
        m_observer->onActiveDocumentChanged(m_presentationOptions.token, m_presentationSession);
    }
}

bool APLDocumentSession::executeRenderDocument(bool firstRender) {
    ACSDK_DEBUG5(LX(__func__).d("firstRender", firstRender));
    if (!executeIsPresentationActive()) {
        ACSDK_WARN(LX("executeRenderDocumentFailed").d("reason", "Presentation is not active"));
        m_observer->onRenderDocumentComplete(
            m_presentationOptions.token, false, "No active presentation", std::chrono::steady_clock::now());
        m_observer->onDocumentFinished(m_presentationOptions.token);
        return false;
    }

    if (auto viewhost = m_viewhost.lock()) {
        m_tokens.insert(m_presentationOptions.token);

        if (firstRender) {
            viewhost->addObserver(shared_from_this());
        }

        m_renderComplete = false;

        viewhost->renderDocument(
            m_presentationSession,
            m_presentationOptions.token,
            m_document,
            m_data,
            m_supportedViewports,
            m_presentationOptions.windowId);
        if (m_presentationOptions.documentReceivedTimestamp.time_since_epoch().count() != 0) {
            viewhost->onRenderDirectiveReceived(
                m_presentationOptions.token, m_presentationOptions.documentReceivedTimestamp);
        }

        return true;
    } else {
        ACSDK_ERROR(LX("executeRenderDocumentFailed").d("reason", "Viewhost pointer expired"));
        if (m_presentation) {
            m_presentation->dismiss();
        }
        m_observer->onRenderDocumentComplete(
            m_presentationOptions.token, false, "Viewhost not available", std::chrono::steady_clock::now());

        return false;
    }
}

void APLDocumentSession::renderDocument(
    const std::string& document,
    const std::string& data,
    const PresentationSession& presentationSession,
    const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions) {
    m_executor.submit([this, document, data, presentationSession, presentationOptions] {
        if (!executeIsPresentationActive()) {
            ACSDK_WARN(LX("renderDocumentFailed").d("reason", "Presentation is not active"));
            m_observer->onRenderDocumentComplete(
                presentationOptions.token, false, "No active presentation", std::chrono::steady_clock::now());
            return;
        }

        if (m_presentationSession.id != presentationSession.id) {
            ACSDK_ERROR(LX("renderDocumentFailed")
                            .d("reason", "Cannot change presentation session ID for an existing session"));
            m_observer->onRenderDocumentComplete(
                presentationOptions.token, false, "Invalid session ID change", std::chrono::steady_clock::now());
            return;
        }

        // Any render of a new document to the session means the previous document is finished
        if (!m_presentationOptions.token.empty()) {
            m_observer->onDocumentFinished(m_presentationOptions.token);
        }

        if (presentationOptions.token != m_presentationOptions.token) {
            if (m_presentation) {
                m_presentation->setMetadata(presentationOptions.token);
            }
        }

        /// If necessary overwrite the window ID, we do not allow a session to exist in two different windows
        auto lastWindowId = m_presentationOptions.windowId;
        m_document = document;
        m_data = data;
        m_presentationOptions = presentationOptions;
        m_presentationSession = presentationSession;

        if (lastWindowId != m_presentationOptions.windowId) {
            if (!presentationOptions.windowId.empty()) {
                ACSDK_WARN(LX("renderDocument")
                               .d("oldWindowId", lastWindowId)
                               .d("newWindowId", m_presentationOptions.windowId)
                               .m("Mismatched window id, using old window ID"));
            }

            m_presentationOptions.windowId = lastWindowId;
        }

        executeRenderDocument();
    });
}

bool APLDocumentSession::executeIsPresentationActive() const {
    // If we have no presentation association, we treat it as 'active', since the absence of the presentation
    // should not gate any behavior of the document session itself in these scenarios.
    return !m_hasPresentationAssociation || (m_presentation && m_presentation->getState() != PresentationState::NONE);
}

bool APLDocumentSession::executeIsPresentationForegrounded() const {
    // If we have no presentation association, we treat it as 'foregrounded', since the absence of the presentation
    // should not gate any behavior of the document session itself in these scenarios.
    return !m_hasPresentationAssociation ||
           (m_presentation && m_presentation->getState() == PresentationState::FOREGROUND);
}

bool APLDocumentSession::onNavigateBack(PresentationRequestToken id) {
    return m_executor
        .submit([this] {
            if (auto viewhost = m_viewhost.lock()) {
                return viewhost->handleBack(m_presentationOptions.windowId);
            }
            return false;
        })
        .get();
}

bool APLDocumentSession::isForegroundFocused() {
    return m_executor.submit([this] { return executeIsPresentationForegrounded(); }).get();
}

bool APLDocumentSession::canHandleToken(const std::string& token) {
    return m_tokens.count(token);
}

std::shared_ptr<APLDocumentSession> APLDocumentSession::getDocumentSessionFromInterface(
    const std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>& aplDocumentSessionInterface) {
    auto documentSessionWrapper = std::dynamic_pointer_cast<APLDocumentSessionWrapper>(aplDocumentSessionInterface);
    if (!documentSessionWrapper) {
        // Try also casting this object to an APLDocumentSession
        if (auto documentSession = std::dynamic_pointer_cast<APLDocumentSession>(aplDocumentSessionInterface)) {
            return documentSession;
        }
        ACSDK_ERROR(
            LX("getDocumentSessionFromInterfaceFailed").d("reason", "Interface does not represent a document session"));
        return nullptr;
    }

    return *documentSessionWrapper;
}

void APLDocumentSession::APLDocumentSessionWrapper::clearDocument() {
    m_documentSession->clearDocument();
}

void APLDocumentSession::APLDocumentSessionWrapper::executeCommands(const std::string& commands) {
    m_documentSession->executeCommands(commands);
}

void APLDocumentSession::APLDocumentSessionWrapper::dataSourceUpdate(
    const std::string& sourceType,
    const std::string& payload) {
    m_documentSession->dataSourceUpdate(sourceType, payload);
}

void APLDocumentSession::APLDocumentSessionWrapper::interruptCommandSequence() {
    m_documentSession->interruptCommandSequence();
}

void APLDocumentSession::APLDocumentSessionWrapper::provideDocumentContext(const unsigned int stateRequestToken) {
    m_documentSession->provideDocumentContext(stateRequestToken);
}

void APLDocumentSession::APLDocumentSessionWrapper::requestForeground() {
    m_documentSession->requestForeground();
}

void APLDocumentSession::APLDocumentSessionWrapper::stopTimeout() {
    m_documentSession->stopTimeout();
}

void APLDocumentSession::APLDocumentSessionWrapper::resetTimeout() {
    m_documentSession->resetTimeout();
}

void APLDocumentSession::APLDocumentSessionWrapper::updateLifespan(
    presentationOrchestratorInterfaces::PresentationLifespan lifespan) {
    m_documentSession->updateLifespan(lifespan);
}

void APLDocumentSession::APLDocumentSessionWrapper::updateTimeout(std::chrono::milliseconds timeout) {
    m_documentSession->updateTimeout(timeout);
}

std::string APLDocumentSession::APLDocumentSessionWrapper::getToken() const {
    return m_documentSession->getToken();
}

bool APLDocumentSession::APLDocumentSessionWrapper::isForegroundFocused() {
    return m_documentSession->isForegroundFocused();
}

APLDocumentSession::APLDocumentSessionWrapper::APLDocumentSessionWrapper(
    std::shared_ptr<APLDocumentSession> documentSession) :
        m_documentSession(std::move(documentSession)) {
}

APLDocumentSession::APLDocumentSessionWrapper::operator std::shared_ptr<APLDocumentSession> const() {
    return m_documentSession;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
