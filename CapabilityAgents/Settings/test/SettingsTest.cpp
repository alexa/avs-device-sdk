/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file SettingsTest.cpp

#include <memory>
#include <sstream>
#include <iterator>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/GlobalSettingsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "Settings/Settings.h"
#include "Settings/SettingsUpdatedEventSender.h"
#include "Settings/SQLiteSettingStorage.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {
namespace test {

using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json::jsonUtils;
using namespace ::testing;

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the namespace field of a message header.
static const std::string MESSAGE_NAMESPACE_KEY = "namespace";

/// JSON key for the name field of a message header.
static const std::string MESSAGE_NAME_KEY = "name";

/// JSON key for the message ID field of a message header.
static const std::string MESSAGE_ID_KEY = "messageId";

/// JSON key for the payload section of an message.
static const std::string MESSAGE_PAYLOAD_KEY = "payload";

/// JSON key for the settings array filed of the message.
static const std::string MESSAGE_SETTINGS_KEY = "settings";

/// The namespace for this event.
static const std::string SETTINGS_NAMESPACE = "Settings";

/// JSON value for a SettingsUpdated event's name.
static const std::string SETTINGS_UPDATED_EVENT_NAME = "SettingsUpdated";

/// JSON value for the settings array's key.
static const std::string SETTINGS_KEY = "key";

/// JSON value for the settings array's value.
static const std::string SETTINGS_VALUE = "value";

/// JSON text for settings config values for initialization of settings Object.
// clang-format off
static const std::string SETTINGS_CONFIG_JSON =
    "{"
      "\"settings\":{"
        "\"databaseFilePath\":\"settingsUnitTest.db\","
          "\"defaultAVSClientSettings\":{"
          "\"locale\":\"en-GB\""
          "}"
        "}"
    "}";
// clang-format on

/**
 * Utility function to determine if the storage component is opened.
 *
 * @param storage The storage component to check.
 * @return True if the storage component's underlying database is opened, false otherwise.
 */
static bool isOpen(const std::shared_ptr<SettingsStorageInterface>& storage) {
    std::unordered_map<std::string, std::string> dummyMapOfSettings;
    return storage->load(&dummyMapOfSettings);
}

/**
 * This class allows us to test SingleSettingObserver interaction.
 */
class MockSingleSettingObserver : public SingleSettingObserverInterface {
public:
    MockSingleSettingObserver() {
    }
    MOCK_METHOD2(onSettingChanged, void(const std::string& key, const std::string& value));
};

/**
 * This class allows us to test GlobalSettingsObserver interaction.
 */
class MockGlobalSettingsObserver : public GlobalSettingsObserverInterface {
public:
    MockGlobalSettingsObserver() {
    }
    MOCK_METHOD1(onSettingChanged, void(const std::unordered_map<std::string, std::string>& mapOfSettings));
};

/**
 * Utility class to take the parameters of SettingsUpdated event and provide functionalities
 * to verify the event being sent.
 */
class SettingsVerifyTest {
public:
    /**
     * Constructs an object which captures <key, value> pair of settings in the map.
     */
    SettingsVerifyTest(std::unordered_map<std::string, std::string> mapOfSettings);

    /**
     * This function verifies that the JSON content of SettingsUpdated event @c MessageRequest is correct.
     * This function signature matches that of @c MessageSenderInterface::sendMessage() so that an
     * @c ON_CALL() can @c Invoke() this function directly.
     *
     * @param request The @c MessageRequest to verify.
     */
    void verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request);

private:
    /// The map of settings which holds the <key, value> pair of settings sent to AVS.
    std::unordered_map<std::string, std::string> m_mapOfSettings;
};

SettingsVerifyTest::SettingsVerifyTest(std::unordered_map<std::string, std::string> mapOfSettings) :
        m_mapOfSettings{mapOfSettings} {
}

