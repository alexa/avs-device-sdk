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

#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <Bluetooth/SQLiteBluetoothStorage.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {
namespace test {

using namespace ::testing;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;

/// Test Database file name. Can be changed if there are conflicts.
static const std::string TEST_DATABASE = "SQLiteBluetoothStorageTestDatabase.db";

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

/// Test MAC Address.
static const std::string TEST_MAC = "01:23:45:67:89:ab";

/// Second Test MAC Address.
static const std::string TEST_MAC_2 = "11:23:45:67:89:ab";

/// Test UUID.
static const std::string TEST_UUID = "650f973b-c2ab-4c6e-bff4-3788cd521340";

/// Second Test UUID.
static const std::string TEST_UUID_2 = "750f973b-c2ab-4c6e-bff4-3788cd521340";

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

class SQLiteBluetoothStorageTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp();

    /// TearDown after each test case.
    void TearDown();

protected:
    /// Cleanup function to close and delete the database.
    void closeAndDeleteDB();

    /**
     * Helper function that abstracts the test logic for getOrderedMac teste cases.
     *
     * @param ascending Whether we're testing the ascending or descending case.
     */
    void getOrderedMacHelper(bool ascending);

    /**
     * Helper function that abstracts the logic for getMacToUuid and getUuidToMac test cases.
     *
     * @param retrieveRows Either the getMacToUuid or getUuidToMac function.
     * @param macToUuids A map of macToUuids to initialize the database with.
     * @param expected The expected results.
     */
    void getRowsHelper(
        std::function<bool(SQLiteBluetoothStorage&, std::unordered_map<std::string, std::string>*)> retrieveRows,
        const std::unordered_map<std::string, std::string>& macToUuids,
        const std::unordered_map<std::string, std::string>& expected);

    /**
     * Helper function that abstracts the logic for getMac and getUuid test cases.
     *
     * @param retrieveValue Either the getMac or getUuid function.
     * @param key The key (either the mac or uuid).
     * @param expectedValue The expected value (either the mac or uuid).
     * @param macToUuids A map of macToUuids to initialize the database with.
     */
    void getMacOrUuidHelper(
        std::function<bool(SQLiteBluetoothStorage&, const std::string&, std::string*)> retrieveValue,
        const std::string& key,
        const std::string& expectedValue,
        const std::unordered_map<std::string, std::string>& macToUuids);

    /// The database instance. Protected because it needs to be accessed in test cases.
    std::unique_ptr<SQLiteBluetoothStorage> m_db;
};

void SQLiteBluetoothStorageTest::closeAndDeleteDB() {
    if (m_db) {
        m_db->close();
    }

    m_db.reset();
    if (fileExists(TEST_DATABASE)) {
        remove(TEST_DATABASE.c_str());
    }
}

void SQLiteBluetoothStorageTest::SetUp() {
    // Ensure the db file does not exist already. We don't want to overwrite anything.
    if (fileExists(TEST_DATABASE)) {
        ADD_FAILURE() << FILE_EXISTS_ERROR;
        exit(1);
    }

    // Initialize Global ConfigurationNode with valid value.
    auto json = std::shared_ptr<std::stringstream>(new std::stringstream());
    *json << BLUETOOTH_JSON;
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(json);
    ASSERT_TRUE(ConfigurationNode::initialize(jsonStream));

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());

    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->createDatabase());
}

void SQLiteBluetoothStorageTest::TearDown() {
    configuration::ConfigurationNode::uninitialize();
    closeAndDeleteDB();
}

void SQLiteBluetoothStorageTest::getOrderedMacHelper(bool ascending) {
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC_2, TEST_UUID_2));

    std::list<std::string> expected;
    if (ascending) {
        expected.push_back(TEST_MAC);
        expected.push_back(TEST_MAC_2);
    } else {
        expected.push_back(TEST_MAC_2);
        expected.push_back(TEST_MAC);
    }

    std::list<std::string> rows;
    ASSERT_TRUE(m_db->getOrderedMac(ascending, &rows));
    ASSERT_THAT(rows, Eq(expected));
}

