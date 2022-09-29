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

#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include "IPCServerSampleApp/AlexaPresentation/AplClientBridge.h"
#include "IPCServerSampleApp/AlexaPresentation/APLRuntimePresentationAdapter.h"
#include "IPCServerSampleApp/CachingDownloadManager.h"
#include "IPCServerSampleApp/DownloadMonitor.h"
#include "IPCServerSampleApp/IPC/Namespaces/APLClientNamespace.h"

#ifdef ENABLE_APL_TELEMETRY
#include "IPCServerSampleApp/TelemetrySink.h"
#endif

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

static const std::string TAG{"AplClientBridge"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

using namespace aplCapabilityCommonInterfaces;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::utils::sds;
using namespace avsCommon::utils::timing;
using namespace APLClient::Extensions;

static const char ARGUMENTS_KEY[] = "arguments";
static const char COMPONENTS_KEY[] = "components";
static const char SOURCE_KEY[] = "source";
static const char VERSION_KEY[] = "version";
static const char VISUAL_CONTEXT_KEY[] = "componentsVisibleOnScreen";
static const char DATASOURCE_CONTEXT_KEY[] = "dataSources";

std::shared_ptr<AplClientBridge> AplClientBridge::create(
    std::shared_ptr<CachingDownloadManager> contentDownloadManager,
    std::shared_ptr<gui::GUIClientInterface> guiClient,
    AplClientBridgeParameter parameters) {
    std::shared_ptr<AplClientBridge> aplClientBridge(
        new AplClientBridge(contentDownloadManager, guiClient, parameters));
    aplClientBridge->m_aplClientBinding.reset(new APLClient::AplClientBinding(aplClientBridge));
    aplClientBridge->initialize();

    return aplClientBridge;
}

AplClientBridge::AplClientBridge(
    std::shared_ptr<CachingDownloadManager> contentDownloadManager,
    std::shared_ptr<gui::GUIClientInterface> guiClient,
    AplClientBridgeParameter parameters) :
        RequiresShutdown{"AplClientBridge"},
        m_contentDownloadManager{contentDownloadManager},
        m_guiClient{guiClient},
        m_renderQueued{false},
        m_parameters{parameters} {
    m_playerActivityState = avsCommon::avs::PlayerActivity::FINISHED;
}

void AplClientBridge::initialize() {
    m_aplClientHandler = ipc::APLClientHandler::create(m_guiClient->getIPCRouter(), shared_from_this());
}

void AplClientBridge::addObserver(const APLViewhostObserverInterfacePtr& newObserver) {
    m_executor.submit([this, newObserver] {
        for (auto& weakObserver : m_observers) {
            if (auto observer = weakObserver.lock()) {
                if (observer == newObserver) {
                    ACSDK_ERROR(LX("addObserverFailed").d("reason", "Attempt to add duplicate observer"));
                    return;
                }
            }
        }

        m_observers.push_back(newObserver);
    });
}
void AplClientBridge::removeObserver(const APLViewhostObserverInterfacePtr& observer) {
    m_executor.submit([this, observer] {
        for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
            if (auto spObserver = it->lock()) {
                if (spObserver == observer) {
                    m_observers.erase(it);
                    return;
                }
            }
        }

        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "Attempt to remove non-existent observer"));
    });
}

