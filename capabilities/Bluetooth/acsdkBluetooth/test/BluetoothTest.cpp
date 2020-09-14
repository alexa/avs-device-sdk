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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/MockBluetoothDevice.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/MockBluetoothDeviceConnectionRule.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/MockBluetoothDeviceManager.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/MockBluetoothHostController.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSinkInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/HIDInterface.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/MockBluetoothService.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/SPPInterface.h>
#include <AVSCommon/SDKInterfaces/MockChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Bluetooth/DeviceCategory.h>
#include <AVSCommon/Utils/Bluetooth/SDPRecords.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Optional.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "acsdkBluetooth/BasicDeviceConnectionRule.h"
#include "acsdkBluetooth/Bluetooth.h"
#include "acsdkBluetooth/SQLiteBluetoothStorage.h"
#include "acsdkBluetoothInterfaces/MockBluetoothDeviceObserver.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::sdkInterfaces::bluetooth::test;
using namespace avsCommon::sdkInterfaces::bluetooth::services::test;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::bluetooth;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace ::testing;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"BluetoothTest"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Test Bluetooth device mac address 1.
static const std::string TEST_BLUETOOTH_DEVICE_MAC = "01:23:45:67:89:ab";

/// Test Bluetooth device friendly name 1.
static const std::string TEST_BLUETOOTH_FRIENDLY_NAME = "test_friendly_name_1";

/// Test Bluetooth device uuid 1.
static const std::string TEST_BLUETOOTH_UUID = "650f973b-c2ab-4c6e-bff4-3788cd521340";

/// Test Bluetooth device mac address 2.
static const std::string TEST_BLUETOOTH_DEVICE_MAC_2 = "11:23:45:67:89:ab";

/// Test Bluetooth device friendly name 2.
static const std::string TEST_BLUETOOTH_FRIENDLY_NAME_2 = "test_friendly_name_2";

/// Test Bluetooth device uuid 2.
static const std::string TEST_BLUETOOTH_UUID_2 = "650f973b-c2ab-4c6e-bff4-3788cd521341";

/// Test Bluetooth device mac address 3.
static const std::string TEST_BLUETOOTH_DEVICE_MAC_3 = "21:23:45:67:89:ab";

/// Test Bluetooth device friendly name 3.
static const std::string TEST_BLUETOOTH_FRIENDLY_NAME_3 = "test_friendly_name_3";

/// Test Bluetooth device uuid 3.
static const std::string TEST_BLUETOOTH_UUID_3 = "650f973b-c2ab-4c6e-bff4-3788cd521342";

/// Test Database file name. Can be changed if there are conflicts.
static const std::string TEST_DATABASE = "BluetoothCATest.db";

// clang-format off
static const std::string BLUETOOTH_JSON = R"(
{
   "bluetooth" : {
        "databaseFilePath":")" + TEST_DATABASE + R"("
     }
}
)";
// clang-format on

/// Error message for when the file already exists.
static const std::string FILE_EXISTS_ERROR = "Database File " + TEST_DATABASE + " already exists.";

/// Namespace of Bluetooth.
static const std::string NAMESPACE_BLUETOOTH = "Bluetooth";

/// THe Bluetooth state portion of the Context.
static const NamespaceAndName BLUETOOTH_STATE{NAMESPACE_BLUETOOTH, "BluetoothState"};

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the name section of a message.
static const std::string MESSAGE_NAME_KEY = "name";

/// JSON key for the payload section of a message.
static const std::string PAYLOAD_KEY = "payload";

/// JSON key for the requester section of a message.
static const std::string REQUESTER_KEY = "requester";

/// JSON value for the cloud requester.
static const std::string CLOUD_REQUESTER_VALUE = "CLOUD";

/// JSON value for the device requester.
static const std::string DEVICE_REQUESTER_VALUE = "DEVICE";

/// ConnectByDevice directive.
static const std::string CONNECT_BY_DEVICE_IDS_DIRECTIVE = "ConnectByDeviceIds";

/// ConnectByProfile directive.
static const std::string CONNECT_BY_PROFILE_DIRECTIVE = "ConnectByProfile";

/// PairDevice directive
static const std::string PAIR_DEVICES_DIRECTIVE = "PairDevices";

/// UnpairDevice directive
static const std::string UNPAIR_DEVICES_DIRECTIVE = "UnpairDevices";

/// DisconnectDevice directive
static const std::string DISCONNECT_DEVICES_DIRECTIVE = "DisconnectDevices";

/// SetDeviceCategories directive
static const std::string SET_DEVICE_CATEGORIES = "SetDeviceCategories";

/// Test message id.
static const std::string TEST_MESSAGE_ID = "MessageId_Test";

/// Test message id.
static const std::string TEST_MESSAGE_ID_2 = "MessageId_Test_2";

// clang-format off
/// ConnectByDeviceIds payload.
static const std::string TEST_CONNECT_BY_DEVICE_IDS_PAYLOAD = R"(
{
    "devices" : [{
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID + R"(",
        "friendlyName" :")" + TEST_BLUETOOTH_FRIENDLY_NAME + R"("
    }, {"uniqueDeviceId":")" + TEST_BLUETOOTH_UUID_2 + R"(",
        "friendlyName" :")" + TEST_BLUETOOTH_FRIENDLY_NAME_2 + R"("
    }]
}
)";
// clang-format on

/// The @c ConnectByDeviceIdSucceeded event name.
static const std::string CONNECT_BY_DEVICE_IDS_SUCCEEDED = "ConnectByDeviceIdsSucceeded";

/// The @c ConnectByProfileSucceeded event name.
static const std::string CONNECT_BY_PROFILE_SUCCEEDED = "ConnectByProfileSucceeded";

/// The @c ConnectByProfileFailed event name.
static const std::string CONNECT_BY_PROFILE_FAILED = "ConnectByProfileFailed";

/// The @c PairDeviceSucceeded event name.
static const std::string PAIR_DEVICES_SUCCEEDED = "PairDevicesSucceeded";

/// The @c UnpairDeviceSucceeded event name.
static const std::string UNPAIR_DEVICES_SUCCEEDED = "UnpairDevicesSucceeded";

/// The @c SetDeviceCategoriesSucceeded event name.
static const std::string SET_DEVICE_CATEGORIES_SUCCEEDED = "SetDeviceCategoriesSucceeded";

/// The @c DisconnectDeviceSucceeded event name.
static const std::string DISCONNECT_DEVICES_SUCCEEDED = "DisconnectDevicesSucceeded";

/// The @c ScanDevicesUpdated event name.
static const std::string SCAN_DEVICES_REPORT = "ScanDevicesReport";