void SettingsVerifyTest::verifyMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent().c_str());
    EXPECT_FALSE(document.HasParseError())
        << "rapidjson detected a parsing error at offset:" << std::to_string(document.GetErrorOffset())
        << ", error message: " << GetParseError_En(document.GetParseError());

    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    ASSERT_NE(event, document.MemberEnd());
    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    ASSERT_NE(header, event->value.MemberEnd());
    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    ASSERT_NE(payload, event->value.MemberEnd());
    auto settings = payload->value.FindMember(MESSAGE_SETTINGS_KEY);
    ASSERT_NE(settings, payload->value.MemberEnd());

    std::string temp_string = "";
    ASSERT_TRUE(retrieveValue(header->value, MESSAGE_NAMESPACE_KEY, &temp_string));
    EXPECT_EQ(temp_string, SETTINGS_NAMESPACE);
    ASSERT_TRUE(retrieveValue(header->value, MESSAGE_NAME_KEY, &temp_string));
    EXPECT_EQ(temp_string, SETTINGS_UPDATED_EVENT_NAME);
    ASSERT_TRUE(retrieveValue(header->value, MESSAGE_ID_KEY, &temp_string));
    ASSERT_NE(temp_string, "");
    ASSERT_TRUE(jsonArrayExists(payload->value, MESSAGE_SETTINGS_KEY));
    rapidjson::Value& array = settings->value;

    ASSERT_TRUE(array.Size() == m_mapOfSettings.size());
    rapidjson::SizeType i;
    std::unordered_map<std::string, std::string>::const_iterator j;
    for (i = array.Size() - 1, j = m_mapOfSettings.begin(); j != m_mapOfSettings.end(); i--, j++) {
        auto key = array[i].FindMember(SETTINGS_KEY);
        ASSERT_NE(key, array[i].MemberEnd());
        auto value = array[i].FindMember(SETTINGS_VALUE);
        ASSERT_NE(value, array[i].MemberEnd());
        ASSERT_TRUE(retrieveValue(array[i], SETTINGS_KEY, &temp_string));
        EXPECT_EQ(temp_string, j->first);
        ASSERT_TRUE(retrieveValue(array[i], SETTINGS_VALUE, &temp_string));
        EXPECT_EQ(temp_string, j->second);
    }
}

/// Test harness for @c Settings class.
class SettingsTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    /**
     * Function to send a SettingsUpdated event and verify that it succeeds. Parameters are passed through
     * @c SettingsVerifyTest::SettingsVerifyTest().
     *
     * @param key The name of the setting to change.
     * @param value The value of the setting to which it has to be set.
     * @return @c true if the SettingsUpdated event was sent correctly, else @c false.
     */
    bool testChangeSettingSucceeds(const std::string& key, const std::string& value);
    /// The mock @c MessageSenderInterface.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;
    /// Global Observer for all the settings which sends the event.
    std::shared_ptr<SettingsUpdatedEventSender> m_settingsEventSender;
    /// Settings Storage object.
    std::shared_ptr<SQLiteSettingStorage> m_storage;
    /// The @c Settings Object to test.
    std::shared_ptr<Settings> m_settingsObject;
    /// The object which verifies the json of message sent.
    std::shared_ptr<SettingsVerifyTest> m_settingsVerifyObject;
    /// The map which stores all the settings for the object.
    std::unordered_map<std::string, std::string> m_mapOfSettings;
    /// The data manager required to build the base object
    std::shared_ptr<registrationManager::CustomerDataManager> m_dataManager;
};

void SettingsTest::SetUp() {
    m_dataManager = std::make_shared<registrationManager::CustomerDataManager>();
    std::istringstream inString(SETTINGS_CONFIG_JSON);
    ASSERT_TRUE(AlexaClientSDKInit::initialize({&inString}));
    m_mockMessageSender = std::make_shared<MockMessageSender>();
    m_settingsEventSender = SettingsUpdatedEventSender::create(m_mockMessageSender);
    ASSERT_NE(m_settingsEventSender, nullptr);
    m_storage = std::make_shared<SQLiteSettingStorage>("settingsUnitTest.db");
    m_settingsObject = Settings::create(m_storage, {m_settingsEventSender}, m_dataManager);
    ASSERT_NE(m_settingsObject, nullptr);
    ASSERT_TRUE(m_storage->load(&m_mapOfSettings));
}

void SettingsTest::TearDown() {
    m_mapOfSettings.clear();
    m_storage->clearDatabase();
    AlexaClientSDKInit::uninitialize();
}

bool SettingsTest::testChangeSettingSucceeds(const std::string& key, const std::string& value) {
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool done = false;

    ON_CALL(*m_mockMessageSender, sendMessage(_))
        .WillByDefault(DoAll(
            Invoke([this, key, value](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
                m_mapOfSettings[key] = value;
                m_settingsVerifyObject = std::make_shared<SettingsVerifyTest>(m_mapOfSettings);
                m_settingsVerifyObject->verifyMessage(request);
            }),
            InvokeWithoutArgs([&] {
                std::lock_guard<std::mutex> lock(mutex);
                done = true;
                conditionVariable.notify_one();
            })));
    m_settingsObject->changeSetting(key, value);
    std::unique_lock<std::mutex> lock(mutex);
    return conditionVariable.wait_for(lock, std::chrono::seconds(1), [&done] { return done; });
}