void AplClientBridge::initializeRenderer(
    const std::string& windowId,
    const std::set<std::string>& supportedExtensions) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, windowId, supportedExtensions] {
        if (!windowId.empty()) {
            auto aplClientRenderer = getAplClientRendererFromWindowId(windowId);
            if (!aplClientRenderer) {
                aplClientRenderer = m_aplClientBinding->createRenderer(windowId);
                m_aplClientRendererMap[windowId] = aplClientRenderer;
            }

            std::unordered_set<std::shared_ptr<APLClient::Extensions::AplCoreExtensionInterface>> extensions;
            for (auto& uri : supportedExtensions) {
                if (aplClientRenderer->getExtension(uri)) {
                    continue;
                }
                if (APLClient::Extensions::Backstack::URI == uri) {
                    extensions.emplace(std::make_shared<Backstack::AplBackstackExtension>(shared_from_this()));
                } else if (APLClient::Extensions::AudioPlayer::URI == uri) {
                    auto audioPlayerExtension =
                        std::make_shared<AudioPlayer::AplAudioPlayerExtension>(shared_from_this());
                    extensions.emplace(audioPlayerExtension);
                    m_audioPlayerExtensions.push_back(audioPlayerExtension);
                } else if (m_sharedRegisteredExtensions.count(uri) != 0) {
                    extensions.emplace(m_sharedRegisteredExtensions[uri]);
                }
            }

            if (!extensions.empty()) {
                aplClientRenderer->addExtensions(std::move(extensions));
            }
        }
    });
}

void AplClientBridge::registerSharedExtension(
    const std::shared_ptr<APLClient::Extensions::AplCoreExtensionInterface>& extension) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, extension] { m_sharedRegisteredExtensions[extension->getUri()] = extension; });
}

void AplClientBridge::sendMessage(const std::string& token, const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    std::string newPayload = payload;
    auto aplClientRenderer = getAplClientRendererFromAplToken(token);
    if (aplClientRenderer && m_aplClientHandler) {
        m_aplClientHandler->dispatchSendMessageToViewhost(aplClientRenderer->getWindowId(), newPayload);
    } else {
        if (!aplClientRenderer) {
            ACSDK_WARN(LX("sendMessageFailed").d("reason", "nullAplClientRenderer"));
        }
        if (!m_aplClientHandler) {
            ACSDK_WARN(LX("sendMessageFailed").d("reason", "nullAplClientHandler"));
        }
    }
}

void AplClientBridge::resetViewhost(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    auto aplClientRenderer = getAplClientRendererFromAplToken(token);
    if (aplClientRenderer && m_aplClientHandler) {
        m_aplClientHandler->dispatchCreateRenderer(aplClientRenderer->getWindowId(), token);
    } else {
        if (!aplClientRenderer) {
            ACSDK_WARN(LX("resetViewHostFailed").d("reason", "nullAplClientRenderer"));
        }
        if (!m_aplClientHandler) {
            ACSDK_WARN(LX("resetViewHostFailed").d("reason", "nullAplClientHandler"));
        }
    }
}

std::string AplClientBridge::downloadResource(const std::string& source) {
    ACSDK_DEBUG9(LX(__func__));
    auto downloadMetricsEmitter = m_aplClientBinding->createDownloadMetricsEmitter();
    auto observer = std::make_shared<DownloadMonitor>(downloadMetricsEmitter);
    return m_contentDownloadManager->retrieveContent(source, observer);
}

std::chrono::milliseconds AplClientBridge::getTimezoneOffset() {
    // Relies on fact that this is always called in the executor thread via either RenderDocument or UpdateTimer
    return m_guiManager->getDeviceTimezoneOffset();
}

void AplClientBridge::onActivityStarted(const std::string& token, const std::string& source) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, source] { m_guiManager->handleGUIActivityEvent(GUIActivityEvent::ACTIVATED, source); });
}

void AplClientBridge::onActivityEnded(const std::string& token, const std::string& source) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, source] { m_guiManager->handleGUIActivityEvent(GUIActivityEvent::DEACTIVATED, source); });
}