/// The @c StreamingStarted event name.
static const std::string STREAMING_STARTED = "StreamingStarted";

/// The @c StreamingEnded event name.
static const std::string STREAMING_ENDED = "StreamingEnded";

/// Test unmatched profile name.
static const std::string TEST_UNMATCHED_PROFILE_NAME = "HFP";

/// Test matched profile name.
static const std::string TEST_MATCHED_PROFILE_NAME = "AVRCP";

/// Test profile version.
static const std::string TEST_PROFILE_VERSION = "1";

// clang-format off
/// ConnectByDeviceProfile payload 1.
static const std::string TEST_CONNECT_BY_PROFILE_PAYLOAD_1 = R"(
{
    "profile" : {
        "name":")" + TEST_UNMATCHED_PROFILE_NAME + R"(",
        "version" :")" + TEST_PROFILE_VERSION + R"("
    }
}
)";
// clang-format on

// clang-format off
/// ConnectByDeviceProfile payload 2
static const std::string TEST_CONNECT_BY_PROFILE_PAYLOAD_2 = R"(
{
   "profile" : {
        "name":")" + TEST_MATCHED_PROFILE_NAME + R"(",
        "version" :")" + TEST_PROFILE_VERSION + R"("
   }
}
)";
// clang-format on

// clang-format off
/// PairDevices payload
static const std::string TEST_PAIR_DEVICES_PAYLOAD = R"(
{
    "devices" : [{
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID + R"("
    }, {
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID_2 + R"("
    }]
}
)";

// clang-format on

// clang-format off
/// UnpairDevices payload
static const std::string TEST_UNPAIR_DEVICES_PAYLOAD = R"(
{
    "devices" : [{
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID + R"("
    }, {
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID_2 + R"("
    }]
}
)";
// clang-format on

// clang-format off
/// DisconnectDevices payload
static const std::string TEST_DISCONNECT_DEVICES_PAYLOAD = R"(
{
    "devices" : [{
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID + R"("
    }, {
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID_2 + R"("
    }]
}
)";
// clang-format on

// clang-format off
/// SetDeviceCategories payload
static const std::string TEST_SET_DEVICE_CATEGORIES_PAYLOAD = R"(
{
    "devices" : [{
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID + R"(",
        "deviceCategory": "PHONE"
    }, {
        "uniqueDeviceId":")" + TEST_BLUETOOTH_UUID_2 + R"(",
        "deviceCategory": "GADGET"
    }]
}
)";
// clang-format on

// clang-format off
/// Mock Context
static const std::string MOCK_CONTEXT = R"(
{
    "context": [{
        "header": {
            "namespace": "Bluetooth",
            "name": "BluetoothState"
        },
        "payload": {
            "alexaDevice": {
                "friendlyName": "{{STRING}}"
            },
            "pairedDevices": [{
                "uniqueDeviceId": "{{STRING}}",
                "friendlyName": "{{STRING}}",
                "supportedProfiles": [{
                    "name": "{{STRING}}",
                    "version": "{{STRING}}"
                }]
            }],
            "activeDevice": {
                "uniqueDeviceId": "{{STRING}}",
                "friendlyName": "{{STRING}}",
                "supportedProfiles": [{
                    "name": "{{STRING}}",
                    "version": "{{STRING}}"
                }],
                "streaming": "{{STRING}}"
            }
        }
    }]
}
)";

/// Wait timeout.
static std::chrono::milliseconds WAIT_TIMEOUT_MS(1000);
// Delay to let events happen / threads catch up
static std::chrono::milliseconds EVENT_PROCESS_DELAY_MS(500);

/**
 * Checks whether a file exists in the file system.
 *
 * @param file The file name.
 * @return Whether the file exists.
 */
static bool fileExists(const std::string& file) {
    std::ifstream dbFile(file);
    return dbFile.good();
}

class BluetoothTest: public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    /// @c Bluetooth to test
    std::shared_ptr<Bluetooth> m_Bluetooth;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// @c FocusManager to request focus to the CONTENT channel.
    std::shared_ptr<MockFocusManager> m_mockFocusManager;

    /// A message sender used to send events to AVS.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    /// An exception sender used to send exception encountered events to AVS.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionSender;

    /// The storage component for @c Bluetooth.
    std::shared_ptr<SQLiteBluetoothStorage> m_bluetoothStorage;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockBluetoothMediaPlayer;

    /// A set of device connection rules.
    std::unordered_set<std::shared_ptr<BluetoothDeviceConnectionRuleInterface>> m_mockEnabledConnectionRules;

    /// A bus to abstract Bluetooth stack specific messages.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;

    /// Object that will track the CustomerDataHandler.
    std::shared_ptr<registrationManager::CustomerDataManager> m_customerDataManager;

    /// @c BluetoothHostController to create @c MockBluetoothDeviceManager
    std::shared_ptr<MockBluetoothHostController> m_mockBluetoothHostController;

    /// The list of discovered devices to create @c MockBluetoothDeviceManager
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceInterface>>
        m_mockDiscoveredBluetoothDevices;

    /// Bluetooth devices used to test the Bluetooth CA connection logic.
    std::shared_ptr<MockBluetoothDevice> m_mockBluetoothDevice1;
    std::shared_ptr<MockBluetoothDevice> m_mockBluetoothDevice2;
    std::shared_ptr<MockBluetoothDevice> m_mockBluetoothDevice3;

    /// Bluetooth device connection rules.
    /// Bluetooth device connection rule for DeviceCategory::REMOTE_CONTROL.
    std::shared_ptr<MockBluetoothDeviceConnectionRule> m_remoteControlConnectionRule;
    /// Bluetooth device connection rule for DeviceCategory::GADGET.
    std::shared_ptr<MockBluetoothDeviceConnectionRule> m_gadgetConnectionRule;

    /// A manager to take care of Bluetooth devices.
    std::unique_ptr<MockBluetoothDeviceManager> m_mockDeviceManager;

    /// A directive handler result to send the result to.
    std::unique_ptr<MockDirectiveHandlerResult> m_mockDirectiveHandlerResult;

    /// An observer to be notified of the Bluetooth connection change.
    std::shared_ptr<acsdkBluetoothInterfaces::test::MockBluetoothDeviceObserver> m_mockBluetoothDeviceObserver;

    /// A @c ChannelVolumeInterface object to control volume
    std::shared_ptr<MockChannelVolumeInterface> m_mockChannelVolumeInterface;

    /// Condition variable to wake on a message being sent.
    std::condition_variable m_messageSentTrigger;

    /// Expected messages.
    std::map<std::string, int> m_messages;

    /// Function to wait for @c m_wakeSetCompleteFuture to be set.
    void wakeOnSetCompleted();

    /**
     * Get the request event name.
     *
     * @param request The @c MessageRequest to verify.
     * @return The event name.
     */
    std::string getRequestName(std::shared_ptr<avsCommon::avs::MessageRequest> request);

    /**
     * Verify that the message name matches the expected name.
     *
     * @param request The @c MessageRequest to verify.
     * @param expectedName The expected name to find in the json header.
     * @return true if the message name matched the expect name.
     */
    bool verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request, std::string expectedName);

    /**
     * Verify that the messages sent matches the ordered list of events.
     *
     * @param request The @c MessageRequest to verify.
     * @param trigger The function to trigger sending the messages.
     * @return true if the messages sent matches the ordered list of events.
     */
    bool verifyMessagesSentInOrder(std::vector<std::string> orderedEvents, std::function<void()> trigger);

    /**
     * Verify that the messages sent matches the count.
     *
     * @param request The @c MessageRequest to verify.
     * @param messages The Map<RequestName, Count> expected messages.
     */
    void verifyMessagesCount(std::shared_ptr<avsCommon::avs::MessageRequest> request,
        std::map<std::string, int>* messages);

    /// A Constructor which initializes the promises and futures needed for the test class.
    BluetoothTest() : m_wakeSetCompletedPromise{}, m_wakeSetCompletedFuture{m_wakeSetCompletedPromise.get_future()} {
    }

