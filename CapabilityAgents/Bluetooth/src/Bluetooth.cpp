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
#include "Bluetooth/Bluetooth.h"

#include <chrono>

#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSinkInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/MacAddressString.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {

using namespace rapidjson;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::utils;
using namespace avsCommon::utils::bluetooth;
using namespace avsCommon::utils::mediaPlayer;

/// String to identify log entries originating from this file.
static const std::string TAG{"Bluetooth"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The Bluetooth interface namespace.
static const std::string NAMESPACE = "Bluetooth";

/// The Bluetooth state portion of the Context.
static const NamespaceAndName BLUETOOTH_STATE{NAMESPACE, "BluetoothState"};

/// The @c ScanDevices directive identifier
static const NamespaceAndName SCAN_DEVICES{NAMESPACE, "ScanDevices"};

/// The @c ScanDevicesUpdated event.
static const NamespaceAndName SCAN_DEVICES_UPDATED{NAMESPACE, "ScanDevicesUpdated"};

/// The @c ScanDevicesFailed event.
static const NamespaceAndName SCAN_DEVICES_FAILED{NAMESPACE, "ScanDevicesFailed"};

/// The @c EnterDiscoverableMode directive identifier.
static const NamespaceAndName ENTER_DISCOVERABLE_MODE{NAMESPACE, "EnterDiscoverableMode"};

/// The @c EnterDiscoverableModeSucceeded event.
static const NamespaceAndName ENTER_DISCOVERABLE_MODE_SUCCEEDED{NAMESPACE, "EnterDiscoverableModeSucceeded"};

/// The @c EnterDiscoverableModeFailed event.
static const NamespaceAndName ENTER_DISCOVERABLE_MODE_FAILED{NAMESPACE, "EnterDiscoverableModeFailed"};

/// The @c ExitDiscoverableMode directive identifier.
static const NamespaceAndName EXIT_DISCOVERABLE_MODE{NAMESPACE, "ExitDiscoverableMode"};

/// The @c PairDeviceSucceeded event.
static const NamespaceAndName PAIR_DEVICE_SUCCEEDED{NAMESPACE, "PairDeviceSucceeded"};

/// The @c PairDeviceFailed event.
static const NamespaceAndName PAIR_DEVICE_FAILED{NAMESPACE, "PairDeviceFailed"};

/// The @c PairDevice directive identifier.
static const NamespaceAndName PAIR_DEVICE{NAMESPACE, "PairDevice"};

/// The @c UnpairDevice directive identifier.
static const NamespaceAndName UNPAIR_DEVICE{NAMESPACE, "UnpairDevice"};

/// The @c UnpairDeviceSucceeded event.
static const NamespaceAndName UNPAIR_DEVICE_SUCCEEDED{NAMESPACE, "UnpairDeviceSucceeded"};

/// The @c UnpairDeviceFailed event.
static const NamespaceAndName UNPAIR_DEVICE_FAILED{NAMESPACE, "UnpairDeviceFailed"};

/// The @c ConnectByDeviceId directive identifier.
static const NamespaceAndName CONNECT_BY_DEVICE_ID{NAMESPACE, "ConnectByDeviceId"};

/// The @c ConnectByDeviceIdSucceeded event.
static const NamespaceAndName CONNECT_BY_DEVICE_ID_SUCCEEDED{NAMESPACE, "ConnectByDeviceIdSucceeded"};

/// The @c ConnectByDeviceIdFailed event.
static const NamespaceAndName CONNECT_BY_DEVICE_ID_FAILED{NAMESPACE, "ConnectByDeviceIdFailed"};

/// The @c ConnectByProfile directive identifier.
static const NamespaceAndName CONNECT_BY_PROFILE{NAMESPACE, "ConnectByProfile"};

/// The @c ConnectByProfileSucceeded event.
static const NamespaceAndName CONNECT_BY_PROFILE_SUCCEEDED{NAMESPACE, "ConnectByProfileSucceeded"};

/// The @c ConnectByProfileFailed event.
static const NamespaceAndName CONNECT_BY_PROFILE_FAILED{NAMESPACE, "ConnectByProfileFailed"};

/// The @c DisconnectDevice directive identifier.
static const NamespaceAndName DISCONNECT_DEVICE{NAMESPACE, "DisconnectDevice"};

/// The @c DisconnectDeviceSucceeded event.
static const NamespaceAndName DISCONNECT_DEVICE_SUCCEEDED{NAMESPACE, "DisconnectDeviceSucceeded"};

/// The @c DisconnectDeviceFailed event.
static const NamespaceAndName DISCONNECT_DEVICE_FAILED{NAMESPACE, "DisconnectDeviceFailed"};

/// The @c Play directive identifier.
static const NamespaceAndName PLAY{NAMESPACE, "Play"};

/// The @c MediaControlPlaySucceeded event.
static const NamespaceAndName MEDIA_CONTROL_PLAY_SUCCEEDED{NAMESPACE, "MediaControlPlaySucceeded"};

/// The @c MediaControlPlayFailed event.
static const NamespaceAndName MEDIA_CONTROL_PLAY_FAILED{NAMESPACE, "MediaControlPlayFailed"};

/// The @c Stop directive identifier.
static const NamespaceAndName STOP{NAMESPACE, "Stop"};

/// The @c MediaControlStopSucceeded event.
static const NamespaceAndName MEDIA_CONTROL_STOP_SUCCEEDED{NAMESPACE, "MediaControlStopSucceeded"};

/// The @c MediaControlStopFailed event.
static const NamespaceAndName MEDIA_CONTROL_STOP_FAILED{NAMESPACE, "MediaControlStopFailed"};

/// The @c Next directive identifier.
static const NamespaceAndName NEXT{NAMESPACE, "Next"};

/// The @c MediaControlNextSucceeded event.
static const NamespaceAndName MEDIA_CONTROL_NEXT_SUCCEEDED{NAMESPACE, "MediaControlNextSucceeded"};

/// The @c MediaControlNextFailed event.
static const NamespaceAndName MEDIA_CONTROL_NEXT_FAILED{NAMESPACE, "MediaControlNextFailed"};

/// The @c Previous directive identifier.
static const NamespaceAndName PREVIOUS{NAMESPACE, "Previous"};

/// The @c MediaControlPreviousSucceeded event.
static const NamespaceAndName MEDIA_CONTROL_PREVIOUS_SUCCEEDED{NAMESPACE, "MediaControlPreviousSucceeded"};

/// The @c MediaControlPreviousFailed event.
static const NamespaceAndName MEDIA_CONTROL_PREVIOUS_FAILED{NAMESPACE, "MediaControlPreviousFailed"};

/// The @c StreamingStarted event.
static const NamespaceAndName STREAMING_STARTED{NAMESPACE, "StreamingStarted"};

/// The @c StreamingEnded event.
static const NamespaceAndName STREAMING_ENDED{NAMESPACE, "StreamingEnded"};

/// The @c Channel name.
static const std::string CHANNEL_NAME = avsCommon::sdkInterfaces::FocusManagerInterface::CONTENT_CHANNEL_NAME;

/// Activity ID for use by FocusManager
static const std::string ACTIVITY_ID = "Bluetooth";

/// A delay to deal with devices that can't process consecutive AVRCP commands.
static const std::chrono::milliseconds CMD_DELAY{100};

/**
 * We get the stream faster than a peer device can process an AVRCP command (for example NEXT/PAUSE).
 * Wait a bit before we get the stream so we don't get any buffers from before the command was processed.
 */
static const std::chrono::milliseconds INITIALIZE_SOURCE_DELAY{1000};

/*
 * The following are keys used for AVS Directives and Events.
 */
/// A key for an active device.
static const char ACTIVE_DEVICE_KEY[] = "activeDevice";

/// A key for the alexa (host) device.
static const char ALEXA_DEVICE_NAME_KEY[] = "alexaDevice";

/// A key for a device.
static const char DEVICE_KEY[] = "device";

/// A key for discovered devices.
static const char DISCOVERED_DEVICES_KEY[] = "discoveredDevices";

/// A key to indicate if more devices are to be reported.
static const char HAS_MORE_KEY[] = "hasMore";

/// An empty payload.
static const std::string EMPTY_PAYLOAD = "{}";

/// The friendly name.
static const char FRIENDLY_NAME_KEY[] = "friendlyName";

/// The name key for a profile.
static const char NAME_KEY[] = "name";

/// The version key for a profile.
static const char VERSION_KEY[] = "version";

/// Identifying paired devices.
static const char PAIRED_DEVICES_KEY[] = "pairedDevices";

/// Identifying a profile.
static const char PROFILE_KEY[] = "profile";

/// Identifying a profile name.
static const char PROFILE_NAME_KEY[] = "profileName";

/// Identifying the requester of the operation.
static const char REQUESTER_KEY[] = "requester";

/// A key indicating streaming status.
static const char STREAMING_KEY[] = "streaming";

/// Indicates the supported profiles.
static const char SUPPORTED_PROFILES_KEY[] = "supportedProfiles";

/// The truncated mac address.
static const char TRUNCATED_MAC_ADDRESS_KEY[] = "truncatedMacAddress";

/// The uuid generated to identify a device.
static const char UNIQUE_DEVICE_ID_KEY[] = "uniqueDeviceId";

/*
 * The following are AVS profile identifiers.
 */
/// A2DP Source.
static const std::string AVS_A2DP_SOURCE = "A2DP-SOURCE";

/// A2DP Sink.
static const std::string AVS_A2DP_SINK = "A2DP-SINK";

/// General A2DP.
static const std::string AVS_A2DP = "A2DP";

/**
 * AVS identifies this as just AVRCP. It technically refers to AVRCPTarget,
 * since the presence of this profile will dictate whether AVS will send down
 * Media control directives.
 */
static const std::string AVS_AVRCP = "AVRCP";

/// A mapping of AVS profile identifiers to the Bluetooth service UUID.
// clang-format off
static const std::unordered_map<std::string, std::string> AVS_PROFILE_MAP{
    {AVS_A2DP_SOURCE, std::string(A2DPSourceInterface::UUID)},
    {AVS_A2DP_SINK, std::string(A2DPSinkInterface::UUID)},
    {AVS_AVRCP, std::string(AVRCPTargetInterface::UUID)}};
// clang-format on

/// Bluetooth capability constants
/// Bluetooth interface type
static const std::string BLUETOOTH_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// Bluetooth interface name
static const std::string BLUETOOTH_CAPABILITY_INTERFACE_NAME = "Bluetooth";

/// Bluetooth interface version. Version 1.0 works with StreamingStarted and StreamingEnded.
static const std::string BLUETOOTH_CAPABILITY_INTERFACE_VERSION = "1.0";

/**
 * Creates the Bluetooth capability configuration.
 *
 * @return The Bluetooth capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getBluetoothCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, BLUETOOTH_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, BLUETOOTH_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, BLUETOOTH_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

/**
 * Utility function to evaluate whether a device supports a specific AVS profile.
 *
 * @param device The device.
 * @param avsProfileName The AVS Profile to match against.
 * @param A bool indicating whether the device supports the specific profile.
 * @return A bool indicating success.
 */
static bool supportsAvsProfile(std::shared_ptr<BluetoothDeviceInterface> device, const std::string& avsProfileName) {
    ACSDK_DEBUG5(LX(__func__).d("avsProfileName", avsProfileName));

    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return false;
    } else if (avsProfileName.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyProfile"));
        return false;
    }

    for (const auto& sdpRecord : device->getSupportedServices()) {
        if (AVS_A2DP == avsProfileName && (sdpRecord->getUuid() == AVS_PROFILE_MAP.at(AVS_A2DP_SOURCE) ||
                                           sdpRecord->getUuid() == AVS_PROFILE_MAP.at(AVS_A2DP_SINK))) {
            return true;
        } else if (AVS_A2DP_SOURCE == avsProfileName && sdpRecord->getUuid() == AVS_PROFILE_MAP.at(AVS_A2DP_SOURCE)) {
            return true;
        } else if (AVS_A2DP_SINK == avsProfileName && sdpRecord->getUuid() == AVS_PROFILE_MAP.at(AVS_A2DP_SINK)) {
            return true;
        } else if (AVS_AVRCP == avsProfileName && sdpRecord->getUuid() == AVS_PROFILE_MAP.at(AVS_AVRCP)) {
            return true;
        }
    }

    ACSDK_DEBUG5(LX(__func__)
                     .d("reason", "profileNotSupported")
                     .d("deviceMac", device->getMac())
                     .d("avsProfile", avsProfileName));

    return false;
}