void AplClientBridge::onSendEvent(const std::string& token, const std::string& event) {
    ACSDK_DEBUG9(LX(__func__));
    rapidjson::Document payload;
    if (payload.Parse(event).HasParseError()) {
        ACSDK_ERROR(LX("onSendEventFailed").d("reason", "Invalid JSON"));
        return;
    }

    std::string arguments;
    std::string components;
    std::string source;
    if (payload.HasMember(ARGUMENTS_KEY)) {
        const rapidjson::Value& argValue = payload[ARGUMENTS_KEY];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!argValue.Accept(writer)) {
            ACSDK_ERROR(LX("onSendEventFailed").d("reason", "Error serializing arguments payload"));
            return;
        }
        arguments = buffer.GetString();
    }
    if (payload.HasMember(COMPONENTS_KEY)) {
        const rapidjson::Value& componentsValue = payload[COMPONENTS_KEY];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!componentsValue.Accept(writer)) {
            ACSDK_ERROR(LX("onSendEventFailed").d("reason", "Error serializing components payload"));
            return;
        }
        components = buffer.GetString();
    }
    if (payload.HasMember(SOURCE_KEY)) {
        const rapidjson::Value& sourceValue = payload[SOURCE_KEY];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!sourceValue.Accept(writer)) {
            ACSDK_ERROR(LX("onSendEventFailed").d("reason", "Error serializing source payload"));
            return;
        }
        source = buffer.GetString();
    }

    m_executor.submit([this, token, arguments, components, source] {
        executeNotifyObservers(&APLViewhostObserverInterface::onSendEvent, token, arguments, components, source);
    });
}

void AplClientBridge::onCommandExecutionComplete(
    const std::string& token,
    APLClient::AplCommandExecutionEvent event,
    const std::string& message) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, event, message] {
        executeNotifyObservers(&APLViewhostObserverInterface::onCommandExecutionComplete, token, event, message);
    });
}

void AplClientBridge::onRenderDocumentComplete(const std::string& token, bool result, const std::string& error) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, result, error] {
        executeNotifyObservers(
            &APLViewhostObserverInterface::onRenderDocumentComplete,
            token,
            result,
            error,
            std::chrono::steady_clock::now());

        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (!result && aplClientRenderer) {
            aplClientRenderer->onRenderingEvent(APLClient::AplRenderingEvent::RENDER_ABORTED);
        }
    });
}

void AplClientBridge::onVisualContextAvailable(
    const std::string& token,
    unsigned int stateRequestToken,
    const std::string& context) {
    ACSDK_DEBUG9(LX(__func__));

    rapidjson::Document payload;
    if (payload.Parse(context).HasParseError()) {
        ACSDK_ERROR(LX("onVisualContextAvailableFailed").d("reason", "Invalid JSON"));
        return;
    }

    std::string version;
    if (payload.HasMember(VERSION_KEY) && payload[VERSION_KEY].IsString()) {
        version = payload[VERSION_KEY].GetString();
    }

    std::string visualContext;
    if (payload.HasMember(VISUAL_CONTEXT_KEY)) {
        const auto& visualContextArray = payload[VISUAL_CONTEXT_KEY];
        if (!visualContextArray.IsArray()) {
            ACSDK_ERROR(LX("onVisualContextAvailableFailed").d("reason", "Visual context is not an array"));
            return;
        }
        if (visualContextArray.GetArray().Empty()) {
            ACSDK_ERROR(LX("onVisualContextAvailableFailed").d("reason", "Visual context array is empty"));
            return;
        }
        const auto& value = visualContextArray.GetArray()[0];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!value.Accept(writer)) {
            ACSDK_ERROR(LX("onVisualContextAvailableFailed").d("reason", "Error serializing components payload"));
            return;
        }
        visualContext = buffer.GetString();
    }

    std::string datasourceContext;
    if (payload.HasMember(DATASOURCE_CONTEXT_KEY)) {
        const auto& value = payload[DATASOURCE_CONTEXT_KEY];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!value.Accept(writer)) {
            ACSDK_ERROR(LX("onVisualContextAvailableFailed").d("reason", "Error serializing components payload"));
            return;
        }
        datasourceContext = buffer.GetString();
    }

    m_executor.submit([this, token, stateRequestToken, version, visualContext, datasourceContext] {
        executeNotifyObservers(
            &APLViewhostObserverInterface::onVisualContextAvailable,
            stateRequestToken,
            token,
            version,
            visualContext,
            datasourceContext);
    });
}

