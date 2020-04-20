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

#include <chrono>

#include <AVSCommon/AVS/ContentType.h>
#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSinkInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/HFPInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/HIDInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/SPPInterface.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/MacAddressString.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Bluetooth/BasicDeviceConnectionRule.h"
#include "Bluetooth/Bluetooth.h"

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
using MediaPlayerState = avsCommon::utils::mediaPlayer::MediaPlayerState;

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

/// The @c ScanDevicesReport event.
static const NamespaceAndName SCAN_DEVICES_REPORT{NAMESPACE, "ScanDevicesReport"};

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

/// The @c PairDevicesSucceeded event.
static const NamespaceAndName PAIR_DEVICES_SUCCEEDED{NAMESPACE, "PairDevicesSucceeded"};

/// The @c PairDevicesFailed event.
static const NamespaceAndName PAIR_DEVICES_FAILED{NAMESPACE, "PairDevicesFailed"};

/// The @c PairDevices directive identifier.
static const NamespaceAndName PAIR_DEVICES{NAMESPACE, "PairDevices"};

/// The @c UnpairDevices directive identifier.
static const NamespaceAndName UNPAIR_DEVICES{NAMESPACE, "UnpairDevices"};

/// The @c UnpairDevicesSucceeded event.
static const NamespaceAndName UNPAIR_DEVICES_SUCCEEDED{NAMESPACE, "UnpairDevicesSucceeded"};

/// The @c UnpairDevicesFailed event.
static const NamespaceAndName UNPAIR_DEVICES_FAILED{NAMESPACE, "UnpairDevicesFailed"};

/// The @c SetDeviceCategories directive identifier
static const NamespaceAndName SET_DEVICE_CATEGORIES{NAMESPACE, "SetDeviceCategories"};

/// The @c SetDeviceCategoriesSucceeded event.
static const NamespaceAndName SET_DEVICE_CATEGORIES_SUCCEEDED{NAMESPACE, "SetDeviceCategoriesSucceeded"};

/// The @c SetDeviceCategoriesFailed event.
static const NamespaceAndName SET_DEVICE_CATEGORIES_FAILED{NAMESPACE, "SetDeviceCategoriesFailed"};

/// The @c ConnectByDeviceIds directive identifier.
static const NamespaceAndName CONNECT_BY_DEVICE_IDS{NAMESPACE, "ConnectByDeviceIds"};

/// The @c ConnectByDeviceIdsSucceeded event.
static const NamespaceAndName CONNECT_BY_DEVICE_IDS_SUCCEEDED{NAMESPACE, "ConnectByDeviceIdsSucceeded"};

/// The @c ConnectByDeviceIdsFailed event.
static const NamespaceAndName CONNECT_BY_DEVICE_IDS_FAILED{NAMESPACE, "ConnectByDeviceIdsFailed"};

/// The @c ConnectByProfile directive identifier.
static const NamespaceAndName CONNECT_BY_PROFILE{NAMESPACE, "ConnectByProfile"};

/// The @c ConnectByProfileSucceeded event.
static const NamespaceAndName CONNECT_BY_PROFILE_SUCCEEDED{NAMESPACE, "ConnectByProfileSucceeded"};

/// The @c ConnectByProfileFailed event.
static const NamespaceAndName CONNECT_BY_PROFILE_FAILED{NAMESPACE, "ConnectByProfileFailed"};

/// The @c DisconnectDevices directive identifier.
static const NamespaceAndName DISCONNECT_DEVICES{NAMESPACE, "DisconnectDevices"};

/// The @c DisconnectDevicesSucceeded event.
static const NamespaceAndName DISCONNECT_DEVICES_SUCCEEDED{NAMESPACE, "DisconnectDevicesSucceeded"};

/// The @c DisconnectDevicesFailed event.
static const NamespaceAndName DISCONNECT_DEVICES_FAILED{NAMESPACE, "DisconnectDevicesFailed"};

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
static const std::string CHANNEL_NAME = FocusManagerInterface::CONTENT_CHANNEL_NAME;

/// Activity ID for use by FocusManager
static const std::string ACTIVITY_ID = "Bluetooth";

/// Max number of paired devices connect by profile should atempt connect to
static const unsigned int MAX_CONNECT_BY_PROFILE_COUNT = 2;

/// A delay to deal with devices that can't process consecutive AVRCP commands.
static const std::chrono::milliseconds CMD_DELAY{100};

/**
 * We get the stream faster than a peer device can process an AVRCP command (for example NEXT/PAUSE).
 * Wait a bit before we get the stream so we don't get any buffers from before the command was processed.
 */
static const std::chrono::milliseconds INITIALIZE_SOURCE_DELAY{1000};

/**
 * The maximum about of time to block on a future from the Bluetooth implementation
 * before it should be rejected.
 */
static const std::chrono::seconds DEFAULT_FUTURE_TIMEOUT{45};

/**
 * The amount of time that the Bluetooth CA can hold onto the foreground channel and keep another CA in the background.
 * If the time that the Bluetooth CA is in the foreground exceeds this value then other CAs will completely lose focus.
 * If the Bluetooth CA loses focus within that time frame then the other CA which had focus will regain it and resume
 * normal operation.
 */
static const std::chrono::minutes TRANSIENT_FOCUS_DURATION{2};

/*
 * The following are keys used for AVS Directives and Events.
 */
/// A key for the alexa (host) device.
static const char ALEXA_DEVICE_NAME_KEY[] = "alexaDevice";

/// A key for a device.
static const char DEVICE_KEY[] = "device";

/// A key for a devices.
static const char DEVICES_KEY[] = "devices";

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

/// Identifying an array of profiles.
static const char PROFILES_KEY[] = "profiles";

/// Identifying a profile name.
static const char PROFILE_NAME_KEY[] = "profileName";

/// Identifying the requester of the operation.
static const char REQUESTER_KEY[] = "requester";

/// A key indicating streaming status.
static const char STREAMING_KEY[] = "streaming";

/// A key indicating state.
static const char STATE_KEY[] = "state";

/// Indicates the supported profiles.
static const char SUPPORTED_PROFILES_KEY[] = "supportedProfiles";

/// The truncated mac address.
static const char TRUNCATED_MAC_ADDRESS_KEY[] = "truncatedMacAddress";

/// The uuid generated to identify a device.
static const char UNIQUE_DEVICE_ID_KEY[] = "uniqueDeviceId";

/// The metadata.
static const char METADATA_KEY[] = "metadata";

/// The Vendor Id.
static const char VENDOR_ID_KEY[] = "vendorId";

/// The Product Id.
static const char PRODUCT_ID_KEY[] = "productId";

/// The Class of Device.
static const char CLASS_OF_DEVICE_KEY[] = "classOfDevice";

/// The Vendor Device SigID.
static const char VENDOR_DEVICE_SIG_ID_KEY[] = "vendorDeviceSigId";

/// The Vendor Device ID.
static const char VENDOR_DEVICE_ID_KEY[] = "vendorDeviceId";

/// Identifying a device category.
static const char DEVICE_CATEGORY_KEY[] = "deviceCategory";

/// Identifying the connection status of a device.
static const char CONNECTION_STATE_KEY[] = "connectionState";

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

/// HFP.
static const std::string AVS_HFP = "HFP";

/// HID.
static const std::string AVS_HID = "HID";

/// SPP.
static const std::string AVS_SPP = "SPP";

/// A mapping of Bluetooth service UUIDs to AVS profile identifiers.
// clang-format off
static const std::map<std::string, std::string> AVS_PROFILE_MAP{
    {std::string(A2DPSourceInterface::UUID), AVS_A2DP_SOURCE},
    {std::string(A2DPSinkInterface::UUID), AVS_A2DP_SINK},
    {std::string(AVRCPTargetInterface::UUID), AVS_AVRCP},
    {std::string(HFPInterface::UUID), AVS_HFP},
    {std::string(HIDInterface::UUID), AVS_HID},
    {std::string(SPPInterface::UUID), AVS_SPP}};
// clang-format on

/**
 * A mapping of DeviceCategory to its dependent service UUIDs.
 * List all necessary profiles needed for a device with the following DeviceCategory:
 * 1)DeviceCategory::REMOTE_CONTROL
 * 2)DeviceCategory::GADGET
 * All other categories are already enforced to have correct dependent profiles by @c BasicDeviceConnectionRule.
 */
static const std::map<DeviceCategory, std::unordered_set<std::string>> DEVICECATEGORY_PROFILES_MAP{
    {DeviceCategory::REMOTE_CONTROL, {std::string(SPPInterface::UUID), std::string(HIDInterface::UUID)}},
    {DeviceCategory::GADGET, {std::string(SPPInterface::UUID), std::string(HIDInterface::UUID)}}};

/// Bluetooth capability constants
/// Bluetooth interface type
static const std::string BLUETOOTH_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// Bluetooth interface name
static const std::string BLUETOOTH_CAPABILITY_INTERFACE_NAME = "Bluetooth";

/// Bluetooth interface version. Version 1.0 works with StreamingStarted and StreamingEnded.
static const std::string BLUETOOTH_CAPABILITY_INTERFACE_VERSION = "2.0";

/// Bluetooth configurations value.
static const std::string BLUETOOTH_CAPABILITY_CONFIGURATION_VALUE = R"(
{
    "profiles": [
        "AVRCP",
        "A2DP_SINK",
        "A2DP_SOURCE"
    ]
}
)";

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
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, BLUETOOTH_CAPABILITY_CONFIGURATION_VALUE});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

/**
 * Utility function to truncate a MAC address.
 *
 * @param mac The mac address.
 * @param defaultString The default truncated mac address to return if the mac address is invalid.
 * @return The truncated mac address.
 */
static std::string truncateWithDefault(const std::string& mac, const std::string& defaultString = "XX:XX:XX:XX:XX:XX") {
    std::unique_ptr<MacAddressString> macAddressString = MacAddressString::create(mac);
    if (macAddressString) {
        return macAddressString->getTruncatedString();
    } else {
        return defaultString;
    }
}

/**
 * Utility function to truncate a Bluetooth device friendly name.
 *
 * @param friendlyName The Bluetooth device friendly name.
 * @return The truncated friendly name.
 */
static std::string truncateFriendlyName(const std::string& friendlyName) {
    return truncateWithDefault(friendlyName, friendlyName);
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
        auto avsProfileIt = AVS_PROFILE_MAP.find(sdpRecord->getUuid());
        if (avsProfileIt != AVS_PROFILE_MAP.end()) {
            if (avsProfileIt->second == avsProfileName) {
                return true;
            } else if (avsProfileIt->second == AVS_A2DP_SINK && avsProfileName == AVS_A2DP) {
                return true;
            } else if (avsProfileIt->second == AVS_A2DP_SOURCE && avsProfileName == AVS_A2DP) {
                return true;
            }
        }
    }
    ACSDK_DEBUG5(LX(__func__)
                     .d("reason", "profileNotSupported")
                     .d("deviceMac", truncateWithDefault(device->getMac()))
                     .d("avsProfile", avsProfileName));

    return false;
}

