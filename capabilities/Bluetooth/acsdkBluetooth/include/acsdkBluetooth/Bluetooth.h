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

#ifndef ACSDKBLUETOOTH_BLUETOOTH_H_
#define ACSDKBLUETOOTH_BLUETOOTH_H_

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <queue>
#include <unordered_set>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothNotifierInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothDeviceConnectionRulesProviderInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothDeviceObserverInterface.h>
#include <acsdkBluetoothInterfaces/BluetoothStorageInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/AVS/Requester.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>
#include <AVSCommon/Utils/Bluetooth/DeviceCategory.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapter.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>

#include "acsdkBluetooth/BluetoothEventState.h"
#include "acsdkBluetooth/BluetoothMediaInputTransformer.h"
#include "acsdkBluetoothInterfaces/BluetoothLocalInterface.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {

/// String to identify the Bluetooth media player to render audio.
static const constexpr char* BLUETOOTH_MEDIA_PLAYER_NAME = "BluetoothMediaPlayer";

/**
 * The Bluetooth Capability Agent is responsible for implementing the Bluetooth AVS interface. This consists
 * of two areas of responsibilities:
 *
 * -# The connectivity of devices. This includes scanning, pairing and connecting.
 * -# The management of profiles. This includes:
 * media control (AVRCP, Audio/Video Remote Control Profile)
 * media playback (A2DP, Advanced Audio Distribution Profile)
 * Human Interface Device Profile
 * Serial Port Profile and
 * Hands-Free Profile.
 *
 * The Bluetooth agent will handle directives from AVS and requests from peer devices. Examples include
 * pairing and connection requests, as well as media playback requests. Some examples of this are:
 *
 * - "Alexa, connect".
 * - Enabling discovery through the companion app.
 * - Initializing connection through a previously paired device on the device.
 * - "Alexa next".
 *
 * Connectivity is defined as when two devices have paired and established connections of all applicable
 * services (A2DP, AVRCP, etc). Alexa supports multiple connected multimedia devices but doesn't support multiple A2DP
 * connected devices. The agent enforces the connected devices to follow some Bluetooth device connection rules based on
 * DeviceCategory. For example, If a A2DP device is currently connected, attempting to connect a second A2DP device
 * should force a disconnect on the currently connected device. However, if a A2DP device is currently connected,
 * attempting to connected a SPP/HID device should not cause a disconnect on the currently connected device.
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
 * -# HFP
 * -# HID
 * -# SPP
 */