void AplClientBridge::onSetDocumentIdleTimeout(const std::string& token, const std::chrono::milliseconds& timeout) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, timeout] {
        executeNotifyObservers(&APLViewhostObserverInterface::onSetDocumentIdleTimeout, token, timeout);
    });
}

void AplClientBridge::onFinish(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token] {
        executeNotifyObservers(&APLViewhostObserverInterface::onDocumentFinished, token);
        // Ideally this action can be handled elsewhere, but for now the call is necessary to ensure that
        // foreground activity is stopped
        m_guiManager->handleDocumentTerminated(token, false);
    });
}

void AplClientBridge::onRuntimeErrorEvent(const std::string& token, const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, payload] {
        executeNotifyObservers(&APLViewhostObserverInterface::onRuntimeError, token, payload);
    });
}

void AplClientBridge::onDataSourceFetchRequestEvent(
    const std::string& token,
    const std::string& type,
    const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, type, payload] {
        executeNotifyObservers(&APLViewhostObserverInterface::onDataSourceFetch, token, type, payload);
    });
}

void AplClientBridge::onExtensionEvent(
    const std::string& aplToken,
    const std::string& uri,
    const std::string& name,
    const std::string& source,
    const std::string& params,
    unsigned int event,
    std::shared_ptr<AplCoreExtensionEventCallbackResultInterface> resultCallback) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, aplToken, uri, name, source, params, event, resultCallback] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(aplToken);
        if (aplClientRenderer) {
            aplClientRenderer->onExtensionEvent(uri, name, source, params, event, resultCallback);
        }
    });
}

void AplClientBridge::logMessage(APLClient::LogLevel level, const std::string& source, const std::string& message) {
    switch (level) {
        case APLClient::LogLevel::CRITICAL:
        case APLClient::LogLevel::ERROR:
            ACSDK_ERROR(LX(source).m(message));
            break;
        case APLClient::LogLevel::WARN:
            ACSDK_WARN(LX(source).m(message));
            break;
        case APLClient::LogLevel::INFO:
            ACSDK_INFO(LX(source).m(message));
            break;
        case APLClient::LogLevel::DBG:
            ACSDK_DEBUG0(LX(source).m(message));
            break;
        case APLClient::LogLevel::TRACE:
            ACSDK_DEBUG9(LX(source).m(message));
            break;
    }
}

void AplClientBridge::onConnectionOpened() {
    ACSDK_DEBUG9(LX("onConnectionOpened"));
    // Start the scheduled event timer to refresh the display at 60fps
    m_executor.submit([this] {
        m_updateTimer.start(
            std::chrono::milliseconds(16),
            Timer::PeriodType::ABSOLUTE,
            Timer::FOREVER,
            std::bind(&AplClientBridge::onUpdateTimer, this));
    });
}

void AplClientBridge::onConnectionClosed() {
    ACSDK_DEBUG9(LX("onConnectionClosed"));
    // Stop the outstanding timer as the client is no longer connected
    m_executor.submit([this] { m_updateTimer.stop(); });
}

void AplClientBridge::provideState(const std::string& aplToken, const unsigned int stateRequestToken) {
    ACSDK_DEBUG9(LX(__func__));

    m_executor.submit([this, stateRequestToken, aplToken] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(aplToken);
        if (aplClientRenderer) {
            aplClientRenderer->requestVisualContext(stateRequestToken);
        }
    });
}

void AplClientBridge::onUpdateTimer() {
    bool renderQueued = m_renderQueued.exchange(true);
    if (renderQueued) {
        // Render was already queued, we can safely skip this rendering
        return;
    }

    m_executor.submit([this] {
        m_renderQueued = false;
        for (auto& aplClientRendererPair : m_aplClientRendererMap) {
            aplClientRendererPair.second->onUpdateTick();
        }

        if (m_mediaProperties && avsCommon::avs::PlayerActivity::PLAYING == m_playerActivityState) {
            int audioItemOffset = m_mediaProperties->getAudioItemOffset().count();
            for (const auto& audioPlayerExtension : m_audioPlayerExtensions) {
                audioPlayerExtension->updatePlaybackProgress(audioItemOffset);
            }
        }
    });
}