/**
 * Wait for a a future function to be resolved.
 *
 * @param future The future
 * @param description The desription of the future function.
 * @param timeout The timeout given to the future.
 * @return A bool indicating the future success.
 */
static bool waitOnFuture(
    std::future<bool> future,
    std::string description = "",
    std::chrono::milliseconds timeout = DEFAULT_FUTURE_TIMEOUT) {
    if (future.valid()) {
        std::future_status status = future.wait_for(timeout);
        switch (status) {
            case std::future_status::timeout:
                ACSDK_ERROR(LX(__func__).d("description", description).m("Timeout waiting on a future."));
                break;
            case std::future_status::deferred:
                ACSDK_WARN(LX(__func__).d("description", description).m("Blocking on a deferred future."));
                /* FALL THROUGH */
            case std::future_status::ready:
                return future.get();
        }
    } else {
        ACSDK_ERROR(LX(__func__).d("description", description).m("Cannot wait on invalid future."));
    }
    return false;
}

/**
 * Utility function to squash @c StreamingState into the streaming states
 * that AVS has defined.
 *
 * @param state The @c StreamingState.
 * @return A string representing the streaming state.
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

/**
 * Utility function to evaluate whether connection rules are valid.
 * Device categories defined in the @c BasicDeviceConnectionRule are enabled by the Bluetoooth CapabilityAgent by
 * default and are not allowed to be overwritten.
 *
 * @param enabledConnectionRules a set of device connection rules defined by customers.
 * @return A bool indicating success.
 */
static bool validateDeviceConnectionRules(
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        enabledConnectionRules) {
    std::set<DeviceCategory> enabledDeviceCategories;
    if (!enabledConnectionRules.empty()) {
        for (const auto& connectionRule : enabledConnectionRules) {
            std::set<DeviceCategory> categories = connectionRule->getDeviceCategories();
            std::set<std::string> dependentProfiles = connectionRule->getDependentProfiles();

            for (const auto& category : categories) {
                // Verify if the device category is already defined in the other device connection rule.
                if (enabledDeviceCategories.find(category) != enabledDeviceCategories.end()) {
                    ACSDK_ERROR(LX(__func__).d("reason", "RedefinedDeviceCategory").d("category", category));
                    return false;
                }
                enabledDeviceCategories.insert(category);

                // Verify if dependent profiles defined in the rule are valid.
                if (DEVICECATEGORY_PROFILES_MAP.find(category) != DEVICECATEGORY_PROFILES_MAP.end()) {
                    std::unordered_set<std::string> requiredProfiles = DEVICECATEGORY_PROFILES_MAP.at(category);
                    for (const auto& requiredProfile : requiredProfiles) {
                        if (dependentProfiles.count(requiredProfile) == 0) {
                            ACSDK_ERROR(LX(__func__).d("reason", "RequiredProfileNotAdded").d("uuid", requiredProfile));
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}

/**
 * Utility function to squash @c DeviceState into the connection state that AVS has defined.
 *
 * @param state The @c DeviceState
 * @return A string representing the connection state.
 */
static std::string convertToAVSConnectedState(const DeviceState& state) {
    switch (state) {
        case DeviceState::CONNECTED:
            return "CONNECTED";
        case DeviceState::FOUND:
        case DeviceState::IDLE:
        case DeviceState::PAIRED:
        case DeviceState::UNPAIRED:
        case DeviceState::DISCONNECTED:
            return "DISCONNECTED";
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
    std::unordered_set<std::shared_ptr<bluetooth::BluetoothDeviceConnectionRuleInterface>> enabledConnectionRules,
    std::shared_ptr<ChannelVolumeInterface> bluetoothChannelVolumeInterface,
    std::shared_ptr<BluetoothMediaInputTransformer> mediaInputTransformer) {
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
    } else if (!validateDeviceConnectionRules(enabledConnectionRules)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidBluetoothDeviceConnectionRules"));
    } else if (!bluetoothChannelVolumeInterface) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullBluetoothChannelVolumeInterface"));
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
            enabledConnectionRules,
            bluetoothChannelVolumeInterface,
            mediaInputTransformer));

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

    m_mediaPlayer->addObserver(shared_from_this());

    syncWithDeviceManager();
    // Calling outside executor context since init() is implicitly thread-safe.
    executeUpdateContext();

    // Start receiving events after manually syncing BTCA state with adapter state.
    m_eventBus->addListener(
        {avsCommon::utils::bluetooth::BluetoothEventType::DEVICE_DISCOVERED,
         avsCommon::utils::bluetooth::BluetoothEventType::DEVICE_STATE_CHANGED,
         avsCommon::utils::bluetooth::BluetoothEventType::STREAMING_STATE_CHANGED,
         avsCommon::utils::bluetooth::BluetoothEventType::SCANNING_STATE_CHANGED,
         avsCommon::utils::bluetooth::BluetoothEventType::TOGGLE_A2DP_PROFILE_STATE_CHANGED},
        shared_from_this());

    return true;
}

void Bluetooth::syncWithDeviceManager() {
    for (const auto& device : m_deviceManager->getDiscoveredDevices()) {
        if (!device->isConnected()) {
            ACSDK_DEBUG9(LX(__func__).d("reason", "deviceNotConnected").m("Excluding"));
            continue;
        }
        executeOnDeviceConnect(device, false);
    }

    if (m_activeA2DPDevice && m_activeA2DPDevice->getService(A2DPSourceInterface::UUID)) {
        auto streamingState = m_activeA2DPDevice->getStreamingState();
        switch (streamingState) {
            case MediaStreamingState::IDLE:
                m_streamingState = StreamingState::PAUSED;
                break;
            case MediaStreamingState::PENDING:
                m_streamingState = StreamingState::INACTIVE;
                break;
            case MediaStreamingState::ACTIVE:
                m_streamingState = StreamingState::ACTIVE;
                m_executor.submit([this] { executeAcquireFocus(__func__); });
                break;
        }
    }
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
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
        enabledConnectionRules,
    std::shared_ptr<ChannelVolumeInterface> bluetoothChannelVolumeInterface,
    std::shared_ptr<BluetoothMediaInputTransformer> mediaInputTransformer) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"Bluetooth"},
        CustomerDataHandler{customerDataManager},
        m_messageSender{messageSender},
        m_contextManager{contextManager},
        m_focusManager{focusManager},
        m_streamingState{StreamingState::INACTIVE},
        m_focusTransitionState{FocusTransitionState::INTERNAL},
        m_scanningTransitionState{ScanningTransitionState::INACTIVE},
        m_focusState{FocusState::NONE},
        m_sourceId{MediaPlayerInterface::ERROR},
        m_deviceManager{std::move(deviceManager)},
        m_mediaPlayer{mediaPlayer},
        m_db{bluetoothStorage},
        m_eventBus{eventBus},
        m_mediaInputTransformer{mediaInputTransformer},
        m_mediaStream{nullptr},
        m_bluetoothChannelVolumeInterface{bluetoothChannelVolumeInterface} {
    m_capabilityConfigurations.insert(getBluetoothCapabilityConfiguration());

    for (const auto& connectionRule : enabledConnectionRules) {
        for (const auto& category : connectionRule->getDeviceCategories()) {
            m_enabledConnectionRules[category] = connectionRule;
        }
    }
}

DirectiveHandlerConfiguration Bluetooth::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));

    DirectiveHandlerConfiguration configuration;
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    configuration[SCAN_DEVICES] = neitherNonBlockingPolicy;
    configuration[ENTER_DISCOVERABLE_MODE] = neitherNonBlockingPolicy;
    configuration[EXIT_DISCOVERABLE_MODE] = neitherNonBlockingPolicy;
    configuration[PAIR_DEVICES] = neitherNonBlockingPolicy;
    configuration[UNPAIR_DEVICES] = neitherNonBlockingPolicy;
    configuration[SET_DEVICE_CATEGORIES] = neitherNonBlockingPolicy;
    configuration[CONNECT_BY_DEVICE_IDS] = neitherNonBlockingPolicy;
    configuration[CONNECT_BY_PROFILE] = neitherNonBlockingPolicy;
    configuration[DISCONNECT_DEVICES] = neitherNonBlockingPolicy;
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
    m_mediaPlayer->removeObserver(shared_from_this());
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
        auto stopScanFuture = hostController->stopScan();
        waitOnFuture(std::move(stopScanFuture), "Stop bluetooth scanning");
        auto exitDiscoverableFuture = hostController->exitDiscoverableMode();
        waitOnFuture(std::move(exitDiscoverableFuture), "Exit discoverable mode");
    }

    m_disabledA2DPDevice.reset();
    m_activeA2DPDevice.reset();
    m_restrictedDevices.clear();

    // Finalizing the implementation

    m_deviceManager.reset();
    m_eventBus.reset();

    m_observers.clear();
    m_bluetoothEventStates.clear();
    m_connectedDevices.clear();
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
            auto stopScanFuture = hostController->stopScan();
            waitOnFuture(std::move(stopScanFuture), "Stop bluetooth scanning");
            ACSDK_DEBUG5(LX("clearData").d("action", "stoppedScanning"));
        }

        if (hostController->isDiscoverable()) {
            auto exitDiscoverableFuture = hostController->exitDiscoverableMode();
            waitOnFuture(std::move(exitDiscoverableFuture), "Exit discoverable mode");
            ACSDK_DEBUG5(LX("clearData").d("action", "disabledDiscoverable"));
        }

        // Unpair all devices.
        for (auto& device : m_deviceManager->getDiscoveredDevices()) {
            if (device->isPaired()) {
                auto unpairFuture = device->unpair();
                waitOnFuture(std::move(unpairFuture), "Unpair device");
                ACSDK_DEBUG5(LX("clearData")
                                 .d("action", "unpairDevice")
                                 .d("device", truncateFriendlyName(device->getFriendlyName())));
            }
        }

        m_db->clear();
    });
}

void Bluetooth::executeInitializeMediaSource() {
    ACSDK_DEBUG5(LX(__func__));

    std::this_thread::sleep_for(INITIALIZE_SOURCE_DELAY);
    auto a2dpSource = getService<A2DPSourceInterface>(m_activeA2DPDevice);
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
        std::string truncatedMac = truncateWithDefault(mac);
        ACSDK_ERROR(LX(__func__).d("reason", "insertingToDBFailed").d("mac", truncatedMac).d("uuid", *uuid));
        return false;
    }

    return true;
}

bool Bluetooth::retrieveDeviceCategoryByUuid(const std::string& uuid, DeviceCategory* category) {
    ACSDK_DEBUG5(LX(__func__));

    if (!category) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDeviceCategory"));
        return false;
    }

    std::string categoryString;
    if (m_db->getCategory(uuid, &categoryString)) {
        *category = stringToDeviceCategory(categoryString);
        return true;
    }

    return false;
}