void SQLiteBluetoothStorageTest::getRowsHelper(
    std::function<bool(SQLiteBluetoothStorage&, std::unordered_map<std::string, std::string>*)> retrieveRows,
    const std::unordered_map<std::string, std::string>& macToUuids,
    const std::unordered_map<std::string, std::string>& expected) {
    for (const auto& macAndUuid : macToUuids) {
        ASSERT_TRUE(m_db->insertByMac(macAndUuid.first, macAndUuid.second));
    }

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(retrieveRows(*m_db, &rows));
    ASSERT_THAT(rows, Eq(expected));
}

void SQLiteBluetoothStorageTest::getMacOrUuidHelper(
    std::function<bool(SQLiteBluetoothStorage&, const std::string&, std::string*)> retrieveValue,
    const std::string& key,
    const std::string& expectedValue,
    const std::unordered_map<std::string, std::string>& macToUuids) {
    for (const auto& macAndUuid : macToUuids) {
        ASSERT_TRUE(m_db->insertByMac(macAndUuid.first, macAndUuid.second));
    }

    std::string value;
    ASSERT_TRUE(retrieveValue(*m_db, key, &value));
    ASSERT_THAT(value, Eq(expectedValue));
}

/// Tests the create function with an invalid root.
TEST_F(SQLiteBluetoothStorageTest, createInvalidConfigurationRoot) {
    ConfigurationNode::uninitialize();
    std::vector<std::shared_ptr<std::istream>> empty;
    ConfigurationNode::initialize(empty);

    ASSERT_THAT(SQLiteBluetoothStorage::create(ConfigurationNode::getRoot()), IsNull());
}

/// Tests creating a database object.
TEST_F(SQLiteBluetoothStorageTest, createValidConfigurationRoot) {
    // SQLite allows simultaneous access to the database.
    ASSERT_THAT(SQLiteBluetoothStorage::create(ConfigurationNode::getRoot()), NotNull());
}

/// Test creating a valid DB. This is implicitly tested in the SetUp() function, but we formally test it here.
TEST_F(SQLiteBluetoothStorageTest, createDatabaseSucceeds) {
    closeAndDeleteDB();
    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());

    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->createDatabase());
}

/// Test that creating an existing DB fails.
TEST_F(SQLiteBluetoothStorageTest, createExistingDatabaseFails) {
    ASSERT_FALSE(m_db->createDatabase());
}

/// Test opening an existing database.
TEST_F(SQLiteBluetoothStorageTest, openExistingDatabaseSucceeds) {
    m_db->close();
    ASSERT_TRUE(m_db->open());
}

/// Test clearing the table with one row.
TEST_F(SQLiteBluetoothStorageTest, clearOnOneRowSucceeds) {
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->clear());
    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getUuidToMac(&rows));
    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test clearing the table with multiple rows.
TEST_F(SQLiteBluetoothStorageTest, clearOnMultipleRowsSucceeds) {
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC_2, TEST_UUID_2));
    ASSERT_TRUE(m_db->clear());
    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getUuidToMac(&rows));
    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test clearing the table when it's already empty.
TEST_F(SQLiteBluetoothStorageTest, clearOnEmptySucceeds) {
    ASSERT_TRUE(m_db->clear());
    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getUuidToMac(&rows));
    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test getUuid with one row containing UUID.
TEST_F(SQLiteBluetoothStorageTest, getUuidWithOneSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getMacOrUuidHelper(&SQLiteBluetoothStorage::getUuid, TEST_MAC, TEST_UUID, data);
}

/// Test getUuid with multiple rows, one of which contains the UUID.
TEST_F(SQLiteBluetoothStorageTest, getUuidWithMultipleSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getMacOrUuidHelper(&SQLiteBluetoothStorage::getUuid, TEST_MAC, TEST_UUID, data);
}

/// Test getUuid with no matching UUID.
TEST_F(SQLiteBluetoothStorageTest, getUuidNoMatchingFails) {
    std::string uuid;
    ASSERT_FALSE(m_db->getUuid(TEST_MAC, &uuid));
}