protected:
    /// Promise to synchronize directive handling through SetCompleted.
    std::promise<void> m_wakeSetCompletedPromise;

    /// Future to synchronize directive handling through SetCompleted.
    std::future<void> m_wakeSetCompletedFuture;
};

void BluetoothTest::SetUp() {
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();

    m_eventBus = std::make_shared<avsCommon::utils::bluetooth::BluetoothEventBus>();
    m_mockBluetoothHostController = std::make_shared<NiceMock<MockBluetoothHostController>>();
    m_mockDirectiveHandlerResult = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    m_mockBluetoothDeviceObserver = std::make_shared<NiceMock<acsdkBluetoothInterfaces::test::MockBluetoothDeviceObserver>>();
    m_mockBluetoothMediaPlayer = MockMediaPlayer::create();
    m_customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();

    /*
     * Create Mock Devices.
     */
    auto metaData =MockBluetoothDevice::MetaData(Optional<int>(), Optional<int>(),
            MockBluetoothDevice::MetaData::UNDEFINED_CLASS_VALUE, Optional<int>(), Optional<std::string>());
    auto a2dpSink = std::make_shared<NiceMock<MockBluetoothService>>(std::make_shared<A2DPSinkRecord>(""));
    auto avrcpTarget = std::make_shared<NiceMock<MockBluetoothService>>(std::make_shared<AVRCPTargetRecord>(""));
    std::vector<std::shared_ptr<BluetoothServiceInterface>> services = {a2dpSink, avrcpTarget};
    m_mockBluetoothDevice1 = std::make_shared<NiceMock<MockBluetoothDevice>>(
        TEST_BLUETOOTH_DEVICE_MAC, TEST_BLUETOOTH_FRIENDLY_NAME, metaData, services);
    m_mockDiscoveredBluetoothDevices.push_back(m_mockBluetoothDevice1);

    auto metaData2 =MockBluetoothDevice::MetaData(Optional<int>(), Optional<int>(),
            MockBluetoothDevice::MetaData::UNDEFINED_CLASS_VALUE, Optional<int>(), Optional<std::string>());
    auto hid = std::make_shared<NiceMock<MockBluetoothService>>(std::make_shared<HIDRecord>(""));
    auto spp = std::make_shared<NiceMock<MockBluetoothService>>(std::make_shared<SPPRecord>(""));
    auto a2dpSource = std::make_shared<NiceMock<MockBluetoothService>>(std::make_shared<A2DPSourceRecord>(""));
    std::vector<std::shared_ptr<BluetoothServiceInterface>> services2 = {spp, hid, a2dpSource};
    m_mockBluetoothDevice2 = std::make_shared<NiceMock<MockBluetoothDevice>>(
        TEST_BLUETOOTH_DEVICE_MAC_2, TEST_BLUETOOTH_FRIENDLY_NAME_2, metaData2, services2);
    m_mockDiscoveredBluetoothDevices.push_back(m_mockBluetoothDevice2);

    auto metaData3 =MockBluetoothDevice::MetaData(Optional<int>(), Optional<int>(),
            MockBluetoothDevice::MetaData::UNDEFINED_CLASS_VALUE, Optional<int>(), Optional<std::string>());
    std::vector<std::shared_ptr<BluetoothServiceInterface>> services3 = {a2dpSink};
    m_mockBluetoothDevice3 = std::make_shared<NiceMock<MockBluetoothDevice>>(
        TEST_BLUETOOTH_DEVICE_MAC_3, TEST_BLUETOOTH_FRIENDLY_NAME_3, metaData3, services3);
    m_mockDiscoveredBluetoothDevices.push_back(m_mockBluetoothDevice3);

    /*
     * Create mock device connection rules.
     */
    std::set<DeviceCategory> remoteCategory{DeviceCategory::REMOTE_CONTROL};
    std::set<std::string> remoteDependentProfiles{HIDInterface::UUID, SPPInterface::UUID};
    m_remoteControlConnectionRule =
        std::make_shared<NiceMock<MockBluetoothDeviceConnectionRule>>(remoteCategory, remoteDependentProfiles);
    std::set<DeviceCategory> gadgetCategory{DeviceCategory::GADGET};
    std::set<std::string> gadgetDependentProfiles{HIDInterface::UUID, SPPInterface::UUID};
    m_gadgetConnectionRule =
        std::make_shared<NiceMock<MockBluetoothDeviceConnectionRule>>(gadgetCategory, gadgetDependentProfiles);
    /*
     * GadgetConnectionRule:
     * 1) No need to explicitly disconnect/connect device.
     * 2) No devices needed to disconnect when a new device with DeviceCategory::GADGET connects.
     */
    m_gadgetConnectionRule->setExplicitlyConnect(false);
    m_gadgetConnectionRule->setExplicitlyDisconnect(true);

    /*
     * RemoteControlConnectionRule:
     * 1) No need to explicitly disconnect/connect device.
     * 2) Devices with DeviceCategory::REMOTE_CONTROL needed to disconnect when a new device with
     * DeviceCategory::REMOTE_CONTROL connects.
     */
    m_remoteControlConnectionRule->setExplicitlyConnect(false);
    m_remoteControlConnectionRule->setExplicitlyDisconnect(false);

    m_mockEnabledConnectionRules =
        {m_remoteControlConnectionRule, m_gadgetConnectionRule, BasicDeviceConnectionRule::create()};

   /**
    * create MockChannelVolumeInterface for ducking.
    */
    m_mockChannelVolumeInterface = std::make_shared<MockChannelVolumeInterface>();
    m_mockChannelVolumeInterface->DelegateToReal();

    /*
     * Generate a Bluetooth database for testing.
     * Ensure the db file does not exist already. We don't want to overwrite anything.
     */
    if (fileExists(TEST_DATABASE)) {
        ADD_FAILURE() << FILE_EXISTS_ERROR;
        exit(1);
    }
    auto json = std::shared_ptr<std::stringstream>(new std::stringstream());
    *json << BLUETOOTH_JSON;
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(json);
    ConfigurationNode::initialize(jsonStream);
    m_bluetoothStorage = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_TRUE(m_bluetoothStorage->createDatabase());
    // Insert the test device data into the test database
    m_bluetoothStorage->insertByMac(TEST_BLUETOOTH_DEVICE_MAC, TEST_BLUETOOTH_UUID, true);
    m_bluetoothStorage->insertByMac(TEST_BLUETOOTH_DEVICE_MAC_2, TEST_BLUETOOTH_UUID_2, true);
    m_bluetoothStorage->insertByMac(TEST_BLUETOOTH_DEVICE_MAC_3, TEST_BLUETOOTH_UUID_3, true);
    m_bluetoothStorage->close();

    m_Bluetooth = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    ASSERT_THAT(m_Bluetooth, NotNull());
    m_Bluetooth->addObserver(m_mockBluetoothDeviceObserver);
}