std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface> Bluetooth::
    retrieveConnectionRuleByUuid(const std::string& uuid) {
    ACSDK_DEBUG5(LX(__func__));

    DeviceCategory category = DeviceCategory::UNKNOWN;
    if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceCategoryFailed"));
        return nullptr;
    }

    auto ruleIt = m_enabledConnectionRules.find(category);
    if (ruleIt != m_enabledConnectionRules.end()) {
        return ruleIt->second;
    }

    return nullptr;
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
        std::string truncatedMac = truncateWithDefault(mac);
        ACSDK_ERROR(LX("retrieveDeviceByUuidFailed").d("reason", "couldNotFindDevice").d("mac", truncatedMac));
        return nullptr;
    }

    return device;
}

std::shared_ptr<BluetoothDeviceInterface> Bluetooth::retrieveDeviceByMac(const std::string& mac) {
    std::string truncatedMac = truncateWithDefault(mac);
    ACSDK_DEBUG5(LX(__func__).d("mac", truncatedMac));
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

    // Construct pairedDevices.
    rapidjson::Value pairedDevices(rapidjson::kArrayType);

    std::unordered_map<std::string, std::string> macToUuids;
    std::unordered_map<std::string, std::string> uuidToCategory;
    if (!m_db->getMacToUuid(&macToUuids)) {
        ACSDK_ERROR(LX(__func__).d("reason", "databaseQueryFailed"));
        macToUuids.clear();
    }

    if (!m_db->getUuidToCategory(&uuidToCategory)) {
        ACSDK_ERROR(LX(__func__).d("reason", "databaseQueryFailed"));
        uuidToCategory.clear();
    }

    for (const auto& device : m_deviceManager->getDiscoveredDevices()) {
        ACSDK_DEBUG9(LX(__func__)
                         .d("friendlyName", truncateFriendlyName(device->getFriendlyName()))
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

        // Retrieve the category from our map.
        std::string category;
        const auto categoryIt = uuidToCategory.find(uuid);
        if (uuidToCategory.end() != categoryIt) {
            category = categoryIt->second;
        } else {
            // Category not found in the map, default to UNKNOWN.
            category = deviceCategoryToString(DeviceCategory::UNKNOWN);
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(DEVICE_CATEGORY_KEY, category, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());

        rapidjson::Value supportedProfiles(rapidjson::kArrayType);
        // If this fails, add an empty array.
        if (!extractAvsProfiles(device, payload.GetAllocator(), &supportedProfiles)) {
            supportedProfiles = rapidjson::Value(rapidjson::kArrayType);
        }
        deviceJson.AddMember(SUPPORTED_PROFILES_KEY, supportedProfiles.GetArray(), payload.GetAllocator());
        deviceJson.AddMember(
            CONNECTION_STATE_KEY, convertToAVSConnectedState(device->getDeviceState()), payload.GetAllocator());
        pairedDevices.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(PAIRED_DEVICES_KEY, pairedDevices, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    ACSDK_DEBUG9(LX(__func__).sensitive("buffer", buffer.GetString()));

    m_contextManager->setState(BLUETOOTH_STATE, buffer.GetString(), StateRefreshPolicy::NEVER);
}

void Bluetooth::executeSendEvent(const std::string& eventName, const std::string& eventPayload) {
    ACSDK_DEBUG5(LX(__func__).d("eventName", eventName));

    auto event = buildJsonEventString(eventName, "", eventPayload, "");

    ACSDK_DEBUG5(LX("onExecuteSendEventLambda").d("event", event.second));
    auto request = std::make_shared<MessageRequest>(event.second);
    m_messageSender->sendMessage(request);
}

bool Bluetooth::extractAvsProfiles(
    std::shared_ptr<BluetoothDeviceInterface> device,
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value* supportedProfiles) {
    if (!device || !supportedProfiles || !supportedProfiles->IsArray()) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidInputParameters"));
        return false;
    }

    for (const auto& sdp : device->getSupportedServices()) {
        auto profileNameIt = AVS_PROFILE_MAP.find(sdp->getUuid());
        if (profileNameIt != AVS_PROFILE_MAP.end()) {
            rapidjson::Value profile(rapidjson::kObjectType);
            profile.AddMember(NAME_KEY, profileNameIt->second, allocator);
            profile.AddMember(VERSION_KEY, sdp->getVersion(), allocator);
            if (A2DPSinkInterface::UUID == profileNameIt->first || A2DPSourceInterface::UUID == profileNameIt->first) {
                rapidjson::Value profileState(rapidjson::kObjectType);
                if (m_activeA2DPDevice && device == m_activeA2DPDevice) {
                    profileState.AddMember(STREAMING_KEY, convertToAVSStreamingState(m_streamingState), allocator);
                } else {
                    profileState.AddMember(
                        STREAMING_KEY, convertToAVSStreamingState(Bluetooth::StreamingState::INACTIVE), allocator);
                }
                profile.AddMember(STATE_KEY, profileState, allocator);
            }
            supportedProfiles->PushBack(profile, allocator);
        }
    }

    return true;
}

void Bluetooth::executeQueueEventAndRequestContext(const std::string& eventName, const std::string& eventPayload) {
    ACSDK_DEBUG5(LX(__func__).d("eventName", eventName));

    m_eventQueue.push(std::make_pair(eventName, eventPayload));
    m_contextManager->getContext(shared_from_this());
}

void Bluetooth::executeDrainQueue() {
    while (!m_cmdQueue.empty()) {
        MediaCommand cmd = m_cmdQueue.front();
        m_cmdQueue.pop_front();

        if (!m_activeA2DPDevice || !m_activeA2DPDevice->getService(AVRCPTargetInterface::UUID)) {
            ACSDK_ERROR(LX(__func__).d("reason", "invalidState"));
            continue;
        }

        auto avrcpTarget = getService<AVRCPTargetInterface>(m_activeA2DPDevice);

        switch (cmd) {
            /*
             * MediaControl Events should only be sent in response to a directive.
             * Don't send events for PLAY or PAUSE because they are never queued
             * as a result of a directive, only as a result of a focus change.
             */
            case MediaCommand::PLAY:
                avrcpTarget->play();
                m_streamingState = StreamingState::PENDING_ACTIVE;
                break;
            case MediaCommand::PAUSE:
                avrcpTarget->pause();
                m_streamingState = StreamingState::PENDING_PAUSED;
                break;
            case MediaCommand::NEXT:
                executeNext(m_activeA2DPDevice);
                break;
            case MediaCommand::PREVIOUS:
                executePrevious(m_activeA2DPDevice);
                break;
            case MediaCommand::PLAY_PAUSE:
                // No-op
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
        executeReleaseFocus(__func__);
    }
}

void Bluetooth::executeEnterForeground() {
    ACSDK_DEBUG5(LX(__func__).d("streamingState", streamingStateToString(m_streamingState)));

    /**
     * If we were previously paused : we need to fall through and resume audio in the AVRCPTarget
     * If we were previously ducked : the delegate will unduck volume and since we are in StreamingState::ACTIVE, we
     * exit early below
     */
    if (!m_bluetoothChannelVolumeInterface->stopDucking()) {
        ACSDK_WARN(LX(__func__).m("Failed To Restore Audio Channel Volume"));
    }

    if (!m_activeA2DPDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "noActiveDevice"));
        executeAbortMediaPlayback();
        return;
    }

    auto avrcpTarget = getService<AVRCPTargetInterface>(m_activeA2DPDevice);

    if (!avrcpTarget) {
        ACSDK_INFO(LX(__func__).d("reason", "avrcpTargetNotSupported"));
    }

    switch (m_streamingState) {
        case StreamingState::ACTIVE:
            executeDrainQueue();
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
            // We push a play when the BT CA has been backgrounded because some devices do not
            // auto start playback next/previous command.
            if (m_focusState == FocusState::BACKGROUND) {
                m_cmdQueue.push_front(MediaCommand::PLAY);
                executeDrainQueue();
            }
            if (m_mediaStream == nullptr) {
                executeInitializeMediaSource();
            }
            if (!m_mediaPlayer->play(m_sourceId)) {
                ACSDK_ERROR(LX(__func__).d("reason", "playFailed").d("sourceId", m_sourceId));
            }
            break;
    }
}

void Bluetooth::executeEnterBackground(MixingBehavior behavior) {
    ACSDK_DEBUG5(LX(__func__).d("streamingState", streamingStateToString(m_streamingState)));
    if (!m_activeA2DPDevice) {
        ACSDK_ERROR(LX(__func__).d("reason", "noActiveDevice"));
        executeAbortMediaPlayback();
        return;
    }

    /**
     * In case of ducking decision : we can exit early (no need to deal with AVRCPTarget)
     * In case of a pausing decision : the delegate shall return pause to indicate that it was not handled
     *                                 in this case the below switch logic will pause music
     */
    if (MixingBehavior::MAY_DUCK == behavior) {
        if (!m_bluetoothChannelVolumeInterface->startDucking()) {
            ACSDK_WARN(LX(__func__).m("Failed to Attenuate Audio Channel Volume"));
            // Fall Through and Pause By Default
        } else {
            // Return Early
            ACSDK_DEBUG4(LX(__func__).d("action", "ducking audio"));
            return;
        }
    }

    auto avrcpTarget = getService<AVRCPTargetInterface>(m_activeA2DPDevice);
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

    /**
     * Restore channel volume when losing focus.
     * Note that this call will be no-op if channel was not attenuated at this point.
     */
    if (!m_bluetoothChannelVolumeInterface->stopDucking()) {
        ACSDK_WARN(LX(__func__).m("Failed To Restore Audio Channel Volume"));
    }

    if (!m_activeA2DPDevice) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "noActiveDevice"));
        executeAbortMediaPlayback();
        return;
    }

    if (FocusTransitionState::EXTERNAL == m_focusTransitionState) {
        auto a2dpSource = getService<A2DPSourceInterface>(m_activeA2DPDevice);
        /**
         * Only disconnect if the remote device is a Bluetooth audio source that is streaming audio and
         * the BT CA is transitioning from FOREGROUND or BACKGROUND to NONE
         */
        if (a2dpSource && m_focusState != FocusState::NONE) {
            if (executeFunctionOnDevice(m_activeA2DPDevice, &BluetoothDeviceInterface::disconnect)) {
                executeOnDeviceDisconnect(m_activeA2DPDevice, Requester::DEVICE);
            } else {
                std::unordered_set<std::string> uuids;
                std::string uuid;
                if (retrieveUuid(m_activeA2DPDevice->getMac(), &uuid)) {
                    uuids.insert(uuid);
                } else {
                    ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
                }
                executeSendDisconnectDevicesFailed(uuids, Requester::DEVICE);
            }
        }
    } else {
        auto avrcpTarget = getService<AVRCPTargetInterface>(m_activeA2DPDevice);
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
}

