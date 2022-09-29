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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUICLIENT_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUICLIENT_H_

#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <string>

#include <rapidjson/document.h>

#include <acsdk/APLCapabilityCommonInterfaces/APLDocumentObserverInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorTypes.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#ifdef ENABLE_COMMS
#include <AVSCommon/SDKInterfaces/CallStateObserverInterface.h>
#endif
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEventObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <acsdkSampleApplicationInterfaces/UIAuthNotifierInterface.h>
#include <acsdkSampleApplicationInterfaces/UIStateAggregatorInterface.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/RegistrationObserverInterface.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <Communication/MessagingServerInterface.h>

#include "IPCServerSampleApp/AlexaPresentation/AplClientBridge.h"
#include "IPCServerSampleApp/AlexaPresentation/APLRuntimePresentationAdapter.h"
#include "IPCServerSampleApp/AlexaPresentation/IPCAPLAgent.h"
#include "IPCServerSampleApp/GUI/GUIClientInterface.h"
#include "IPCServerSampleApp/GUI/GUIServerInterface.h"
#include "IPCServerSampleApp/IPC/Components/AudioFocusManagerHandler.h"
#include "IPCServerSampleApp/IPC/Components/SystemHandler.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/AudioFocusManagerHandlerInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/SystemHandlerInterface.h"
#include "IPCServerSampleApp/IPC/IPCRouter.h"
#include "IPCServerSampleApp/IPC/IPCDispatcherInterface.h"
#include "IPCServerSampleApp/IPC/IPCVersionManager.h"
#include "IPCServerSampleApp/IPC/Namespaces/AudioFocusManagerNamespace.h"
#include "IPCServerSampleApp/IPC/Namespaces/SystemNamespace.h"
#include "IPCServerSampleApp/Messages/MessageInterface.h"
#include "IPCServerSampleApp/SampleApplicationReturnCodes.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {
/**
 * Manages all GUI related operations to be called from the GUI and the SDK
 * Encapsulates APL core Client implementation and APL Core integration point.
 */