/**
 * TODO: Move this to MacAddressString class.
 * Utility function to truncate a valid MAC address. The first 8 octets are X'd out.
 *
 * @param mac The untruncated MAC address.
 * @param[out] truncatedMac The truncated MAC address.
 * @return The truncated mac address if successful, otherwise the original string.
 */
static bool truncate(const std::string& mac, std::string* truncatedMac) {
    // The expected size of a MAC address in the format XX:XX:XX:XX:XX:XX.
    const unsigned int MAC_SIZE = 17;

    // Not a valid mac string, return.
    if (!MacAddressString::create(mac)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidMac"));
        return false;
    } else if (mac.length() != MAC_SIZE) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidMac"));
        return false;
    } else if (!truncatedMac) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullTruncatedMac"));
        return false;
    }

    *truncatedMac = mac;

    char X = 'X';
    truncatedMac->at(0) = X;
    truncatedMac->at(1) = X;
    truncatedMac->at(3) = X;
    truncatedMac->at(4) = X;
    truncatedMac->at(6) = X;
    truncatedMac->at(7) = X;
    truncatedMac->at(9) = X;
    truncatedMac->at(10) = X;

    return true;
}

/**
 * Utility function to extract AVS compliant profiles. This returns a rapidjson node
 * containing an array of supported profiles.
 *
 * @param device The device.
 * @param allocator The allocator which will be used to create @c supportedProfiles.
 * @param[out] supportedProfiles A rapidjson node containing the supported profiles.
 * @return A bool indicating success.
 */
static bool extractAvsProfiles(
    std::shared_ptr<BluetoothDeviceInterface> device,
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value* supportedProfiles) {
    if (!device || !supportedProfiles || !supportedProfiles->IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidInputParameters"));
        return false;
    }

    for (const auto& sdp : device->getSupportedServices()) {
        if (A2DPSourceInterface::UUID == sdp->getUuid()) {
            rapidjson::Value profile(rapidjson::kObjectType);
            profile.AddMember("name", AVS_A2DP_SOURCE, allocator);
            profile.AddMember("version", sdp->getVersion(), allocator);
            supportedProfiles->PushBack(profile, allocator);
        } else if (AVRCPTargetInterface::UUID == sdp->getUuid()) {
            rapidjson::Value profile(rapidjson::kObjectType);
            profile.AddMember("name", AVS_AVRCP, allocator);
            profile.AddMember("version", sdp->getVersion(), allocator);
            supportedProfiles->PushBack(profile, allocator);
        } else if (A2DPSinkInterface::UUID == sdp->getUuid()) {
            rapidjson::Value profile(rapidjson::kObjectType);
            profile.AddMember("name", AVS_A2DP_SINK, allocator);
            profile.AddMember("version", sdp->getVersion(), allocator);
            supportedProfiles->PushBack(profile, allocator);
        }
    }

    return true;
}

/**
 * Utility function to squash @c StreamingState into the streaming states
 * that AVS has defined.
 *
 * @param state The @c StreamingState.
 * @return A string represening the streaming state.
 */
static std::string convertToAVSStreamingState(const Bluetooth::StreamingState& state) {
    switch (state) {
        case Bluetooth::StreamingState::INACTIVE:
            return streamingStateToString(Bluetooth::StreamingState::INACTIVE);
        case Bluetooth::StreamingState::PAUSED:
            return streamingStateToString(Bluetooth::StreamingState::PAUSED);
        // TODO ACSDK-1397: More accurately send INACTIVE/PAUSED for PENDING_ACTIVE.
        case Bluetooth::StreamingState::PENDING_ACTIVE:
        case Bluetooth::StreamingState::PENDING_PAUSED:
        case Bluetooth::StreamingState::ACTIVE:
            return streamingStateToString(Bluetooth::StreamingState::ACTIVE);
    }

    return "UNKNOWN";
}