void BluetoothTest::TearDown() {
    if (m_Bluetooth) {
        m_Bluetooth->shutdown();
    }
    m_mockBluetoothMediaPlayer->shutdown();
    if (fileExists(TEST_DATABASE)) {
        remove(TEST_DATABASE.c_str());
    }
}

void BluetoothTest::wakeOnSetCompleted() {
    m_wakeSetCompletedPromise.set_value();
}

std::string BluetoothTest::getRequestName(std::shared_ptr<alexaClientSDK::avsCommon::avs::MessageRequest> request) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    EXPECT_NE(header, event->value.MemberEnd());

    auto payload = event->value.FindMember(PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());

    std::string requestName;
    jsonUtils::retrieveValue(header->value, MESSAGE_NAME_KEY, &requestName);
    return requestName;
}

bool BluetoothTest::verifyMessage(std::shared_ptr<alexaClientSDK::avsCommon::avs::MessageRequest> request,
                                  std::string expectedName) {
    return getRequestName(request) == expectedName;
}

bool BluetoothTest::verifyMessagesSentInOrder(std::vector<std::string> orderedEvents, std::function<void()> trigger) {
    size_t curIndex = 0;
    std::mutex waitMutex;

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(
            Invoke([this, orderedEvents, &curIndex](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                if (curIndex < orderedEvents.size()) {
                    if (verifyMessage(request, orderedEvents.at(curIndex))) {
                        if (curIndex < orderedEvents.size()) {
                            curIndex++;
                        }
                    }
                }
                m_messageSentTrigger.notify_one();
            }));

    trigger();

    bool result;
    {
        std::unique_lock<std::mutex> lock(waitMutex);
        result = m_messageSentTrigger.wait_for(lock, WAIT_TIMEOUT_MS, [orderedEvents, &curIndex] {
            if (curIndex == orderedEvents.size()) {
                return true;
            }
            return false;
        });
    }

    return result;
}

void BluetoothTest::verifyMessagesCount(std::shared_ptr<alexaClientSDK::avsCommon::avs::MessageRequest> request,
    std::map<std::string, int>* messages) {
    std::string requestName = getRequestName(request);

    if (messages->find(requestName) != messages->end()) {
        messages->at(requestName) += 1;
    }
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(BluetoothTest, test_createBTWithNullParams) {
    // Create Bluetooth CapabilityAgent with null @c ContextManager.
    auto bluetooth1 = Bluetooth::create(
        nullptr,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth1, IsNull());

    // Create Bluetooth CapabilityAgent with null @c FocusManager
    auto bluetooth2 = Bluetooth::create(
        m_mockContextManager,
        nullptr,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth2, IsNull());

    // Create Bluetooth CapabilityAgent with null @c MessageSender
    auto bluetooth3 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        nullptr,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth3, IsNull());

    // Create Bluetooth CapabilityAgent with null @c ExceptionEncounterSender
    auto bluetooth4 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        nullptr,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth4, IsNull());

    // Create Bluetooth CapabilityAgent with null @c BluetoothStorage
    auto bluetooth5 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        nullptr,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth5, IsNull());

    // Create Bluetooth CapabilityAgent with null @c DeviceManager
    auto bluetooth6 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        nullptr,
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth6, IsNull());

    // Create Bluetooth CapabilityAgent with null @c BluetoothEventBus
    auto bluetooth7 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        nullptr,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth7, IsNull());

    // Create Bluetooth CapabilityAgent with null @c MediaPlayer
    auto bluetooth8 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        nullptr,
        m_customerDataManager,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth8, IsNull());

    // Create Bluetooth CapabilityAgent with null @c MediaPlayer
    auto bluetooth9 = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        nullptr,
        m_mockEnabledConnectionRules,
        m_mockChannelVolumeInterface,
        nullptr);
    EXPECT_THAT(bluetooth9, IsNull());
}

/**
 * Test that create() returns a nullptr if called with an invalid set of device connection rules.
 * Fail due to re-defined device category.
 */
TEST_F(BluetoothTest, test_createBTWithDuplicateDeviceCategoriesInConnectionRules) {
    std::set<DeviceCategory> categories1{DeviceCategory::REMOTE_CONTROL};
    std::set<DeviceCategory> categories2{DeviceCategory::REMOTE_CONTROL, DeviceCategory::GADGET};
    std::set<std::string> dependentProfiles{HIDInterface::UUID, SPPInterface::UUID};
    auto mockDeviceConnectionRule1 =
        std::make_shared<MockBluetoothDeviceConnectionRule>(categories1, dependentProfiles);
    auto mockDeviceConnectionRule2 =
        std::make_shared<MockBluetoothDeviceConnectionRule>(categories2, dependentProfiles);
    std::unordered_set<std::shared_ptr<BluetoothDeviceConnectionRuleInterface>> enabledRules =
        {mockDeviceConnectionRule1, mockDeviceConnectionRule2};
    auto bluetooth = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        enabledRules,
        m_mockChannelVolumeInterface);
    ASSERT_THAT(bluetooth, IsNull());
}

/**
 * Test that create() returns a nullptr if called with an invalid set of device connection rules.
 * Fail due to lack of dependent profiles defined in the device connection rule.
 */
