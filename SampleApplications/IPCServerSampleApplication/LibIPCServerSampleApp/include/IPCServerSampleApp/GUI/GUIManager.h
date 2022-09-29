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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIMANAGER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIMANAGER_H_

#include <Audio/MicrophoneInterface.h>
#include <AFML/FocusManager.h>
#include <AVSCommon/AVS/FocusState.h>
#include <acsdkAudioPlayerInterfaces/AudioPlayerObserverInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h>
#include <acsdk/Notifier/Notifier.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateObserverInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/Sample/InteractionManager/InteractionManager.h>
#include <acsdk/Sample/Endpoint/EndpointAlexaLauncherHandler.h>
#include <acsdk/SDKClient/SDKClientRegistry.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsSerializerInterface.h>
#ifdef ENABLE_COMMS
#include <AVSCommon/SDKInterfaces/CallStateObserverInterface.h>
#endif
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingCallbacks.h>
#include <DefaultClient/DefaultClient.h>

#include "IPCServerSampleApp/AlexaPresentation/APLRuntimePresentationAdapter.h"
#include "IPCServerSampleApp/GUILogBridge.h"
#include "IPCServerSampleApp/GUI/CaptureState.h"
#include "IPCServerSampleApp/GUI/GUIActivityEventNotifierInterface.h"
#include "IPCServerSampleApp/GUI/GUIClientInterface.h"
#include "IPCServerSampleApp/GUI/GUIServerInterface.h"
#include "IPCServerSampleApp/GUI/TemplateRuntimePresentationAdapterBridge.h"
#include "IPCServerSampleApp/GUI/NavigationEvent.h"
#include "IPCServerSampleApp/IPC/Components/DoNotDisturbHandler.h"
#include "IPCServerSampleApp/IPC/Components/InteractionManagerHandler.h"
#include "IPCServerSampleApp/IPC/Components/LoggerHandler.h"
#include "IPCServerSampleApp/IPC/Components/SessionSetupHandler.h"
#include "IPCServerSampleApp/IPC/Components/WindowManagerHandler.h"
#include "IPCServerSampleApp/IPC/Namespaces/DoNotDisturbNamespace.h"
#include "IPCServerSampleApp/IPC/Namespaces/InteractionManagerNamespace.h"
#include "IPCServerSampleApp/IPC/Namespaces/LoggerNamespace.h"
#include "IPCServerSampleApp/IPC/Namespaces/SessionSetupNamespace.h"
#include "IPCServerSampleApp/IPC/Namespaces/WindowManagerNamespace.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/DoNotDisturbHandlerInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/InteractionManagerHandlerInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/LoggerHandlerInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/SessionSetupHandlerInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/WindowManagerHandlerInterface.h"
#include "IPCServerSampleApp/IPC/IPCVersionManager.h"
#include "IPCServerSampleApp/TimezoneHelper.h"

#ifdef UWP_BUILD
#include <SSSDKCommon/NullMicrophone.h>
#endif

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/**
 * Manages all GUI related operations to be called from the SDK and the GUIClient
 */
class GUIManager
        : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface
        , public gui::GUIServerInterface
        , public acsdkAudioPlayerInterfaces::AudioPlayerObserverInterface
        , public presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface
        , public avsCommon::sdkInterfaces::FocusManagerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public avsCommon::sdkInterfaces::ChannelObserverInterface
        , public ipc::DoNotDisturbHandlerInterface
        , public ipc::SessionSetupHandlerInterface
        , public ipc::WindowManagerHandlerInterface
        , public ipc::InteractionManagerHandlerInterface
        , public ipc::LoggerHandlerInterface
#ifdef ENABLE_COMMS
        , public avsCommon::sdkInterfaces::CallStateObserverInterface