void Bluetooth::onFocusChanged(FocusState newFocus, MixingBehavior behavior) {
    ACSDK_DEBUG5(LX(__func__).d("current", m_focusState).d("new", newFocus).d("MixingBehavior", behavior));

    m_executor.submit([this, newFocus, behavior] {
        switch (newFocus) {
            case FocusState::FOREGROUND: {
                executeEnterForeground();
                break;
            }
            case FocusState::BACKGROUND: {
                executeEnterBackground(behavior);
                break;
            }
            case FocusState::NONE: {
                executeEnterNone();
                break;
            }
        }

        if (FocusState::NONE == newFocus && FocusTransitionState::PENDING_INTERNAL == m_focusTransitionState) {
            m_focusTransitionState = FocusTransitionState::INTERNAL;
        } else if (FocusState::NONE != newFocus && FocusTransitionState::PENDING_INTERNAL != m_focusTransitionState) {
            m_focusTransitionState = FocusTransitionState::EXTERNAL;
        }

        m_focusState = newFocus;
    });
}

void Bluetooth::onContextAvailable(const std::string& jsonContext) {
    m_executor.submit([this, jsonContext] {
        ACSDK_DEBUG9(LX("onContextAvailableLambda"));

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

std::unordered_set<std::string> Bluetooth::retrieveUuidsFromConnectionPayload(const rapidjson::Document& payload) {
    std::unordered_set<std::string> uuids;
    rapidjson::Value::ConstMemberIterator it;

    if (json::jsonUtils::findNode(payload, DEVICES_KEY, &it) && it->value.IsArray()) {
        for (auto deviceIt = it->value.Begin(); deviceIt != it->value.End(); deviceIt++) {
            std::string uuid;
            if (!json::jsonUtils::retrieveValue(*deviceIt, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                ACSDK_ERROR(LX("retrieveValueFailed").d("reason", "uuidNotFound"));
                continue;
            }
            uuids.insert(uuid);
        }
    }

    return uuids;
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
        } else if (directiveName == PAIR_DEVICES.name) {
            std::unordered_set<std::string> uuids = retrieveUuidsFromConnectionPayload(payload);
            if (!uuids.empty()) {
                /*
                 * AVS expects this sequence of implicit behaviors.
                 * AVS should send individual directives, but we will handle this for now.
                 *
                 * Don't send ScanDeviceReport event to cloud in pairing mode. Otherwise, another
                 * SCAN_DEVICES directive would be sent down to start scan mode again.
                 *
                 * If the device fails to pair, start scan mode again.
                 */
                executeSetScanMode(false, false);
                executeSetDiscoverableMode(false);

                if (!executePairDevices(uuids)) {
                    executeSetScanMode(true, false);
                }
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == UNPAIR_DEVICES.name) {
            std::unordered_set<std::string> uuids = retrieveUuidsFromConnectionPayload(payload);
            if (!uuids.empty()) {
                executeUnpairDevices(uuids);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == SET_DEVICE_CATEGORIES.name) {
            rapidjson::Value::ConstMemberIterator it;

            if (json::jsonUtils::findNode(payload, DEVICES_KEY, &it) && it->value.IsArray()) {
                std::map<std::string, std::string> uuidCategoryMap;
                std::string uuid;
                std::string category;

                for (auto deviceIt = it->value.Begin(); deviceIt != it->value.End(); deviceIt++) {
                    if (!json::jsonUtils::retrieveValue(*deviceIt, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                        ACSDK_ERROR(LX("parsingSetDeviceCategoriesFailed").d("reason", "uuidNotFound"));
                        continue;
                    }
                    if (!json::jsonUtils::retrieveValue(*deviceIt, DEVICE_CATEGORY_KEY, &category)) {
                        ACSDK_ERROR(LX("parsingSetDeviceCategoriesFailed").d("reason", "categoryNotFound"));
                        continue;
                    }
                    uuidCategoryMap.insert({uuid, category});
                }

                executeSetDeviceCategories(uuidCategoryMap);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == CONNECT_BY_DEVICE_IDS.name) {
            std::unordered_set<std::string> uuids = retrieveUuidsFromConnectionPayload(payload);
            if (!uuids.empty()) {
                executeConnectByDeviceIds(uuids);
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
        } else if (directiveName == DISCONNECT_DEVICES.name) {
            std::unordered_set<std::string> uuids = retrieveUuidsFromConnectionPayload(payload);
            if (!uuids.empty()) {
                executeDisconnectDevices(uuids);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == PLAY.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string uuid;

            if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                auto device = retrieveDeviceByUuid(uuid);
                if (!device) {
                    ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
                    executeSendMediaControlPlayFailed(device);
                }
                executePlay(device);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == STOP.name) {
            rapidjson::Value::ConstMemberIterator it;
            std::string uuid;

            if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                auto device = retrieveDeviceByUuid(uuid);
                if (!device) {
                    ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
                    executeSendMediaControlStopFailed(device);
                }
                executeStop(device);
            } else {
                sendExceptionEncountered(
                    info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                return;
            }
        } else if (directiveName == NEXT.name) {
            if (FocusState::FOREGROUND == m_focusState) {
                rapidjson::Value::ConstMemberIterator it;
                std::string uuid;

                if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                    json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                    auto device = retrieveDeviceByUuid(uuid);
                    if (!device) {
                        ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
                        executeSendMediaControlNextFailed(device);
                    }
                    executeNext(device);
                } else {
                    sendExceptionEncountered(
                        info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                    return;
                }
            } else {
                // The queue is drained when we enter the foreground.
                m_cmdQueue.push_back(MediaCommand::NEXT);
            }
        } else if (directiveName == PREVIOUS.name) {
            if (FocusState::FOREGROUND == m_focusState) {
                rapidjson::Value::ConstMemberIterator it;
                std::string uuid;

                if (json::jsonUtils::findNode(payload, DEVICE_KEY, &it) &&
                    json::jsonUtils::retrieveValue(it->value, UNIQUE_DEVICE_ID_KEY, &uuid)) {
                    auto device = retrieveDeviceByUuid(uuid);
                    if (!device) {
                        ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
                        executeSendMediaControlPreviousFailed(device);
                    }
                    executePrevious(device);
                } else {
                    sendExceptionEncountered(
                        info, "uniqueDeviceId not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
                    return;
                }
            } else {
                // The queue is drained when we enter the foreground.
                m_cmdQueue.push_back(MediaCommand::PREVIOUS);
            }
        } else {
            sendExceptionEncountered(info, "Unexpected Directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        executeSetHandlingCompleted(info);
    });
}

void Bluetooth::executePlay(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) {
    ACSDK_DEBUG5(LX(__func__));

    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    auto avrcpTarget = getService<AVRCPTargetInterface>(device);

    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlPlayFailed(device);
        return;
    }

    if (device == m_activeA2DPDevice) {
        // Some applications treat PLAY/PAUSE as a toggle. Don't send if we're already about to play.
        if (StreamingState::ACTIVE == m_streamingState || StreamingState::PENDING_ACTIVE == m_streamingState) {
            executeSendMediaControlPlaySucceeded(device);
            return;
        }

        bool success = true;
        /// This means that we have not yet sent an AVRCP play command yet. Do so.
        if (StreamingState::PAUSED == m_streamingState || StreamingState::INACTIVE == m_streamingState) {
            success = avrcpTarget->play();
        }

        if (success) {
            m_streamingState = StreamingState::PENDING_ACTIVE;
            executeSendMediaControlPlaySucceeded(device);
            executeAcquireFocus(__func__);
        } else {
            executeSendMediaControlPlayFailed(device);
        }
    } else {
        if (avrcpTarget->play()) {
            executeSendMediaControlPlaySucceeded(device);
        } else {
            executeSendMediaControlPlayFailed(device);
        }
    }
}

void Bluetooth::executeStop(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) {
    ACSDK_DEBUG5(LX(__func__));

    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    auto avrcpTarget = getService<AVRCPTargetInterface>(device);

    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlStopFailed(device);
        return;
    }

    if (device == m_activeA2DPDevice) {
        // Some applications treat PLAY/PAUSE as a toggle. Don't send if we're already paused.
        if (StreamingState::PAUSED == m_streamingState || StreamingState::PENDING_PAUSED == m_streamingState) {
            executeSendMediaControlStopSucceeded(device);
            executeReleaseFocus(__func__);
            return;
        }

        bool success = true;
        if (StreamingState::ACTIVE == m_streamingState) {
            success = avrcpTarget->pause();
        }

        if (success) {
            m_streamingState = StreamingState::PENDING_PAUSED;
            executeSendMediaControlStopSucceeded(device);
        } else {
            executeSendMediaControlStopFailed(device);
        }

        // Even if we failed to stop the stream, release the channel so we stop audio playback.
        executeReleaseFocus(__func__);
    } else {
        if (avrcpTarget->pause()) {
            executeSendMediaControlStopSucceeded(device);
        } else {
            executeSendMediaControlStopFailed(device);
        }
    }
}

void Bluetooth::executeNext(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) {
    ACSDK_DEBUG5(LX(__func__));

    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    auto avrcpTarget = getService<AVRCPTargetInterface>(device);

    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlNextFailed(device);
        return;
    }

    if (avrcpTarget->next()) {
        executeSendMediaControlNextSucceeded(device);
    } else {
        executeSendMediaControlNextFailed(device);
    }
}

void Bluetooth::executePrevious(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) {
    ACSDK_DEBUG5(LX(__func__));

    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    auto avrcpTarget = getService<AVRCPTargetInterface>(device);

    if (!avrcpTarget) {
        ACSDK_ERROR(LX(__func__).d("reason", "notSupported"));
        executeSendMediaControlPreviousFailed(device);
        return;
    }

    if (avrcpTarget->previous()) {
        executeSendMediaControlPreviousSucceeded(device);
    } else {
        executeSendMediaControlPreviousFailed(device);
    }
}

bool Bluetooth::executePairDevices(const std::unordered_set<std::string>& uuids) {
    ACSDK_DEBUG5(LX(__func__));

    bool pairingSuccess = true;

    for (const auto& uuid : uuids) {
        auto device = retrieveDeviceByUuid(uuid);
        if (!device) {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
            executeSendPairDevicesFailed({uuid});
            pairingSuccess = false;
            continue;
        }

        DeviceCategory category = DeviceCategory::UNKNOWN;
        if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
            ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceCategoryFailed"));
            pairingSuccess = false;
            continue;
        }

        if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::pair)) {
            executeInsertBluetoothEventState(
                device, DeviceState::PAIRED, Optional<Requester>(), Optional<std::string>());
            /**
             * If pairing is successful, connect the device if itself doesn't handle connection logic.
             */
            auto connectionRule = retrieveConnectionRuleByUuid(uuid);
            if (connectionRule && connectionRule->shouldExplicitlyConnect()) {
                executeConnectByDeviceIds({uuid});
            }
        } else {
            executeSendPairDevicesFailed({uuid});
            pairingSuccess = false;
        }
    }
    return pairingSuccess;
}

bool Bluetooth::executeUnpairDevices(const std::unordered_set<std::string>& uuids) {
    ACSDK_DEBUG5(LX(__func__));

    bool unpairingSuccess = true;

    for (const auto& uuid : uuids) {
        auto device = retrieveDeviceByUuid(uuid);
        if (!device) {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
            executeSendUnpairDevicesFailed({uuid});
            unpairingSuccess = false;
            continue;
        }

        DeviceCategory category = DeviceCategory::UNKNOWN;
        if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
            ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceCategoryFailed"));
            unpairingSuccess = false;
            continue;
        }

        /**
         * If the device is connected, disconnect it before unpairing if itself doesn't handle the disconnection logic.
         */
        auto connectionRule = retrieveConnectionRuleByUuid(uuid);
        if (device->isConnected()) {
            if (connectionRule && connectionRule->shouldExplicitlyDisconnect()) {
                executeDisconnectDevices({uuid});
            }
        }

        if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::unpair)) {
            executeInsertBluetoothEventState(
                device, DeviceState::UNPAIRED, Optional<Requester>(), Optional<std::string>());
        } else {
            executeSendUnpairDevicesFailed({uuid});
            unpairingSuccess = false;
        }
    }
    return unpairingSuccess;
}

std::map<std::string, std::string> Bluetooth::executeSetDeviceCategories(
    const std::map<std::string, std::string>& uuidCategoryMap) {
    ACSDK_DEBUG5(LX(__func__));

    std::map<std::string, std::string> uuidCategorySucceededMap;
    std::map<std::string, std::string> uuidCategoryFailedMap;

    for (const auto& uuidCategory : uuidCategoryMap) {
        if (m_db->updateByCategory(uuidCategory.first, uuidCategory.second)) {
            uuidCategorySucceededMap.insert({uuidCategory.first, uuidCategory.second});
        } else {
            uuidCategoryFailedMap.insert({uuidCategory.first, uuidCategory.second});
        }
    }

    if (!uuidCategorySucceededMap.empty()) {
        executeSetDeviceCategoriesSucceeded(uuidCategorySucceededMap);
    }

    if (!uuidCategoryFailedMap.empty()) {
        executeSetDeviceCategoriesFailed(uuidCategoryFailedMap);
    }

    return uuidCategoryFailedMap;
}

void Bluetooth::executeConnectByDeviceIds(const std::unordered_set<std::string>& uuids) {
    ACSDK_DEBUG5(LX(__func__));

    for (const auto& uuid : uuids) {
        auto device = retrieveDeviceByUuid(uuid);
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>> devices;
        if (!device) {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
            executeSendConnectByDeviceIdsFailed({uuid}, Requester::CLOUD);
            continue;
        }

        devices.insert(device);

        if (device->isConnected()) {
            executeSendConnectByDeviceIdsSucceeded(devices, Requester::CLOUD);
            continue;
        }

        if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::connect)) {
            executeOnDeviceConnect(device, true);
            executeInsertBluetoothEventState(
                device, DeviceState::CONNECTED, Optional<Requester>(Requester::CLOUD), Optional<std::string>());

        } else {
            executeSendConnectByDeviceIdsFailed({uuid}, Requester::CLOUD);
        }
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

    std::list<std::shared_ptr<BluetoothDeviceInterface>> matchedDevices;

    for (const auto& mac : descendingMacs) {
        std::string truncatedMac = truncateWithDefault(mac);
        auto device = retrieveDeviceByMac(mac);
        if (!device) {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("mac", truncatedMac));
            continue;
        }
        /*
         * We're only connecting devices that have been
         * previously connected, have the target profile that matches requirement
         * and are currently paired.
         */
        if (!device->isPaired()) {
            ACSDK_DEBUG0(LX(__func__).d("reason", "deviceUnpaired").d("mac", truncatedMac));
            continue;
        }

        if (supportsAvsProfile(device, profileName)) {
            matchedDevices.push_back(device);
        }
        if (matchedDevices.size() == MAX_CONNECT_BY_PROFILE_COUNT) {
            break;
        }
    }

    bool deviceConnected = false;
    for (auto& matchedDevice : matchedDevices) {
        if (executeFunctionOnDevice(matchedDevice, &BluetoothDeviceInterface::connect)) {
            deviceConnected = true;
            executeOnDeviceConnect(matchedDevice);
            executeInsertBluetoothEventState(
                matchedDevice,
                DeviceState::CONNECTED,
                Optional<Requester>(Requester::CLOUD),
                Optional<std::string>(profileName));
            break;
        }
    }
    if (!deviceConnected) {
        executeSendConnectByProfileFailed(profileName, Requester::CLOUD);
    }
}