/**
 * Test to verify the @c create function of @c Settings class.
 */
TEST_F(SettingsTest, createTest) {
    ASSERT_EQ(
        nullptr,
        m_settingsObject->create(
            m_storage, std::unordered_set<std::shared_ptr<GlobalSettingsObserverInterface>>(), m_dataManager));
    ASSERT_EQ(nullptr, m_settingsObject->create(nullptr, {m_settingsEventSender}, m_dataManager));
    ASSERT_EQ(nullptr, m_settingsObject->create(m_storage, {nullptr}, m_dataManager));
}

/**
 * Test to verify if by adding a global observer and changing the setting,
 * the global observer is notified of the change. It also verifies that event is being sent in correct JSON format.
 */
TEST_F(SettingsTest, addGlobalSettingsObserverTest) {
    std::shared_ptr<MockGlobalSettingsObserver> mockGlobalSettingObserver;
    mockGlobalSettingObserver = std::make_shared<MockGlobalSettingsObserver>();
    m_settingsObject->addGlobalSettingsObserver(mockGlobalSettingObserver);
    EXPECT_CALL(*mockGlobalSettingObserver, onSettingChanged(_)).Times(1);
    ASSERT_TRUE(testChangeSettingSucceeds("locale", "en-US"));
}

/**
 * Test to verify if by removing a global observer and changing the setting,
 * the global observer is not notified of the change. It also verifies that event is being sent in correct JSON format.
 */
TEST_F(SettingsTest, removeGlobalSettingsObserverTest) {
    std::shared_ptr<MockGlobalSettingsObserver> mockGlobalSettingObserver;
    mockGlobalSettingObserver = std::make_shared<MockGlobalSettingsObserver>();
    m_settingsObject->removeGlobalSettingsObserver(mockGlobalSettingObserver);
    EXPECT_CALL(*mockGlobalSettingObserver, onSettingChanged(_)).Times(0);
    ASSERT_TRUE(testChangeSettingSucceeds("locale", "en-US"));
}

/**
 * Test to verify @c addSingleSettingObserver function. This test verifies if the setting key is invalid
 * i.e. it doesn't exist in the list of @c SETTINGS_ACCEPTED_KEYS, the observer corresponding to it
 * will not be added.
 */
TEST_F(SettingsTest, addSingleSettingObserverWithInvalidKeyTest) {
    std::shared_ptr<MockSingleSettingObserver> wakewordObserver;
    wakewordObserver = std::make_shared<MockSingleSettingObserver>();
    m_settingsObject->addSingleSettingObserver("wakeword", wakewordObserver);
    EXPECT_CALL(*wakewordObserver, onSettingChanged(_, _)).Times(0);
    ASSERT_FALSE(testChangeSettingSucceeds("wakeword", "Alexa"));
    std::shared_ptr<MockSingleSettingObserver> localeObserver;
    localeObserver = std::make_shared<MockSingleSettingObserver>();
    m_settingsObject->addSingleSettingObserver("local", localeObserver);
    EXPECT_CALL(*localeObserver, onSettingChanged(_, _)).Times(0);
    ASSERT_FALSE(testChangeSettingSucceeds("local", "en-US"));
}

/**
 * Test to verify the function @c removeSingleSettingObserver. The test checks that if an observer is added for
 * an invalid key even if it is a typo, the observer will not be removed.
 * It also verifies that event is being sent in correct JSON format.
 */
TEST_F(SettingsTest, removeSingleSettingObserverWithInvalidKeyTest) {
    std::shared_ptr<MockSingleSettingObserver> localeObserver;
    localeObserver = std::make_shared<MockSingleSettingObserver>();
    m_settingsObject->addSingleSettingObserver("locale", localeObserver);
    m_settingsObject->removeSingleSettingObserver("local", localeObserver);
    EXPECT_CALL(*localeObserver, onSettingChanged(_, _)).Times(1);
    ASSERT_TRUE(testChangeSettingSucceeds("locale", "en-US"));
}

/**
 * Test verifies that if an observer is removed with a valid key, the observer gets removed
 * and is not notified when the setting changes. It also verifies that event is being sent in correct JSON format.
 */