std::shared_ptr<Bluetooth> Bluetooth::create(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<BluetoothStorageInterface> bluetoothStorage,
    std::unique_ptr<BluetoothDeviceManagerInterface> deviceManager,
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<BluetoothAVRCPTransformer> avrcpTransformer) {
    ACSDK_DEBUG5(LX(__func__));

    if (!contextManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullContextManager"));
    } else if (!focusManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullFocusManager"));
    } else if (!messageSender) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMessageSender"));
    } else if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullExceptionEncounteredSender"));
    } else if (!bluetoothStorage) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullBluetoothStorage"));
    } else if (!deviceManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDeviceManager"));
    } else if (!eventBus) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEventBus"));
    } else if (!mediaPlayer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMediaPlayer"));
    } else if (!customerDataManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullCustomerDataManager"));
    } else {
        auto bluetooth = std::shared_ptr<Bluetooth>(new Bluetooth(
            contextManager,
            focusManager,
            messageSender,
            exceptionEncounteredSender,
            bluetoothStorage,
            deviceManager,
            eventBus,
            mediaPlayer,
            customerDataManager,
            avrcpTransformer));

        if (bluetooth->init()) {
            return bluetooth;
        } else {
            ACSDK_ERROR(LX(__func__).d("reason", "initFailed"));
        }
    }

    return nullptr;
}

bool Bluetooth::init() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_db->open()) {
        ACSDK_INFO(LX("init").m("Couldn't open database.  Creating."));
        if (!m_db->createDatabase()) {
            ACSDK_ERROR(LX("initFailed").d("reason", "createDatabaseFailed"));
            return false;
        }
    }

    m_eventBus->addListener(
        {avsCommon::utils::bluetooth::BluetoothEventType::DEVICE_DISCOVERED,
         avsCommon::utils::bluetooth::BluetoothEventType::DEVICE_STATE_CHANGED,
         avsCommon::utils::bluetooth::BluetoothEventType::STREAMING_STATE_CHANGED},
        shared_from_this());

    m_mediaPlayer->setObserver(shared_from_this());

    executeUpdateContext();

    return true;
}

Bluetooth::Bluetooth(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<BluetoothStorageInterface> bluetoothStorage,
    std::unique_ptr<BluetoothDeviceManagerInterface>& deviceManager,
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
    std::shared_ptr<MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<BluetoothAVRCPTransformer> avrcpTransformer) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"Bluetooth"},
        CustomerDataHandler{customerDataManager},
        m_messageSender{messageSender},
        m_contextManager{contextManager},
        m_focusManager{focusManager},
        m_streamingState{StreamingState::INACTIVE},
        m_focusState{FocusState::NONE},
        m_sourceId{MediaPlayerInterface::ERROR},
        m_deviceManager{std::move(deviceManager)},
        m_mediaPlayer{mediaPlayer},
        m_db{bluetoothStorage},
        m_eventBus{eventBus},
        m_avrcpTransformer{avrcpTransformer},
        m_mediaStream{nullptr} {
    m_capabilityConfigurations.insert(getBluetoothCapabilityConfiguration());
}

DirectiveHandlerConfiguration Bluetooth::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));

    DirectiveHandlerConfiguration configuration;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    configuration[SCAN_DEVICES] = neitherNonBlockingPolicy;
    configuration[ENTER_DISCOVERABLE_MODE] = neitherNonBlockingPolicy;
    configuration[EXIT_DISCOVERABLE_MODE] = neitherNonBlockingPolicy;
    configuration[PAIR_DEVICE] = neitherNonBlockingPolicy;
    configuration[UNPAIR_DEVICE] = neitherNonBlockingPolicy;
    configuration[CONNECT_BY_DEVICE_ID] = neitherNonBlockingPolicy;
    configuration[CONNECT_BY_PROFILE] = neitherNonBlockingPolicy;
    configuration[DISCONNECT_DEVICE] = neitherNonBlockingPolicy;
    configuration[PLAY] = audioNonBlockingPolicy;
    configuration[STOP] = audioNonBlockingPolicy;
    configuration[NEXT] = audioNonBlockingPolicy;
    configuration[PREVIOUS] = audioNonBlockingPolicy;

    return configuration;
}

void Bluetooth::doShutdown() {
    ACSDK_DEBUG5(LX(__func__));

    m_executor.shutdown();

    // Finalizing dependencies first

    m_db->close();
    m_db.reset();

    // Media Stream
    m_mediaStream.reset();
    m_mediaAttachment.reset();
    m_mediaAttachmentWriter.reset();

    // MediaPlayer
    m_mediaPlayer->setObserver(nullptr);
    if (MediaPlayerInterface::ERROR != m_sourceId) {
        m_mediaPlayer->stop(m_sourceId);
    }
    cleanupMediaSource();
    m_mediaPlayer.reset();

    // AVS
    m_messageSender.reset();
    m_focusManager.reset();
    m_contextManager->setStateProvider(BLUETOOTH_STATE, nullptr);
    m_contextManager.reset();

    auto hostController = m_deviceManager->getHostController();
    if (hostController) {
        hostController->stopScan().get();
        hostController->exitDiscoverableMode().get();
    }

    m_activeDevice.reset();

    // Finalizing the implementation

    m_deviceManager.reset();
    m_eventBus.reset();

    m_observers.clear();
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> Bluetooth::getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void Bluetooth::clearData() {
    m_executor.submit([this] {
        ACSDK_DEBUG5(LX("clearData"));

        // Stop scanning and discoverability.
        auto hostController = m_deviceManager->getHostController();
        if (hostController->isScanning()) {
            hostController->stopScan().get();
            ACSDK_DEBUG5(LX("clearData").d("action", "stoppedScanning"));
        }

        if (hostController->isDiscoverable()) {
            hostController->exitDiscoverableMode().get();
            ACSDK_DEBUG5(LX("clearData").d("action", "disabledDiscoverable"));
        }

        // Unpair all devices.
        for (auto& device : m_deviceManager->getDiscoveredDevices()) {
            if (device->isPaired()) {
                device->unpair().get();
                ACSDK_DEBUG5(LX("clearData").d("action", "unpairDevice").d("device", device->getFriendlyName()));
            }
        }

        m_db->clear();
    });
}

void Bluetooth::executeInitializeMediaSource() {
    ACSDK_DEBUG5(LX(__func__));

    std::this_thread::sleep_for(INITIALIZE_SOURCE_DELAY);
    auto a2dpSource = m_activeDevice->getA2DPSource();
    if (!a2dpSource) {
        ACSDK_CRITICAL(LX(__func__).d("reason", "a2dpSourceNotSupported"));
        return;
    }

    auto stream = a2dpSource->getSourceStream();
    if (!stream) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullA2DPStream"));
        return;
    }

    setCurrentStream(stream);
}

void Bluetooth::cleanupMediaSource() {
    ACSDK_DEBUG5(LX(__func__));

    setCurrentStream(nullptr);
    m_sourceId = MediaPlayerInterface::ERROR;
}

bool Bluetooth::retrieveUuid(const std::string& mac, std::string* uuid) {
    ACSDK_DEBUG5(LX(__func__));

    if (!uuid) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullUuid"));
        return false;
    }

    if (m_db->getUuid(mac, uuid)) {
        return true;
    }

    ACSDK_INFO(LX(__func__).d("reason", "noMatchingUUID").d("mac", mac));
    *uuid = uuidGeneration::generateUUID();
    if (!m_db->insertByMac(mac, *uuid, false)) {
        ACSDK_ERROR(LX(__func__).d("reason", "insertingToDBFailed").d("mac", mac).d("uuid", *uuid));
        return false;
    }

    return true;
}

void Bluetooth::clearUnusedUuids() {
    ACSDK_DEBUG5(LX(__func__));

    std::list<std::string> descendingMacs;
    if (!m_db->getOrderedMac(false, &descendingMacs)) {
        ACSDK_ERROR(LX(__func__).d("reason", "databaseQueryFailed"));
        return;
    }

    for (const auto& mac : descendingMacs) {
        std::shared_ptr<BluetoothDeviceInterface> device = retrieveDeviceByMac(mac);
        if (!device) {
            m_db->remove(mac);
        }
    }
}