void AplClientBridge::setGUIManager(std::shared_ptr<gui::GUIServerInterface> guiManager) {
    m_executor.submit([this, guiManager] { m_guiManager = guiManager; });
}

void AplClientBridge::renderDocument(
    const PresentationSession& presentationSession,
    const std::string& token,
    const std::string& document,
    const std::string& dataSources,
    const std::string& supportedViewports,
    const std::string& windowId) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, presentationSession, token, document, dataSources, supportedViewports, windowId] {
        if (windowId.empty()) {
            ACSDK_ERROR(LX("renderDocumentFailed").d("reason", "window id cannot be empty, finishing document"));
            onFinish(token);
            return;
        }
        std::shared_ptr<APLClient::AplClientRenderer> aplClientRenderer;
        if (m_aplClientRendererMap.find(windowId) != m_aplClientRendererMap.end()) {
            aplClientRenderer = m_aplClientRendererMap[windowId];
        } else {
            /// Will be reached for targetWindowId not found in configurations
            aplClientRenderer = m_aplClientBinding->createRenderer(windowId);
            m_aplClientRendererMap[windowId] = aplClientRenderer;
        }

        // Don't need to check for existence first
        const auto& lastSessionForWindow = m_windowIdToPresentationSession[windowId];

        /** /// TODO Change this to operator once APL Runtime team has the changes.
         * Presentation session is a match if the last session and current session for a window are same. Here are
         * the rules governing this equivalency:
         * 1. Skill Id should match.
         * 2. Presentation Id should match.
         * 3. Should have same supported extensions.
         * 4. Same list of auto-initialized extensions in the APL client renderer.
         */
        bool isSamePresentationSession = lastSessionForWindow.skillId == presentationSession.skillId &&
                                         lastSessionForWindow.id == presentationSession.id;

        if (!isSamePresentationSession) {
            // Session IDs do not match
            executeOnPresentationSessionChanged(windowId, presentationSession);
            m_windowTokenMapping.eraseWindow(windowId);
        }

        setTokenToWindow(token, windowId);

        if (auto backExtension = getBackExtensionForRenderer(aplClientRenderer)) {
            if (backExtension->shouldCacheActiveDocument()) {
                if (auto documentState = aplClientRenderer->getActiveDocumentState()) {
                    backExtension->addDocumentStateToBackstack(documentState);
                }
            }
        }

        aplClientRenderer->renderDocument(document, dataSources, supportedViewports, token);
    });
}

void AplClientBridge::setMediaProperties(
    std::shared_ptr<avsCommon::sdkInterfaces::MediaPropertiesInterface> mediaProperties) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, mediaProperties] { m_mediaProperties = mediaProperties; });
}

void AplClientBridge::clearDocument(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (aplClientRenderer) {
            m_windowTokenMapping.eraseToken(token);
            aplClientRenderer->clearDocument();

            // Reset the renderer's backstack on document clear
            if (auto backExtension = getBackExtensionForRenderer(aplClientRenderer)) {
                backExtension->reset();
            }
        } else {
            ACSDK_DEBUG0(LX("clearDocumentFailed").d("reason", "No renderer for token").d("token", token));
        }
        // We inform observer either way, as the document renderer either was successfully cleared or had already been
        // cleared.
        executeNotifyObservers(&APLViewhostObserverInterface::onDocumentCleared, token);
    });
}

void AplClientBridge::executeCommands(const std::string& jsonPayload, const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, jsonPayload, token] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (aplClientRenderer) {
            aplClientRenderer->executeCommands(jsonPayload, token);
        } else {
            executeNotifyObservers(
                &APLViewhostObserverInterface::onCommandExecutionComplete,
                token,
                APLClient::AplCommandExecutionEvent::FAILED,
                "No document renderer instance for token.");
        }
    });
}