void Bluetooth::executeOnDeviceConnect(std::shared_ptr<BluetoothDeviceInterface> device, bool shouldNotifyConnection) {
    ACSDK_DEBUG5(LX(__func__));
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceUuidFailed"));
        return;
    }
    DeviceCategory category = DeviceCategory::UNKNOWN;
    if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceCategoryFailed"));
        return;
    }
    ACSDK_DEBUG5(LX(__func__).d("uuid", uuid).d("deviceCategory", category));

    auto connectionRule = retrieveConnectionRuleByUuid(uuid);
    if (connectionRule) {
        std::set<std::shared_ptr<BluetoothDeviceInterface>> devicesToDisconnect =
            connectionRule->devicesToDisconnect(m_connectedDevices);

        for (const auto& deviceToDisconnect : devicesToDisconnect) {
            if (deviceToDisconnect == device) {
                continue;
            }

            auto disconnectionFuture = deviceToDisconnect->disconnect();
            if (waitOnFuture(std::move(disconnectionFuture), "Disconnect the connected device")) {
                executeOnDeviceDisconnect(deviceToDisconnect, Requester::DEVICE);
            } else {
                // Failed to disconnect the device with same category , user will have to manually disconnect the
                // device.
                std::string truncatedMac = truncateWithDefault(deviceToDisconnect->getMac());
                ACSDK_ERROR(LX(__func__).d("reason", "disconnectExistingActiveDeviceFailed").d("mac", truncatedMac));
            }
        }
    }

    if (getService<A2DPSinkInterface>(device) || getService<A2DPSourceInterface>(device)) {
        m_activeA2DPDevice = device;
    }

    if (m_connectedDevices.find(category) != m_connectedDevices.end()) {
        auto& connectedDevices = m_connectedDevices[category];
        connectedDevices.insert(device);
    } else {
        m_connectedDevices[category] = {device};
    }

    // Notify observers when a bluetooth device is connected.
    if (shouldNotifyConnection) {
        for (const auto& observer : m_observers) {
            observer->onActiveDeviceConnected(generateDeviceAttributes(device));
        }
    }

    // Reinsert into the database for ordering.
    m_db->insertByMac(device->getMac(), uuid, true);
}

void Bluetooth::executeOnDeviceDisconnect(
    std::shared_ptr<BluetoothDeviceInterface> device,
    avsCommon::avs::Requester requester) {
    ACSDK_DEBUG5(LX(__func__));

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceUuidFailed"));
        return;
    }
    DeviceCategory category = DeviceCategory::UNKNOWN;
    if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveDeviceCategoryFailed"));
        return;
    }

    ACSDK_DEBUG5(LX(__func__).d("uuid", uuid).d("deviceCategory", category));

    if (device == m_activeA2DPDevice) {
        if (StreamingState::INACTIVE != m_streamingState) {
            ACSDK_DEBUG5(LX(__func__)
                             .d("currentState", streamingStateToString(m_streamingState))
                             .d("newState", streamingStateToString(StreamingState::INACTIVE)));
            // Needs to be sent while we still have an active A2DP source device in the Context.
            if (StreamingState::ACTIVE == m_streamingState) {
                executeSendStreamingEnded(m_activeA2DPDevice);
                executeReleaseFocus(__func__);
            }
            m_streamingState = StreamingState::INACTIVE;
        }
        m_activeA2DPDevice.reset();
    }

    if (m_connectedDevices.find(category) != m_connectedDevices.end()) {
        auto& connectedDevices = m_connectedDevices[category];
        connectedDevices.erase(device);
        if (connectedDevices.empty()) {
            m_connectedDevices.erase(category);
        }
    }

    // Notify observers when a bluetooth device is disconnected.
    for (const auto& observer : m_observers) {
        observer->onActiveDeviceDisconnected(generateDeviceAttributes(device));
    }

    executeInsertBluetoothEventState(
        device, DeviceState::DISCONNECTED, Optional<Requester>(requester), Optional<std::string>());
}

void Bluetooth::executeDisconnectDevices(const std::unordered_set<std::string>& uuids) {
    ACSDK_DEBUG5(LX(__func__));

    for (const auto& uuid : uuids) {
        auto device = retrieveDeviceByUuid(uuid);
        if (!device) {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuid));
            executeSendDisconnectDevicesFailed({uuid}, Requester::CLOUD);
            continue;
        }

        if (executeFunctionOnDevice(device, &BluetoothDeviceInterface::disconnect)) {
            executeOnDeviceDisconnect(device, Requester::CLOUD);
        } else {
            executeSendDisconnectDevicesFailed({uuid}, Requester::CLOUD);
        }
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

