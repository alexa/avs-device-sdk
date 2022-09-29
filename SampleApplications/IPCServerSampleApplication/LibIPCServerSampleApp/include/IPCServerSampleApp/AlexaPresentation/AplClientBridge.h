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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLCLIENTBRIDGE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLCLIENTBRIDGE_H_

#include <acsdk/APLCapabilityCommonInterfaces/PresentationSession.h>
#include <acsdk/Sample/TemplateRuntime/TemplateRuntimePresentationAdapterObserverInterface.h>
#include <acsdkAudioPlayerInterfaces/AudioPlayerObserverInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerObserverInterface.h>

// Disable warnings from APLClient
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <APLClient/AplClientBinding.h>
#include <APLClient/AplRenderingEvent.h>
#include <APLClient/AplRenderingEventObserver.h>
#include <APLClient/Extensions/AplCoreExtensionInterface.h>
#include <APLClient/Extensions/AudioPlayer/AplAudioPlayerExtension.h>
#include <APLClient/Extensions/AudioPlayer/AplAudioPlayerExtensionObserverInterface.h>
#include <APLClient/Extensions/Backstack/AplBackstackExtension.h>
#include <APLClient/Extensions/Backstack/AplBackstackExtensionObserver.h>
#pragma GCC diagnostic pop
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <Communication/MessagingServerObserverInterface.h>
#include "IPCServerSampleApp/AlexaPresentation/APLViewhostInterface.h"
#include "IPCServerSampleApp/AlexaPresentation/APLViewhostObserverInterface.h"
#include "IPCServerSampleApp/CachingDownloadManager.h"
#include "IPCServerSampleApp/GUI/GUIManager.h"
#include "IPCServerSampleApp/IPC/Components/APLClientHandler.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/APLClientHandlerInterface.h"
#include "IPCServerSampleApp/IPC/Namespaces/APLClientNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * A struct that helps storing additional parameters for APLClientBridge.
 */
struct AplClientBridgeParameter {
    // Maximum number of concurrent downloads allowed.
    int maxNumberOfConcurrentDownloads;
};

