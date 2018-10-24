/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>
#include "BlueZ/DBusConnection.h"
#include "BlueZ/MPRISPlayer.h"
#include "MockDBusProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::utils::bluetooth;

/**
 * The global dbus connection object for the tests.
 *
 * The connection object is global because the bus name was being lost
 * when we try to create/destroy temporary connection instances for each test case.
 *
 * @c G_BUS_TYPE_SESSION is being used because @c G_BUS_TYPE_SYSTEM has stricter permissions.
 */
static std::shared_ptr<DBusConnection> g_connection = DBusConnection::create(G_BUS_TYPE_SESSION);

/// Unique bus name to allow DBus method invocations.
static const std::string UNIQUE_BUS = "org.avscppsdk.test.unique";

/// The interface name.
static const std::string MPRIS_PLAYER_INTERFACE = "org.mpris.MediaPlayer2.Player";

/// A mock listener for the @c BluetoothEventBus.
class MockListener : public BluetoothEventListenerInterface {
public:
    MOCK_METHOD1(onEventFired, void(const BluetoothEvent& event));
};

/**
 * Test fixture for the base tests. DBus objects are not mockable, so we use live calls to DBus for these tests.
 */
class MPRISPlayerTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    MPRISPlayerTest();

    /// TearDown after each test case.
    virtual ~MPRISPlayerTest();

    /// Creates an instance of the @c MPRISPlayer.
    void init();

    /// Acquires the bus name and then starts the main loop.
    void gMainLoop();

    /// A mock @c DBusProxy object to test with.
    std::shared_ptr<MockDBusProxy> m_mockMedia;

    /// An event bus instance.
    std::shared_ptr<BluetoothEventBus> m_eventBus;

    /// A mock listener for the event bus.
    std::shared_ptr<MockListener> m_mockListener;

    /// An @c MPRISPlayer instance.
    std::unique_ptr<MPRISPlayer> m_player;

    /// Waits for the bus name to be acquired.
    std::promise<void> m_nameAcquiredPromise;

    /// Pointer to the GMainLoop.
    GMainLoop* m_loop;

    /// The id for the bus.
    guint m_busId;
};

void MPRISPlayerTest::init() {
    m_player = MPRISPlayer::create(g_connection, m_mockMedia, m_eventBus);
    ASSERT_THAT(m_player, NotNull());
}

MPRISPlayerTest::MPRISPlayerTest() {
    m_mockMedia = std::make_shared<NiceMock<MockDBusProxy>>();
    m_mockListener = std::shared_ptr<MockListener>(new MockListener());
    m_eventBus = std::make_shared<BluetoothEventBus>();
    m_eventBus->addListener({avsCommon::utils::bluetooth::BluetoothEventType::AVRCP_COMMAND_RECEIVED}, m_mockListener);
}

MPRISPlayerTest::~MPRISPlayerTest() {
    m_player.reset();
    m_eventBus->removeListener(
        {avsCommon::utils::bluetooth::BluetoothEventType::AVRCP_COMMAND_RECEIVED}, m_mockListener);
    m_eventBus.reset();
}

/// Callback passed to GDBus for when a bus name is acquired.
static void onNameAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    MPRISPlayerTest* test = (MPRISPlayerTest*)user_data;
    test->init();
    test->m_nameAcquiredPromise.set_value();
}

/// Callback passed to GDBus for when a bus name is lost.
static void onNameLost(GDBusConnection* connection, const gchar* name, gpointer user_data) {
}

void MPRISPlayerTest::gMainLoop() {
    m_busId = g_bus_own_name_on_connection(
        g_connection->getGDBusConnection(),
        UNIQUE_BUS.c_str(),
        G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
        onNameAcquired,
        onNameLost,
        this,
        nullptr);
    m_loop = g_main_loop_new(NULL, FALSE);

    g_main_loop_run(m_loop);
}

/// Test that the happy create() case succeeds.
TEST_F(MPRISPlayerTest, createSucceeds) {
    init();
    ASSERT_THAT(m_player, NotNull());
}

/// Tests that missing parameters result in a nullptr.
TEST_F(MPRISPlayerTest, createWithNullFails) {
    ASSERT_THAT(MPRISPlayer::create(nullptr, m_mockMedia, m_eventBus), IsNull());
    ASSERT_THAT(MPRISPlayer::create(g_connection, nullptr, m_eventBus), IsNull());
    ASSERT_THAT(MPRISPlayer::create(g_connection, m_mockMedia, nullptr), IsNull());
}

/// Tests that the MPRISPlayer properly registers and unregisters throughout its life time.
TEST_F(MPRISPlayerTest, playerRegistration) {
    Expectation registerPlayer = EXPECT_CALL(*m_mockMedia, callMethod("RegisterPlayer", _, _)).Times(1);
    EXPECT_CALL(*m_mockMedia, callMethod("UnregisterPlayer", _, _)).Times(1).After(registerPlayer);
    init();
    m_player.reset();
}