bool Bluetooth::executeSetScanMode(bool scanning, bool shouldReport) {
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

    bool success = waitOnFuture(std::move(future));
    if (success) {
        // If we're scanning, there will be more devices. If we're not, then there won't be.
        if (shouldReport) {
            executeSendScanDevicesReport(m_deviceManager->getDiscoveredDevices(), scanning);
        }
        m_scanningTransitionState =
            scanning ? ScanningTransitionState::ACTIVE : ScanningTransitionState::PENDING_INACTIVE;
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

    return waitOnFuture(std::move(future));
}

bool Bluetooth::executeFunctionOnDevice(
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>& device,
    std::function<std::future<bool>(std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>&)>
        function) {
    if (!device) {
        ACSDK_ERROR(LX("executeFunctionOnDeviceFailed").d("reason", "nullDevice"));
        return false;
    }

    std::string truncatedMac = truncateWithDefault(device->getMac());
    ACSDK_DEBUG5(LX(__func__).d("mac", truncatedMac));

    std::stringstream description;
    description << "executeFunctionOnDevice mac=" << device->getMac();
    auto future = function(device);
    return waitOnFuture(std::move(future), description.str());
}

void Bluetooth::onFirstByteRead(MediaPlayerObserverInterface::SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
}

void Bluetooth::onPlaybackStarted(MediaPlayerObserverInterface::SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
    m_executor.submit([this] {
        // It means we were pending a pause before the onPlaybackStarted was received.
        if (m_streamingState == StreamingState::PENDING_PAUSED) {
            executeSendStreamingStarted(m_activeA2DPDevice);
            if (!m_mediaPlayer->stop(m_sourceId)) {
                ACSDK_ERROR(LX("onPlaybackStartedLambdaFailed").d("reason", "stopFailed"));
                cleanupMediaSource();
            }
            return;
        }
        m_streamingState = StreamingState::ACTIVE;
        if (!m_activeA2DPDevice) {
            ACSDK_ERROR(LX(__func__).d("reason", "noActiveDevice"));
        } else {
            executeSendStreamingStarted(m_activeA2DPDevice);
        }
    });
}

void Bluetooth::onPlaybackStopped(MediaPlayerObserverInterface::SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
    m_executor.submit([this] {
        // Playback has been stopped, cleanup the source.
        cleanupMediaSource();
        if (m_activeA2DPDevice) {
            m_streamingState = StreamingState::PAUSED;
            executeSendStreamingEnded(m_activeA2DPDevice);
        } else {
            m_streamingState = StreamingState::INACTIVE;
        }
    });
}

void Bluetooth::onPlaybackFinished(MediaPlayerObserverInterface::SourceId id, const MediaPlayerState&) {
    ACSDK_DEBUG5(LX(__func__).d("sourceId", id));
    m_executor.submit([this] {
        m_streamingState = StreamingState::INACTIVE;

        cleanupMediaSource();
        if (m_activeA2DPDevice) {
            executeSendStreamingEnded(m_activeA2DPDevice);
        }
    });
}

void Bluetooth::onPlaybackError(
    MediaPlayerObserverInterface::SourceId id,
    const ErrorType& type,
    std::string error,
    const MediaPlayerState&) {
    ACSDK_DEBUG5(LX(__func__).d("id", id).d("type", type).d("error", error));

    m_executor.submit([this, id] {
        // Do not attempt to stop the media player here. This could result in a infinite loop if the stop fails.
        if (id == m_sourceId) {
            cleanupMediaSource();
        }
    });
}

// Events.
void Bluetooth::executeSendScanDevicesReport(
    const std::list<std::shared_ptr<BluetoothDeviceInterface>>& devices,
    bool hasMore) {
    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesArray(rapidjson::kArrayType);
    ACSDK_DEBUG5(LX(__func__).d("count", devices.size()));

    for (const auto& device : devices) {
        std::string truncatedMac = truncateWithDefault(device->getMac());
        ACSDK_DEBUG(LX("foundDevice").d("deviceMac", truncatedMac));
        std::string uuid;
        if (!retrieveUuid(device->getMac(), &uuid)) {
            ACSDK_ERROR(LX("executeSendScanDevicesReportFailed").d("reason", "retrieveUuidFailed"));
            return;
        }

        if (device->isPaired()) {
            ACSDK_DEBUG(LX(__func__)
                            .d("reason", "deviceAlreadyPaired")
                            .d("mac", truncatedMac)
                            .d("uuid", uuid)
                            .d("action", "ommitting"));
            continue;
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
        deviceJson.AddMember(TRUNCATED_MAC_ADDRESS_KEY, truncatedMac, payload.GetAllocator());

        rapidjson::Value metadataJson(rapidjson::kObjectType);
        auto metadata = device->getDeviceMetaData();
        if (metadata.vendorId.hasValue()) {
            metadataJson.AddMember(VENDOR_ID_KEY, metadata.vendorId.value(), payload.GetAllocator());
        }
        if (metadata.productId.hasValue()) {
            metadataJson.AddMember(PRODUCT_ID_KEY, metadata.productId.value(), payload.GetAllocator());
        }
        metadataJson.AddMember(CLASS_OF_DEVICE_KEY, metadata.classOfDevice, payload.GetAllocator());
        if (metadata.vendorDeviceSigId.hasValue()) {
            metadataJson.AddMember(
                VENDOR_DEVICE_SIG_ID_KEY, metadata.vendorDeviceSigId.value(), payload.GetAllocator());
        }
        if (metadata.vendorDeviceId.hasValue()) {
            metadataJson.AddMember(VENDOR_DEVICE_ID_KEY, metadata.vendorDeviceId.value(), payload.GetAllocator());
        }

        deviceJson.AddMember(METADATA_KEY, metadataJson.GetObject(), payload.GetAllocator());
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

    executeSendEvent(SCAN_DEVICES_REPORT.name, buffer.GetString());
}

void Bluetooth::executeSendScanDevicesFailed() {
    executeSendEvent(SCAN_DEVICES_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendEnterDiscoverableModeSucceeded() {
    executeSendEvent(ENTER_DISCOVERABLE_MODE_SUCCEEDED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendEnterDiscoverableModeFailed() {
    executeSendEvent(ENTER_DISCOVERABLE_MODE_FAILED.name, EMPTY_PAYLOAD);
}

void Bluetooth::executeSendPairDevicesSucceeded(
    const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>& devices) {
    if (devices.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyDevices"));
        return;
    }

    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesJson(rapidjson::kArrayType);

    for (const auto& device : devices) {
        std::string uuid;
        if (!retrieveUuid(device->getMac(), &uuid)) {
            ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
            continue;
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());

        DeviceCategory category = DeviceCategory::UNKNOWN;
        if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
            ACSDK_WARN(LX(__func__).d("reason", "retrieveDeviceCategoryByUuidFailed"));
            category = DeviceCategory::UNKNOWN;
        }
        deviceJson.AddMember(DEVICE_CATEGORY_KEY, deviceCategoryToString(category), payload.GetAllocator());

        rapidjson::Value supportedProfiles(rapidjson::kArrayType);
        // If this fails, add an empty array.
        if (!extractAvsProfiles(device, payload.GetAllocator(), &supportedProfiles)) {
            supportedProfiles = rapidjson::Value(rapidjson::kArrayType);
        }
        deviceJson.AddMember(PROFILES_KEY, supportedProfiles, payload.GetAllocator());

        devicesJson.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, devicesJson.GetArray(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeSendEvent(PAIR_DEVICES_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendPairFailedEvent(const std::string& eventName, const std::unordered_set<std::string>& uuids) {
    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesJson(rapidjson::kArrayType);

    if (!uuids.empty()) {
        for (const auto& uuid : uuids) {
            rapidjson::Value deviceJson(rapidjson::kObjectType);
            deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
            devicesJson.PushBack(deviceJson, payload.GetAllocator());
        }

        payload.AddMember(DEVICES_KEY, devicesJson.GetArray(), payload.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!payload.Accept(writer)) {
            ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
            return;
        }

        executeUpdateContext();

        executeSendEvent(eventName, buffer.GetString());
    }
}

void Bluetooth::executeSendPairDevicesFailed(const std::unordered_set<std::string>& uuids) {
    executeSendPairFailedEvent(PAIR_DEVICES_FAILED.name, uuids);
}

void Bluetooth::executeSendUnpairDevicesSucceeded(
    const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>& devices) {
    if (devices.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyDevices"));
        return;
    }
    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesJson(rapidjson::kArrayType);

    for (const auto& device : devices) {
        std::string uuid;
        if (!retrieveUuid(device->getMac(), &uuid)) {
            ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
            continue;
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        devicesJson.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, devicesJson.GetArray(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeSendEvent(UNPAIR_DEVICES_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendUnpairDevicesFailed(const std::unordered_set<std::string>& uuids) {
    executeSendPairFailedEvent(UNPAIR_DEVICES_FAILED.name, uuids);
}

void Bluetooth::executeSetDeviceCategoriesSucceeded(const std::map<std::string, std::string>& uuidCategoryMap) {
    rapidjson::Document payload(rapidjson::kObjectType);

    rapidjson::Value categorizedDevices(rapidjson::kArrayType);

    for (const auto& uuidCategory : uuidCategoryMap) {
        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuidCategory.first, payload.GetAllocator());
        deviceJson.AddMember(DEVICE_CATEGORY_KEY, uuidCategory.second, payload.GetAllocator());
        categorizedDevices.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, categorizedDevices.GetArray(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();
    executeQueueEventAndRequestContext(SET_DEVICE_CATEGORIES_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSetDeviceCategoriesFailed(const std::map<std::string, std::string>& uuidCategoryMap) {
    rapidjson::Document payload(rapidjson::kObjectType);

    rapidjson::Value categorizedDevices(rapidjson::kArrayType);

    for (const auto& uuidCategory : uuidCategoryMap) {
        rapidjson::Value deviceJson(rapidjson::kObjectType);

        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuidCategory.first, payload.GetAllocator());

        auto device = retrieveDeviceByUuid(uuidCategory.first);
        if (device) {
            rapidjson::Value metadataJson(rapidjson::kObjectType);

            auto metadata = device->getDeviceMetaData();
            if (metadata.vendorId.hasValue()) {
                metadataJson.AddMember(VENDOR_ID_KEY, metadata.vendorId.value(), payload.GetAllocator());
            }
            if (metadata.productId.hasValue()) {
                metadataJson.AddMember(PRODUCT_ID_KEY, metadata.productId.value(), payload.GetAllocator());
            }
            metadataJson.AddMember(CLASS_OF_DEVICE_KEY, metadata.classOfDevice, payload.GetAllocator());
            if (metadata.vendorDeviceSigId.hasValue()) {
                metadataJson.AddMember(
                    VENDOR_DEVICE_SIG_ID_KEY, metadata.vendorDeviceSigId.value(), payload.GetAllocator());
            }
            if (metadata.vendorDeviceId.hasValue()) {
                metadataJson.AddMember(VENDOR_DEVICE_ID_KEY, metadata.vendorDeviceId.value(), payload.GetAllocator());
            }
            deviceJson.AddMember(METADATA_KEY, metadataJson.GetObject(), payload.GetAllocator());
        } else {
            ACSDK_ERROR(LX(__func__).d("reason", "deviceNotFound").d("uuid", uuidCategory.first));
        }
        categorizedDevices.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, categorizedDevices.GetObject(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();
    executeQueueEventAndRequestContext(SET_DEVICE_CATEGORIES_FAILED.name, buffer.GetString());
}

void Bluetooth::executeSendConnectByDeviceIdsSucceeded(
    const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>& devices,
    Requester requester) {
    if (devices.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyDevices"));
        return;
    }

    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesJson(rapidjson::kArrayType);

    for (const auto& device : devices) {
        std::string uuid;
        if (!retrieveUuid(device->getMac(), &uuid)) {
            ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
            continue;
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
        devicesJson.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, devicesJson.GetArray(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeSendEvent(CONNECT_BY_DEVICE_IDS_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendConnectFailedEvent(
    const std::string& eventName,
    const std::unordered_set<std::string>& uuids,
    Requester requester) {
    if (uuids.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyUuids").d("eventName", eventName));
        return;
    }

    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesJson(rapidjson::kArrayType);

    for (const auto& uuid : uuids) {
        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, "", payload.GetAllocator());
        devicesJson.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, devicesJson.GetArray(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeSendEvent(eventName, buffer.GetString());
}

void Bluetooth::executeSendConnectByDeviceIdsFailed(const std::unordered_set<std::string>& uuids, Requester requester) {
    executeSendConnectFailedEvent(CONNECT_BY_DEVICE_IDS_FAILED.name, uuids, requester);
}

void Bluetooth::executeSendConnectByProfileSucceeded(
    std::shared_ptr<BluetoothDeviceInterface> device,
    const std::string& profileName,
    Requester requester) {
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    rapidjson::Document payload(rapidjson::kObjectType);

    std::string uuid;
    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());
    payload.AddMember(PROFILE_NAME_KEY, profileName, payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeSendEvent(CONNECT_BY_PROFILE_SUCCEEDED.name, buffer.GetString());
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

    executeSendEvent(CONNECT_BY_PROFILE_FAILED.name, buffer.GetString());
}

void Bluetooth::executeSendDisconnectDevicesSucceeded(
    const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>& devices,
    Requester requester) {
    if (devices.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyDevices"));
        return;
    }

    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value devicesJson(rapidjson::kArrayType);

    for (const auto& device : devices) {
        std::string uuid;
        if (!retrieveUuid(device->getMac(), &uuid)) {
            ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
            continue;
        }

        rapidjson::Value deviceJson(rapidjson::kObjectType);
        deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
        deviceJson.AddMember(FRIENDLY_NAME_KEY, device->getFriendlyName(), payload.GetAllocator());
        devicesJson.PushBack(deviceJson, payload.GetAllocator());
    }

    payload.AddMember(DEVICES_KEY, devicesJson.GetArray(), payload.GetAllocator());
    payload.AddMember(REQUESTER_KEY, requesterToString(requester), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeSendEvent(DISCONNECT_DEVICES_SUCCEEDED.name, buffer.GetString());
}

void Bluetooth::executeSendDisconnectDevicesFailed(const std::unordered_set<std::string>& uuids, Requester requester) {
    executeSendConnectFailedEvent(DISCONNECT_DEVICES_FAILED.name, uuids, requester);
}

void Bluetooth::executeSendMediaControlEvent(
    const std::string& eventName,
    std::shared_ptr<BluetoothDeviceInterface> device) {
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

    executeSendEvent(eventName, buffer.GetString());
}

void Bluetooth::executeSendMediaControlPlaySucceeded(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_PLAY_SUCCEEDED.name, device);
}

void Bluetooth::executeSendMediaControlPlayFailed(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_PLAY_FAILED.name, device);
}

void Bluetooth::executeSendMediaControlStopSucceeded(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_STOP_SUCCEEDED.name, device);
}

void Bluetooth::executeSendMediaControlStopFailed(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_STOP_FAILED.name, device);
}

void Bluetooth::executeSendMediaControlNextSucceeded(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_NEXT_SUCCEEDED.name, device);
}

void Bluetooth::executeSendMediaControlNextFailed(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_NEXT_FAILED.name, device);
}

void Bluetooth::executeSendMediaControlPreviousSucceeded(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_PREVIOUS_SUCCEEDED.name, device);
}

void Bluetooth::executeSendMediaControlPreviousFailed(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendMediaControlEvent(MEDIA_CONTROL_PREVIOUS_FAILED.name, device);
}

void Bluetooth::executeSendStreamingEvent(
    const std::string& eventName,
    std::shared_ptr<BluetoothDeviceInterface> device) {
    rapidjson::Document payload(rapidjson::kObjectType);
    std::string uuid;
    if (!device) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullDevice"));
        return;
    }

    if (!retrieveUuid(device->getMac(), &uuid)) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveUuidFailed"));
        return;
    }

    rapidjson::Value deviceJson(rapidjson::kObjectType);
    deviceJson.AddMember(UNIQUE_DEVICE_ID_KEY, uuid, payload.GetAllocator());
    if (supportsAvsProfile(device, AVS_A2DP_SINK)) {
        deviceJson.AddMember(PROFILE_NAME_KEY, AVS_A2DP_SINK, payload.GetAllocator());
    } else if (supportsAvsProfile(device, AVS_A2DP_SOURCE)) {
        deviceJson.AddMember(PROFILE_NAME_KEY, AVS_A2DP_SOURCE, payload.GetAllocator());
    }
    payload.AddMember(DEVICE_KEY, deviceJson.GetObject(), payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).d("reason", "writerRefusedJsonObject"));
        return;
    }

    executeUpdateContext();

    executeQueueEventAndRequestContext(eventName, buffer.GetString());
}

void Bluetooth::executeSendStreamingStarted(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendStreamingEvent(STREAMING_STARTED.name, device);
}

void Bluetooth::executeSendStreamingEnded(std::shared_ptr<BluetoothDeviceInterface> device) {
    executeSendStreamingEvent(STREAMING_ENDED.name, device);
}

void Bluetooth::executeAcquireFocus(const std::string& callingMethodName) {
    if (FocusState::FOREGROUND == m_focusState || FocusState::BACKGROUND == m_focusState) {
        ACSDK_DEBUG9(LX(__func__)
                         .d("focus", m_focusState)
                         .d("callingMethodName", callingMethodName)
                         .m("Already acquired channel"));
        return;
    }

    // Create our delayed release activity to initiate delayed release mechanism in AFML.
    auto activity = FocusManagerInterface::Activity::create(
        ACTIVITY_ID, shared_from_this(), TRANSIENT_FOCUS_DURATION, avsCommon::avs::ContentType::MIXABLE);

    if (!activity) {
        ACSDK_ERROR(LX(__func__).d("reason", "activityCreateFailed").d("callingMethodName", callingMethodName));
    } else if (!m_focusManager->acquireChannel(CHANNEL_NAME, activity)) {
        ACSDK_ERROR(LX(__func__).d("reason", "acquireChannelFailed").d("callingMethodName", callingMethodName));
    } else {
        ACSDK_DEBUG1(LX(__func__).d("callingMethodName", callingMethodName).m("Acquiring channel"));
    }
}

void Bluetooth::executeReleaseFocus(const std::string& callingMethodName) {
    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    m_focusTransitionState = FocusTransitionState::PENDING_INTERNAL;
    ACSDK_DEBUG1(LX(__func__).d("callingMethodName", callingMethodName).m("Releasing channel"));
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

template <typename ServiceType>
std::shared_ptr<ServiceType> Bluetooth::getService(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device) {
    ACSDK_DEBUG5(LX(__func__).d("uuid", ServiceType::UUID));

    std::shared_ptr<ServiceType> service = nullptr;
    {
        if (!device) {
            ACSDK_DEBUG5(LX(__func__).d("reason", "nullDevice"));
        } else {
            service = std::static_pointer_cast<ServiceType>(device->getService(ServiceType::UUID));
        }
    }

    return service;
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

    if (m_mediaAttachmentReader) {
        m_mediaAttachmentReader->close();
        m_mediaAttachmentReader.reset();
    }
    if (m_mediaAttachmentWriter) {
        m_mediaAttachmentWriter->close();
        m_mediaAttachmentWriter.reset();
    }

    if (m_mediaStream) {
        m_mediaStream->setListener(nullptr);
    }

    m_mediaStream = stream;

    if (m_mediaStream) {
        m_mediaAttachment = std::make_shared<avsCommon::avs::attachment::InProcessAttachment>("Bluetooth");

        m_mediaAttachmentReader = m_mediaAttachment->createReader(avsCommon::utils::sds::ReaderPolicy::NONBLOCKING);

        m_mediaAttachmentWriter = m_mediaAttachment->createWriter(avsCommon::utils::sds::WriterPolicy::ALL_OR_NOTHING);

        m_mediaStream->setListener(shared_from_this());

        auto audioFormat = m_mediaStream->getAudioFormat();
        m_sourceId = m_mediaPlayer->setSource(m_mediaAttachmentReader, &audioFormat);
        if (MediaPlayerInterface::ERROR == m_sourceId) {
            ACSDK_ERROR(LX(__func__).d("reason", "setSourceFailed"));
            m_mediaAttachment.reset();
            m_mediaAttachmentWriter.reset();
            m_mediaStream.reset();
        }
    }
}

void Bluetooth::executeRestrictA2DPDevices() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_restrictedDevices.empty()) {
        ACSDK_ERROR(LX("failedToRestrictDevices").d("reason", "restrictedListNotEmpty"));
        return;
    }

    m_disabledA2DPDevice = m_activeA2DPDevice;
    for (const auto& device : m_deviceManager->getDiscoveredDevices()) {
        if (device->isPaired()) {
            auto supportA2DP = false;
            if (device->getService(A2DPSinkInterface::UUID) != nullptr) {
                device->toggleServiceConnection(false, device->getService(A2DPSinkInterface::UUID));
                supportA2DP = true;
            }

            if (device->getService(A2DPSourceInterface::UUID) != nullptr) {
                device->toggleServiceConnection(false, device->getService(A2DPSourceInterface::UUID));
                supportA2DP = true;
            }

            if (supportA2DP) {
                m_restrictedDevices.push_back(device);
            }
        }
    }
}

void Bluetooth::executeUnrestrictA2DPDevices() {
    ACSDK_INFO(LX(__func__));
    if (!m_restrictedDevices.empty()) {
        for (const auto& device : m_restrictedDevices) {
            if (device->getService(A2DPSinkInterface::UUID) != nullptr) {
                device->toggleServiceConnection(true, device->getService(A2DPSinkInterface::UUID));
            }

            if (device->getService(A2DPSourceInterface::UUID) != nullptr) {
                device->toggleServiceConnection(true, device->getService(A2DPSourceInterface::UUID));
            }
        }
        m_restrictedDevices.clear();
    }

    if (m_disabledA2DPDevice) {
        bool supportsA2DP = supportsAvsProfile(m_disabledA2DPDevice, AVS_A2DP);
        if (!supportsA2DP) {
            ACSDK_DEBUG0(LX(__func__).d("reason", "noSupportedA2DPRoles").m("Connect Request Rejected"));
        } else {
            if (!executeFunctionOnDevice(m_disabledA2DPDevice, &BluetoothDeviceInterface::connect)) {
                std::string uuid;
                if (!retrieveUuid(m_disabledA2DPDevice->getMac(), &uuid)) {
                    ACSDK_ERROR(LX("executeUnrestrictA2DPDevicesFailed").d("reason", "retrieveUuidFailed"));
                } else {
                    executeSendConnectByDeviceIdsFailed({uuid}, Requester::DEVICE);
                }
            }
        }
        m_disabledA2DPDevice.reset();
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

void Bluetooth::executeInsertBluetoothEventState(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
    DeviceState state,
    avsCommon::utils::Optional<avsCommon::avs::Requester> requester,
    avsCommon::utils::Optional<std::string> profileName) {
    if (!device) {
        ACSDK_ERROR(LX("insertBluetoothEventStateFailed").d("reason", "nullDevice"));
        return;
    }

    std::unordered_set<std::shared_ptr<BluetoothEventState>> bluetoothEventStates;
    const std::string mac = device->getMac();
    auto it = m_bluetoothEventStates.find(mac);
    if (it != m_bluetoothEventStates.end()) {
        bluetoothEventStates.insert(it->second.begin(), it->second.end());
        m_bluetoothEventStates.erase(mac);
    }

    std::shared_ptr<BluetoothEventState> eventState =
        std::make_shared<BluetoothEventState>(state, requester, profileName);
    bluetoothEventStates.insert(eventState);
    m_bluetoothEventStates.insert({mac, bluetoothEventStates});
}

std::shared_ptr<BluetoothEventState> Bluetooth::executeRemoveBluetoothEventState(
    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface> device,
    DeviceState state) {
    const std::string mac = device->getMac();
    auto it = m_bluetoothEventStates.find(mac);
    if (it != m_bluetoothEventStates.end()) {
        std::unordered_set<std::shared_ptr<BluetoothEventState>> bluetoothEventStates = it->second;
        for (const auto& bluetoothEventState : bluetoothEventStates) {
            if (bluetoothEventState->getDeviceState() == state) {
                auto event = bluetoothEventState;
                bluetoothEventStates.erase(event);
                if (!bluetoothEventStates.empty()) {
                    m_bluetoothEventStates[mac] = bluetoothEventStates;
                } else {
                    m_bluetoothEventStates.erase(mac);
                }

                return event;
            }
        }
        ACSDK_DEBUG5(LX(__func__).d("reason", "noDeviceStateFound"));
        return nullptr;
    }

    ACSDK_DEBUG5(LX(__func__).d("reason", "noDeviceFound"));
    return nullptr;
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
                           .d("deviceName", truncateFriendlyName(device->getFriendlyName()))
                           .d("mac", truncateWithDefault(device->getMac())));
            m_executor.submit([this] {
                if (ScanningTransitionState::ACTIVE == m_scanningTransitionState) {
                    executeSendScanDevicesReport(m_deviceManager->getDiscoveredDevices(), true);
                }
            });
            break;
        }
        case avsCommon::utils::bluetooth::BluetoothEventType::SCANNING_STATE_CHANGED: {
            ACSDK_INFO(LX(__func__).d("reason", "SCANNING_STATE_CHANGED").d("isScanning", event.isScanning()));
            m_executor.submit([this, event] {
                bool isScanning = event.isScanning();
                if (!isScanning) {
                    if (ScanningTransitionState::PENDING_INACTIVE == m_scanningTransitionState) {
                        ACSDK_DEBUG5(LX(__func__)
                                         .d("reason", "PENDING_INACTIVE resolved")
                                         .d("m_scanningTransitionState", m_scanningTransitionState)
                                         .d("isScanning", event.isScanning()));
                        m_scanningTransitionState = ScanningTransitionState::INACTIVE;
                    } else {
                        executeSendScanDevicesReport(m_deviceManager->getDiscoveredDevices(), isScanning);
                    }
                }
            });
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
                           .d("deviceName", truncateFriendlyName(device->getFriendlyName()))
                           .d("mac", truncateWithDefault(device->getMac()))
                           .d("state", event.getDeviceState()));

            switch (event.getDeviceState()) {
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::FOUND:
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::IDLE:
                    break;
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::PAIRED: {
                    m_executor.submit([this, device] {
                        /*
                         * Send one of these so we remove the freshly paired device
                         * from the "Available Devices" page.
                         */
                        executeSendScanDevicesReport(m_deviceManager->getDiscoveredDevices(), true);
                        executeRemoveBluetoothEventState(device, DeviceState::PAIRED);
                        std::unordered_set<
                            std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
                            devices({device});
                        executeSendPairDevicesSucceeded(devices);
                    });
                    break;
                }
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::DISCONNECTED: {
                    m_executor.submit([this, device] {
                        std::shared_ptr<BluetoothEventState> disconnectEvent =
                            executeRemoveBluetoothEventState(device, DeviceState::DISCONNECTED);
                        if (disconnectEvent) {
                            /// Cloud initiated disconnect.
                            Optional<Requester> requester = disconnectEvent->getRequester();
                            if (requester.hasValue()) {
                                std::unordered_set<
                                    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
                                    devices({device});
                                executeSendDisconnectDevicesSucceeded(devices, requester.value());
                            } else {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "sendDisconnectDeviceSucceededEventFailed")
                                                .d("error", "retrieveDisconnectRequesterFailed"));
                            }
                        } else {
                            /// Device initiated disconnect.
                            DeviceCategory category;
                            std::string uuid;

                            if (!retrieveUuid(device->getMac(), &uuid)) {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "disconnectDeviceFailed")
                                                .d("error", "retrieveDisconnectedDeviceUuidFailed"));
                                return;
                            }

                            if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "disconnectDeviceFailed")
                                                .d("error", "retrieveDisconnectedDeviceCategoryFailed"));
                                return;
                            }

                            ACSDK_DEBUG9(LX(__func__).d("uuid", uuid).d("deviceCategory", category));

                            auto it = m_connectedDevices.find(category);

                            if (it != m_connectedDevices.end() && it->second.find(device) != it->second.end()) {
                                executeOnDeviceDisconnect(device, Requester::DEVICE);
                                executeRemoveBluetoothEventState(device, DeviceState::DISCONNECTED);
                                std::unordered_set<
                                    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
                                    devices({device});
                                executeSendDisconnectDevicesSucceeded(devices, Requester::DEVICE);
                            } else {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "disconnectDeviceFailed")
                                                .d("error", "deviceNotConnectedBefore")
                                                .d("deviceName", truncateFriendlyName(device->getFriendlyName()))
                                                .d("mac", truncateWithDefault(device->getMac())));
                            }
                        }
                    });

                    break;
                }
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::UNPAIRED: {
                    m_executor.submit([this, device] {
                        executeRemoveBluetoothEventState(device, DeviceState::UNPAIRED);

                        std::unordered_set<
                            std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
                            devices({device});
                        executeSendUnpairDevicesSucceeded(devices);
                    });
                    break;
                }
                case avsCommon::sdkInterfaces::bluetooth::DeviceState::CONNECTED: {
                    m_executor.submit([this, device] {
                        std::shared_ptr<BluetoothEventState> connectEvent =
                            executeRemoveBluetoothEventState(device, DeviceState::CONNECTED);
                        if (connectEvent) {
                            /// Cloud initiated connect.
                            Optional<Requester> requester = connectEvent->getRequester();
                            Optional<std::string> profileName = connectEvent->getProfileName();
                            if (!requester.hasValue()) {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason",
                                                   profileName.hasValue() ? "sendConnectByProfileFailed"
                                                                          : "sendConnectByDeviceIdsFailed")
                                                .d("error", "retrieveConnectRequesterFailed"));
                                return;
                            }

                            if (profileName.hasValue()) {
                                executeSendConnectByProfileSucceeded(device, profileName.value(), requester.value());
                            } else {
                                std::unordered_set<
                                    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
                                    devices({device});
                                executeSendConnectByDeviceIdsSucceeded(devices, requester.value());
                            }
                        } else {
                            /// Device initiated connect.
                            DeviceCategory category;
                            std::string uuid;

                            if (!retrieveUuid(device->getMac(), &uuid)) {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "connectDeviceFailed")
                                                .d("error", "retrieveConnectedDeviceCategoryFailed"));
                                return;
                            }

                            if (!retrieveDeviceCategoryByUuid(uuid, &category)) {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "connectDeviceFailed")
                                                .d("error", "retrieveConnectedDeviceCategoryFailed"));
                                return;
                            }

                            auto it = m_connectedDevices.find(category);

                            if (it == m_connectedDevices.end() ||
                                (it != m_connectedDevices.end() && it->second.find(device) == it->second.end())) {
                                executeOnDeviceConnect(device, true);
                                /*
                                 * Default to sending a ConnectByDeviceIds event since this wasn't a result of a profile
                                 * specific connection.
                                 */
                                std::unordered_set<
                                    std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
                                    devices({device});
                                executeSendConnectByDeviceIdsSucceeded(devices, Requester::DEVICE);
                            } else {
                                ACSDK_ERROR(LX(__func__)
                                                .d("reason", "connectDeviceFailed")
                                                .d("error", "deviceAlreadyConnectedBefore")
                                                .d("deviceName", truncateFriendlyName(device->getFriendlyName()))
                                                .d("mac", truncateWithDefault(device->getMac())));
                            }
                        }
                    });
                    break;
                }
            }
            break;
        }
        case avsCommon::utils::bluetooth::BluetoothEventType::STREAMING_STATE_CHANGED: {
            if (event.getDevice() != m_activeA2DPDevice) {
                ACSDK_ERROR(LX(__func__)
                                .d("reason", "mismatchedDevices")
                                .d("eventDevice", event.getDevice() ? event.getDevice()->getMac() : "null")
                                .d("activeDevice", m_activeA2DPDevice ? m_activeA2DPDevice->getMac() : "null"));
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
                            executeAcquireFocus(__func__);
                        }
                    });
                    /*
                     * TODO: Using MediaStreamingState signal as a proxy for playback state. There is latency with this.
                     * We should be observing a separate playback based signal instead for these decisions.
                     */
                } else if (MediaStreamingState::IDLE == event.getMediaStreamingState()) {
                    m_executor.submit([this] {
                        if (StreamingState::ACTIVE == m_streamingState) {
                            if (FocusState::FOREGROUND == m_focusState || FocusState::BACKGROUND == m_focusState) {
                                executeReleaseFocus(__func__);
                            }
                        }
                    });
                }
            } else if (A2DPRole::SOURCE == *event.getA2DPRole()) {
                // Ignore the PENDING state and only act on ACTIVE.
                if (MediaStreamingState::ACTIVE == event.getMediaStreamingState()) {
                    m_executor.submit([this] {
                        if (StreamingState::ACTIVE != m_streamingState) {
                            m_streamingState = StreamingState::ACTIVE;
                            executeSendStreamingStarted(m_activeA2DPDevice);
                        }
                    });
                } else if (MediaStreamingState::IDLE == event.getMediaStreamingState()) {
                    m_executor.submit([this] {
                        if (StreamingState::ACTIVE == m_streamingState) {
                            m_streamingState = StreamingState::PAUSED;
                            executeSendStreamingEnded(m_activeA2DPDevice);
                        }
                    });
                }
            }
            break;
        }
        case avsCommon::utils::bluetooth::BluetoothEventType::TOGGLE_A2DP_PROFILE_STATE_CHANGED: {
            ACSDK_DEBUG5(
                LX(__func__).d("event", "TOGGLE_A2DP_PROFILE_STATE_CHANGED").d("a2dpEnable", event.isA2DPEnabled()));
            if (event.isA2DPEnabled()) {
                m_executor.submit([this] { executeUnrestrictA2DPDevices(); });
            } else {
                m_executor.submit([this] { executeRestrictA2DPDevices(); });
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