void AplClientBridge::interruptCommandSequence(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (aplClientRenderer) {
            aplClientRenderer->interruptCommandSequence();
        }
    });
}

void AplClientBridge::dataSourceUpdate(
    const std::string& sourceType,
    const std::string& jsonPayload,
    const std::string& token) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, sourceType, jsonPayload, token] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (aplClientRenderer) {
            aplClientRenderer->dataSourceUpdate(sourceType, jsonPayload, token);
        }
    });
}

void AplClientBridge::onMessage(const std::string& windowId, const std::string& message) {
    ACSDK_DEBUG9(LX(__func__));

    auto aplClientRenderer = getAplClientRendererFromWindowId(windowId);
    if (aplClientRenderer && aplClientRenderer->shouldHandleMessage(message)) {
        m_executor.submit([message, aplClientRenderer] { aplClientRenderer->handleMessage(message); });
    }
}

bool AplClientBridge::handleBack(const std::string& windowId) {
    return m_executor
        .submit([this, windowId] {
            if (auto aplClientRenderer = getAplClientRendererFromWindowId(windowId)) {
                if (auto backExtension = getBackExtensionForRenderer(aplClientRenderer)) {
                    return backExtension->handleBack();
                }
            }
            return false;
        })
        .get();
}

void AplClientBridge::executeOnPresentationSessionChanged(
    const std::string& windowId,
    const PresentationSession& presentationSession) {
    ACSDK_DEBUG9(LX(__func__).d("windowId", windowId));
    // Reset the active window's backstack on session change
    if (auto aplClientRenderer = getAplClientRendererFromWindowId(windowId)) {
        if (auto backExtension = getBackExtensionForRenderer(aplClientRenderer)) {
            backExtension->reset();
        }
    }

    // Notify all audio player extensions of presentation session change.
    for (const auto& audioPlayerExtension : m_audioPlayerExtensions) {
        audioPlayerExtension->setActivePresentationSession(presentationSession.id, presentationSession.skillId);
    }

    m_windowIdToPresentationSession[windowId] = presentationSession;
}

void AplClientBridge::onRenderingEvent(const std::string& token, APLClient::AplRenderingEvent event) {
    ACSDK_DEBUG9(LX(__func__));
    /// no-op - metrics are handled by APL Client
}

int AplClientBridge::getMaxNumberOfConcurrentDownloads() {
    return m_parameters.maxNumberOfConcurrentDownloads;
}

void AplClientBridge::onRestoreDocumentState(std::shared_ptr<APLClient::AplDocumentState> documentState) {
    // We restore into the last rendered (active) window
    auto aplClientRenderer = getAplClientRendererFromAplToken(documentState->token);
    if (aplClientRenderer) {
        // The restored document's token is now associated with the active renderer's window id
        setTokenToWindow(documentState->token, aplClientRenderer->getWindowId());
        return aplClientRenderer->restoreDocumentState(documentState);
    }
}

void AplClientBridge::onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) {
    m_executor.submit([this, state, context]() {
        m_playerActivityState = state;
        for (const auto& audioPlayerExtension : m_audioPlayerExtensions) {
            audioPlayerExtension->updatePlayerActivity(
                playerActivityToString(m_playerActivityState), context.offset.count());
        }
    });
}

void AplClientBridge::renderPlayerInfoCard(
    const std::string& payload,
    templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) {
    setMediaProperties(audioPlayerInfo.mediaProperties);
}

void AplClientBridge::renderTemplateCard(const std::string& payload) {
    // no-op
}

void AplClientBridge::clearPlayerInfoCard() {
    // no-op
}

void AplClientBridge::clearRenderTemplateCard() {
    // no-op
}

void AplClientBridge::onLoginStateProvided(
    const std::string& playerId,
    const acsdkExternalMediaPlayerInterfaces::ObservableSessionProperties sessionStateProperties) {
    // Unused ExternalMediaPlayerObserverInterface function.
}