std::shared_ptr<BluetoothDeviceInterface> Bluetooth::retrieveDeviceByUuid(const std::string& uuid) {
    ACSDK_DEBUG5(LX(__func__).d("uuid", uuid));

    std::string mac;
    if (!m_db->getMac(uuid, &mac)) {
        ACSDK_ERROR(LX("retrieveDeviceByUuidFailed").d("reason", "macNotFound").d("uuid", uuid));

        return nullptr;
    }

    std::shared_ptr<BluetoothDeviceInterface> device = retrieveDeviceByMac(mac);
    if (!device) {
        ACSDK_ERROR(LX("retrieveDeviceByUuidFailed").d("reason", "couldNotFindDevice").d("mac", mac));
        return nullptr;
    }

    return device;
}

std::shared_ptr<BluetoothDeviceInterface> Bluetooth::retrieveDeviceByMac(const std::string& mac) {
    ACSDK_DEBUG5(LX(__func__).d("mac", mac));
    auto devices = m_deviceManager->getDiscoveredDevices();
    for (const auto& device : devices) {
        if (device->getMac() == mac) {
            return device;
        }
    }

    return nullptr;
}

void Bluetooth::executeUpdateContext() {
    rapidjson::Document payload(rapidjson::kObjectType);

    // Construct alexaDevice.
    rapidjson::Value alexaDevice(rapidjson::kObjectType);
    alexaDevice.AddMember(
        FRIENDLY_NAME_KEY, m_deviceManager->getHostController()->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(ALEXA_DEVICE_NAME_KEY, alexaDevice, payload.GetAllocator());

    // Construct pairedDevices and activeDevice.
    rapidjson::Value pairedDevices(rapidjson::kArrayType);

    std::unordered_map<std::string, std::string> macToUuids;
    if (!m_db->getMacToUuid(&macToUuids)) {
        ACSDK_ERROR(LX(__func__).d("reason", "databaseQueryFailed"));
        macToUuids.clear();
    }

    for (const auto& device : m_deviceManager->getDiscoveredDevices()) {
        ACSDK_DEBUG9(LX(__func__)
                         .d("friendlyName", device->getFriendlyName())
                         .d("mac", device->getMac())
                         .d("paired", device->isPaired()));
        if (!device->isPaired()) {
            ACSDK_DEBUG9(LX(__func__).d("reason", "deviceNotPaired").m("Excluding"));
            continue;
        }

        // Retrieve the UUID from our map.
        std::string uuid;
        const auto uuidIt = macToUuids.find(device->getMac());

        if (macToUuids.end() != uuidIt) {
            uuid = uuidIt->second;
        } else {
            // We couldn't find it. This is unexpected, but proceed and attempt to generate a UUID.
            ACSDK_DEBUG5(LX(__func__).d("reason", "uuidNotFound").d("mac", device->getMac()));
            uuid.clear();

            if (!retrieveUuid(device->getMac(), &uuid)) {
                // Unable to create UUID.
                ACSDK_ERROR(LX("executeUpdateContextFailed").d("reason", "retrieveUuidFailed"));
                continue;
            }
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());

        rapidjson::Value supportedProfiles(rapidjson::kArrayType);
        // If this fails, add an empty array.
        if (!extractAvsProfiles(device, payload.GetAllocator(), &supportedProfiles)) {
            supportedProfiles = rapidjson::Value(rapidjson::kArrayType);
        }

        deviceJson.AddMember(SUPPORTED_PROFILES_KEY, supportedProfiles.GetArray(), payload.GetAllocator());

        // Add activeDevice.
        if (m_activeDevice && device == m_activeDevice) {
            // Deep copy, otherwise move will occur.
            rapidjson::Value activeDevice(rapidjson::kObjectType);
            activeDevice.CopyFrom(deviceJson, payload.GetAllocator());
            activeDevice.AddMember(STREAMING_KEY, convertToAVSStreamingState(m_streamingState), payload.GetAllocator());
            payload.AddMember(ACTIVE_DEVICE_KEY, activeDevice, payload.GetAllocator());
        }
        pairedDevices.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(PAIRED_DEVICES_KEY, pairedDevices, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    ACSDK_INFO(LX(__func__).d("buffer", buffer.GetString()));

    m_contextManager->setState(BLUETOOTH_STATE, buffer.GetString(), StateRefreshPolicy::NEVER);
}

void Bluetooth::executeQueueEventAndRequestContext(const std::string& eventName, const std::string& eventPayload) {
    ACSDK_DEBUG5(LX(__func__).d("eventName", eventName));

    m_eventQueue.push(std::make_pair(eventName, eventPayload));
    m_contextManager->getContext(shared_from_this());
}

void Bluetooth::executeDrainQueue() {
    while (!m_cmdQueue.empty()) {
        AVRCPCommand cmd = m_cmdQueue.front();
        m_cmdQueue.pop_front();

        if (!m_activeDevice || !m_activeDevice->getAVRCPTarget()) {
            ACSDK_ERROR(LX(__func__).d("reason", "invalidState"));
            continue;
        }

        auto avrcpTarget = m_activeDevice->getAVRCPTarget();
        switch (cmd) {
            /*
             * MediaControl Events should only be sent in response to a directive.
             * Don't send events for PLAY or PAUSE because they are never queued
             * as a result of a directive, only as a result of a focus change.
             */
            case AVRCPCommand::PLAY:
                avrcpTarget->play();
                m_streamingState = StreamingState::PENDING_ACTIVE;
                break;
            case AVRCPCommand::PAUSE:
                avrcpTarget->pause();
                m_streamingState = StreamingState::PENDING_PAUSED;
                break;
            case AVRCPCommand::NEXT:
                executeNext();
                break;
            case AVRCPCommand::PREVIOUS:
                executePrevious();
                break;
        }

        // Some devices can't process two consecutive commands.
        std::this_thread::sleep_for(CMD_DELAY);
    }
}

void Bluetooth::executeAbortMediaPlayback() {
    // If the sourceId is set, stop the MediaPlayer.
    if (MediaPlayerInterface::ERROR != m_sourceId && !m_mediaPlayer->stop(m_sourceId)) {
        /*
         * We couldn't stop the MediaPlayer successfully. Something is really wrong at this point,
         * at least we'll stop sending any more data to the MediaPlayer by cleaning up the source.
         */
        cleanupMediaSource();
    }

    if (FocusState::FOREGROUND == m_focusState || FocusState::BACKGROUND == m_focusState) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "releasingFocus").d("focusState", m_focusState));
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
}

void Bluetooth::executeEnterForeground() {
    ACSDK_DEBUG5(LX(__func__).d("streamingState", streamingStateToString(m_streamingState)));
    if (!m_activeDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "noActiveDevice"));
        executeAbortMediaPlayback();
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_INFO(LX(__func__).d("reason", "avrcpTargetNotSupported"));
    }

    switch (m_streamingState) {
        case StreamingState::ACTIVE:
            break;
        case StreamingState::PENDING_ACTIVE:
            if (m_mediaStream == nullptr) {
                executeInitializeMediaSource();
            }
            executeDrainQueue();
            if (!m_mediaPlayer->play(m_sourceId)) {
                ACSDK_ERROR(LX(__func__).d("reason", "playFailed").d("sourceId", m_sourceId));
            }
            break;
        case StreamingState::PENDING_PAUSED:
        case StreamingState::PAUSED:
        case StreamingState::INACTIVE:
            // We push a play because some devices do not auto start playback next/previous command.
            m_cmdQueue.push_front(AVRCPCommand::PLAY);
            executeDrainQueue();
            if (m_mediaStream == nullptr) {
                executeInitializeMediaSource();
            }
            if (!m_mediaPlayer->play(m_sourceId)) {
                ACSDK_ERROR(LX(__func__).d("reason", "playFailed").d("sourceId", m_sourceId));
            }
            break;
    }
}