/*
 * The following tests are for the org.mpris.MediaPlayer2.Player methods that the SDK MPRIS player supports.
 */

/// Parameterized test fixture for supported org.mpris.MediaPlayer2.Player DBus AVRCP Methods.
class MPRISPlayerSupportedAVRCPTest
        : public MPRISPlayerTest
        , public ::testing::WithParamInterface<std::pair<std::string, AVRCPCommand>> {};

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    MPRISPlayerSupportedAVRCPTest,
    ::testing::Values(
        std::make_pair<std::string, AVRCPCommand>("Play", AVRCPCommand::PLAY),
        std::make_pair<std::string, AVRCPCommand>("Pause", AVRCPCommand::PAUSE),
        std::make_pair<std::string, AVRCPCommand>("Next", AVRCPCommand::NEXT),
        std::make_pair<std::string, AVRCPCommand>("Previous", AVRCPCommand::PREVIOUS)),
    // Append method name to the test case name.
    [](testing::TestParamInfo<std::pair<std::string, AVRCPCommand>> info) { return info.param.first; });

/**
 * Custom matcher for AVRCPCommand expectations. This matcher is only used for @c BluetoothEvent objects.
 *
 * Returns true if the event is of type @c AVRCP_COMMAND_RECEIVED and contains the expected @c AVRCPCommand.
 */
MATCHER_P(ContainsAVRCPCommand, /* AVRCPCommand */ cmd, "") {
    // Throw obvious compile error if not a BluetoothEvent.
    const BluetoothEvent* event = &arg;

    return BluetoothEventType::AVRCP_COMMAND_RECEIVED == event->getType() && nullptr != event->getAVRCPCommand() &&
           cmd == *(event->getAVRCPCommand());
}

/// Test that a supported DBus method sends the correct corresponding AVRCPCommand.
TEST_P(MPRISPlayerSupportedAVRCPTest, avrcpCommand) {
    std::string dbusMethodName;
    AVRCPCommand command;

    std::tie(dbusMethodName, command) = GetParam();

    EXPECT_CALL(*m_mockListener, onEventFired(ContainsAVRCPCommand(command))).Times(1);
    auto eventThread = std::thread(&MPRISPlayerTest::gMainLoop, this);
    ASSERT_NE(m_nameAcquiredPromise.get_future().wait_for(std::chrono::seconds(1)), std::future_status::timeout);

    GDBusProxy* proxy = g_dbus_proxy_new_sync(
        g_connection->getGDBusConnection(),
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        UNIQUE_BUS.c_str(),
        MPRISPlayer::MPRIS_OBJECT_PATH.c_str(),
        MPRIS_PLAYER_INTERFACE.c_str(),
        nullptr,
        nullptr);

    g_dbus_proxy_call_sync(proxy, dbusMethodName.c_str(), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    g_bus_unown_name(m_busId);
    g_main_loop_quit(m_loop);
    eventThread.join();
}

/*
 * The following tests are for the org.mpris.MediaPlayer2.Player methods that the SDK MPRIS player does not support.
 */

/// Parameterized test fixture for unsupported DBus AVRCP Methods.
class MPRISPlayerUnsupportedTest
        : public MPRISPlayerTest
        , public ::testing::WithParamInterface<std::string> {};

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    MPRISPlayerUnsupportedTest,
    ::testing::Values("PlayPause", "Stop", "Seek", "SetPosition", "OpenUri"),
    // Append method name to the test case name.
    [](testing::TestParamInfo<std::string> info) { return info.param; });

/// Test that unsupported DBus methods do not send any Bluetooth events.
TEST_P(MPRISPlayerUnsupportedTest, unsupportedMethod) {
    std::string dbusMethodName = GetParam();

    EXPECT_CALL(*m_mockListener, onEventFired(_)).Times(0);
    auto eventThread = std::thread(&MPRISPlayerTest::gMainLoop, this);
    ASSERT_NE(m_nameAcquiredPromise.get_future().wait_for(std::chrono::seconds(1)), std::future_status::timeout);

    GDBusProxy* proxy = g_dbus_proxy_new_sync(
        g_connection->getGDBusConnection(),
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        UNIQUE_BUS.c_str(),
        MPRISPlayer::MPRIS_OBJECT_PATH.c_str(),
        MPRIS_PLAYER_INTERFACE.c_str(),
        nullptr,
        nullptr);

    g_dbus_proxy_call_sync(proxy, dbusMethodName.c_str(), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    g_bus_unown_name(m_busId);
    g_main_loop_quit(m_loop);
    eventThread.join();
}

}  // namespace test
}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