void AplClientBridge::onPlaybackStateProvided(
    const std::string& playerId,
    const acsdkExternalMediaPlayerInterfaces::ObservablePlaybackStateProperties playbackStateProperties) {
    m_executor.submit([this, playbackStateProperties]() {
        if (playbackStateProperties.state == "IDLE") {
            m_playerActivityState = avsCommon::avs::PlayerActivity::IDLE;
        } else if (playbackStateProperties.state == "STOPPED") {
            m_playerActivityState = avsCommon::avs::PlayerActivity::STOPPED;
        } else if (playbackStateProperties.state == "FINISHED") {
            m_playerActivityState = avsCommon::avs::PlayerActivity::FINISHED;
        } else if (playbackStateProperties.state == "PLAYING") {
            m_playerActivityState = avsCommon::avs::PlayerActivity::PLAYING;
        } else if (playbackStateProperties.state == "PAUSED") {
            m_playerActivityState = avsCommon::avs::PlayerActivity::PAUSED;
        }

        int audioItemOffset = m_mediaProperties->getAudioItemOffset().count();
        for (const auto& audioPlayerExtension : m_audioPlayerExtensions) {
            audioPlayerExtension->updatePlayerActivity(playbackStateProperties.state, audioItemOffset);
        }
    });
}

std::shared_ptr<APLClient::Extensions::Backstack::AplBackstackExtension> AplClientBridge::getBackExtensionForRenderer(
    const std::shared_ptr<APLClient::AplClientRenderer>& aplClientRenderer) {
    return std::dynamic_pointer_cast<APLClient::Extensions::Backstack::AplBackstackExtension>(
        aplClientRenderer->getExtension(APLClient::Extensions::Backstack::URI));
}

void AplClientBridge::onAudioPlayerPlay() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() { m_guiManager->handlePlaybackPlay(); });
}

void AplClientBridge::onAudioPlayerPause() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() { m_guiManager->handlePlaybackPause(); });
}

void AplClientBridge::onAudioPlayerNext() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() { m_guiManager->handlePlaybackNext(); });
}

void AplClientBridge::onAudioPlayerPrevious() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() { m_guiManager->handlePlaybackPrevious(); });
}

void AplClientBridge::onAudioPlayerSeekToPosition(int offsetInMilliseconds) {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this, offsetInMilliseconds]() {
        for (const auto& audioPlayerExtension : m_audioPlayerExtensions) {
            audioPlayerExtension->updatePlaybackProgress(offsetInMilliseconds);
        }
        m_guiManager->handlePlaybackSeekTo(offsetInMilliseconds);
    });
}

void AplClientBridge::onAudioPlayerSkipForward() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() { m_guiManager->handlePlaybackSkipForward(); });
}

void AplClientBridge::onAudioPlayerSkipBackward() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() { m_guiManager->handlePlaybackSkipBackward(); });
}

void AplClientBridge::onAudioPlayerToggle(const std::string& name, bool checked) {
    ACSDK_DEBUG3(LX(__func__).d("toggle", name).d("checked", checked));
    m_executor.submit([this, name, checked]() { m_guiManager->handlePlaybackToggle(name, checked); });
}

void AplClientBridge::setMetricRecorder(
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
#ifdef ENABLE_APL_TELEMETRY
    if (metricRecorder) {
        auto sink = std::make_shared<TelemetrySink>(metricRecorder);
        m_aplClientBinding->onTelemetrySinkUpdated(sink);
    }
#endif
}

void AplClientBridge::handleRenderingEvent(const std::string& token, APLClient::AplRenderingEvent event) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, event] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (aplClientRenderer) {
            aplClientRenderer->onRenderingEvent(event);
        }
    });
}

void AplClientBridge::handleDisplayMetrics(const std::string& windowId, const std::string& jsonPayload) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, windowId, jsonPayload] {
        auto aplClientRenderer = getAplClientRendererFromWindowId(windowId);
        if (aplClientRenderer) {
            aplClientRenderer->onMetricsReported(jsonPayload);
        }
    });
}

