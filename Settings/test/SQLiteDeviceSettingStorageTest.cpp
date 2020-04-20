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

#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

namespace alexaClientSDK {
namespace settings {
namespace storage {
namespace test {

using namespace ::testing;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace settings;

/// The database file name.
static const std::string TEST_DATABASE_FILE_NAME = "SQLiteDeviceSettingStorageTest.db";

/// The path delimiter used by the OS to identify file locations.
static const std::string PATH_DELIMITER = "/";

/// Test Database file path. Initialized at @c main().
static std::string testDatabase;

/// Test configuration JSON. Initialized at @c main().
static std::string deviceSettingJSON;

/// Error message for when the file already exists.
static std::string fileExistError;

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

class SQLiteDeviceSettingStorageTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp();

    /// TearDown after each test case.
    void TearDown();

protected:
    /// Open database;
    /// @return @c true if it succeed; @c false otherwise.
    bool openDB();

    /// Cleanup function to close.
    void closeDB();

    /// The database instance. Protected because it needs to be accessed in test cases.
    std::unique_ptr<SQLiteDeviceSettingStorage> m_db;
};

void SQLiteDeviceSettingStorageTest::closeDB() {
    if (m_db) {
        m_db->close();
    }
    m_db.reset();
}

bool SQLiteDeviceSettingStorageTest::openDB() {
    m_db = SQLiteDeviceSettingStorage::create(ConfigurationNode::getRoot());
    EXPECT_THAT(m_db, NotNull());
    EXPECT_TRUE(m_db && m_db->open());
    return !Test::HasFailure();
}

void SQLiteDeviceSettingStorageTest::SetUp() {
    // Initialize Global ConfigurationNode with valid value.
    auto json = std::shared_ptr<std::stringstream>(new std::stringstream());
    *json << deviceSettingJSON;
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(json);
    ASSERT_TRUE(ConfigurationNode::initialize(jsonStream));

    if (fileExists(testDatabase)) {
        remove(testDatabase.c_str());
    }
}

void SQLiteDeviceSettingStorageTest::TearDown() {
    configuration::ConfigurationNode::uninitialize();
    closeDB();

    if (fileExists(testDatabase)) {
        remove(testDatabase.c_str());
    }
}

/**
 * Test storing status and values to the database.
 */
TEST_F(SQLiteDeviceSettingStorageTest, insertUpdateSettingValue) {
    const std::string key = "AAAA";
    const std::string value1 = "BBBBB";
    const std::string value2 = "CCCCC";
    const SettingStatus localStatus = SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
    const SettingStatus notAvailableStatus = SettingStatus::NOT_AVAILABLE;
    DeviceSettingStorageInterface::SettingStatusAndValue loadedValue;

    // Open the database.
    openDB();

    // Try loading a non-existent key.
    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, SettingStatus::NOT_AVAILABLE);

    // Store some values.
    ASSERT_TRUE(m_db->storeSetting(key, value1, localStatus));

    // Reopen database to simulate application restarting.
    closeDB();
    openDB();

    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, localStatus);
    ASSERT_EQ(loadedValue.second, value1);

    // Update the value.
    ASSERT_TRUE(m_db->storeSetting(key, value2, localStatus));

    // Reopen database to simulate application restarting.
    closeDB();
    openDB();

    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, localStatus);
    ASSERT_EQ(loadedValue.second, value2);

    // Update just the status.
    ASSERT_TRUE(m_db->updateSettingStatus(key, notAvailableStatus));

    // Reopen database to simulate application restarting.
    closeDB();
    openDB();

    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, notAvailableStatus);
    ASSERT_EQ(loadedValue.second, value2);
}

/**
 * Test that create an entry with non escaped key works.
 */
TEST_F(SQLiteDeviceSettingStorageTest, test_storeSettingWithNonEscapedStringKey) {
    const std::string key = R"(non-escaped'\%$#*?!`"key)";
    const std::string value = "value2";
    const SettingStatus localStatus = SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
    DeviceSettingStorageInterface::SettingStatusAndValue loadedValue;

    // Open the database.
    ASSERT_TRUE(openDB());

    // Check if store succeeds.
    EXPECT_TRUE(m_db->storeSetting(key, value, localStatus));

    loadedValue = m_db->loadSetting(key);
    EXPECT_EQ(loadedValue.first, localStatus);
    EXPECT_EQ(loadedValue.second, value);
}

/**
 * Test that create an entry with non escaped characters works.
 */