class AplClientBridge
        : public APLClient::AplOptionsInterface
        , public APLViewhostInterface
        , public communication::MessagingServerObserverInterface
        , public acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface
        , public common::TemplateRuntimePresentationAdapterObserverInterface
        , public acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface
        , public APLClient::Extensions::Backstack::AplBackstackExtensionObserverInterface
        , public APLClient::Extensions::AudioPlayer::AplAudioPlayerExtensionObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public ipc::APLClientHandlerInterface
        , public std::enable_shared_from_this<AplClientBridge> {
public:
    static std::shared_ptr<AplClientBridge> create(
        std::shared_ptr<CachingDownloadManager> contentDownloadManager,
        std::shared_ptr<gui::GUIClientInterface> guiClient,
        AplClientBridgeParameter parameters);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name AplOptionsInterface Functions
    /// {
    void sendMessage(const std::string& token, const std::string& payload) override;

    void resetViewhost(const std::string& token) override;

    std::string downloadResource(const std::string& source) override;

    std::chrono::milliseconds getTimezoneOffset() override;

    void onActivityStarted(const std::string& token, const std::string& source) override;

    void onActivityEnded(const std::string& token, const std::string& source) override;

    void onSendEvent(const std::string& token, const std::string& event) override;

    void onCommandExecutionComplete(
        const std::string& token,
        APLClient::AplCommandExecutionEvent event,
        const std::string& message) override;

    void onRenderDocumentComplete(const std::string& token, bool result, const std::string& error) override;

    void onVisualContextAvailable(const std::string& token, unsigned int stateRequestToken, const std::string& context)
        override;

    void onSetDocumentIdleTimeout(const std::string& token, const std::chrono::milliseconds& timeout) override;

    void onRenderingEvent(const std::string& token, APLClient::AplRenderingEvent event) override;

    void onFinish(const std::string& token) override;

    void onDataSourceFetchRequestEvent(const std::string& token, const std::string& type, const std::string& payload)
        override;

    void onRuntimeErrorEvent(const std::string& token, const std::string& payload) override;

    void onExtensionEvent(
        const std::string& aplToken,
        const std::string& uri,
        const std::string& name,
        const std::string& source,
        const std::string& params,
        unsigned int event,
        std::shared_ptr<APLClient::Extensions::AplCoreExtensionEventCallbackResultInterface> resultCallback) override;

    void logMessage(APLClient::LogLevel level, const std::string& source, const std::string& message) override;

    int getMaxNumberOfConcurrentDownloads() override;

    /// }

    /// @name MessagingServerObserverInterface Functions
    /// @{
    void onConnectionOpened() override;

    void onConnectionClosed() override;
    /// @}

    /// @name AudioPlayerObserverInterface methods
    /// @{
    void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) override;
    /// }

    /// @name TemplateRuntimePresentationAdapterObserverInterface Functions
    /// @{
    void renderTemplateCard(const std::string& jsonPayload) override;
    void renderPlayerInfoCard(
        const std::string& jsonPayload,
        templateRuntimeInterfaces::TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo) override;
    void clearRenderTemplateCard() override;
    void clearPlayerInfoCard() override;
    /// @}

    /// @name ExternalMediaPlayerObserverInterface methods
    /// @{
    void onLoginStateProvided(
        const std::string& playerId,
        acsdkExternalMediaPlayerInterfaces::ObservableSessionProperties sessionStateProperties) override;

    void onPlaybackStateProvided(
        const std::string& playerId,
        acsdkExternalMediaPlayerInterfaces::ObservablePlaybackStateProperties playbackStateProperties) override;
    /// }

    /// @name AplBackstackExtensionObserverInterface Functions
    /// @{
    void onRestoreDocumentState(std::shared_ptr<APLClient::AplDocumentState> documentState) override;
    /// @}

    /// @name AplAudioPlayerExtensionObserverInterface methods
    /// @{
    void onAudioPlayerPlay() override;

    void onAudioPlayerPause() override;

    void onAudioPlayerNext() override;

    void onAudioPlayerPrevious() override;

    void onAudioPlayerSeekToPosition(int offsetInMilliseconds) override;

    void onAudioPlayerToggle(const std::string& name, bool checked) override;

    void onAudioPlayerSkipForward() override;

    void onAudioPlayerSkipBackward() override;

    void onAudioPlayerLyricDataFlushed(
        const std::string& token,
        long durationInMilliseconds,
        const std::string& lyricData) override;
    /// }

    void onUpdateTimer();

    void setGUIManager(std::shared_ptr<gui::GUIServerInterface> guiManager);

    /// @name APLViewhostInterface functions
    /// @{
    void renderDocument(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::string& token,
        const std::string& document,
        const std::string& datasources,
        const std::string& supportedViewports,
        const std::string& windowId) override;

    void clearDocument(const std::string& token) override;

    void executeCommands(const std::string& jsonPayload, const std::string& token) override;

    void interruptCommandSequence(const std::string& token) override;

    void dataSourceUpdate(const std::string& sourceType, const std::string& jsonPayload, const std::string& token)
        override;

    void addObserver(const APLViewhostObserverInterfacePtr& observer) override;

    void removeObserver(const APLViewhostObserverInterfacePtr& observer) override;

    void onRenderDirectiveReceived(const std::string& token, const std::chrono::steady_clock::time_point& receiveTime)
        override;

    std::string getMaxAPLVersion() const override;

    void setMetricRecorder(std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) override;

    void provideState(const std::string& aplToken, const unsigned int stateRequestToken) override;

    bool handleBack(const std::string& windowId) override;
    /// @}

    /// @name APLClientHandlerInterface methods
    /// @{
    void initializeRenderersRequest(const std::string& message) override;
    void metricsReport(const std::string& message) override;
    void viewhostEvent(const std::string& message) override;
    void renderCompleted(const std::string& message) override;
    void renderDocumentRequest(const std::string& message) override;
    void executeCommandsRequest(const std::string& message) override;
    void clearDocumentRequest(const std::string& message) override;
    /// }

    void onMessage(const std::string& windowId, const std::string& message);

    void handleRenderingEvent(const std::string& token, APLClient::AplRenderingEvent event);

    void handleDisplayMetrics(const std::string& windowId, const std::string& jsonPayload);

    /**
     *  Initialize empty client renderer and load corresponding supported extensions
     *  @param windowId id of the window to be created
     *  @param supportedExtensions URIs of all supported APL extensions for this window
     */
    void initializeRenderer(const std::string& windowId, const std::set<std::string>& supportedExtensions);

    /**
     * Adds the given extension to @c m_sharedRegisteredExtensions map.
     *
     * @param extension Pointer to @c AplCoreExtensionInterface which is being added to @c m_sharedRegisteredExtensions.
     */
    void registerSharedExtension(const std::shared_ptr<APLClient::Extensions::AplCoreExtensionInterface>& extension);

    /**
     * Returns a shared pointer to the @c AplClientRenderer holding root-context for a given aplToken
     * Note:- This is not a thread safe method, avoid calling this method outside @c executor context
     *
     * @param the APL token in context
     * @return the instance of @c APLClientRenderer if found, else nullptr
     */
    std::shared_ptr<APLClient::AplClientRenderer> getAplClientRendererFromAplToken(const std::string& aplToken);

    /**
     * Returns a shared pointer to the @c AplClientRenderer holding root-context for a target window ID
     * Note:- This is not a thread safe method, avoid calling this method outside @c executor context
     *
     * @param the window id in context
     * @return the instance of @c APLClientRenderer if found, else nullptr
     */
    std::shared_ptr<APLClient::AplClientRenderer> getAplClientRendererFromWindowId(const std::string& windowId);

    /**
     * Sets the media properties to be used to extract the media player state
     * @param mediaProperties
     */
    void setMediaProperties(std::shared_ptr<avsCommon::sdkInterfaces::MediaPropertiesInterface> mediaProperties);

    /**
     * Initializes the IPC handlers implemented by the APLClientBridge.
     */
    void initialize();

