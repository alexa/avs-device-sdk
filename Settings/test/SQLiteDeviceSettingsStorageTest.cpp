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

// clang-format off
/// Test configuration JSON. Initialized at @c main().
static std::string deviceSettingJSON;
// clang-format on

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

class SQLiteDeviceSettingsStorageTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp();

    /// TearDown after each test case.
    void TearDown();

protected:
    /// Open dataabse;
    void openDB();

    /// Cleanup function to close.
    void closeDB();

    /// The database instance. Protected because it needs to be accessed in test cases.
    std::unique_ptr<SQLiteDeviceSettingStorage> m_db;
};

void SQLiteDeviceSettingsStorageTest::closeDB() {
    if (m_db) {
        m_db->close();
    }
    m_db.reset();
}

void SQLiteDeviceSettingsStorageTest::openDB() {
    m_db = SQLiteDeviceSettingStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());
}

void SQLiteDeviceSettingsStorageTest::SetUp() {
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

void SQLiteDeviceSettingsStorageTest::TearDown() {
    configuration::ConfigurationNode::uninitialize();
    closeDB();

    if (fileExists(testDatabase)) {
        remove(testDatabase.c_str());
    }
}

/**
 * Test storing status and values to the database.
 */
TEST_F(SQLiteDeviceSettingsStorageTest, insertUpdateSettingValue) {
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
 * Test removing an entry from the database.
 */
TEST_F(SQLiteDeviceSettingsStorageTest, deleteSetting) {
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