#endif
        , public std::enable_shared_from_this<GUIManager> {
public:
    /**
     * Create a GUIManager.
     *
     * @param guiClient A pointer to a GUI Client.
     * @param activityEventNotifier A pointer to a activity event notifier.
     * @param micWrapper A pointer to a microphone wrapper.
     * @param interactionManager A pointer to interaction manager.
     * @param launcherHandler A pointer to launcher handler.
     *
     * @return An instance of GUIManager.
     */
    static std::shared_ptr<GUIManager> create(
        std::shared_ptr<gui::GUIClientInterface> guiClient,
        std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier,
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
        std::shared_ptr<common::InteractionManager> interactionManager,
        std::shared_ptr<common::EndpointAlexaLauncherHandler> launcherHandler);

    /// @name GUIServerInterface methods
    /// @{
    void handleRecognizeSpeechRequest(capabilityAgents::aip::Initiator initiator, bool start) override;

    void handleMicrophoneToggle() override;

    void handlePlaybackPlay() override;

    void handlePlaybackPause() override;

    void handlePlaybackNext() override;

    void handlePlaybackPrevious() override;

    void handlePlaybackSeekTo(int offset) override;

    void handlePlaybackSkipForward() override;

    void handlePlaybackSkipBackward() override;

    void handlePlaybackToggle(const std::string& name, bool checked) override;

    void onUserEvent() override;

    bool handleFocusAcquireRequest(
        std::string avsInterface,
        std::string channelName,
        avsCommon::avs::ContentType contentType,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;

    bool handleFocusReleaseRequest(
        std::string avsInterface,
        std::string channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;

    void handleGUIActivityEvent(avsCommon::sdkInterfaces::GUIActivityEvent event, const std::string& source = "")
        override;

    void handleNavigationEvent(NavigationEvent event) override;

    std::vector<visualCharacteristicsInterfaces::WindowTemplate> getWindowTemplates() override;

    std::vector<visualCharacteristicsInterfaces::InteractionMode> getInteractionModes() override;

    visualCharacteristicsInterfaces::DisplayCharacteristics getDisplayCharacteristics() override;

    void setWindowInstances(
        const std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance>& instances,
        const std::string& defaultWindowInstanceId,
        const std::string& audioPlaybackUIWindowId) override;

    bool addWindowInstance(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) override;

    bool removeWindowInstance(const std::string& windowInstanceId) override;

    void updateWindowInstance(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) override;

    bool setDefaultWindowInstance(const std::string& windowInstanceId) override;

    bool serializeInteractionMode(
        const std::vector<visualCharacteristicsInterfaces::InteractionMode>& interactionModes,
        std::string& serializedJson) override;

    bool serializeWindowTemplate(
        const std::vector<visualCharacteristicsInterfaces::WindowTemplate>& windowTemplates,
        std::string& serializedJson) override;

    bool serializeDisplayCharacteristics(
        const visualCharacteristicsInterfaces::DisplayCharacteristics& display,
        std::string& serializedJson) override;

    void forceExit() override;

    std::chrono::milliseconds getDeviceTimezoneOffset() override;

    void handleOnMessagingServerConnectionOpened() override;

    void handleDocumentTerminated(const std::string& token, bool failed) override;

    void acceptCall() override;

    void stopCall() override;

    void enableLocalVideo() override;

    void disableLocalVideo() override;

#ifdef ENABLE_COMMS
    void sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone) override;
#endif

    void handleLocaleChange() override;

    void initClient() override;
    /// }

    /// @name DoNotDisturbHandlerInterface methods
    /// @{
    void doNotDisturbStateChanged(const std::string& message) override;
    void doNotDisturbStateRequest(const std::string& message) override;
    /// }

    /// @name SessionSetupHandlerInterface methods
    /// @{
    void namespaceVersionsReport(const std::string& message) override;
    void clientInitialized(const std::string& message) override;
    void clientConfigRequest(const std::string& message) override;
    /// }

    /// @name WindowManagerHandlerInterface methods
    /// @{
    void visualCharacteristicsRequest(const std::string& message) override;
    void defaultWindowInstanceChanged(const std::string& message) override;
    void windowInstancesReport(const std::string& message) override;
    void windowInstancesAdded(const std::string& message) override;
    void windowInstancesRemoved(const std::string& message) override;
    void windowInstancesUpdated(const std::string& message) override;
    /// }

    /// @name InteractionManagerHandlerInterface methods
    /// @{
    void recognizeSpeechRequest(const std::string& message) override;
    void navigationEvent(const std::string& message) override;
    void guiActivityEvent(const std::string& message) override;
    /// }

    /// @name LoggerHandlerInterface methods
    /// @{
    void logEvent(const std::string& message) override;
    /// }

    /// @name FocusManagerObserverInterface methods
    /// @{
    void onFocusChanged(const std::string& channelName, avsCommon::avs::FocusState newFocus) override;
    /// }

    /// @name AudioPlayerObserverInterface methods
    /// @{
    void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state, const Context& context) override;
    /// }

    /// @name PresentationOrchestratorStateObserverInterface Functions
    /// @{
    void onStateChanged(
        const std::string& windowId,
        const presentationOrchestratorInterfaces::PresentationMetadata& metadata) override;
    /// @}

    /**
     * UXDialogObserverInterface methods
     */
    void onDialogUXStateChanged(DialogUXState newState) override;

    /// @name AudioInputProcessorObserverInterface methods.
    /// @{
    void onStateChanged(AudioInputProcessorObserverInterface::State state) override;
    /// @}

    /// @name CallStateObserverInterface methods.
    /// @{
#ifdef ENABLE_COMMS
    void onCallStateInfoChange(
        const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& newStateInfo) override;
    void onCallStateChange(avsCommon::sdkInterfaces::CallStateObserverInterface::CallState newCallState) override;
#endif
    /// @}

    /**
     * Sets the SDK Client Registry
     * @param clientRegistry Reference to the SDKClientRegistry
     */
    void setClient(std::shared_ptr<sdkClient::SDKClientRegistry> clientRegistry);

    /**
     * Configure settings notifications.
     *
     * @return @true if it succeeds to configure the settings notifications; @c false otherwise.
     */
    bool configureSettingsNotifications();

    /**
     * Set the IPCVersion manager.
     *
     * @param ipcVersionManager Pointer to the @c IPCVersionManager.
     */
    void setIpcVersionManager(std::shared_ptr<ipc::IPCVersionManager> ipcVersionManager);

    /**
     * Parse window instance from the specified json.
     * @param payload The payload to be parsed
     * @param windowInstance [ out ] Parsed object for @c WindowInstance.
     * @return true if parsed successfully, false otherwise.
     */
    bool parseWindowInstance(
        const rapidjson::Value& payload,
        presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance);

#ifdef UWP_BUILD
    void inputAudioFile(const std::string& audioFile);
#endif

    /**
     * This will force clear the DIALOG channel and reset it, allowing for proper cloud-side context when locally
     * stopping DIALOG channel.
     */
    void forceClearDialogChannelFocus();

    /// @name ChannelObserverInterface Methods
    /// @{
    void onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) override;
    /// }

    /**
     * Sets the APL Runtime adapter
     * @param aplRuntimePresentationAdapter Shared pointer to APL runtime adapter
     */
    void setAPLRuntimePresentationAdapter(std::shared_ptr<APLRuntimePresentationAdapter> aplRuntimePresentationAdapter);

    /**
     * Sets the TemplateRuntime Presentation Adapter Bridge
     * @param templateRuntimePresentationAdapterBridge The @c TemplateRuntimePresentationAdapterBridge
     */
    void setTemplateRuntimePresentationAdapterBridge(
        std::shared_ptr<TemplateRuntimePresentationAdapterBridge> templateRuntimePresentationAdapterBridge);

    /**
     * Initializes the IPC handlers implemented by the GUIManager.
     */
    void initialize();