class GUIClient
        : public communication::MessagingServerInterface
        , public communication::MessageListenerInterface
        , public registrationManager::RegistrationObserverInterface
        , public communication::MessagingServerObserverInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::sdkInterfaces::CapabilitiesObserverInterface
        , public gui::GUIClientInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler
        , public std::enable_shared_from_this<GUIClient>
        , public acsdkSampleApplicationInterfaces::UIAuthNotifierInterface
        , public acsdkSampleApplicationInterfaces::UIStateAggregatorInterface
        , public ipc::IPCDispatcherInterface
        , public ipc::AudioFocusManagerHandlerInterface
        , public ipc::SystemHandlerInterface {
public:
    /**
     * Create a @c GUIClient
     *
     * @param serverImplementation An implementation of @c MessagingInterface
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @note The @c serverImplementation should implement the @c start method in a blocking fashion.
     * @param deviceInfo DeviceInfo which reflects the device setup credentials.
     * @return an instance of GUIClient.
     */
    static std::shared_ptr<GUIClient> create(
        std::shared_ptr<MessagingServerInterface> serverImplementation,
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
        const std::shared_ptr<ipc::IPCVersionManager> ipcVersionManager,
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo);

    /// @name GUIClientInterface Functions
    /// @{
    void setGUIManager(std::shared_ptr<gui::GUIServerInterface> guiManager) override;
    bool acquireFocus(
        std::string avsInterface,
        std::string channelName,
        avsCommon::avs::ContentType contentType,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;
    bool releaseFocus(
        std::string avsInterface,
        std::string channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;
#ifdef ENABLE_COMMS
    void sendCallStateInfo(
        const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& callStateInfo) override;
#endif
    void sendMessage(messages::MessageInterface& message) override;
    bool finalizeClientInitialization(bool errorState) override;
    void handleInitializeAPLRenderers(rapidjson::Document& payload) override;
    void handleDisplayMetrics(rapidjson::Document& payload) override;
    void handleAplEvent(rapidjson::Document& payload) override;
    void handleRenderComplete(rapidjson::Document& payload) override;
    void handleLocalRenderDocument(rapidjson::Document& payload) override;
    void handleLocalExecuteCommands(rapidjson::Document& payload) override;
    void handleLocalClearDocument(rapidjson::Document& payload) override;
    void handleOnFocusChangedReceivedConfirmation(GUIToken token) override;
    void focusAcquireRequest(
        const GUIToken token,
        const std::string& avsInterface,
        const std::string& channelName,
        avsCommon::avs::ContentType contentType) override;
    void focusReleaseRequest(const GUIToken token, const std::string& avsInterface, const std::string& channelName)
        override;
    void setLocales(const std::string& localeStr) override;
    std::shared_ptr<ipc::IPCRouter> getIPCRouter() override;
    /// @}

    /// @name MessagingServerInterface Functions
    /// @{
    bool start() override;
    void writeMessage(const std::string& payload) override;
    void setMessageListener(std::shared_ptr<MessageListenerInterface> messageListener) override;
    void stop() override;
    bool isReady() override;
    void setObserver(const std::shared_ptr<communication::MessagingServerObserverInterface>& observer) override;
    /// @}

    /// @name MessagingServerObserverInterface Function
    /// @{
    void onConnectionOpened() override;
    void onConnectionClosed() override;
    /// @}

    /// @name MessageListenerInterface Function
    /// @{
    void onMessage(const std::string& jsonMessage) override;
    /// @}

    /// @name AuthObserverInterface Function
    /// @{
    void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
    /// @}

    /// @name CapabilitiesObserverInterface Methods
    /// @{
    void onCapabilitiesStateChange(
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addedOrUpdatedEndpoints,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) override;
    /// }

    /// @name CustomerDataHandlerInterface Function
    /// @{
    void clearData() override;
    /// @}

    /// @name UIAuthNotifierInterface Function
    /// @{
    void notifyAuthorizationRequest(const std::string& url, const std::string& code) override;
    void notifyAuthorizationStateChange(avsCommon::sdkInterfaces::AuthObserverInterface::State state) override;
    /// @}

    /// @name UIStateAggregatorInterface Function
    /// @{
    void notifyAlexaState(const std::string& state) override;
    /// @}

    /**
     * Processes user input until a quit command or a device reset is triggered.
     *
     * @return Returns a @c SampleAppReturnCode.
     */
    SampleAppReturnCode run();

    /// @name RegistrationObserverInterface Functions
    /// @{
    void onLogout() override;
    /// @}

    /// @name IPCDispatcherInterface Functions
    /// @{
    bool dispatch(const std::string& message) override;
    /// @}

    /// @name AudioFocusManagerHandlerInterface methods
    /// @{
    void acquireChannelRequest(const std::string& message) override;
    void releaseChannelRequest(const std::string& message) override;
    void focusChangedReport(const std::string& message) override;
    /// }

    /// @name SystemHandlerInterface methods
    /// @{
    void authorizationStateRequest(const std::string& message) override;
    void alexaStateRequest(const std::string& message) override;
    void authorizationInfoRequest(const std::string& message) override;
    void localesRequest(const std::string& message) override;
    /// }

    /**
     * Sets the APL Client Bridge
     * @param aplClientBridge The APL Client bridge
     * @param aplVersionChanged True if the APL Client version has changed from last run.
     */
    void setAplClientBridge(std::shared_ptr<AplClientBridge> aplClientBridge, bool aplVersionChanged);

    /**
     * Sets the APL Runtime adapter
     * @param aplRuntimePresentationAdapter Shared pointer to APL runtime adapter
     */
    void setAPLRuntimePresentationAdapter(
        const std::shared_ptr<APLRuntimePresentationAdapter>& aplRuntimePresentationAdapter);

    /// Adds a mediaplayer to the m_mediaPlayerMap with the key as the name and value as the mediaplayer
    void addToMediaPlayerMap(
        const std::string& name,
        const std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>& mediaPlayer);

    /// Initializes the IPC router.
    void initIPCRouter();

    /**
     * Provides the IPC Client with information to complete CBL based authorization.
     *
     * @param url The url to use to complete CBL-based LWA authrorization.
     * @param code The CBL code to use to complete authorization.
     * @param clientId AVS Device Id.
     */
    void sendCompleteAuthorizationMessage(const std::string& url, const std::string& code, const std::string& clientId);

    /**
     * Informs the IPC Client of changes in the state of the Alexa client connection.
     *
     * @param state Enumerated state of the Alexa client.
     */
    void sendSetAlexaStateMessage(const std::string& state);

    /**
     * Informs the IPC Client of changes in Alexa Authorization status.
     *
     * @param state Enumerated authorization state.
     */
    void sendSetAuthorizationStateMessage(const std::string& state);

    /**
     * Informs the  IPC Client of changes in supported locales for the SDK.
     *
     * @param localeStr The locale(s) for the device. In single-locale mode, contains one locale string.
     * In multi-locale mode, the first string indicates the primary locale, and any other strings correspond to
     * secondary locales.
     */
    void sendSetLocalesMessage(const std::string& localeStr);

private:
    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * This class represents requestors as clients of FocusManager and handle notifications.
     */
    class ProxyFocusObserver : public avsCommon::sdkInterfaces::ChannelObserverInterface {
    public:
        /**
         * Constructor.
         *
         * @param avsInterface Name of related AVS Interface.
         * @param token Requester token.
         * @param focusBridge FocusBridge to perform operations through.
         * @param channelName Name of related channel.
         */
        ProxyFocusObserver(
            std::string avsInterface,
            GUIToken token,
            std::shared_ptr<GUIClient> focusBridge,
            std::string channelName);

        /// @name ChannelObserverInterface Functions
        /// @{
        void onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) override;
        ///@}

    private:
        /// AVS Interface Name
        const std::string m_avsInterface;

        /// Related requester token.
        GUIToken m_token;

        /// Parent FocusBridge.
        std::shared_ptr<GUIClient> m_focusBridge;

        /// Focus channel name.
        const std::string m_channelName;
    };

    /**
     * Constructor
     */
    GUIClient(
        std::shared_ptr<MessagingServerInterface> serverImplementation,
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager,
        std::shared_ptr<ipc::IPCVersionManager> ipcVersionManager,
        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo);

    /// Server worker thread.
    void serverThread();

    /// Send initRequest message to the client and wait for init response.
    void sendInitRequestAndWait();

    /**
     * Handle SendCallStateInfo event.
     */
#ifdef ENABLE_COMMS
    void executeSendCallStateInfo(
        const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& callStateInfo);
#endif

    /**
     * Send focus response.
     *
     * @param token Requester token.
     * @param result Result of focus operation.
     */
    void executeSendFocusResponse(const GUIToken token, const bool result);

    /**
     * Starting timer to release channel in situations when focus operation result or
     * onFocusChanged event was not received by GUI so it will not know if it needs to release it.
     *
     * @param avsInterface The AVS Interface to release.
     * @param token Requester token.
     * @param channelName The channel to release.
     */
    void startAutoreleaseTimer(const std::string& avsInterface, const GUIToken token, const std::string& channelName);

    /**
     * Handle autoRelease.
     *
     * @param avsInterface The AVS Interface to release.
     * @param token Requester token.
     * @param channelName Channel name.
     */
    void autoRelease(const std::string& avsInterface, const GUIToken token, const std::string& channelName);

    /**
     * Send focus change event notification.
     *
     * @param token Requester token.
     * @param state Resulting channel state.
     */
    void sendOnFocusChanged(const GUIToken token, const avsCommon::avs::FocusState state);

    /**
     * Sends a GUI Message to the server.
     * @param message The message to be written.
     */
    void executeSendMessage(messages::MessageInterface& message);

    /**
     * Write a message to the server.
     *
     * @param payload an arbitrary string
     */
    void executeWriteMessage(const std::string& payload);

    /**
     * An internal function handling audio focus requests in the executor thread.
     * @param avsInterface The AVS Interface requesting focus.
     * @param channelName The channel to be requested.
     * @param contentType The type of content acquiring focus.
     * @param channelObserver the channelObserver to be notified.
     */
    bool executeAcquireFocus(
        std::string avsInterface,
        std::string channelName,
        avsCommon::avs::ContentType contentType,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver);

    /**
     * An internal function handling release audio focus requests in the executor thread.
     * @param avsInterface The avsInterface to be released.
     * @param channelName The channel to be released.
     * @param channelObserver the channelObserver to be notified.
     */
    bool executeReleaseFocus(
        std::string avsInterface,
        std::string channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver);

    /**
     * Registers namespaces and versions that will be handled by the IPC Server
     */
    void registerNamespaces();

    /**
     * Handle accept call message
     * @param payload The payload retrieved from message holding this event.
     */
    void executeHandleAcceptCall(rapidjson::Document& payload);

    /**
     * Handle stop call message
     * @param payload The payload retrieved from message holding this event.
     */
    void executeHandleStopCall(rapidjson::Document& payload);

    /**
     * Handle enable local video message.
     * @param payload The payload retrieved from message holding this event.
     */
    void executeHandleEnableLocalVideo(rapidjson::Document& payload);

    /**
     * Handle disable local video message.
     * @param payload The payload retrieved from message holding this event.
     */
    void executeHandleDisableLocalVideo(rapidjson::Document& payload);

#ifdef ENABLE_COMMS
    /**
     * Handle send DTMF key message.
     * @param payload The payload retrieved from message holding this event.
     */
    void executeHandleSendDtmf(rapidjson::Document& payload);
#endif

    /**
     * Creates a runtime error payload for invalid windowId reported found in a directive
     * @param errorMsg Error message to be sent with runtime error
     * @param aplToken APL Token for directive with error
     */
    void reportInvalidWindowIdRuntimeError(const std::string& errorMsg, const std::string& aplToken);

    // The GUI manager implementation.
    std::shared_ptr<gui::GUIServerInterface> m_guiManager;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;

    // The server implementation.
    std::shared_ptr<MessagingServerInterface> m_serverImplementation;

    /// The thread used by the underlying server
    std::thread m_serverThread;

    // The thread used for init messages.
    std::thread m_initThread;

    /// Synchronize access between threads.
    std::mutex m_mutex;

    /// condition variable notify server state changed
    std::condition_variable m_cond;

    /// Has the underlying server started.
    std::atomic_bool m_hasServerStarted;

    /// Has initialization message from been received.
    std::atomic_bool m_initMessageReceived;

    /// Is the server in unrecoverable error state.
    std::atomic_bool m_errorState;

    /// The Listener to receive the messages
    std::shared_ptr<MessageListenerInterface> m_messageListener;

    /// Has the user logged out.
    std::atomic_bool m_shouldRestart;

    /// Server observer
    std::shared_ptr<communication::MessagingServerObserverInterface> m_observer;

    /// The APL Client Bridge
    std::shared_ptr<AplClientBridge> m_aplClientBridge;

    /// The IPC Version Manager
    std::shared_ptr<ipc::IPCVersionManager> m_ipcVersionManager;

    /// Flag to indicate that a fatal failure occurred. In this case, customer can either reset the device or kill
    /// the app.
    std::atomic_bool m_limitedInteraction;

    /// Map from message type to handling function.
    std::map<std::string, std::function<void(rapidjson::Document&)>> m_messageHandlers;

    /// Mutex for requester maps.
    std::mutex m_mapMutex;

    /// A map of GUI side focus observers (proxies).
    std::map<GUIToken, std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface>> m_focusObservers;

    /// Autorelease timers for case when client not received channel state change message.
    std::map<GUIToken, std::shared_ptr<avsCommon::utils::timing::Timer>> m_autoReleaseTimers;

    /// AudioFocusManager handler.
    std::shared_ptr<ipc::AudioFocusManagerHandler> m_audioFocusManagerIPCHandler;

    /// System handler.
    std::shared_ptr<ipc::SystemHandler> m_systemIPCHandler;

    /// Map to store IPC media players
    std::map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>> m_mediaPlayerMap;

    /// Pointer to APL runtime presentation adapter for handling GUI client initiated APL rendering.
    std::shared_ptr<APLRuntimePresentationAdapter> m_aplRuntimePresentationAdapter;

    /// An instance of an IPC Router.
    std::shared_ptr<ipc::IPCRouter> m_ipcRouter;

    /// Pointer to the agent that handles GUI client initiated APL runtime callbacks;
    std::shared_ptr<IPCAPLAgent> m_ipcAPLAgent;

    /// DeviceInfo object for reporting config information.
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// Cached value of the Alexa client state.
    std::string m_alexaState;

    /// Cached value of the authorization state.
    std::string m_authState;

    /// Cached value of the url to use to complete the CBL-based LWA authorization.
    std::string m_authUrl;

    /// Cached value of the CBL code to use to complete the authorization.
    std::string m_authCode;

    /// Cached value of the locales for the device.
    std::string m_localeStr;

    /// Cached value of the AVS device id.
    std::string m_clientId;
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUICLIENT_H_