void Bluetooth::executeEnterBackground() {
    ACSDK_DEBUG5(LX(__func__).d("streamingState", streamingStateToString(m_streamingState)));

    if (!m_activeDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "noActiveDevice"));
        executeAbortMediaPlayback();
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_INFO(LX(__func__).d("reason", "avrcpTargetNotSupported"));
    }

    switch (m_streamingState) {
        case StreamingState::ACTIVE:
            if (avrcpTarget && !avrcpTarget->pause()) {
                ACSDK_ERROR(LX(__func__).d("reason", "avrcpPauseFailed"));
            }
            m_streamingState = StreamingState::PENDING_PAUSED;
            if (!m_mediaPlayer->stop(m_sourceId)) {
                ACSDK_ERROR(LX(__func__).d("reason", "stopFailed").d("sourceId", m_sourceId));
            }
            break;
        // TODO: ACSDK-1306. We should just be able to stop the MediaPlayer, but there was a deadlock
        // in trying to set the pipeline to null. So we wait and clear it in the callback.
        case StreamingState::PENDING_ACTIVE:
            if (avrcpTarget && !avrcpTarget->pause()) {
                ACSDK_ERROR(LX(__func__).d("reason", "avrcpPauseFailed"));
            }
            m_streamingState = StreamingState::PENDING_PAUSED;
            break;
        case StreamingState::PAUSED:
        case StreamingState::PENDING_PAUSED:
        case StreamingState::INACTIVE:
            break;
    }
}

// Either you were kicked off or you're done with the channel.
void Bluetooth::executeEnterNone() {
    ACSDK_DEBUG5(LX(__func__).d("streamingState", streamingStateToString(m_streamingState)));

    if (!m_activeDevice) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "noActiveDevice"));
        executeAbortMediaPlayback();
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_INFO(LX(__func__).d("reason", "avrcpTargetNotSupported"));
    }

    switch (m_streamingState) {
        case StreamingState::ACTIVE:
        case StreamingState::PENDING_ACTIVE:
            if (avrcpTarget && !avrcpTarget->pause()) {
                ACSDK_ERROR(LX(__func__).d("reason", "avrcpPauseFailed"));
            }
            m_streamingState = StreamingState::PENDING_PAUSED;
            if (!m_mediaPlayer->stop(m_sourceId)) {
                ACSDK_ERROR(LX(__func__).d("reason", "stopFailed").d("sourceId", m_sourceId));
                cleanupMediaSource();
            }
            break;
        case StreamingState::PENDING_PAUSED:
            if (!m_mediaPlayer->stop(m_sourceId)) {
                ACSDK_ERROR(LX(__func__).d("reason", "stopFailed").d("sourceId", m_sourceId));
                cleanupMediaSource();
            }
            break;
        case StreamingState::PAUSED:
        case StreamingState::INACTIVE:
            break;
    }
}

void Bluetooth::onFocusChanged(FocusState newFocus) {
    ACSDK_DEBUG5(LX(__func__).d("current", m_focusState).d("new", newFocus));

    m_executor.submit([this, newFocus] {
        switch (newFocus) {
            case FocusState::FOREGROUND: {
                executeEnterForeground();
                break;
            }
            case FocusState::BACKGROUND: {
                executeEnterBackground();
                break;
            }
            case FocusState::NONE: {
                executeEnterNone();
                break;
            }
        }

        m_focusState = newFocus;
    });
}

void Bluetooth::onContextAvailable(const std::string& jsonContext) {
    m_executor.submit([this, jsonContext] {
        ACSDK_DEBUG5(LX("onContextAvailableLambda"));

        if (m_eventQueue.empty()) {
            ACSDK_ERROR(LX("contextRequestedWithNoQueuedEvents"));
            return;
        }

        std::pair<std::string, std::string> nameAndPayload = m_eventQueue.front();
        m_eventQueue.pop();
        auto event = buildJsonEventString(nameAndPayload.first, "", nameAndPayload.second, jsonContext);

        ACSDK_DEBUG5(LX("onContextAvailableLambda").d("event", event.second));
        // Send Message happens on the calling thread. Do not block the ContextManager thread.
        auto request = std::make_shared<MessageRequest>(event.second);
        m_messageSender->sendMessage(request);
    });
}

void Bluetooth::onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    std::pair<std::string, std::string> nameAndPayload = m_eventQueue.front();
    m_eventQueue.pop();
    ACSDK_ERROR(LX(__func__)
                    .d("error", error)
                    .d("eventName", nameAndPayload.first)
                    .sensitive("payload", nameAndPayload.second));
}

void Bluetooth::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void Bluetooth::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // No-op
}

static bool parseDirectivePayload(const std::string& payload, Document* document) {
    ACSDK_DEBUG5(LX(__func__));
    if (!document) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed").d("reason", "nullDocument"));
        return false;
    }

    ParseResult result = document->Parse(payload);
    if (!result) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return false;
    }

    return true;
}

// TODO: ACSDK-1369 Refactor this method.
void Bluetooth::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        const std::string directiveName = info->directive->getName();

        Document payload(kObjectType);
        if (!parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncountered(
                info, "Payload Parsing Failed", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (directiveName == SCAN_DEVICES.name) {
            clearUnusedUuids();
            executeSetScanMode(true);
        } else if (directiveName == ENTER_DISCOVERABLE_MODE.name) {
            if (executeSetDiscoverableMode(true)) {
                executeSendEnterDiscoverableModeSucceeded();
            } else {
                executeSendEnterDiscoverableModeFailed();
            }
        } else if (directiveName == EXIT_DISCOVERABLE_MODE.name) {
            // There are no events to send in case this operation fails. The best we can do is log.
            executeSetScanMode(false);
            executeSetDiscoverableMode(false);
        } else if (directiveName == PAIR_DEVICE.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string uuid;

            if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                /*
                 * AVS expects this sequence of implicit behaviors.
                 * AVS should send individual directives, but we will handle this for now.
                 */
                executeSetScanMode(false);
                executeSetDiscoverableMode(false);
                if (executePairDevice(uuid)) {
                    executeConnectByDeviceId(uuid);
                }
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == UNPAIR_DEVICE.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string uuid;

            if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                executeUnpairDevice(uuid);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == CONNECT_BY_DEVICE_ID.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string uuid;

            if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                executeConnectByDeviceId(uuid);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == CONNECT_BY_PROFILE.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string profileName;
            std::string version;

            if (json::jsonUtils::findNode(payload, PROFILE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, NAME_KEY, &profileName)) {
                // TODO ACSDK-1395. Parse and utilize the version.
                if (!json::jsonUtils::retrieveValue(it->value, VERSION_KEY, &version)) {
                    // Some use cases are missing the version. We're not using the version, so proceed.
                    ACSDK_INFO(LX("parsingConnectByProfileDirective")
                                   .d("reason", "versionMissing")
                                   .d("action", "defaultToEmptyAndProceed"));
                    version.clear();
                }

                executeConnectByProfile(profileName, version);
            } else {
                sendExceptionEncountered(
                    info, "profileName not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == DISCONNECT_DEVICE.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string uuid;

            if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                executeDisconnectDevice(uuid);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == PLAY.name) {
            executePlay();
        } else if (directiveName == STOP.name) {
            executeStop();
        } else if (directiveName == NEXT.name) {
            if (FocusState::FOREGROUND == m_focusState) {
                executeNext();
            } else {
                // The queue is drained when we enter the foreground.
                m_cmdQueue.push_back(AVRCPCommand::NEXT);
            }
        } else if (directiveName == PREVIOUS.name) {
            if (FocusState::FOREGROUND == m_focusState) {
                executePrevious();
            } else {
                // The queue is drained when we enter the foreground.
                m_cmdQueue.push_back(AVRCPCommand::PREVIOUS);
            }
        } else {
            sendExceptionEncountered(info, "Unexpected Directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        executeSetHandlingCompleted(info);
    });
}

void Bluetooth::executePlay() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_activeDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullActiveDevice"));
        executeSendMediaControlPlayFailed();
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlPlayFailed();
        return;
    }

    // Some applications treat PLAY/PAUSE as a toggle. Don't send if we're already about to play.
    if (StreamingState::ACTIVE == m_streamingState || StreamingState::PENDING_ACTIVE == m_streamingState) {
        executeSendMediaControlPlaySucceeded();
        return;
    }

    bool success = true;
    /// This means that we have not yet sent an AVRCP play command yet. Do so.
    if (StreamingState::PAUSED == m_streamingState || StreamingState::INACTIVE == m_streamingState) {
        success = avrcpTarget->play();
    }

    if (success) {
        m_streamingState = StreamingState::PENDING_ACTIVE;
        executeSendMediaControlPlaySucceeded();
        if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), ACTIVITY_ID)) {
            ACSDK_ERROR(LX(__func__).d("reason", "acquireChannelFailed"));
        }
    } else {
        executeSendMediaControlPlayFailed();
    }
}