void AplClientBridge::onRenderDirectiveReceived(
    const std::string& token,
    const std::chrono::steady_clock::time_point& receiveTime) {
    ACSDK_DEBUG9(LX(__func__));
    m_executor.submit([this, token, receiveTime] {
        auto aplClientRenderer = getAplClientRendererFromAplToken(token);
        if (aplClientRenderer) {
            aplClientRenderer->onRenderDirectiveReceived(receiveTime);
        }
    });
}

std::shared_ptr<APLClient::AplClientRenderer> AplClientBridge::getAplClientRendererFromWindowId(
    const std::string& windowId) {
    std::shared_ptr<APLClient::AplClientRenderer> renderer = nullptr;
    auto m_aplClientRendererMapIter = m_aplClientRendererMap.find(windowId);
    if (m_aplClientRendererMapIter != m_aplClientRendererMap.end()) {
        renderer = m_aplClientRendererMapIter->second;
    }

    if (!renderer) {
        ACSDK_WARN(LX("getAplClientRendererFromWindowIdFailed")
                       .d("targetWindowId", windowId)
                       .m("Unable to find renderer for this windowId"));
    }

    return renderer;
}

std::shared_ptr<APLClient::AplClientRenderer> AplClientBridge::getAplClientRendererFromAplToken(
    const std::string& aplToken) {
    std::shared_ptr<APLClient::AplClientRenderer> renderer = nullptr;
    auto windowId = m_windowTokenMapping.getWindowByToken(aplToken);
    if (!windowId.empty()) {
        auto m_aplClientRendererMapIter = m_aplClientRendererMap.find(windowId);
        if (m_aplClientRendererMapIter != m_aplClientRendererMap.end()) {
            renderer = m_aplClientRendererMapIter->second;
        }

        if (!renderer) {
            ACSDK_WARN(LX("getAplClientRendererFromAplTokenFailed")
                           .d("APLToken", aplToken)
                           .m("Unable to find renderer for this token"));
        }
    }

    return renderer;
}

void AplClientBridge::setTokenToWindow(const std::string& token, const std::string& windowId) {
    m_windowTokenMapping.insert(token, windowId);
}

/// TODO: Implement the onAudioPlayerLyricDataFlushed function from the @c AplAudioPlayerExtensionObserverInterface
void AplClientBridge::onAudioPlayerLyricDataFlushed(
    const std::string& token,
    long durationInMilliseconds,
    const std::string& lyricData) {
    ACSDK_DEBUG3(LX(__func__));
}

void AplClientBridge::doShutdown() {
    m_updateTimer.stop();
    m_executor.shutdown();
}

std::string AplClientBridge::getMaxAPLVersion() const {
    return m_aplClientBinding->getAPLVersionReported();
}

template <typename... Args1, typename... Args2>
void AplClientBridge::executeNotifyObservers(APLViewhostObserverFunc<Args1...> func, Args2&&... args) {
    auto it = m_observers.begin();
    while (it != m_observers.end()) {
        if (auto observer = it->lock()) {
            (*observer.*func)(std::forward<Args2>(args)...);
            ++it;
        } else {
            // Weak ptr expired, remove this from the list of observers
            it = m_observers.erase(it);
        }
    }
}

void AplClientBridge::initializeRenderersRequest(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleInitializeAPLRenderers(payload);
}

void AplClientBridge::metricsReport(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleDisplayMetrics(payload);
}

void AplClientBridge::viewhostEvent(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleAplEvent(payload);
}

void AplClientBridge::renderCompleted(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleRenderComplete(payload);
}

void AplClientBridge::renderDocumentRequest(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleLocalRenderDocument(payload);
}

void AplClientBridge::executeCommandsRequest(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleLocalExecuteCommands(payload);
}

void AplClientBridge::clearDocumentRequest(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }
    m_guiClient->handleLocalClearDocument(payload);
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