TEST_F(BluetoothTest, test_createBTWithLackOfProfilesInConnectionRules) {
    std::set<DeviceCategory> categories{DeviceCategory::REMOTE_CONTROL};
    std::set<std::string> dependentProfiles{HIDInterface::UUID};
    auto mockDeviceConnectionRule = std::make_shared<MockBluetoothDeviceConnectionRule>(categories, dependentProfiles);
    std::unordered_set<std::shared_ptr<BluetoothDeviceConnectionRuleInterface>> enabledRules =
        {mockDeviceConnectionRule};
    auto bluetooth = Bluetooth::create(
        m_mockContextManager,
        m_mockFocusManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_bluetoothStorage,
        avsCommon::utils::memory::make_unique<NiceMock<MockBluetoothDeviceManager>>(
            m_mockBluetoothHostController, m_mockDiscoveredBluetoothDevices, m_eventBus),
        m_eventBus,
        m_mockBluetoothMediaPlayer,
        m_customerDataManager,
        enabledRules,
        m_mockChannelVolumeInterface);
    ASSERT_THAT(bluetooth, IsNull());
}

/**
 * Test call to handle ConnectByDeviceIds directive with two matched A2DP device UUIDs.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2. Both of them belong to
 * DeviceCategory::UNKNOWN. Both devices are in disconnected state.
 * Use case:
 * A ConnectByDeviceIds directive is sent down, whose payload contains the mockBluetoothDevice1 and
 * mockBluetoothDevice2 UUIDs.
 * Expected result:
 * 1) Only one device which connects later(m_mockBluetoothDevice1) will be connected eventually even
 * though both of them were connected successfully. However, The earlier connected device(m_mockBluetoothDevice2
 * in this case) will be disconnected due to @c BasicDeviceConnectionRule.
 * 2) The observer should be notified device connection twice and disconnection once.
 * 3) Two @c ConnectByDeviceIdsSucceed events, both of which have CLOUD as @c Requester, and one
 * @c DisconnectDevicesSucceed event, should be sent to cloud.
 */
TEST_F(BluetoothTest, test_handleConnectByDeviceIdsDirectiveWithTwoA2DPDevices) {
    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceConnected(_)).Times(Exactly(2));
    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceDisconnected(_)).Times(Exactly(1));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &BluetoothTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockContextManager, setState(BLUETOOTH_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(3));
    std::vector<std::string> events = {CONNECT_BY_DEVICE_IDS_SUCCEEDED, CONNECT_BY_DEVICE_IDS_SUCCEEDED,
                                       DISCONNECT_DEVICES_SUCCEEDED};
    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        // Create ConnectedByDeviceId Directive.
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, CONNECT_BY_DEVICE_IDS_DIRECTIVE, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_CONNECT_BY_DEVICE_IDS_PAYLOAD, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

        /*
         * Mimic the @c DeviceStateChangedEvent which should happen after device connection status changes.
         * In order to guarantee that all @c DeviceStateChangedEvent happen after the corresponding device connection
         * status changes, force the test to wait EVENT_PROCESS_DELAY_MS.
         *
         * TODO: Add send event to @c BluetoothEventBus within @c MockBluetoothDevice.
         */
        std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::CONNECTED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::DISCONNECTED));
    }));

    // Verify the connection result.
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());
    ASSERT_FALSE(m_mockBluetoothDevice2->isConnected());
}

/**
 * Test call to handle ConnectByDeviceIds directive with two matched device UUIDs with different device categories.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2. m_mockBluetoothDevice1
 * belongs to DeviceCategory::PHONE, m_mockBluetoothDevice2 belongs to DeviceCategory::GADGET. Both devices are
 * in disconnect state.
 * Use case:
 * A ConnectByDeviceIds directive is sent down, whose payload contains the mockBluetoothDevice1 and
 * mockBluetoothDevice2.
 * Expected result:
 * 1)Both devices should be connected successfully.
 * 2)The observer should be notified deivce connection twice.
 * 3)Two @c ConnectByDeviceIdsSucceed events, both of which have CLOUD as @c Requester,should be sent to cloud.
 */
TEST_F(BluetoothTest, test_handleConnectByDeviceIdsDirectiveWithOnePhoneOneGadget) {
    m_mockBluetoothDevice1->disconnect();
    m_mockBluetoothDevice2->disconnect();
    // Initially, all devices are stored as DeviceCategory::UNKNOWN. Need to manually update device category in order
    // to test.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::PHONE));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::GADGET));

    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceConnected(_)).Times(Exactly(2));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &BluetoothTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockContextManager, setState(BLUETOOTH_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(2));
    // Verify the @c ConnectByDeviceIdsSucceed event.
    std::vector<std::string> events = {CONNECT_BY_DEVICE_IDS_SUCCEEDED, CONNECT_BY_DEVICE_IDS_SUCCEEDED};
    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        // Create ConnectedByDeviceId Directive.
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, CONNECT_BY_DEVICE_IDS_DIRECTIVE, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_CONNECT_BY_DEVICE_IDS_PAYLOAD, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

        std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::CONNECTED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
    }));

    // Verify the connection result.
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());
    ASSERT_TRUE(m_mockBluetoothDevice2->isConnected());
    // Reset the device category info stored in the database.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::UNKNOWN));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::UNKNOWN));
}

/**
 * Test call to handle ConnectByDeviceProfile directive with an unmatched profile name.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2.
 * m_mockBluetoothDevice1 has @c A2DPSinkInterface and @c AVRCPTargetInterface services.
 * m_mockBluetoothDevice2 has @c HIDInterface and @c SPPInterface services.
 * Use case:
 * A @c ConnectByProfile directive is sent down, which contains @c HFPInterface as a profile.
 * Expected result:
 * 1) m_mockBluetoothDevice1 and m_mockBluetoothDevice2 should not be connected successfully.
 * 2) Because no device is connected, the observer should not be notified.
 * 3) One @c ConnectByDeviceProfileFailed event should be sent to cloud.
 */
TEST_F(BluetoothTest, test_handleConnectByProfileWithUnmatchedProfileName) {
    /*
     * Because We're only connecting devices that have been previously connected and currently paired.
     * Therefore, assume m_mockBluetoothDevice1 and m_mockBluetoothDevice2 are currently paired.
     */
    m_mockBluetoothDevice1->pair();
    m_mockBluetoothDevice2->pair();

    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceConnected(_)).Times(Exactly(0));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &BluetoothTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockContextManager, setState(BLUETOOTH_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
    std::vector<std::string> events = {CONNECT_BY_PROFILE_FAILED};
    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        // Create ConnectedByDeviceId Directive.
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, CONNECT_BY_PROFILE_DIRECTIVE, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_CONNECT_BY_PROFILE_PAYLOAD_1, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    }));

    ASSERT_FALSE(m_mockBluetoothDevice1->isConnected());
    ASSERT_FALSE(m_mockBluetoothDevice2->isConnected());
}