void Bluetooth::executeStop() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_activeDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullActiveDevice"));
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlStopFailed();
        return;
    }

    // Some applications treat PLAY/PAUSE as a toggle. Don't send if we're already paused.
    if (StreamingState::PAUSED == m_streamingState || StreamingState::PENDING_PAUSED == m_streamingState) {
        executeSendMediaControlStopSucceeded();
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
        return;
    }

    bool success = true;
    if (StreamingState::ACTIVE == m_streamingState) {
        success = avrcpTarget->pause();
    }

    if (success) {
        m_streamingState = StreamingState::PENDING_PAUSED;
        executeSendMediaControlStopSucceeded();
    } else {
        executeSendMediaControlStopFailed();
    }

    // Even if we failed to stop the stream, release the channel so we stop audio playback.
    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
}

void Bluetooth::executeNext() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_activeDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullActiveDevice"));
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlNextFailed();
        return;
    }

    if (avrcpTarget->next()) {
        executeSendMediaControlNextSucceeded();
    } else {
        executeSendMediaControlNextFailed();
    }
}

void Bluetooth::executePrevious() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_activeDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullActiveDevice"));
        return;
    }

    auto avrcpTarget = m_activeDevice->getAVRCPTarget();
    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlPreviousFailed();
        return;
    }

    if (avrcpTarget->previous()) {
        executeSendMediaControlPreviousSucceeded();
    } else {
        executeSendMediaControlPreviousFailed();
    }
}

bool Bluetooth::executePairDevice(const std::string& uuid) {
    ACSDK_DEBUG5(LX(__func__));

    auto device = retrieveDeviceByUuid(uuid);
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
        return false;
    }

    if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::pair)) {
        m_lastPairMac = device->getMac();
        executeSendPairDeviceSucceeded(device);
        return true;
    } else {
        executeSendPairDeviceFailed();
        return false;
    }
}

bool Bluetooth::executeUnpairDevice(const std::string& uuid) {
    ACSDK_DEBUG5(LX(__func__));

    auto device = retrieveDeviceByUuid(uuid);
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
        return false;
    }

    // If the device is connected, disconnect it before unpairing
    if (device->isConnected()) {
        executeDisconnectDevice(uuid);
    }

    if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::unpair)) {
        m_lastPairMac.clear();
        m_activeDevice.reset();

        executeSendUnpairDeviceSucceeded(device);
        return true;
    } else {
        executeSendUnpairDeviceFailed();
        return false;
    }
}

void Bluetooth::executeConnectByDeviceId(const std::string& uuid) {
    ACSDK_DEBUG5(LX(__func__));

    auto device = retrieveDeviceByUuid(uuid);
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
        return;
    }

    bool supportsA2DP = supportsAvsProfile(device, AVS_A2DP);

    if (!supportsA2DP) {
        ACSDK_INFO(LX(__func__).d("reason", "noSupportedA2DPRoles").m("Connect Request Rejected"));
    }

    if (supportsA2DP && executeFunctionOnDevice(device, &BluetoothDeviceInterface::connect)) {
        executeOnDeviceConnect(device);
        executeSendConnectByDeviceIdSucceeded(device, Requester::CLOUD);
    } else {
        executeSendConnectByDeviceIdFailed(device, Requester::CLOUD);
    }
}

void Bluetooth::executeConnectByProfile(const std::string& profileName, const std::string& profileVersion) {
    // TODO: ACSDK-1395 Utilize the verison as an additional filter.
    ACSDK_DEBUG5(LX(__func__).d("profileName", profileName).d("profileVersion", profileVersion));

    std::list<std::string> descendingMacs;
    if (!m_db->getOrderedMac(false, &descendingMacs)) {
        ACSDK_ERROR(LX(__func__).d("reason", "databaseQueryFailed"));
        executeSendConnectByProfileFailed(profileName, Requester::CLOUD);
        return;
    }

    bool matchFound = false;
    std::shared_ptr<BluetoothDeviceInterface> matchedDevice;

    for (const auto& mac : descendingMacs) {
        auto device = retrieveDeviceByMac(mac);
        if (!device) {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("mac", mac));
            continue;
        }
        /*
         * We're only connecting devices that have been
         * previously connected and currently paired.
         */
        if (!device->isPaired()) {
            ACSDK_INFO(LX(__func__).d("reason", "deviceUnpaired").d("mac", mac));
            continue;
        }

        matchFound = supportsAvsProfile(device, profileName);
        if (matchFound) {
            matchedDevice = device;
            break;
        }
    }

    if (matchFound && executeFunctionOnDevice(matchedDevice, &BluetoothDeviceInterface::connect)) {
        executeOnDeviceConnect(matchedDevice);
        executeSendConnectByProfileSucceeded(matchedDevice, profileName, Requester::CLOUD);
    } else {
        executeSendConnectByProfileFailed(profileName, Requester::CLOUD);
    }
}

void Bluetooth::executeOnDeviceConnect(std::shared_ptr<BluetoothDeviceInterface> device) {
    ACSDK_DEBUG5(LX(__func__));

    // Currently there is an active device. Disconnect it since the new device will have priority.
    if (m_activeDevice) {
        ACSDK_DEBUG(LX(__func__).d("reason", "activeDeviceExists"));
        if (m_activeDevice->disconnect().get()) {
            executeOnDeviceDisconnect(Requester::DEVICE);
        } else {
            // Failed to disconnect activeDevice, user will have to manually disconnect the device.
            ACSDK_ERROR(
                LX(__func__).d("reason", "disconnectExistingActiveDeviceFailed").d("mac", m_activeDevice->getMac()));
        }
    }

    m_activeDevice = device;

    // Notify observers when a bluetooth device is connected.
    for (const auto& observer : m_observers) {
        observer->onActiveDeviceConnected(generateDeviceAttributes(m_activeDevice));
    }

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    // Reinsert into the database for ordering.
    m_db->insertByMac(device->getMac(), uuid, true);
}

void Bluetooth::executeOnDeviceDisconnect(avsCommon::avs::Requester requester) {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_activeDevice) {
        ACSDK_WARN(LX(__func__).d("reason", "noActiveDevice"));
        return;
    }

    if (StreamingState::INACTIVE != m_streamingState) {
        ACSDK_DEBUG5(LX(__func__)
                         .d("currentState", streamingStateToString(m_streamingState))
                         .d("newState", streamingStateToString(StreamingState::INACTIVE)));
        // Needs to be sent while we still have an activeDevice in the Context.
        if (StreamingState::ACTIVE == m_streamingState) {
            executeSendStreamingEnded(m_activeDevice);
        }
        m_streamingState = StreamingState::INACTIVE;
    }

    auto device = m_activeDevice;

    // Notify observers when a bluetooth device is disconnected.
    for (const auto& observer : m_observers) {
        observer->onActiveDeviceDisconnected(generateDeviceAttributes(m_activeDevice));
    }

    m_activeDevice.reset();
    executeSendDisconnectDeviceSucceeded(device, requester);
}

void Bluetooth::executeDisconnectDevice(const std::string& uuid) {
    ACSDK_DEBUG5(LX(__func__));

    auto device = retrieveDeviceByUuid(uuid);
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
        return;
    }

    if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::disconnect)) {
        executeOnDeviceDisconnect(Requester::CLOUD);
    } else {
        executeSendDisconnectDeviceFailed(device, Requester::CLOUD);
    }
}

void Bluetooth::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    removeDirective(info);
}

void Bluetooth::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

void Bluetooth::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void Bluetooth::sendExceptionEncountered(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const std::string& message,
    avsCommon::avs::ExceptionErrorType type) {
    m_exceptionEncounteredSender->sendExceptionEncountered(info->directive->getUnparsedDirective(), type, message);
    if (info && info->result) {
        info->result->setFailed(message);
    }
    removeDirective(info);
}

bool Bluetooth::executeSetScanMode(bool scanning) {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_deviceManager->getHostController()) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullHostController"));
        return false;
    }

    auto future =
        scanning ? m_deviceManager->getHostController()->startScan() : m_deviceManager->getHostController()->stopScan();
    if (!future.valid()) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidFuture"));
        return false;
    }

    bool success = future.get();
    if (success) {
        // If we're scanning, there will be more devices. If we're not, then there won't be.
        executeSendScanDevicesUpdated(m_deviceManager->getDiscoveredDevices(), scanning);
    } else {
        ACSDK_ERROR(LX("executeSetScanModeFailed").d("scanning", scanning));
        // This event is only sent in response to a failed processing of "ScanDevices" directive.
        if (scanning) {
            executeSendScanDevicesFailed();
        }
    }

    return success;
}