private:
    /**
     * Constructor.
     *
     * @param guiClient A pointer to a GUI Client.
     * @param activityEventNotifier A pointer to a activity event notifier.
     * @param micWrapper A pointer to a microphone wrapper.
     * @param interactionManager A pointer to interaction manager.
     * @param launcherHandler A pointer to launcher handler.
     */
    GUIManager(
        std::shared_ptr<gui::GUIClientInterface> guiClient,
        std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier,
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper,
        std::shared_ptr<common::InteractionManager> interactionManager,
        std::shared_ptr<common::EndpointAlexaLauncherHandler> launcherHandler);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Internal function for handling a @c NavigationEvent::BACK event
     */
    void executeBackNavigation();

    /**
     * Internal function for handling a @c NavigationEvent::EXIT event
     */
    void executeExitNavigation();

    /**
     * Update the firmware version.
     *
     * @param firmwareVersion The new firmware version.
     */
    void setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion);

    /**
     * Should be called after a user wishes to modify the volume.
     */
    void adjustVolume(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type, int8_t delta);

    /**
     * Should be called after a user wishes to set mute.
     */
    void setMute(avsCommon::sdkInterfaces::ChannelVolumeInterface::Type type, bool mute);

    /**
     * Internal function to update cached defaultWindowId and inform adapters.
     *
     * @param windowId the window id to use by default.
     */
    void setDefaultWindowId(const std::string& windowId);

    /**
     * Internal function to update cached audioPlaybackUIWindowId and inform adapters.
     *
     * @param windowId the window id to use for audio playback UI.
     */
    void setAudioPlaybackUIWindowId(const std::string& windowId);

    /**
     * Reset the device and remove any customer data.
     */
    void resetDevice();

    /**
     * A reference to the GUI Client.
     */
    std::shared_ptr<gui::GUIClientInterface> m_guiClient;

    /**
     * A reference to @c ActivityEvent notifier
     */
    std::shared_ptr<gui::GUIActivityEventNotifierInterface> m_activityEventNotifier;

    /**
     * A reference to the smart screen client.
     */
    std::shared_ptr<alexaClientSDK::defaultClient::DefaultClient> m_defaultClient;

    /// Whether Alexa is speaking or listening.
    bool m_isSpeakingOrListening;

    /// The microphone managing object.