/**
 * Test call to handle ConnectByDeviceProfile directive with a matched profile.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2.
 * m_mockBluetoothDevice1 has @c A2DPSinkInterface and @c AVRCPTargetInterface services.
 * m_mockBluetoothDevice2 has @c HIDInterface and @c SPPInterface services.
 * Use case:
 * A @c ConnectByProfile directive is sent down, which contains @c AVRCPTargetInterface as a profile.
 * Expected result:
 * 1) m_mockBluetoothDevice1 should be connected successfully.
 * 2) m_mockBluetoothDevice2 should not be connected.
 * 3) Because m_mockBluetoothDevice is connected, the observer should be notificed once.
 * 4) One @c ConnectByDeviceProfileSucceeded event should be sent to cloud.
 */
TEST_F(BluetoothTest, test_handleConnectByProfileWithMatchedProfileName) {
    std::mutex waitMutex;
    std::unique_lock<std::mutex> waitLock(waitMutex);

    // Assume devices are paired previously.
    m_mockBluetoothDevice1->pair();
    m_mockBluetoothDevice2->pair();

    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceConnected(_)).Times(Exactly(1));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &BluetoothTest::wakeOnSetCompleted));
        std::vector<std::string> events = {CONNECT_BY_PROFILE_SUCCEEDED};
    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        // Create ConnectedByDeviceId Directive.
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, CONNECT_BY_PROFILE_DIRECTIVE, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_CONNECT_BY_PROFILE_PAYLOAD_2, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

        std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
    }));

    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());
    ASSERT_FALSE(m_mockBluetoothDevice2->isConnected());
}

/**
 * Test call to handle PairDevices directive with matched device UUIDs.
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2.
 * m_mockBluetoothDevice1 belongs to DeviceCategory::PHONE, which follows @c BasicDeviceConnectionRule.
 * m_mockBluetoothDevice2 belongs to DeviceCategory::GADGET, which follows @c GadgetConnectionRule.
 * Use case:
 * A @c PairDevices directive is sent down, whose payload contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2.
 * Expected result:
 * 1) Both devices should be paired successfully.
 * 2) Based on the connection rule (m_mockBluetoothDevice1 follows @c BasicDeviceConnectionRule,
 * m_mockBluetoothDevice2 follows @c GadgetConnectionRule), m_mockBluetoothDevice1 should be connected successfully.
 * 3) A sequence of {PAIR_DEVICES_SUCCEEDED, CONNECT_BY_DEVICE_IDS_SUCCEEDED, PAIR_DEVICES_SUCCEEDED)
 * should be sent to cloud.
 * a. {PAIR_DEVICES_SUCCEEDED, CONNECT_BY_DEVICE_IDS_SUCCEEDED} are sent when Bluetooth CapabilityAgent tries to pair
 * m_mockBluetoothDevice1.
 * b. {PAIR_DEVICES_SUCCEEDED} is sent when Bluetooth CapabilityAgent tries to pair m_mockBluetoothDevice2.
 */
TEST_F(BluetoothTest, DISABLED_test_handlePairDevices) {
    // Initially, all devices are stored as DeviceCategory::UNKNOWN. Need to manually update device category in order
    // to test.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::PHONE));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::GADGET));

    m_messages.insert({PAIR_DEVICES_SUCCEEDED, 0});
    m_messages.insert({CONNECT_BY_DEVICE_IDS_SUCCEEDED, 0});
    std::mutex waitMutex;

    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &BluetoothTest::wakeOnSetCompleted));

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            verifyMessagesCount(request, &m_messages);
            m_messageSentTrigger.notify_one();
        }));

    // Create PairDevices Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, PAIR_DEVICES_DIRECTIVE, TEST_MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEST_PAIR_DEVICES_PAYLOAD, attachmentManager, "");
    // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
    std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
    agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::PAIRED));
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::PAIRED));

    std::unique_lock<std::mutex> lock(waitMutex);
    bool result;
    result = m_messageSentTrigger.wait_for(lock, WAIT_TIMEOUT_MS, [this] {
       for(auto message : m_messages) {
           if (message.first == PAIR_DEVICES_SUCCEEDED) {
               if (message.second != 2) {
                   return false;
               }
           } else if (message.first == CONNECT_BY_DEVICE_IDS_SUCCEEDED) {
               if (message.second != 1) {
                   return false;
               }
           }
       }
       return true;
    });
    ASSERT_TRUE(result);

    // Verify the pair result.
    ASSERT_TRUE(m_mockBluetoothDevice1->isPaired());
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());
    ASSERT_TRUE(m_mockBluetoothDevice2->isPaired());
    ASSERT_FALSE(m_mockBluetoothDevice2->isConnected());

    // Reset device categories.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::UNKNOWN));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::UNKNOWN));
}

/**
 * Test call to handle UnpairDevices directive with matched device UUIDs.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2.
 * m_mockBluetoothDevice1 belongs to DeviceCategory::PHONE, whcih follows @c BasicDeviceConnectionRule.
 * m_mockBluetoothDevice2 belongs to DeviceCategory::GADGET, which follows @c GadgetConnectionRule.
 * Both devices are in paired and connected state.
 * Use case:
 * First a @c PairDevices directive is sent down, whose payload contains m_mockBluetoothDevice1
 * and m_mockBluetoothDevice2.
 * Then a @c UnpairDevices directive is sent down, whose payload contains m_mockBluetoothDevice1
 * and m_mockBluetoothDevice2.
 * Expected result:
 * 1) Both m_mockBluetoothDevice1 and m_mockBluetoothDevice2 should be unpaired successfully.
 * 2) Based on the connection rule, both m_mockBluetoothDevice1 and m_mockBluetoothDevice2 should be diconnected
 * successfully.
 * 3) Events:
 * A sequence of {DISCONNECT_DEVICES_SUCCEEDED, UNPAIR_DEVICES_SUCCEEDED, DISCONNECT_DEVICES_SUCCEEDED,
 * UNPAIR_DEVICES_SUCCEEDED} should be sent to cloud:
 * a. {DISCONNECT_DEVICES_SUCCEEDED, UNPAIR_DEVICES_SUCCEEDED} are sent when Bluetooth CapabilityAgents tries to unpair
 * m_mockBluetoothDevice1.
 * b. {DISCONNECT_DEVICES_SUCCEEDED, UNPAIR_DEVICES_SUCCEEDED} are sent when Bluetooth CapabilityAgents tries to unpair
 * m_mockBluetoothDevice2.
 */