private:
    AplClientBridge(
        std::shared_ptr<CachingDownloadManager> contentDownloadManager,
        std::shared_ptr<gui::GUIClientInterface> guiClient,
        AplClientBridgeParameter parameters);

    /**
     * Set token to window id in the managed m_aplTokenToWindowIdMap
     * @param token of the apl document.
     * @param windowId the id of the window presenting the document with provided token.
     */
    void setTokenToWindow(const std::string& token, const std::string& windowId);

    /**
     * Gets the back extension associated with the provided renderer.
     *
     * @param aplClientRenderer shared pointer of @c APLClientRenderer to check for associated backstack extension.
     * @return Shared pointer to the back extension instance if available, else nullptr
     */
    static std::shared_ptr<APLClient::Extensions::Backstack::AplBackstackExtension> getBackExtensionForRenderer(
        const std::shared_ptr<APLClient::AplClientRenderer>& aplClientRenderer);

    /// Alias for APLViewhostObserverInterface function
    template <typename... Args>
    using APLViewhostObserverFunc = void (APLViewhostObserverInterface::*)(Args...);

    /**
     * Call the given function with the given args on all registered observers
     * @tparam Args1 Function argument types
     * @tparam Args2 Argument types, must be convertible to corresponding Args1 type
     * @param func The function to call
     * @param args The args to pass to the function
     */
    template <typename... Args1, typename... Args2>
    void executeNotifyObservers(APLViewhostObserverFunc<Args1...> func, Args2&&... args);

    /**
     * Executor method for handling the a presentation state change event, must be called in executor context
     * @param windowId The new window ID
     * @param presentationSession The new presentation session
     */
    void executeOnPresentationSessionChanged(
        const std::string& windowId,
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession);

    /// Helper class for mapping tokens to windows and vice-versa (1-N mapping from window to tokens)
    class TokenWindowBiMap {
    public:
        /**
         * Inserts the token window combination into the map
         * @param token The token
         * @param window The corresponding window
         */
        void insert(const std::string& token, const std::string& window) {
            m_tokenToWindow[token] = window;
            m_windowToToken[window].insert(token);
        }

        /**
         * Erases the given token from the mapping
         * @param token The token to erase
         */
        void eraseToken(const std::string& token) {
            auto tokenToWindowIt = m_tokenToWindow.find(token);
            if (tokenToWindowIt == m_tokenToWindow.end()) {
                return;
            }

            auto windowId = tokenToWindowIt->second;
            m_tokenToWindow.erase(tokenToWindowIt);
            m_windowToToken[windowId].erase(token);
        }

        /**
         * Erases the given window and all corresponding tokens from the mapping
         * @param window The window to erase
         */
        void eraseWindow(const std::string& window) {
            for (auto& token : m_windowToToken[window]) {
                m_tokenToWindow.erase(token);
            }
            m_windowToToken.erase(window);
        }

        /**
         * Gets the window associated with the given token
         * @param token The token to search for
         * @return The window ID, or an empty string if no mapping exists
         */
        std::string getWindowByToken(const std::string& token) {
            auto tokenToWindowIt = m_tokenToWindow.find(token);
            if (tokenToWindowIt == m_tokenToWindow.end()) {
                return "";
            }

            return tokenToWindowIt->second;
        }

    private:
        /// Token to window mapping
        std::unordered_map<std::string, std::string> m_tokenToWindow;

        /// Window to token mapping
        std::unordered_map<std::string, std::unordered_set<std::string>> m_windowToToken;
    };

    /// Pointer to the download manager for retrieving resources
    std::shared_ptr<CachingDownloadManager> m_contentDownloadManager;

    /// An internal timer use to run the APL Core update loop
    avsCommon::utils::timing::Timer m_updateTimer;

    /// Pointer to the APL Client
    std::unique_ptr<APLClient::AplClientBinding> m_aplClientBinding;

    /// Pointer to the GUI Manager
    std::shared_ptr<gui::GUIServerInterface> m_guiManager;

    /// Pointer to the GUI Client
    std::shared_ptr<gui::GUIClientInterface> m_guiClient;

    /// APLClient handler.
    std::shared_ptr<ipc::APLClientHandler> m_aplClientHandler;

    /// Whether a render is currently queued
    std::atomic_bool m_renderQueued;

    /// An internal struct that stores additional parameters for AplClientBridge.
    AplClientBridgeParameter m_parameters;

    /// Collection of all @c AudioPlayerExtensions
    std::vector<std::shared_ptr<APLClient::Extensions::AudioPlayer::AplAudioPlayerExtension>> m_audioPlayerExtensions;

    /// The @c PlayerActivity state of the @c AudioPlayer
    avsCommon::avs::PlayerActivity m_playerActivityState;

    /// Collection of Pointer to the @c AplClientRenderer for every windowId
    std::unordered_map<std::string, std::shared_ptr<APLClient::AplClientRenderer>> m_aplClientRendererMap;

    /// Map containing the active presentation session for each windowId
    std::unordered_map<std::string, aplCapabilityCommonInterfaces::PresentationSession> m_windowIdToPresentationSession;

    /// Map containing the extensions for each URI shared by all renderers
    std::unordered_map<std::string, std::shared_ptr<APLClient::Extensions::AplCoreExtensionInterface>>
        m_sharedRegisteredExtensions;

    /// Observer to APL document activity.
    std::vector<std::weak_ptr<APLViewhostObserverInterface>> m_observers;

    /// Mapping from windows to tokens - this includes all tokens which may be in the backstack
    TokenWindowBiMap m_windowTokenMapping;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;

    /// The @c MediaPropertiesInterface for the current @c AudioPlayer
    std::shared_ptr<avsCommon::sdkInterfaces::MediaPropertiesInterface> m_mediaProperties;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLCLIENTBRIDGE_H_