bool Bluetooth::executeSetDiscoverableMode(bool discoverable) {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_deviceManager->getHostController()) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullHostController"));
        return false;
    }

    std::future<bool> future;

    if (discoverable) {
        future = m_deviceManager->getHostController()->enterDiscoverableMode();
    } else {
        future = m_deviceManager->getHostController()->exitDiscoverableMode();
    }

    if (!future.valid()) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidFuture"));
        return false;
    }

    return future.get();
}

bool Bluetooth::executeFunctionOnDevice(
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device,
    std::function<std::future<bool>(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>&)>
        function) {
    if (!device) {
        ACSDK_ERROR(LX("executeFunctionOnDeviceFailed").d("reason", "nullDevice"));
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("mac", device->getMac()));

    return function(device).get();
}

void Bluetooth::onPlaybackStarted(MediaPlayerObserverInterface::SourceId id) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
    m_executor.submit([this] {
        // It means we were pending a pause before the onPlaybackStarted was received.
        if (m_streamingState == StreamingState::PENDING_PAUSED) {
            executeSendStreamingStarted(m_activeDevice);
            if (!m_mediaPlayer->stop(m_sourceId)) {
                ACSDK_ERROR(LX("onPlaybackStartedLambdaFailed").d("reason", "stopFailed"));
                cleanupMediaSource();
            }
            return;
        }
        m_streamingState = StreamingState::ACTIVE;
        if (!m_activeDevice) {
            ACSDK_ERROR(LX(__func__).d("reason", "noActiveDevice"));
        } else {
            executeSendStreamingStarted(m_activeDevice);
        }
    });
}

void Bluetooth::onPlaybackStopped(MediaPlayerObserverInterface::SourceId id) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
    m_executor.submit([this] {
        // Playback has been stopped, cleanup the source.
        cleanupMediaSource();
        if (m_activeDevice) {
            m_streamingState = StreamingState::PAUSED;
            executeSendStreamingEnded(m_activeDevice);
        } else {
            m_streamingState = StreamingState::INACTIVE;
        }
    });
}

void Bluetooth::onPlaybackFinished(MediaPlayerObserverInterface::SourceId id) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
    m_executor.submit([this] {
        m_streamingState = StreamingState::INACTIVE;

        cleanupMediaSource();
        if (m_activeDevice) {
            executeSendStreamingEnded(m_activeDevice);
        }
    });
}

void Bluetooth::onPlaybackError(MediaPlayerObserverInterface::SourceId id, const ErrorType& type, std::string error) {
    ACSDK_DEBUG5(LX(__func__).d("id", id).d("type", type).d("error", error));

    m_executor.submit([this, id] {
        // Do not attempt to stop the media player here. This could result in a infinite loop if the stop fails.
        if (id == m_sourceId) {
            cleanupMediaSource();
        }
    });
}

