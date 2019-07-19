/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_BLUETOOTH_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_BLUETOOTH_H_

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/AVS/Requester.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapter.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include "Bluetooth/BluetoothAVRCPTransformer.h"
#include "Bluetooth/BluetoothStorageInterface.h"
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {

/**
 * The Bluetooth Capability Agent is responsible for implementing the Bluetooth AVS interface. This consists
 * of two areas of responsibilities:
 *
 * -# The connectivity of devices. This includes scanning, pairing and connecting.
 * -# The management of profiles. This includes:
 * media control (AVRCP, Audio/Video Remote Control Profile) and
 * media playback (A2DP, Advanced Audio Distribution Profile).
 *
 * The Bluetooth agent will handle directives from AVS and requests from peer devices. Examples include
 * pairing and connection requests, as we as media playback requests. Some examples of this are:
 *
 * - "Alexa, connect".
 * - Enabling discovery through the companion app.
 * - Initializing connection through a previously paired device on the device.
 * - "Alexa next".
 *
 * Connectivity is defined as when two devices have paired and established connections of all applicable
 * services (A2DP, AVRCP, etc). Alexa does not support multiple connected multimedia devices. If a device is
 * currently connected, attempting to connect a second device should force a disconnect on the
 * currently connected device.
 *
 * At this time, the agent does not enforce the disconnect of the currently connected device.
 * It is theoretically possible to connect two devices simultaneously, but the behavior is undefined.
 * It is advised to disconnect a currently connected device before connecting a new one.
 * Enforcement of this will be available in an upcoming release.
 *
 * Interfaces in AVSCommon/SDKInterfaces/Bluetooth can be implemented for customers
 * who wish to use their own Bluetooth stack. The Bluetooth agent operates based on events.
 * Please refer to the BluetoothEvents.h file for a list of events that must be sent.
 *
 * Supported Profiles
 *
 * Profiles listed under here refer to the Capability Agent's support of these profiles in relation to AVS.
 * This does not speak about support for them at other layers (the stack, client applications, etc).
 *
 * -# AVRCP (Controller, Target)
 * -# A2DP (Sink, Source)
 */
class Bluetooth
        : public std::enable_shared_from_this<Bluetooth>
        , public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::bluetooth::BluetoothEventListenerInterface
        , public avsCommon::utils::bluetooth::FormattedAudioStreamAdapterListener
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler {
public:
    using ObserverInterface = avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceObserverInterface;

    /**
     * An enum representing the streaming states.
     */
    enum class StreamingState {
        /// Initial state or after a disconnect.
        INACTIVE,

        /// Media playback is no longer occurring.
        PAUSED,

        /// AVRCP Pause is sent, waiting for local playback to stop.
        PENDING_PAUSED,

        /// AVRCP Play is sent, waiting for local playback to start.
        PENDING_ACTIVE,

        /// Media playback is currently ongoing.
        ACTIVE
    };

    /**
     * Creates an instance of the Bluetooth capability agent.
     *
     * @param contextManager Responsible for managing the context.
     * @param focusManager Responsible for managing the focus.
     * @param messageSender Responsible for sending events to AVS.
     * @param exceptionEncounteredSender Responsible for sending exceptions to AVS.
     * @param bluetoothStorage The storage component for the Bluetooth CA.
     * @param deviceManager Responsible for management of Bluetooth devices.
     * @param eventBus A bus to abstract Bluetooth stack specific messages.
     * @param mediaPlayer The Media Player which will handle playback.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @param avrcpTransformer Transforms incoming AVRCP commands if supported.
     */
    static std::shared_ptr<Bluetooth> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<BluetoothStorageInterface> bluetoothStorage,
        std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> deviceManager,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        std::shared_ptr<BluetoothAVRCPTransformer> avrcpTransformer = nullptr);

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void onFocusChanged(avsCommon::avs::FocusState newFocus) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    // @name ContextRequester Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name MediaPlayerObserverInterface Functions
    /// @{
    void onPlaybackStarted(avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id) override;
    void onPlaybackStopped(avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id) override;
    void onPlaybackFinished(avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id) override;
    void onPlaybackError(
        avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error) override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

    /**
     * Adds a bluetooth device observer.
     *
     * @param observer The @c BluetoothDeviceObserverInterface to add.
     */
    void addObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * Removes a bluetooth device observer.
     *
     * @param observer The @c BluetoothDeviceObserverInterface to remove.
     */
    void removeObserver(std::shared_ptr<ObserverInterface> observer);

protected:
    /// @name BluetoothEventBusListenerInterface Functions
    /// @{
    void onEventFired(const avsCommon::utils::bluetooth::BluetoothEvent& event) override;
    /// @}

private:
    /**
     * Creates an instance of the Bluetooth capability agent.
     *
     * @param contextManager Responsible for managing the context.
     * @param focusManager Responsible for managing the focus.
     * @param messageSender Responsible for sending events to AVS.
     * @param exceptionEncounteredSender Responsible for sending exceptions to AVS.
     * @param bluetoothStorage The storage component for the Bluetooth CA.
     * @param deviceManager Responsible for management of Bluetooth devices.
     * @param eventBus A bus to abstract Bluetooth stack specific messages.
     * @param mediaPlayer The Media Player which will handle playback.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @param avrcpTransformer Transforms incoming AVRCP commands.
     */
    Bluetooth(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<BluetoothStorageInterface> bluetoothStorage,
        std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface>& deviceManager,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        std::shared_ptr<BluetoothAVRCPTransformer> avrcpTransformer);

    /**
     * Initializes the agent.
     *
     * @return A bool indicating success.
     */
    bool init();

    // TODO ACSDK-1392: Optimize by updating the context only when there is a delta.
    /// Helper function to update the context.
    void executeUpdateContext();

    /**
     * Marks the directive as completed.
     *
     * @param info The directive currently being handled.
     */
    void executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Removes the directive from the @c CapabilityAgents internal map.
     *
     * @param info The directive currently being handled.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Alert AVS that an exception has occurred while handling a directive.
     *
     * @param info The directive currently being handled.
     * @param message A string to send to AVS.
     * @param type The type of exception.
     */
    void sendExceptionEncountered(
        std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const std::string& message,
        avsCommon::avs::ExceptionErrorType type);

    /// A state transition function for entering the foreground.
    void executeEnterForeground();

    /// A state transition function for entering the background.
    void executeEnterBackground();

    /// A state transition function for entering the none state.
    void executeEnterNone();

    /**
     * Puts the device into the desired discoverable mode.
     *
     * @param discoverable A bool indicating whether it should be discoverable.
     * @return A bool indicating success.
     */
    bool executeSetDiscoverableMode(bool discoverable);

    /**
     * Puts the device into the desired scan mode.
     *
     * @param scanning A bool indicating whether it should be scanning.
     * @return A bool indicating success.
     */
    bool executeSetScanMode(bool scanning);

    /**
     * Pair with the device matching the given uuid.
     *
     * @param uuid The uuid associated with the device.
     * @return A bool indicating success.
     */
    bool executePairDevice(const std::string& uuid);

    /**
     * Unpair with the device matching the given uuid.
     *
     * @param uuid The uuid associated with the device.
     * @return A bool indicating success.
     */
    bool executeUnpairDevice(const std::string& uuid);

    /**
     * Connect with the device matching the given uuid. This will connect all available services between
     * the two devices.
     *
     * @param uuid The uuid associated with the device.
     */
    void executeConnectByDeviceId(const std::string& uuid);

    /**
     * Connect with the most recently connected device that supports the given profile.
     * The profile is only a selector; this will connect all available services between the two devices.
     * The version information is not used currently.
     *
     * @param profileName The profile name to select devices by.
     * @param profileVersion The profile version to select devices by.
     */
    void executeConnectByProfile(const std::string& profileName, const std::string& profileVersion);

    /**
     * Disconnect with the device matching the given uuid. This will disconnect all available services between
     * the two devices.
     *
     * @param uuid The uuid associated with the device.
     */
    void executeDisconnectDevice(const std::string& uuid);

    /**
     * Helper function that encapsulates disconnect logic.
     *
     * @param requester The @c Requester who initiated the disconnect.
     */
    void executeOnDeviceDisconnect(avsCommon::avs::Requester requester);

    /**
     * Helper function that encapsulates connect logic.
     *
     * @param requester The @c Requester who initiated the disconnect.
     */
    void executeOnDeviceConnect(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Helper function to abstract shared logic in pairing/unpairing/connecting/disconnecting operations.
     *
     * @param device The device to operate on.
     * @param function The function to call (one of pair/unpair/connect/disconnect).
     */
    bool executeFunctionOnDevice(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device,
        std::function<std::future<bool>(
            std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>&)> function);

    /// Send a play command to the activeDevice.
    void executePlay();

    /// Send a stop command to the activeDevice.
    void executeStop();

    /// Send a next command to the activeDevice.
    void executeNext();

    /// Send a previous command to the activeDevice.
    void executePrevious();

    /**
     * Drain the command queue of @c AVRCPCommands. We use a queue so we can process the commands
     * after the Bluetooth agent has entered the foreground.
     */
    void executeDrainQueue();

    /// Obtain the incoming stream and set as source into the MediaPlayer.
    void executeInitializeMediaSource();

    /// Reset the source to the MediaPlayer and reset the sourceId.
    void cleanupMediaSource();

    /// Stop the mediaplayer if applicable and release focus if we have it.
    void executeAbortMediaPlayback();

    /**
     * This handles the details of sending the incoming A2DP stream into MediaPlayer.
     * A callback is set up to copy incoming buffers into an @c AttachmentReader which
     * can be consumed by the MediaPlayer.
     *
     * @param stream The incoming A2DP stream.
     */
    void setCurrentStream(std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> stream);

    /// @name FormattedAudioStreamAdapterListener Functions
    /// @{

    void onFormattedAudioStreamAdapterData(
        avsCommon::utils::AudioFormat audioFormat,
        const unsigned char* buffer,
        size_t size) override;

    /// @}

    /**
     * Retrieve the @BluetoothDeviceInterface by its MAC.
     *
     * @param mac The MAC adddress.
     * @return The @BluetoothDeviceInterface if found, otherwise a nullptr.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> retrieveDeviceByMac(
        const std::string& mac);

    /**
     * Retrieve the @BluetoothDeviceInterface by its UUID.
     *
     * @param uuid The generated UUID associated with a device.
     * @return The @BluetoothDeviceInterface if found, otherwise a nullptr.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> retrieveDeviceByUuid(
        const std::string& uuid);

    /**
     * Retrieve the UUID by its MAC address. If no UUID is found, then one will be generated and inserted.
     *
     * @param mac The MAC address of the associated UUID.
     * @param[out] uuid The UUID.
     *
     * @return Whether an UUID is successfully obtained either by retrieval or generation.
     */
    bool retrieveUuid(const std::string& mac, std::string* uuid);

    /// Clears the databse of mac,uuid that are not known by the @BluetoothDeviceManager.
    void clearUnusedUuids();

    /**
     * Most events require the context, this method queues the event and requests the context.
     * Once the context is available in onContextAvailable, the event will be dequeued and sent.
     *
     * @param eventName The name of the event.
     * @param eventPayload The payload of the event.
     */
    void executeQueueEventAndRequestContext(const std::string& eventName, const std::string& eventPayload);

    /**
     * Sends an event to alert AVS that the list of found and paired devices has changed.
     *
     * @param devices A list of devices.
     * @param hasMore A bool indicating if we're still looking for more devices.
     */
    void executeSendScanDevicesUpdated(
        const std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>& devices,
        bool hasMore);

    /// Sends a scanDevicesFailed event to alert AVS that attempting to scan for devices failed.
    void executeSendScanDevicesFailed();

    /// Sends an event to indicate the adapter successfully entered discoverable mode.
    void executeSendEnterDiscoverableModeSucceeded();

    /// Sends an event to indicate the adapter failed to enter discoverable mode.
    void executeSendEnterDiscoverableModeFailed();

    /**
     * Sends an event to indicate that pairing with a device succeeded.
     *
     * @param device The paired device.
     */
    void executeSendPairDeviceSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /// Sends an event to indicate that a device pairing attempt failed.
    void executeSendPairDeviceFailed();

    /**
     * Sends an event to indicate that unpairing with a device succeeded.
     *
     * @param device The unpaired device.
     */
    void executeSendUnpairDeviceSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /// Sends an event to indicate that unpairing with a device failed.
    void executeSendUnpairDeviceFailed();

    /**
     * Sends an event to indicate that connecting with a device by uuid succeeded.
     *
     * @param device The device.
     * @param requester The @c Requester who initiated the operation.
     */
    void executeSendConnectByDeviceIdSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that connecting with a device by uuid failed.
     *
     * @param device The device.
     * @param requester The @c Requester who initiated the operation.
     */
    void executeSendConnectByDeviceIdFailed(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that connecting with a device by profile succeeded.
     *
     * @param device The device.
     * @param profileName The profileName to connect by.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendConnectByProfileSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        const std::string& profileName,
        avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that connecting with a device by profile failed.
     *
     * @param device The device.
     * @param profileName The profileName to connect by.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendConnectByProfileFailed(const std::string& profileName, avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that disconnecting with a device succeeded.
     *
     * @param device The device.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendDisconnectDeviceSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that disconnecting with a device failed.
     *
     * @param device The device.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendDisconnectDeviceFailed(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::avs::Requester requester);

    /// Sends an event to indicate we successfully sent an AVRCP play to the target.
    void executeSendMediaControlPlaySucceeded();

    /// Sends an event to indicate we failed to send an AVRCP play to the target.
    void executeSendMediaControlPlayFailed();

    /// Sends an event to indicate we successfully sent an AVRCP pause to the target.
    void executeSendMediaControlStopSucceeded();

    /// Sends an event to indicate we failed to send an AVRCP pause to the target.
    void executeSendMediaControlStopFailed();

    /// Sends an event to indicate we successfully sent an AVRCP next to the target.
    void executeSendMediaControlNextSucceeded();

    /// Sends an event to indicate we failed to send an AVRCP next to the target.
    void executeSendMediaControlNextFailed();

    /// Sends an event to indicate we successfully sent an AVRCP previous to the target.
    void executeSendMediaControlPreviousSucceeded();

    /// Sends an event to indicate we failed to send an AVRCP previous to the target.
    void executeSendMediaControlPreviousFailed();

    /**
     * Sends an event that we have started streaming.
     *
     * @param device The activeDevice.
     */
    void executeSendStreamingStarted(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event that we have stopped streaming.
     *
     * @param device The activeDevice.
     */
    void executeSendStreamingEnded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Set device attributes for notifying the observers
     */
    ObserverInterface::DeviceAttributes generateDeviceAttributes(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /// Set of capability configurations that will get published using DCF
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c FocusManager used to manage focus.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    // TODO 1396: Combine @c m_streamingState and @c m_focusState into a single state machine if possible.
    /**
     * The current @c StreamingState of the device. This represents the internal media streaming state of
     * the Bluetooth agent in relation to a connected device.
     */
    StreamingState m_streamingState;

    /// The current @c FocusState of the device.
    avsCommon::avs::FocusState m_focusState;

    /// The id associated with MediaPlayer requests for a specific source.
    avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId m_sourceId;

    /// The @c BluetoothDeviceManagerInterface instance responsible for device management.
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> m_deviceManager;

    /// A queue to store AVRCP commands.
    std::deque<avsCommon::sdkInterfaces::bluetooth::services::AVRCPCommand> m_cmdQueue;

    /// An event queue used to store events which need to be sent. The pair is <eventName, eventPayload>.
    std::queue<std::pair<std::string, std::string>> m_eventQueue;

    /// Keeps track of last paired device to prevent sending duplicate events.
    std::string m_lastPairMac;

    /// The current activeDevice. This is the one that is connected and sending media via A2DP.
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> m_activeDevice;

    /// The MediaPlayer responsible for media playback.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// Used to persist data necessary for Bluetooth. This includes UUID, MAC, and connection order.
    std::shared_ptr<BluetoothStorageInterface> m_db;

    /// An eventbus used to abstract Bluetooth stack specific messages.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;

    /// Transforms incoming AVRCP commands.
    std::shared_ptr<BluetoothAVRCPTransformer> m_avrcpTransformer;

    /// The A2DP media stream.
    std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> m_mediaStream;

    /// An InProcessAttachment used to feed A2DP stream data into the MediaPlayer.
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> m_mediaAttachment;

    /// A writer to write the A2DP stream buffers into the InProcessAttachment.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> m_mediaAttachmentWriter;

    /// An executor used for serializing requests on the Bluetooth agent's own thread of execution.
    avsCommon::utils::threading::Executor m_executor;

    /// Set of bluetooth device observers that will get notified on connects or disconnects.
    std::unordered_set<std::shared_ptr<ObserverInterface>> m_observers;
};

/**
 * Converts an enum to a string.
 *
 * @param state The @c StreamingState.
 * @return The string form of the enum.
 */
inline std::string streamingStateToString(Bluetooth::StreamingState state) {
    switch (state) {
        case Bluetooth::StreamingState::INACTIVE:
            return "INACTIVE";
        case Bluetooth::StreamingState::PAUSED:
            return "PAUSED";
        case Bluetooth::StreamingState::PENDING_PAUSED:
            return "PENDING_PAUSED";
        case Bluetooth::StreamingState::PENDING_ACTIVE:
            return "PENDING_ACTIVE";
        case Bluetooth::StreamingState::ACTIVE:
            return "ACTIVE";
    }

    return "UNKNOWN";
}

}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_BLUETOOTH_H_