class Bluetooth
        : public std::enable_shared_from_this<Bluetooth>
        , public acsdkBluetoothInterfaces::BluetoothLocalInterface
        , public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::bluetooth::BluetoothEventListenerInterface
        , public avsCommon::utils::bluetooth::FormattedAudioStreamAdapterListener
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler {
public:
    using ObserverInterface = acsdkBluetoothInterfaces::BluetoothDeviceObserverInterface;

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
     * An enum that represents how the Bluetooth class expects to lose focus.
     */
    enum class FocusTransitionState {
        /// No focus transition in progress,
        NONE,

        /// Focus in Bluetooth class is lost because it explicitly released focus.
        INTERNAL,

        /**
         * Focus in Bluetooth class that will be lost because it explicitly released focus.
         * This state prevents foreground or background focus changes from setting the state to EXTERNAL before the
         * none focus change has had the chance to set the state to INTERNAL.
         */
        PENDING_INTERNAL,

        /// Focus in Bluetooth class is lost because another class has taken focus.
        EXTERNAL
    };

    /**
     * An enum that is used to represent the Bluetooth scanning state and if a state change should result in a scan
     * report being sent to the Alexa service.
     */
    enum class ScanningTransitionState {
        /**
         * The device is currently scanning.
         *
         * Any state change should result in sending a scan report.
         *
         * This state is set when a SCAN_DEVICES directive is sent from the Alexa service.
         */
        ACTIVE,

        /**
         * The device is not scanning.
         *
         * A state change to inactive should not result in sending a scan report.
         *
         * This state is set when a EXIT_DISCOVERABLE_MODE directive is sent or scan mode is disabled as part of the
         * PAIR_DEVICES directive.
         */
        PENDING_INACTIVE,

        /**
         * The device is not scanning.
         *
         * A state change to inactive should not result in sending a scan report.
         *
         * This state is set when a state change to inactive is recieved and the previous state was
         * PENDING_INACTIVE.
         */
        INACTIVE
    };

    /**
     * Creates an instance of the @c Bluetooth.
     *
     * @param contextManager Responsible for managing the context.
     * @param messageSender Responsible for sending events to AVS.
     * @param exceptionEncounteredSender Responsible for sending exceptions to AVS.
     * @param bluetoothStorage The storage component for the Bluetooth CA.
     * @param deviceManager Responsible for management of Bluetooth devices.
     * @param eventBus A bus to abstract Bluetooth stack specific messages.
     * @param customerDataManager Object that will track the CustomerDataHandler.
     * @param audioPipelineFactory Object to create a Bluetooth media player and related interfaces.
     * @param audioFocusManager Responsible for managing the focus.
     * @param shutdownNotifier Object that will notify this CA when it is time to shut down.
     * @param endpointCapabilitiesRegistrar The default endpoint registrar with which this CA will register its
     * capabilities.
     * @param enabledConnectionRules The set of devices connection rules enabled by the Bluetooth stack from
     * customers.
     * @param mediaInputTransformer Transforms incoming Media commands if supported.
     * @param bluetoothNotifier The object with which to notify observers of Bluetooth device connections or
     * disconnects.
     */
    static std::shared_ptr<Bluetooth> createBluetoothCapabilityAgent(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface> bluetoothStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> deviceManager,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
            audioPipelineFactory,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::AudioFocusAnnotation,
            avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
        acsdkManufactory::Annotated<
            avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
            avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothDeviceConnectionRulesProviderInterface>
            connectionRulesProvider,
        std::shared_ptr<BluetoothMediaInputTransformer> mediaInputTransformer,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface> bluetoothNotifier);

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void onFocusChanged(avsCommon::avs::FocusState newFocus, avsCommon::avs::MixingBehavior behavior) override;
    /// @}

    /// @name BluetoothLocalInterface Functions
    /// @{
    void setDiscoverableMode(bool discoverable) override;
    void setScanMode(bool scanning) override;
    void pair(const std::string& addr) override;
    void unpair(const std::string& addr) override;
    void connect(const std::string& addr) override;
    void disconnect(const std::string& addr) override;
    void setPairingPin(const std::string& addr, const std::string& pin) override;
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
    void onFirstByteRead(
        avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStarted(
        avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackStopped(
        avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackFinished(
        avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    void onPlaybackError(
        avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId id,
        const avsCommon::utils::mediaPlayer::ErrorType& type,
        std::string error,
        const avsCommon::utils::mediaPlayer::MediaPlayerState& state) override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}
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
     * @param enabledConnectionRules The set of devices connection rules enabled by the Bluetooth stack from
     * customers.
     * @param bluetoothChannelVolumeInterface Responsible for Volume Control/Attenuation of the underlying
     * SpeakerInterface
     * @param mediaInputTransformer Transforms incoming Media commands.
     * @param bluetoothNotifier The object with which to notify observers of Bluetooth device connections or
     * disconnects.
     */
    Bluetooth(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface> bluetoothStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> deviceManager,
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
            enabledConnectionRules,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> bluetoothChannelVolumeInterface,
        std::shared_ptr<BluetoothMediaInputTransformer> mediaInputTransformer,
        std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface> bluetoothNotifier);

    /**
     * Initializes the agent.
     *
     * @return A bool indicating success.
     */
    bool init();

    /**
     * Sync the connected state with the device manager upon initialization. This will ensure the state
     * is consistent with any devices that were connected before the BTCA was initialized.
     */
    void syncWithDeviceManager();

    // TODO ACSDK-1392: Optimize by updating the context only when there is a delta.
    /// Helper function to update the context.
    void executeUpdateContext();

    /**
     * Helper function to extract AVS compliant profiles. This returns a rapidjson node
     * containing an array of supported profiles.
     *
     * @param device The device.
     * @param allocator The allocator which will be used to create @c supportedProfiles.
     * @param[out] supportedProfiles A rapidjson node containing the supported profiles.
     * @return A bool indicating success.
     */
    bool extractAvsProfiles(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        rapidjson::Document::AllocatorType& allocator,
        rapidjson::Value* supportedProfiles);

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
    void executeEnterBackground(avsCommon::avs::MixingBehavior behavior);

    /// A state transition function for entering the none state.
    void executeEnterNone();

    /**
     * Handles setting the device into discoverable mode.
     */
    void executeHandleEnterDiscoverableMode();

    /**
     * Handles setting the device into undiscoverable mode.
     */
    void executeHandleExitDiscoverableMode();

    /**
     * Handles setting the device into scan mode.
     */
    void executeHandleScanDevices();

    /**
     * Handles pairing with the devices matching the given uuids.
     *
     * @param uuids The uuids associated with the devices.
     * @return A bool indicating success.
     */
    bool executeHandlePairDevices(const std::unordered_set<std::string>& uuids);

    /**
     * Handles unpairing with the devices matching the given uuids.
     *
     * @param uuids The uuids associated with the devices.
     * @return A bool indicating success.
     */
    bool executeHandleUnpairDevices(const std::unordered_set<std::string>& uuids);

    /**
     * Handles connecting with the devices matching the given uuids.
     * This will connect all available services between the two devices.
     *
     * @param uuids The uuids associated with the devices.
     * @return A bool indicating success.
     */
    bool executeHandleConnectByDeviceIds(const std::unordered_set<std::string>& uuids);

    /**
     * Disconnect with the devices matching the given uuids.
     * This will disconnect all available services between the two devices.
     *
     * @param uuids The uuids associated with the devices.
     * @return A bool indicating success.
     */
    bool executeHandleDisconnectDevices(const std::unordered_set<std::string>& uuids);

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
     * @param shouldReport A bool that indicates if the scan report should be reported to the Alexa service.
     * @return A bool indicating success.
     */
    bool executeSetScanMode(bool scanning, bool shouldReport = true);

    /**
     * Pair with the devices matching the given uuids.
     *
     * @param uuids The uuids associated with the devices.
     * @return A bool indicating that all devices have been paired.
     */
    bool executePairDevices(const std::unordered_set<std::string>& uuids);

    /**
     * Unpair with the devices matching the given uuids.
     *
     * @param uuids The uuids associated with the devices.
     * @return A bool indicating that all devices have been unpaired.
     */
    bool executeUnpairDevices(const std::unordered_set<std::string>& uuids);

    /**
     * Set Device Category with the device matching the given uuid.
     *
     * @param uuidCategoryMap Map of <UUID, Category> of the devices.
     * @return Map of <UUID, Category> of devices that failed to update.
     */
    std::map<std::string, std::string> executeSetDeviceCategories(
        const std::map<std::string, std::string>& uuidCategoryMap);

    /**
     * Connect with the devices matching the given uuids. This will connect all available services between
     * the two devices.
     *
     * @param uuids The uuids associated with the devices.
     */
    void executeConnectByDeviceIds(const std::unordered_set<std::string>& uuids);

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
     * Disconnect with the devices matching the given uuids. This will disconnect all available services between
     * the two devices.
     *
     * @param uuids The uuids associated with the devices.
     */
    void executeDisconnectDevices(const std::unordered_set<std::string>& uuids);

    /**
     * Sets the pairing pin for the current pairing attempt
     *
     * @param addr The MAC address associated with the device
     * @param pin BT pairing pin
     */
    void executeSetPairingPin(const std::string& addr, const std::string& pin);

    /**
     * Helper function that encapsulates disconnect logic.
     *
     * @param device The disconnected device.
     * @param requester The @c Requester who initiated the disconnect.
     */
    void executeOnDeviceDisconnect(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::avs::Requester requester);

    /**
     * Helper function that encapsulates connect logic.
     *
     * @param device The connected device.
     * @param shouldNotifyConnection A bool that indicates if observers should be notified the device connection.
     */
    void executeOnDeviceConnect(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        bool shouldNotifyConnection = true);

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

    /**
     * Send a play command to the device.
     *
     * @param device The device to play.
     */
    void executePlay(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Send a stop command to the device.
     *
     * @param device The device to stop.
     */
    void executeStop(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Send a next command to the device.
     *
     * @param device The device to play next.
     */
    void executeNext(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Send a previous command to the device.
     *
     * @param device The device to play previous.
     */
    void executePrevious(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

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
     * Retrieve the @DeviceCategory by its UUID.
     *
     * @param uuid The generated UUID associated with a device.
     * @param category The device category associated with a device.
     * @return whether a @c DeviceCategory is successfully obtained by retrieval.
     */
    bool retrieveDeviceCategoryByUuid(const std::string& uuid, DeviceCategory* category);

    /**
     * Retrieve the @BluetoothDeviceConnectionRuleInterface by the device uuid.
     * @param uuid the UUID of the device.
     * @return The @BluetoothDeviceConnectionRuleInterface if found, otherwise a nullptr.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>
    retrieveConnectionRuleByUuid(const std::string& uuid);

    /**
     * Retrieve the UUID by its MAC address. If no UUID is found, then one will be generated and inserted.
     *
     * @param mac The MAC address of the associated UUID.
     * @param[out] uuid The UUID.
     *
     * @return Whether an UUID is successfully obtained either by retrieval or generation.
     */
    bool retrieveUuid(const std::string& mac, std::string* uuid);

    /**
     * Retrieve a set of UUIDs from the payload.
     *
     * @param payload The payload sent down.
     * @return A set of UUIDs in the payload.
     */
    std::unordered_set<std::string> retrieveUuidsFromConnectionPayload(const rapidjson::Document& payload);

    /// Clears the databse of mac,uuid that are not known by the @BluetoothDeviceManager.
    void clearUnusedUuids();

    /**
     * Event immediately submitted to the executor and is sent.
     *
     * @param eventName The name of the event.
     * @param eventPayload The payload of the event.
     */
    void executeSendEvent(const std::string& eventName, const std::string& eventPayload);

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
    void executeSendScanDevicesReport(
        const std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>& devices,
        bool hasMore);

    /// Sends a scanDevicesFailed event to alert AVS that attempting to scan for devices failed.
    void executeSendScanDevicesFailed();

    /// Sends an event to indicate the adapter successfully entered discoverable mode.
    void executeSendEnterDiscoverableModeSucceeded();

    /// Sends an event to indicate the adapter failed to enter discoverable mode.
    void executeSendEnterDiscoverableModeFailed();

    /**
     * Sends an event to indicate that pairing with devices succeeded.
     *
     * @param devices The paired devices.
     */
    void executeSendPairDevicesSucceeded(
        const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>&
            devices);

    /**
     * Sends a failed pair event.
     *
     * @param eventName The pair event name.
     * @param uuids The uuids of devices.
     */
    void executeSendPairFailedEvent(const std::string& eventName, const std::unordered_set<std::string>& uuids);

    /**
     * Sends an event to indicate that devices pairing attempt failed.
     *
     * @param uuids The uuids of devices.
     */
    void executeSendPairDevicesFailed(const std::unordered_set<std::string>& uuids);

    /**
     * Sends an event to indicate that unpairing with devices succeeded.
     *
     * @param devices The unpaired devices.
     */
    void executeSendUnpairDevicesSucceeded(
        const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>&
            devices);

    /**
     * Sends an event to indicate that unpairing with devices failed.
     *
     * @param uuids The uuids of devices.
     */
    void executeSendUnpairDevicesFailed(const std::unordered_set<std::string>& uuids);

    /**
     * Sends an event to indicate that setting device category for each device succeeded.
     *
     * @param uuidCategoryMap Map of <UUID, Category> of the devices.
     */
    void executeSetDeviceCategoriesSucceeded(const std::map<std::string, std::string>& uuidCategoryMap);

    /**
     * Sends an event to indicate that setting device category for each device failed.
     *
     * @param uuidCategoryMap Map of <UUID, Category> of the devices.
     */
    void executeSetDeviceCategoriesFailed(const std::map<std::string, std::string>& uuidCategoryMap);

    /**
     * Sends an event to indicate that connecting with devices by uuids succeeded.
     *
     * @param devices The devices.
     * @param requester The @c Requester who initiated the operation.
     */
    void executeSendConnectByDeviceIdsSucceeded(
        const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>&
            devices,
        avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that connecting with devices by uuids failed.
     *
     * @param uuids The uuids of devices.
     * @param requester The @c Requester who initiated the operation.
     */
    void executeSendConnectByDeviceIdsFailed(
        const std::unordered_set<std::string>& uuids,
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
     * Sends an event to indicate that disconnecting with devices succeeded.
     *
     * @param devices The devices.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendDisconnectDevicesSucceeded(
        const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>&
            devices,
        avsCommon::avs::Requester requester);

    /**
     * Sends a connection failed event.
     *
     * @param eventName The event name.
     * @param uuids The uuids of devices.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendConnectFailedEvent(
        const std::string& eventName,
        const std::unordered_set<std::string>& uuids,
        avsCommon::avs::Requester requester);

    /**
     * Sends an event to indicate that disconnecting with devices failed.
     *
     * @param uuids The uuids of devices.
     * @param requester Whether this was initiated by the CLOUD or DEVICE.
     */
    void executeSendDisconnectDevicesFailed(
        const std::unordered_set<std::string>& uuids,
        avsCommon::avs::Requester requester);

    /**
     * Sends an AVRCP event.
     *
     * @param eventName The AVRCP media event name.
     * @param device The device.
     */
    void executeSendMediaControlEvent(
        const std::string& eventName,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we successfully sent an AVRCP play to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlPlaySucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we failed to send an AVRCP play to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlPlayFailed(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we successfully sent an AVRCP pause to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlStopSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we failed to send an AVRCP pause to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlStopFailed(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we successfully sent an AVRCP next to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlNextSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we failed to send an AVRCP next to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlNextFailed(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we successfully sent an AVRCP previous to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlPreviousSucceeded(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends an event to indicate we failed to send an AVRCP previous to the target.
     *
     * @param device The device.
     */
    void executeSendMediaControlPreviousFailed(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Sends a media streaming event.
     *
     * @param eventName The streaming event name.
     * @param device The device.
     */
    void executeSendStreamingEvent(
        const std::string& eventName,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

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

    /**
     * Helper function to get a service from a device.
     *
     * @tparam ServiceType The type of the @c BluetoothServiceInterface.
     * @param device the activeDevice.
     * @return The instance of the service if successful, else nullptr.
     */
    template <typename ServiceType>
    std::shared_ptr<ServiceType> getService(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device);

    /**
     * Insert the @c BluetoothEventState into m_bluetoothEventStates to keep track of a bunch of succeeded events needed
     * to send to cloud.
     * @param device The device.
     * @param state The @c DeviceState of the @c BluetoothEventState.
     * @param requester The @c Requester of the @c BluetoothEventState.
     * @param profile The profile name of the @c BluetoothEventState.
     */
    void executeInsertBluetoothEventState(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::sdkInterfaces::bluetooth::DeviceState state,
        avsCommon::utils::Optional<avsCommon::avs::Requester> requester,
        avsCommon::utils::Optional<std::string> profileName);

    /**
     * Remove the @c BluetoothEventState from m_bluetoothEventStates to keep track of a bunch of succeeded events needed
     * to send to cloud.
     * @param device The device.
     * @param state  The @c DeviceState of the @c BluetoothEventState.
     * @return The @c BluetoothEventState removed from m_bluetoothEventStates.
     */
    std::shared_ptr<BluetoothEventState> executeRemoveBluetoothEventState(
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
        avsCommon::sdkInterfaces::bluetooth::DeviceState state);

    /**
     * This method is used to restrict A2DP profiles of all paired devices.
     */
    void executeRestrictA2DPDevices();

    /**
     * This method is used to unrestrict all previously A2DP restricted devices, and reconnect to previous
     * active device.
     */
    void executeUnrestrictA2DPDevices();

    /**
     * Acquires focus from the focus manager for the Bluetooth CA.
     *
     * @param the name of the calling method for logging.
     */
    void executeAcquireFocus(const std::string& callingMethodName = "");

    /**
     * Releases focus from the focus manager for the Bluetooth CA.
     *
     * @param the name of the calling method for logging.
     */
    void executeReleaseFocus(const std::string& callingMethodName = "");

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

    /**
     * The current state transition that the Bluetooth CA expects to experience when losing focus.
     */
    FocusTransitionState m_focusTransitionState;

    /**
     * The current scanning transition state. This should only be accessed from a method running on the executor.
     */
    ScanningTransitionState m_scanningTransitionState;

    /// The current @c FocusState of the device.
    avsCommon::avs::FocusState m_focusState;

    /// The id associated with MediaPlayer requests for a specific source.
    avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface::SourceId m_sourceId;

    /// The @c BluetoothDeviceManagerInterface instance responsible for device management.
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> m_deviceManager;

    /// An event queue used to store events which need to be sent. The pair is <eventName, eventPayload>.
    std::queue<std::pair<std::string, std::string>> m_eventQueue;

    /// The current activeA2DPDevice. This is the one that is connected and sending media via A2DP.
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> m_activeA2DPDevice;

    /// The cached activeDevice. This is used to help to reconnect to previous connected A2DP device.
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> m_disabledA2DPDevice;

    /// The cached restricted device list. This is used to help to unrestricted previous paired A2DP devices.
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> m_restrictedDevices;

    /// The MediaPlayer responsible for media playback.
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_mediaPlayer;

    /// Used to persist data necessary for Bluetooth. This includes UUID, MAC, and connection order.
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothStorageInterface> m_db;

    /// An eventbus used to abstract Bluetooth stack specific messages.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;

    /// Transforms incoming Media commands.
    std::shared_ptr<BluetoothMediaInputTransformer> m_mediaInputTransformer;

    /// The A2DP media stream.
    std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> m_mediaStream;

    /// An InProcessAttachment used to feed A2DP stream data into the MediaPlayer.
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> m_mediaAttachment;

    /// A writer to write the A2DP stream buffers into the InProcessAttachment.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> m_mediaAttachmentWriter;

    /// A reader that reads the InProcessAttachment.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_mediaAttachmentReader;

    /// Map of <DeviceCategory, BluetoothDeviceConnectionRuleInterface> device connection rules
    std::map<
        DeviceCategory,
        std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        m_enabledConnectionRules;

    /// Map of <DeviceCategory, Set<BluetoothDeviceInterface>> connected Bluetooth devices.
    std::map<DeviceCategory, std::set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>>
        m_connectedDevices;

    /// Map of <mac, Set<BluetoothEventState>> used to keep track of Bluetooth event state needed to send to cloud.
    std::map<std::string, std::unordered_set<std::shared_ptr<BluetoothEventState>>> m_bluetoothEventStates;

    /// A ChannelVolumeInterface that handles Volume Settings / Volume Attenuation for the underlying Bluetooth
    /// SpeakerInterface
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> m_bluetoothChannelVolumeInterface;

    /// The object to notify of Bluetooth device connections or disconnections.
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface> m_bluetoothNotifier;

    /// counter to track the number of pending focus requests/releases
    std::atomic<uint8_t> m_pendingFocusTransitions;

    /// An executor used for serializing requests on the Bluetooth agent's own thread of execution.
    avsCommon::utils::threading::Executor m_executor;
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

/**
 * Overload for the @c StreamingState enum. This will write the @c StreamingState as a string to the provided stream.
 *
 * @param stream An ostream to send the @c StreamingState as a string.
 * @param state The @c StreamingState to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const Bluetooth::StreamingState state) {
    return stream << streamingStateToString(state);
}

/**
 * Converts an enum @c FocusTransitionState to a string.
 *
 * @param state The @c FocusTransitionState.
 * @return The string form of the enum.
 */
inline std::string focusTransitionStateToString(Bluetooth::FocusTransitionState state) {
    switch (state) {
        case Bluetooth::FocusTransitionState::NONE:
            return "NONE";
        case Bluetooth::FocusTransitionState::INTERNAL:
            return "INTERNAL";
        case Bluetooth::FocusTransitionState::PENDING_INTERNAL:
            return "PENDING_INTERNAL";
        case Bluetooth::FocusTransitionState::EXTERNAL:
            return "EXTERNAL";
    }

    return "UNKNOWN";
}

/**
 * Overload for the @c FocusTransitionState enum. This will write the @c FocusTransitionState as a string to
 * the provided stream.
 *
 * @param stream An ostream to send the @c FocusTransitionState as a string.
 * @param state The @c FocusTransitionState to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const Bluetooth::FocusTransitionState state) {
    return stream << focusTransitionStateToString(state);
}

/**
 * Converts an enum @c ScanningTransitionState to a string.
 *
 * @param state The @c ScanningTransitionState.
 * @return The string form of the enum.
 */
inline std::string scanningStateToString(Bluetooth::ScanningTransitionState state) {
    switch (state) {
        case Bluetooth::ScanningTransitionState::ACTIVE:
            return "ACTIVE";
        case Bluetooth::ScanningTransitionState::PENDING_INACTIVE:
            return "PENDING_ACTIVE";
        case Bluetooth::ScanningTransitionState::INACTIVE:
            return "INACTIVE";
    }

    return "UNKNWON";
}

/**
 * Overload for the @c ScanningTransitionState enum. This will write the @c ScanningTransitionState as a string to
 * the provided stream.
 *
 * @param stream An ostream to send the @c ScanningTransitionState as a string.
 * @param state The @c ScanningTransitionState to convert.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const Bluetooth::ScanningTransitionState state) {
    return stream << scanningStateToString(state);
}

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_BLUETOOTH_H_