TEST_F(SettingsTest, removeSingleSettingObserverWithCorrectKeyTest) {
    std::shared_ptr<MockSingleSettingObserver> localeObserver;
    localeObserver = std::make_shared<MockSingleSettingObserver>();
    m_settingsObject->addSingleSettingObserver("locale", localeObserver);
    m_settingsObject->removeSingleSettingObserver("locale", localeObserver);
    EXPECT_CALL(*localeObserver, onSettingChanged(_, _)).Times(0);
    ASSERT_TRUE(testChangeSettingSucceeds("locale", "en-US"));
}

/**
 * Test to check if the settings loaded from the database are same as the default settings.
 */
TEST_F(SettingsTest, defaultSettingsCorrect) {
    std::string DEFAULT_SETTINGS = "defaultAVSClientSettings";
    std::string settings_json = "";
    retrieveValue(SETTINGS_CONFIG_JSON, MESSAGE_SETTINGS_KEY, &settings_json);
    std::string default_settings = "";
    retrieveValue(settings_json, DEFAULT_SETTINGS, &default_settings);
    rapidjson::Document document;
    ASSERT_TRUE(parseJSON(default_settings, &document));
    std::string value = "";
    for (auto& it : m_mapOfSettings) {
        auto key = document.FindMember(it.first);
        ASSERT_NE(key, document.MemberEnd());
        ASSERT_TRUE(retrieveValue(document, it.first, &value));
        EXPECT_EQ(it.second, value);
    }
}

/**
 * Test to check that @c clearData() removes any setting stored in the database.
 */
TEST_F(SettingsTest, clearDataTest) {
    ASSERT_TRUE(testChangeSettingSucceeds("locale", "en-CA"));
    m_settingsObject->clearData();

    std::unordered_map<std::string, std::string> tempMap;
    ASSERT_TRUE(m_storage->load(&tempMap));
    ASSERT_TRUE(tempMap.empty());
}

/**
 * Test to check clear database works as expected.
 */
TEST_F(SettingsTest, clearDatabaseTest) {
    std::unordered_map<std::string, std::string> tempMap;
    ASSERT_TRUE(m_storage->clearDatabase());
    ASSERT_TRUE(m_storage->load(&tempMap));
    ASSERT_TRUE(tempMap.empty());
}

/**
 * Test to check the store function of SQLiteSettingStorage class.
 */
TEST_F(SettingsTest, storeDatabaseTest) {
    ASSERT_TRUE(m_storage->clearDatabase());
    std::map<std::string, std::string> MapToStore = {{"wakeword", "Alexa"}, {"locale", "en-US"}};
    for (auto& it : MapToStore) {
        ASSERT_TRUE(m_storage->store(it.first, it.second));
    }
    std::unordered_map<std::string, std::string> LoadMap;
    ASSERT_TRUE(m_storage->load(&LoadMap));
    ASSERT_EQ(LoadMap.size(), MapToStore.size());
    ASSERT_TRUE(std::equal(MapToStore.rbegin(), MapToStore.rend(), LoadMap.begin()));
}

/**
 * Test to check the modify function of SQLiteSettingStorage class.
 */
TEST_F(SettingsTest, modifyDatabaseTest) {
    ASSERT_TRUE(m_storage->modify("locale", "en-US"));
    ASSERT_FALSE(m_storage->modify("local", "en-GB"));
    ASSERT_TRUE(m_storage->clearDatabase());
    ASSERT_FALSE(m_storage->settingExists("locale"));
}
/**
 * Test to check the erase function of SQLiteSettingStorage class.
 */
TEST_F(SettingsTest, eraseTest) {
    ASSERT_TRUE(m_storage->erase("locale"));
    ASSERT_FALSE(m_storage->settingExists("locale"));
    ASSERT_FALSE(m_storage->erase("local"));
}

/**
 * Test to check the createDatabase function of SQLiteSettingStorage class.
 */
TEST_F(SettingsTest, createDatabaseTest) {
    m_storage->close();
    ASSERT_FALSE(m_storage->createDatabase());
}

/**
 * Test to check the open and close functions of SQLiteSettingStorage class.
 */
TEST_F(SettingsTest, openAndCloseDatabaseTest) {
    ASSERT_FALSE(m_storage->open());
    ASSERT_TRUE(isOpen(m_storage));
    m_storage->close();
    ASSERT_TRUE(m_storage->open());
    ASSERT_TRUE(isOpen(m_storage));
}
}  // namespace test
}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 1) {
        std::cerr << "USAGE: " << std::string(argv[0]) << std::endl;
        return 1;
    }
    return RUN_ALL_TESTS();
}