/// Test getMac with one row containing MAC.
TEST_F(SQLiteBluetoothStorageTest, getMacWithOneSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getMacOrUuidHelper(&SQLiteBluetoothStorage::getMac, TEST_UUID, TEST_MAC, data);
}

/// Test getMac with multiple rows, one of which contains the MAC.
TEST_F(SQLiteBluetoothStorageTest, getMacWithMultipleSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getMacOrUuidHelper(&SQLiteBluetoothStorage::getMac, TEST_UUID, TEST_MAC, data);
}

/// Test getMac with no matching MAC.
TEST_F(SQLiteBluetoothStorageTest, getMacNoMatchingFails) {
    std::string mac;
    ASSERT_FALSE(m_db->getMac(TEST_UUID, &mac));
}

/// Test getMacToUuid with one row.
TEST_F(SQLiteBluetoothStorageTest, getMacToUuidWithOneRowSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToUuid, data, data);
}

/// Test getMacToUuid with multiple expected.
TEST_F(SQLiteBluetoothStorageTest, getMacToUuidWithMultipleRowsSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToUuid, data, data);
}

/// Test getMacToUuid when empty.
TEST_F(SQLiteBluetoothStorageTest, getMacToUuidWithEmptySucceeds) {
    std::unordered_map<std::string, std::string> data;
    getRowsHelper(&SQLiteBluetoothStorage::getMacToUuid, data, data);
}

/// Test getUuidToMac with one row.
TEST_F(SQLiteBluetoothStorageTest, getUuidToMacWithOneRowSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_MAC}};

    getRowsHelper(&SQLiteBluetoothStorage::getUuidToMac, data, expected);
}

/// Test getUuidToMac with multiple expected.
TEST_F(SQLiteBluetoothStorageTest, getUuidToMacWithMultipleRowsSucceeds) {
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_MAC}, {TEST_UUID_2, TEST_MAC_2}};
    getRowsHelper(&SQLiteBluetoothStorage::getUuidToMac, data, expected);
}

/// Test getUuidToMac when empty.
TEST_F(SQLiteBluetoothStorageTest, getUuidToMacWithEmptySucceeds) {
    std::unordered_map<std::string, std::string> data;
    getRowsHelper(&SQLiteBluetoothStorage::getUuidToMac, data, data);
}

/// Test getOrderedMac and retrieve the macs in ascending insertion order (oldest first).
TEST_F(SQLiteBluetoothStorageTest, getOrderedMacAscending) {
    getOrderedMacHelper(true);
}

/// Test getOrderedMac and retrieve the macs in descending insertion order (newest first).
TEST_F(SQLiteBluetoothStorageTest, getOrderedMacDescending) {
    getOrderedMacHelper(false);
}

/// Test insertByMac succeeds.
TEST_F(SQLiteBluetoothStorageTest, insertByMacSucceeds) {
    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UUID}};

    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));
    ASSERT_THAT(rows, Eq(expected));
}

/// Test insertByMac existing fails.
TEST_F(SQLiteBluetoothStorageTest, insertByMacDuplicateFails) {
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_FALSE(m_db->insertByMac(TEST_MAC, TEST_UUID, false));
}

/// Test insertByMac with overwrite succeeds.
TEST_F(SQLiteBluetoothStorageTest, insertByMacOverwriteSucceeds) {
    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UUID}};

    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID_2));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));
    ASSERT_THAT(rows, Eq(expected));
}

/// Test remove succeeds.
TEST_F(SQLiteBluetoothStorageTest, removeExistingSucceeds) {
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->remove(TEST_MAC));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));

    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test remove on non-existing record succeeds.
TEST_F(SQLiteBluetoothStorageTest, removeNonExistingSucceeds) {
    ASSERT_TRUE(m_db->remove(TEST_MAC));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));

    ASSERT_THAT(rows.size(), Eq(0U));
}

}  // namespace test
}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