TEST_F(SQLiteDeviceSettingStorageTest, test_storeSettingWithNonEscapedStringValue) {
    const std::string key = "key";
    const std::string value = R"(non-escaped'\%$#*?!`"chars)";
    const SettingStatus localStatus = SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
    DeviceSettingStorageInterface::SettingStatusAndValue loadedValue;

    // Open the database.
    ASSERT_TRUE(openDB());

    // Check if store succeeds.
    EXPECT_TRUE(m_db->storeSetting(key, value, localStatus));

    loadedValue = m_db->loadSetting(key);
    EXPECT_EQ(loadedValue.first, localStatus);
    EXPECT_EQ(loadedValue.second, value);
}

/**
 * Test that delete an entry with non escaped key works.
 */
TEST_F(SQLiteDeviceSettingStorageTest, test_deleteSettingWithNonEscapedStringKey) {
    const std::string key = R"(non-escaped'\%$#*?!`"key)";

    // Open the database and add an entry with the given key.
    ASSERT_TRUE(openDB());
    ASSERT_TRUE(m_db->storeSetting(key, "value", SettingStatus::NOT_AVAILABLE));

    // Check if delete succeeds.
    EXPECT_TRUE(m_db->deleteSetting(key));
}

/**
 * Test removing an entry from the database.
 */
TEST_F(SQLiteDeviceSettingStorageTest, deleteSetting) {
    const std::string key = "AAAA";
    const std::string value1 = "BBBBB";
    const SettingStatus localStatus = SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
    DeviceSettingStorageInterface::SettingStatusAndValue loadedValue;

    // Open the database.
    openDB();
    // Store some values.
    ASSERT_TRUE(m_db->storeSetting(key, value1, localStatus));

    // Reopen database to simulate application restarting.
    closeDB();
    openDB();

    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, localStatus);
    ASSERT_EQ(loadedValue.second, value1);

    // Delete the setting
    m_db->deleteSetting(key);

    // Try to retrieve the setting.
    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, SettingStatus::NOT_AVAILABLE);
}

/**
 * Test replace values of multiple rows.
 */
TEST_F(SQLiteDeviceSettingStorageTest, replaceMultipleEntries) {
    const std::string key = "AAAA";
    const std::string key2 = "KKKK";
    const std::string value1 = "BBBBB";
    const std::string value2 = "CCCCC";
    const std::string value3 = "DDDDD";
    const SettingStatus localStatus = SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
    const SettingStatus notAvailableStatus = SettingStatus::NOT_AVAILABLE;
    DeviceSettingStorageInterface::SettingStatusAndValue loadedValue;

    // Open the database.
    openDB();

    // Try loading a non-existent key.
    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, SettingStatus::NOT_AVAILABLE);

    // Store some values.
    // key = value1, localStatus
    // key2 = value1, localStatus
    ASSERT_TRUE(m_db->storeSetting(key, value1, localStatus));
    ASSERT_TRUE(m_db->storeSetting(key2, value1, localStatus));

    // Now do multiple replace
    const std::vector<std::tuple<std::string, std::string, SettingStatus>> newData = {
        {std::make_tuple(key, value2, notAvailableStatus)}, {std::make_tuple(key2, value3, notAvailableStatus)}};

    m_db->storeSettings(newData);

    loadedValue = m_db->loadSetting(key);
    ASSERT_EQ(loadedValue.first, notAvailableStatus);
    ASSERT_EQ(loadedValue.second, value2);

    loadedValue = m_db->loadSetting(key2);
    ASSERT_EQ(loadedValue.first, notAvailableStatus);
    ASSERT_EQ(loadedValue.second, value3);
}

}  // namespace test
}  // namespace storage
}  // namespace settings
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0]) << " <absolute path to test database file>" << std::endl;
    } else {
        alexaClientSDK::settings::storage::test::testDatabase =
            std::string(argv[1]) + alexaClientSDK::settings::storage::test::PATH_DELIMITER +
            alexaClientSDK::settings::storage::test::TEST_DATABASE_FILE_NAME;

        // clang-format off
        alexaClientSDK::settings::storage::test::deviceSettingJSON = R"(
        {
           "deviceSettings" : {
                "databaseFilePath":")" + alexaClientSDK::settings::storage::test::testDatabase + R"("
             }
        }
        )";
        // clang-format on

        alexaClientSDK::settings::storage::test::fileExistError =
            "Database File " + alexaClientSDK::settings::storage::test::testDatabase + " already exists.";

        return RUN_ALL_TESTS();
    }
}