TEST_F(BluetoothTest, test_handleUnpairDevices) {
    // Initially, all devices are stored as DeviceCategory::UNKNOWN. Need to manually update device category in order
    // to test.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::PHONE));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::GADGET));

    // Assume all devices are paired and connected before.
    m_mockBluetoothDevice1->pair();
    m_mockBluetoothDevice1->connect();
    m_mockBluetoothDevice2->pair();
    m_mockBluetoothDevice2->connect();
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());
    ASSERT_TRUE(m_mockBluetoothDevice2->isConnected());

    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceDisconnected(_)).Times(Exactly(2));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &BluetoothTest::wakeOnSetCompleted));
    std::vector<std::string> events = {DISCONNECT_DEVICES_SUCCEEDED, UNPAIR_DEVICES_SUCCEEDED,
                                       DISCONNECT_DEVICES_SUCCEEDED, UNPAIR_DEVICES_SUCCEEDED};

    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, UNPAIR_DEVICES_DIRECTIVE, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_UNPAIR_DEVICES_PAYLOAD, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;

        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

        std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::DISCONNECTED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::UNPAIRED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::DISCONNECTED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::UNPAIRED));
    }));

    // Verify the Unpair result.
    ASSERT_FALSE(m_mockBluetoothDevice1->isPaired());
    ASSERT_FALSE(m_mockBluetoothDevice1->isConnected());
    ASSERT_FALSE(m_mockBluetoothDevice2->isPaired());
    ASSERT_FALSE(m_mockBluetoothDevice2->isConnected());

    // Reset device categories.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::UNKNOWN));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::UNKNOWN));
}

/**
 * Test call to handle DisconnectDevices directive with matched device UUIDs.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2.
 * Both devices are in paired and connected state.
 * Use case:
 * A @c DisconnectDevices directive is sent down, whose payload contains m_mockBluetoothDevice1
 * and m_mockBluetoothDevice2.
 * Expected result:
 * 1) Both m_mockBluetoothDevice1 and m_mockBluetoothDevice2 should be disconnected successfully.
 * 2) The observer should be notified the device disconnection once.
 * 3) Events:
 * A sequnce of {DISCONNECT_DEVICES_SUCCEEDED, DISCONNECT_DEVICES_SUCCEEDED} should be sent to cloud. One for
 * m_mockBluetoothDevice1, one for m_mockBluetoothDevice2.
 */
TEST_F(BluetoothTest, test_handleDisconnectDevices) {
    // Assume all devices are paired and connected before.
    m_mockBluetoothDevice1->pair();
    m_mockBluetoothDevice1->connect();
    m_mockBluetoothDevice2->pair();
    m_mockBluetoothDevice2->connect();
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());
    ASSERT_TRUE(m_mockBluetoothDevice2->isConnected());

    EXPECT_CALL(*m_mockBluetoothDeviceObserver, onActiveDeviceDisconnected(_)).Times(Exactly(2));
    std::vector<std::string> events = {DISCONNECT_DEVICES_SUCCEEDED, DISCONNECT_DEVICES_SUCCEEDED};
    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader=
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, DISCONNECT_DEVICES_DIRECTIVE, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_DISCONNECT_DEVICES_PAYLOAD, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedPromise = std::promise<void>();
        m_wakeSetCompletedFuture = m_wakeSetCompletedPromise.get_future();
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

        std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::DISCONNECTED));
        m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::DISCONNECTED));
    }));

    ASSERT_FALSE(m_mockBluetoothDevice1->isConnected());
    ASSERT_FALSE(m_mockBluetoothDevice2->isConnected());
}

/**
 * Test call to handle SetDeviceCategories directive with matched device UUID.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2. Both of them belong to
 * DeviceCategory::UNKNOWN
 * Use case:
 * A @c SetDeviceCategories directive is sent down, whose payload contains m_mockBluetoothDevice1 (need to be set
 * to DeviceCategory::PHONE) and m_mockBluetoothDevice2 (need to be set to DeviceCategory::GADGET).
 * Expected result:
 * 1) Both devices are set the assigned device category successfully.
 * 2) Events:
 * A sequnce of {SET_DEVICE_CATEGORIES_SUCCEEDED, SET_DEVICE_CATEGORIES_SUCCEEDED} should be sent to cloud. One for
 * m_mockBluetoothDevice1, one for m_mockBluetoothDevice2.
 */
TEST_F(BluetoothTest, test_handleSetDeviceCategories) {
    std::vector<std::string> events = {SET_DEVICE_CATEGORIES_SUCCEEDED};
    ASSERT_TRUE(verifyMessagesSentInOrder(events, [this]() {
        auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
        auto avsMessageHeader=
            std::make_shared<AVSMessageHeader>(NAMESPACE_BLUETOOTH, SET_DEVICE_CATEGORIES, TEST_MESSAGE_ID);
        std::shared_ptr<AVSDirective> directive =
            AVSDirective::create("", avsMessageHeader, TEST_SET_DEVICE_CATEGORIES_PAYLOAD, attachmentManager, "");
        // cast to DirectiveHandlerInterface so the compiler can find the correct preHandleDirective signature
        std::shared_ptr<DirectiveHandlerInterface> agentAsDirectiveHandler = m_Bluetooth;
        agentAsDirectiveHandler->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
        agentAsDirectiveHandler->handleDirective(TEST_MESSAGE_ID);
        m_wakeSetCompletedPromise = std::promise<void>();
        m_wakeSetCompletedFuture = m_wakeSetCompletedPromise.get_future();
        m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

        m_Bluetooth->onContextAvailable(MOCK_CONTEXT);
    }));

    std::string category1;
    std::string category2;
    m_bluetoothStorage->getCategory(TEST_BLUETOOTH_UUID, &category1);
    m_bluetoothStorage->getCategory(TEST_BLUETOOTH_UUID_2, &category2);
    ASSERT_EQ(deviceCategoryToString(DeviceCategory::PHONE), category1);
    ASSERT_EQ(deviceCategoryToString(DeviceCategory::GADGET), category2);

    // Reset device categories.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::UNKNOWN));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::UNKNOWN));
}