// Events.
void Bluetooth::executeSendScanDevicesUpdated(
    const std::list<std::shared_ptr<BluetoothDeviceInterface>>& devices,
    bool hasMore) {
    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesArray(rapidjson::kArrayType);
    ACSDK_DEBUG5(LX(__func__).d("count", devices.size()));

    for (const auto& device : devices) {
        ACSDK_DEBUG(LX("foundDevice").d("deviceMac", device->getMac()));
        std::string uuid;
        if (!retrieveUuid(device->getMac(), &uuid)) {
            ACSDK_ERROR(LX("executeSendScanDevicesUpdatedFailed").d("reason", "retrieveUuidFailed"));
            return;
        }

        if (device->isPaired()) {
            ACSDK_DEBUG(LX(__func__)
                            .d("reason", "deviceAlreadyPaired")
                            .d("mac", device->getMac())
                            .d("uuid", uuid)
                            .d("action", "ommitting"));
            continue;
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());

        std::string truncatedMac;
        if (truncate(device->getMac(), &truncatedMac)) {
            deviceJson.AddMember(TRUNCATED_MAC_ADDRESS_KEY, truncatedMac, payload.GetAllocator());
        }

        devicesArray.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DISCOVERED_DEVICES_KEY, devicesArray.GetArray(), payload.GetAllocator());
    payload.AddMember(HAS_MORE_KEY, hasMore, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(SCAN_DEVICES_UPDATED.name, buffer.GetString());
}

void Bluetooth::executeSendScanDevicesFailed() {
    executeQueueEventAndRequestContext(SCAN_DEVICES_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendEnterDiscoverableModeSucceeded() {
    executeQueueEventAndRequestContext(ENTER_DISCOVERABLE_MODE_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendEnterDiscoverableModeFailed() {
    executeQueueEventAndRequestContext(ENTER_DISCOVERABLE_MODE_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendPairDeviceSucceeded(std::shared_ptr<BluetoothDeviceInterface> device) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(PAIR_DEVICE_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendPairDeviceFailed() {
    executeQueueEventAndRequestContext(PAIR_DEVICE_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendUnpairDeviceSucceeded(std::shared_ptr<BluetoothDeviceInterface> device) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(UNPAIR_DEVICE_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendUnpairDeviceFailed() {
    executeQueueEventAndRequestContext(UNPAIR_DEVICE_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendConnectByDeviceIdSucceeded(
    std::shared_ptr<BluetoothDeviceInterface> device,
    Requester requester) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(CONNECT_BY_DEVICE_ID_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendConnectByDeviceIdFailed(
    std::shared_ptr<BluetoothDeviceInterface> device,
    Requester requester) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(CONNECT_BY_DEVICE_ID_FAILED.name, buffer.GetString());
}

void Bluetooth::executeSendConnectByProfileSucceeded(
    std::shared_ptr<BluetoothDeviceInterface> device,
    const std::string& profileName,
    Requester requester) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());
    payload.AddMember(PROFILE_NAME_KEY, profileName, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(CONNECT_BY_PROFILE_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendConnectByProfileFailed(const std::string& profileName, Requester requester) {
    rapidjson::Document payload(rapidjson::kObjectType);

    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());
    payload.AddMember(PROFILE_NAME_KEY, profileName, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(CONNECT_BY_PROFILE_FAILED.name, buffer.GetString());
}

void Bluetooth::executeSendDisconnectDeviceSucceeded(
    std::shared_ptr<BluetoothDeviceInterface> device,
    Requester requester) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(DISCONNECT_DEVICE_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendDisconnectDeviceFailed(
    std::shared_ptr<BluetoothDeviceInterface> device,
    Requester requester) {
    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(DISCONNECT_DEVICE_FAILED.name, buffer.GetString());
}

void Bluetooth::executeSendMediaControlPlaySucceeded() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_PLAY_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlPlayFailed() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_PLAY_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlStopSucceeded() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_STOP_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlStopFailed() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_STOP_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlNextSucceeded() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_NEXT_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlNextFailed() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_NEXT_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlPreviousSucceeded() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_PREVIOUS_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendMediaControlPreviousFailed() {
    executeQueueEventAndRequestContext(MEDIA_CONTROL_PREVIOUS_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendStreamingStarted(std::shared_ptr<BluetoothDeviceInterface> device) {
    rapidjson::Document payload(rapidjson::kObjectType);
    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(STREAMING_STARTED.name, buffer.GetString());
}

void Bluetooth::executeSendStreamingEnded(std::shared_ptr<BluetoothDeviceInterface> device) {
    rapidjson::Document payload(rapidjson::kObjectType);
    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(STREAMING_ENDED.name, buffer.GetString());
}

/*
 * TODO ACSDK-1402: Create an adapter object between SAS and AttachmentReader to encapsulate logic.
 * The BTCA should not be responsible for this conversion.
 */
void Bluetooth::onFormattedAudioStreamAdapterData(AudioFormat audioFormat, const unsigned char* buffer, size_t size) {
    avsCommon::avs::attachment::AttachmentWriter::WriteStatus writeStatus;
    /*
     * We may ignore the write status since in case of possible overrun (or any other kind of errors)
     * we may safely drop the packet.
     */
    if (m_mediaAttachment) {
        m_mediaAttachmentWriter->write(buffer, size, &writeStatus);
    }
}

/*
 * TODO ACSDK-1402: Create an adapter object between SAS and AttachmentReader to encapsulate logic.
 * The BTCA should not be responsible for this conversion.
 */
void Bluetooth::setCurrentStream(std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> stream) {
    ACSDK_DEBUG5(LX(__func__));

    if (stream == m_mediaStream) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "sameStream"));
        return;
    }

    if (m_mediaStream) {
        m_mediaStream->setListener(nullptr);
    }

    m_mediaStream = stream;

    if (m_mediaStream) {
        m_mediaAttachment = std::make_shared<avsCommon::avs::attachment::InProcessAttachment>("Bluetooth");

        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader =
            m_mediaAttachment->createReader(avsCommon::utils::sds::ReaderPolicy::NONBLOCKING);

        m_mediaAttachmentWriter = m_mediaAttachment->createWriter(avsCommon::utils::sds::WriterPolicy::ALL_OR_NOTHING);

        m_mediaStream->setListener(shared_from_this());

        auto audioFormat = m_mediaStream->getAudioFormat();
        m_sourceId = m_mediaPlayer->setSource(attachmentReader, &audioFormat);
        if (MediaPlayerInterface::ERROR == m_sourceId) {
            ACSDK_ERROR(LX(__func__).d("reason", "setSourceFailed"));
            m_mediaAttachment.reset();
            m_mediaAttachmentWriter.reset();
            m_mediaStream.reset();
        }
    }
}

void Bluetooth::addObserver(std::shared_ptr<ObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.submit([this, observer]() { m_observers.insert(observer); });
}

void Bluetooth::removeObserver(std::shared_ptr<ObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_executor.submit([this, observer]() { m_observers.erase(observer); });
}

Bluetooth::ObserverInterface::DeviceAttributes Bluetooth::generateDeviceAttributes(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) {
    BluetoothDeviceObserverInterface::DeviceAttributes deviceAttributes;

    deviceAttributes.name = device->getFriendlyName();
    for (const auto& supportedServices : device->getSupportedServices()) {
        deviceAttributes.supportedServices.insert(supportedServices->getName());
    }

    return deviceAttributes;
}

// Conceptually these are actions that are initiated by the peer device.
void Bluetooth::onEventFired(const avsCommon::utils::bluetooth::BluetoothEvent& event) {
    ACSDK_DEBUG5(LX(__func__));
    switch (event.getType()) {
        case avsCommon::utils::bluetooth::BluetoothEventType::DEVICE_DISCOVERED: {
            auto device = event.getDevice();
            if (!device) {
                ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
                return;
            }

            ACSDK_INFO(LX(__func__)
                           .d("reason", "DEVICE_DISCOVERED")
                           .d("deviceName", device->getFriendlyName())
                           .d("mac", device->getMac()));
            m_executor.submit([this] { executeSendScanDevicesUpdated(m_deviceManager->getDiscoveredDevices(), true); });
            break;
        }
        case avsCommon::utils::bluetooth::BluetoothEventType::DEVICE_STATE_CHANGED: {
            auto device = event.getDevice();

            if (!device) {
                ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
                return;
            }

            ACSDK_INFO(LX(__func__)
                           .d("event", "DEVICE_STATE_CHANGED")
                           .d("deviceName", device->getFriendlyName())
                           .d("mac", device->getMac())
                           .d("state", event.getDeviceState()));

            switch (event.getDeviceState()) {
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::FOUND:
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE:
                    break;
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::PAIRED: {
                    m_executor.submit([this, device] {
                        if (m_lastPairMac != device->getMac()) {
                            /*
                             * Send one of these so we remove the freshly paired device
                             * from the "Available Devices" page.
                             */
                            executeSendScanDevicesUpdated(m_deviceManager->getDiscoveredDevices(), true);
                            executeSendPairDeviceSucceeded(device);
                        }
                    });
                    break;
                }
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::DISCONNECTED:
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::UNPAIRED: {
                    m_executor.submit([this, device] {
                        if (device == m_activeDevice) {
                            executeOnDeviceDisconnect(Requester::DEVICE);
                        }
                    });
                    break;
                }
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::CONNECTED: {
                    m_executor.submit([this, device] {
                        if (!supportsAvsProfile(device, AVS_A2DP)) {
                            /*
                             * This device does not support A2DP. We will attempt to disconnect,
                             * AVS won't be made aware of it, and if unsuccessful, it is up to the
                             * user/client to disconnect.
                             */
                            ACSDK_WARN(LX("deviceConnected")
                                           .d("reason", "deviceDoesNotSupportA2DP")
                                           .m("Please disconnect device"));

                            if (device->disconnect().get()) {
                                executeOnDeviceDisconnect(Requester::DEVICE);
                            } else {
                                ACSDK_ERROR(LX("deviceConnected")
                                                .d("reason", "disconnectInvalidDeviceFailed")
                                                .d("mac", device->getMac())
                                                .d("name", device->getFriendlyName())
                                                .m("Please disconnect device"));
                            }

                            return;
                        }
                        /*
                         * Otherwise set the device as the new active device. We don't need to call connect()
                         * again because the device is already connected from the Bluetooth stack's perspective.
                         */
                        else if (device != m_activeDevice) {
                            executeOnDeviceConnect(device);
                            /*
                             * Default to sending a ConnectByDeviceId event since this wasn't a result of a profile
                             * specific connection.
                             */
                            executeSendConnectByDeviceIdSucceeded(device, Requester::DEVICE);
                        }
                    });
                    break;
                }
            }
            break;
        }
        case avsCommon::utils::bluetooth::BluetoothEventType::STREAMING_STATE_CHANGED: {
            if (event.getDevice() != m_activeDevice) {
                ACSDK_ERROR(LX(__func__)
                                .d("reason", "mismatchedDevices")
                                .d("eventDevice", event.getDevice() ? event.getDevice()->getMac() : "null")
                                .d("activeDevice", m_activeDevice ? m_activeDevice->getMac() : "null"));
                break;
            }

            if (!event.getA2DPRole()) {
                ACSDK_ERROR(LX(__func__).d("reason", "nullRole"));
                break;
            }

            if (A2DPRole::SINK == *event.getA2DPRole()) {
                if (MediaStreamingState::ACTIVE == event.getMediaStreamingState()) {
                    ACSDK_DEBUG5(LX(__func__).m("Streaming is active"));

                    m_executor.submit([this] {
                        if (m_streamingState == StreamingState::INACTIVE ||
                            m_streamingState == StreamingState::PAUSED ||
                            m_streamingState == StreamingState::PENDING_PAUSED) {
                            // Obtain Focus.
                            if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), ACTIVITY_ID)) {
                                ACSDK_ERROR(LX(__func__).d("reason", "acquireChannelFailed"));
                            }
                        }
                    });
                    /*
                     * TODO: Using MediaStreamingState signal as a proxy for playback state. There is latency with this.
                     * We should be observing a separate playback based signal instead for these decisions.
                     */
                } else if (MediaStreamingState::IDLE == event.getMediaStreamingState()) {
                    m_executor.submit([this] {
                        if (FocusState::FOREGROUND == m_focusState) {
                            m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
                        }
                    });
                }
            } else if (A2DPRole::SOURCE == *event.getA2DPRole()) {
                // Ignore the PENDING state and only act on ACTIVE.
                if (MediaStreamingState::ACTIVE == event.getMediaStreamingState()) {
                    m_executor.submit([this] {
                        if (StreamingState::ACTIVE != m_streamingState) {
                            m_streamingState = StreamingState::ACTIVE;
                            executeSendStreamingStarted(m_activeDevice);
                        }
                    });
                } else if (MediaStreamingState::IDLE == event.getMediaStreamingState()) {
                    m_executor.submit([this] {
                        if (StreamingState::ACTIVE == m_streamingState) {
                            m_streamingState = StreamingState::PAUSED;
                            executeSendStreamingEnded(m_activeDevice);
                        }
                    });
                }
            }
            break;
        }
        default:
            ACSDK_ERROR(LX("onEventFired").d("reason", "unexpectedEventType"));
            break;
    }
}

}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