#ifdef UWP_BUILD
    std::shared_ptr<alexaSmartScreenSDK::sssdkCommon::NullMicrophone> m_micWrapper;
#else
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> m_micWrapper;
#endif

    /// The @c PlayerActivity of the @c AudioPlayer.
    avsCommon::avs::PlayerActivity m_playerActivityState;

    /// The last state reported by AudioInputProcessor.
    avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State m_audioInputProcessorState;

    /// Map of channel focus states by channelName.
    std::unordered_map<std::string, avsCommon::avs::FocusState> m_channelFocusStates;

    /// Utility flag used for clearing Alert Channel when Foregrounded.
    bool m_clearAlertChannelOnForegrounded;

    /// Object that manages settings callbacks.
    std::shared_ptr<settings::SettingCallbacks<settings::DeviceSettingsManager>> m_callbacks;

    /// Object that manages settings.
    std::shared_ptr<settings::DeviceSettingsManager> m_settingsManager;

    /// The @c DoNotDisturbHandler.
    std::shared_ptr<ipc::DoNotDisturbHandler> m_doNotDisturbIPCHandler;

    /// The @c InteractionManagerHandler.
    std::shared_ptr<ipc::InteractionManagerHandler> m_interactionManagerIPCHandler;

    /// The @c LoggerHandler.
    std::shared_ptr<ipc::LoggerHandler> m_loggerIPCHandler;

    /// SessionSetup handler.
    std::shared_ptr<ipc::SessionSetupHandler> m_sessionSetupIPCHandler;

    /// The IPC Version Manager.
    std::shared_ptr<ipc::IPCVersionManager> m_ipcVersionManager;

    /// The launcher handler.
    std::shared_ptr<common::EndpointAlexaLauncherHandler> m_launcherHandler;

    /// WindowManager handler.
    std::shared_ptr<ipc::WindowManagerHandler> m_windowManagerIPCHandler;

    /// Default Window Id.
    std::string m_defaultWindowId;

    /// Audio Playback UI Window Id.
    std::string m_audioPlaybackUIWindowId;

    /// GUI log bridge to be used to handle log events.
    GUILogBridge m_guiLogBridge;

    /// The interface holding audio focus.
    std::string m_interfaceHoldingAudioFocus;

    /// Object that provides timezone offsets for the device.
    std::shared_ptr<TimezoneHelper> m_timezoneHelper;

    /// Interaction Manager.
    std::shared_ptr<common::InteractionManager> m_interactionManager;

    /// Presentation Orchestrator State Tracker.
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
        m_presentationOrchestratorStateTracker;

    /// Visual Characteristics component.
    std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsInterface> m_visualCharacteristics;

    /// Visual Characteristics Serializer component.
    std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface>
        m_visualCharacteristicsSerializer;

    /// Presentation Orchestrator.
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> m_presentationOrchestrator;

    /// Alexa Presentation APL CA.
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> m_alexaPresentationAPL;

    /// Pointer to th APL Runtime Presentation Adapter.
    std::shared_ptr<APLRuntimePresentationAdapter> m_aplRuntimePresentationAdapter;

    /// Pointer to the Template Runtime Presentation Adapter Bridge.
    std::shared_ptr<TemplateRuntimePresentationAdapterBridge> m_templateRuntimePresentationAdapterBridge;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIMANAGER_H_