TEST_F(BluetoothTest, test_contentDucksUponReceivingBackgroundFocus) {
    // Assume all devices are paired and connected before.
    m_mockBluetoothDevice1->pair();
    m_mockBluetoothDevice1->connect();
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());

    // change streaming state to ACTIVE
    m_eventBus->sendEvent(MediaStreamingStateChangedEvent(avsCommon::utils::bluetooth::MediaStreamingState::ACTIVE, avsCommon::utils::bluetooth::A2DPRole::SOURCE, m_mockBluetoothDevice1));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    // ensure that music is not stopped when ducking.
    EXPECT_CALL(*m_mockBluetoothMediaPlayer, stop(_)).Times(0);
    EXPECT_CALL(*m_mockChannelVolumeInterface, startDucking()).Times(1);
    m_Bluetooth->onFocusChanged(avsCommon::avs::FocusState::BACKGROUND, avsCommon::avs::MixingBehavior::MAY_DUCK);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
}

TEST_F(BluetoothTest, test_contentUnducksUponReceivingForegroundOrNoneFocus) {
    // Assume all devices are paired and connected before.
    m_mockBluetoothDevice1->pair();
    m_mockBluetoothDevice1->connect();
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    ASSERT_TRUE(m_mockBluetoothDevice1->isConnected());

    // change streaming state to ACTIVE.
    m_eventBus->sendEvent(MediaStreamingStateChangedEvent(avsCommon::utils::bluetooth::MediaStreamingState::ACTIVE, avsCommon::utils::bluetooth::A2DPRole::SOURCE, m_mockBluetoothDevice1));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    // ensure that music is not stopped when ducking upon receiving background focus.
    EXPECT_CALL(*m_mockBluetoothMediaPlayer, stop(_)).Times(0);
    EXPECT_CALL(*m_mockChannelVolumeInterface, startDucking()).Times(1);
    m_Bluetooth->onFocusChanged(avsCommon::avs::FocusState::BACKGROUND, avsCommon::avs::MixingBehavior::MAY_DUCK);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    // upon receiving foreground focus , content must be unducked.
    EXPECT_CALL(*m_mockChannelVolumeInterface, stopDucking()).Times(1);
    m_Bluetooth->onFocusChanged(avsCommon::avs::FocusState::FOREGROUND, avsCommon::avs::MixingBehavior::PRIMARY);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    // upon receiving foreground focus , content must be unducked.
    EXPECT_CALL(*m_mockChannelVolumeInterface, stopDucking()).Times(1);
    m_Bluetooth->onFocusChanged(avsCommon::avs::FocusState::NONE, avsCommon::avs::MixingBehavior::MUST_STOP);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
}

/**
 * Test streaming state change of multiple device connections.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice1 and m_mockBluetoothDevice2. m_mockBluetoothDevice1 belongs to
 * DeviceCategory::AUDIO_VIDEO. m_mockBluetoothDevice2 belongs to DeviceCategory::PHONE.
 * Use case:
 * m_mockBluetoothDevice1 initiates connection and starts streaming audio.
 * Then m_mockBluetoothDevice2 initiates connection.
 * Expected Result:
 * 1) m_mockBluetoothDevice1 should connect. A @c StreamingStarted event should be sent.
 * 2) When m_mockBluetoothDevice2 connects, m_mockBluetoothDevice1 should be disconnected. A @c StreamingEnded event
 * should be sent.
 */
TEST_F(BluetoothTest, test_streamingStateChange) {
    // Initially, all devices are stored as DeviceCategory::UNKNOWN. Need to manually update device category in order
    // to test.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::AUDIO_VIDEO));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::PHONE));

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
    .Times(AtLeast(1))
    .WillOnce(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifyMessage(request, STREAMING_STARTED);
    }))
    .WillOnce(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifyMessage(request, STREAMING_ENDED);
    }));

    // m_mockBluetoothDevice1 initiates connection and starts streaming media.
    m_mockBluetoothDevice1->connect();
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice1, DeviceState::CONNECTED));
    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));
    m_eventBus->sendEvent(MediaStreamingStateChangedEvent(
        MediaStreamingState::ACTIVE, A2DPRole::SOURCE, m_mockBluetoothDevice1));
    std::this_thread::sleep_for(std::chrono::milliseconds(EVENT_PROCESS_DELAY_MS));

    // m_mockBluetoothDevice2 initiates connection.
    m_mockBluetoothDevice2->connect();
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice2, DeviceState::CONNECTED));

    // Reset device categories.
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID, deviceCategoryToString(DeviceCategory::UNKNOWN));
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_2, deviceCategoryToString(DeviceCategory::UNKNOWN));
}

/**
 * Test focus state change of barge-in scenario.
 *
 * Assumption:
 * A @c DeviceManager contains m_mockBluetoothDevice3, which belongs to DeviceCategory::PHONE.
 * Use case:
 * 1. m_mockBluetoothDevice3 initiates connection. Stream audio from m_mockBluetoothDevice3.
 * 2. A job(e.g, TTS) barge-in, which moves Bluetooth to background.
 * Exepected Result:
 * 1. m_mockBluetoothDevice3 should connect and start streaming audio. It should acquire focus.
 * 2. m_mockBluetoothDevice3 should pause streaming audio. However, it should NOT release focus.
 */
TEST_F(BluetoothTest, test_focusStateChange) {
    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_3, deviceCategoryToString(DeviceCategory::PHONE));

    EXPECT_CALL(*m_mockFocusManager, acquireChannel(_, _)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockBluetoothMediaPlayer, play(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(_, _)).Times(0);
    EXPECT_CALL(*m_mockBluetoothMediaPlayer, stop(_)).Times(1).WillOnce(Return(true));

    m_mockBluetoothDevice3->connect();
    m_eventBus->sendEvent(DeviceStateChangedEvent(m_mockBluetoothDevice3, DeviceState::CONNECTED));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    m_eventBus->sendEvent(MediaStreamingStateChangedEvent(
        MediaStreamingState::ACTIVE, A2DPRole::SINK, m_mockBluetoothDevice3));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    m_Bluetooth->onFocusChanged(avsCommon::avs::FocusState::FOREGROUND, avsCommon::avs::MixingBehavior::PRIMARY);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    m_Bluetooth->onPlaybackStarted(m_mockBluetoothMediaPlayer->getCurrentSourceId(), {std::chrono::milliseconds(0)});
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    // The other job barges in, which moves Bluetooth to background.
    m_Bluetooth->onFocusChanged(avsCommon::avs::FocusState::BACKGROUND, avsCommon::avs::MixingBehavior::MUST_STOP);
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);
    m_eventBus->sendEvent(MediaStreamingStateChangedEvent(
        MediaStreamingState::IDLE, A2DPRole::SINK, m_mockBluetoothDevice3));
    m_wakeSetCompletedFuture.wait_for(WAIT_TIMEOUT_MS);

    m_bluetoothStorage->updateByCategory(TEST_BLUETOOTH_UUID_3, deviceCategoryToString(DeviceCategory::UNKNOWN));
}
}  // namespace test
}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
