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
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include "acsdkBluetooth/SQLiteBluetoothStorage.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {
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

/// Test Unknown MAC/Category.
static const std::string TEST_UNKNOWN = "UNKNOWN";

/// Test Other Category.
static const std::string TEST_OTHER = "OTHER";

/// Test Phone Category.
static const std::string TEST_PHONE = "PHONE";

/// Table name.
static const std::string UUID_TABLE_NAME = "uuidMapping";

/// The UUID column.
static const std::string COLUMN_UUID = "uuid";

/// The MAC address column.
static const std::string COLUMN_MAC = "mac";

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

    /// Function to create legacy database.
    bool createLegacyDatabase();

    /// Insert Entry for legacy database.
    bool insertEntryLegacy(const std::string& uuid, const std::string& mac);

    /**
     * Helper function to setup database.
     *
     * @param migrated Whether using migrated database.
     * @return bool Whether database setup successful.
     */
    bool setupDatabase(bool migratedDatabase);

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
     * Helper function that abstracts the logic for insertByMac given a macToUuids map and
     * verifies the expected value with the one returned by the retrieveValue function.
     *
     * @param retrieveValue function to retrieve value from database (getMac/getUuid/getCategory).
     * @param key The key (either the mac or uuid).
     * @param expectedValue The expected value from the given retrieveValue function call.
     * @param macToUuids A map of macToUuids to initialize the database with.
     */
    void getRetrieveValueHelper(
        std::function<bool(SQLiteBluetoothStorage&, const std::string&, std::string*)> retrieveValue,
        const std::string& key,
        const std::string& expectedValue,
        const std::unordered_map<std::string, std::string>& macToUuids);

    /// The database instance. Protected because it needs to be accessed in test cases.
    std::unique_ptr<SQLiteBluetoothStorage> m_db;

    /// SQLiteDatabase instance.
    std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteDatabase> m_sqLiteDb;
};

void SQLiteBluetoothStorageTest::closeAndDeleteDB() {
    if (m_db) {
        m_db->close();
    }
    if (m_sqLiteDb) {
        m_sqLiteDb->close();
    }

    m_db.reset();
    m_sqLiteDb.reset();
    if (fileExists(TEST_DATABASE)) {
        remove(TEST_DATABASE.c_str());
    }
}

bool SQLiteBluetoothStorageTest::createLegacyDatabase() {
    m_sqLiteDb = std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteDatabase>(
        new alexaClientSDK::storage::sqliteStorage::SQLiteDatabase(TEST_DATABASE));

    if (!m_sqLiteDb || !m_sqLiteDb->initialize()) {
        return false;
    }

    if (!m_sqLiteDb->performQuery(
            "CREATE TABLE " + UUID_TABLE_NAME + "(" + COLUMN_UUID + " text not null unique, " + COLUMN_MAC +
            " text not null unique);")) {
        m_sqLiteDb->close();
        return false;
    }

    return true;
}

bool SQLiteBluetoothStorageTest::insertEntryLegacy(const std::string& uuid, const std::string& mac) {
    // clang-format off
    const std::string sqlString = "INSERT INTO " + UUID_TABLE_NAME +
                                  " (" + COLUMN_UUID + "," + COLUMN_MAC + ") VALUES (?,?);";
    // clang-format on

    auto statement = m_sqLiteDb->createStatement(sqlString);
    if (!statement) {
        return false;
    }

    const int UUID_INDEX = 1;
    const int MAC_INDEX = 2;

    if (!statement->bindStringParameter(UUID_INDEX, uuid) || !statement->bindStringParameter(MAC_INDEX, mac)) {
        return false;
    }

    // This could be due to a mac or uuid already existing in the db.
    if (!statement->step()) {
        return false;
    }

    return true;
}

bool SQLiteBluetoothStorageTest::setupDatabase(bool migratedDatabase) {
    if (migratedDatabase) {
        // Create legacy database and migrate.
        createLegacyDatabase();

        m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
        if (!m_db || !(m_db->open())) {
            return false;
        }
    } else {
        // Create a new database.
        m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
        if (!m_db || !(m_db->createDatabase())) {
            return false;
        }
    }

    return true;
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

void SQLiteBluetoothStorageTest::getRetrieveValueHelper(
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

/// Test database not created yet, open should fail.
TEST_F(SQLiteBluetoothStorageTest, uninitializedDatabase) {
    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_FALSE(m_db->open());
}

/// Test if 2.0 database already created, open should succeed.
TEST_F(SQLiteBluetoothStorageTest, openDatabase) {
    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->createDatabase());
    m_db->close();

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());
}

/// Test if 1.0 database already created, open should succeed.
TEST_F(SQLiteBluetoothStorageTest, openLegacyDatabase) {
    createLegacyDatabase();

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());
}

/// Test retrieving category for a UUID that does not exist after database migration.
TEST_F(SQLiteBluetoothStorageTest, retrieveCategoryforUnknownUUID) {
    createLegacyDatabase();

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());

    std::string category;
    ASSERT_FALSE(m_db->getCategory(TEST_UUID, &category));
    ASSERT_THAT(category, Eq(""));
}

/// Test insertByMac after database migration.
TEST_F(SQLiteBluetoothStorageTest, insertByMacPostDatabaseUpgrade) {
    createLegacyDatabase();

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());

    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));

    std::string category;
    ASSERT_TRUE(m_db->getCategory(TEST_UUID, &category));
    ASSERT_THAT(category, Eq(TEST_UNKNOWN));
}

/// Test retrieving mac for a UUID saved before migration after database migration.
TEST_F(SQLiteBluetoothStorageTest, retrieveMacforKnownUUID) {
    createLegacyDatabase();

    insertEntryLegacy(TEST_UUID, TEST_MAC);

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());

    std::string mac;
    ASSERT_TRUE(m_db->getMac(TEST_UUID, &mac));
    ASSERT_THAT(mac, Eq(TEST_MAC));
}

/// Test retrieving category for a UUID saved before migration after database migration.
TEST_F(SQLiteBluetoothStorageTest, retrieveCategoryforKnownUUID) {
    createLegacyDatabase();

    insertEntryLegacy(TEST_UUID, TEST_MAC);

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());

    std::string category;
    ASSERT_TRUE(m_db->getCategory(TEST_UUID, &category));
    ASSERT_THAT(category, Eq(TEST_OTHER));
}

/// Test retrieving category for multiple UUIDs saved before migration after database migration.
TEST_F(SQLiteBluetoothStorageTest, retrieveCategoryforKnownMultipleUUID) {
    createLegacyDatabase();

    insertEntryLegacy(TEST_UUID, TEST_MAC);
    insertEntryLegacy(TEST_UUID_2, TEST_MAC_2);

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());

    std::string category;
    ASSERT_TRUE(m_db->getCategory(TEST_UUID, &category));
    ASSERT_THAT(category, Eq(TEST_OTHER));

    ASSERT_TRUE(m_db->getCategory(TEST_UUID_2, &category));
    ASSERT_THAT(category, Eq(TEST_OTHER));
}

/// Test when a database is empty.
TEST_F(SQLiteBluetoothStorageTest, testEmptyDatabase) {
    m_sqLiteDb = std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteDatabase>(
        new alexaClientSDK::storage::sqliteStorage::SQLiteDatabase(TEST_DATABASE));

    // Setup raw database.
    ASSERT_THAT(m_sqLiteDb, NotNull());
    ASSERT_TRUE(m_sqLiteDb->initialize());

    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());

    ASSERT_FALSE(m_sqLiteDb->tableExists(UUID_TABLE_NAME));
    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->open());

    ASSERT_TRUE(m_sqLiteDb->tableExists(UUID_TABLE_NAME));
}

/// Parameterized tests to test both migrated and newly created databases.
class SQLiteBluetoothStorageParameterizedTests
        : public SQLiteBluetoothStorageTest
        , public ::testing::WithParamInterface<bool> {};

INSTANTIATE_TEST_CASE_P(Parameterized, SQLiteBluetoothStorageParameterizedTests, ::testing::Values(true, false));

/// Tests the create function with an invalid root.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_createInvalidConfigurationRoot) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ConfigurationNode::uninitialize();
    std::vector<std::shared_ptr<std::istream>> empty;
    ConfigurationNode::initialize(empty);

    ASSERT_THAT(SQLiteBluetoothStorage::create(ConfigurationNode::getRoot()), IsNull());
}

/// Tests creating a database object.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_createValidConfigurationRoot) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    // SQLite allows simultaneous access to the database.
    ASSERT_THAT(SQLiteBluetoothStorage::create(ConfigurationNode::getRoot()), NotNull());
}

/// Test creating a valid DB. This is implicitly tested in the SetUp() function, but we formally test it here.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_createDatabaseSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    closeAndDeleteDB();
    m_db = SQLiteBluetoothStorage::create(ConfigurationNode::getRoot());

    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->createDatabase());
}

/// Test that creating an existing DB fails.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_createExistingDatabaseFails) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_FALSE(m_db->createDatabase());
}

/// Test opening an existing database.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_openExistingDatabaseSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    m_db->close();
    ASSERT_TRUE(m_db->open());
}

/// Test clearing the table with one row.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_clearOnOneRowSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->clear());
    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getUuidToMac(&rows));
    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test clearing the table with multiple rows.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_clearOnMultipleRowsSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC_2, TEST_UUID_2));
    ASSERT_TRUE(m_db->clear());
    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getUuidToMac(&rows));
    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test clearing the table when it's already empty.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_clearOnEmptySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->clear());
    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getUuidToMac(&rows));
    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test getUuid with one row containing UUID.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getUuidWithOneSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getUuid, TEST_MAC, TEST_UUID, data);
}

/// Test getUuid with multiple rows, one of which contains the UUID.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getUuidWithMultipleSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getUuid, TEST_MAC, TEST_UUID, data);
}

/// Test getUuid with no matching UUID.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getUuidNoMatchingFails) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::string uuid;
    ASSERT_FALSE(m_db->getUuid(TEST_MAC, &uuid));
}

/// Test getMac with one row containing MAC.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getMacWithOneSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getMac, TEST_UUID, TEST_MAC, data);
}

/// Test getMac with multiple rows, one of which contains the MAC.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getMacWithMultipleSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getMac, TEST_UUID, TEST_MAC, data);
}

/// Test getMac with no matching MAC.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getMacNoMatchingFails) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::string mac;
    ASSERT_FALSE(m_db->getMac(TEST_UUID, &mac));
}

/// Test getCategory with one row containing Unknown Category.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getCategoryWithOneSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getCategory, TEST_UUID, TEST_UNKNOWN, data);
}

/// Test getCategory with multiple rows, two of which contains UNKNOWN, one is updated to PHONE.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getCategoryWithMultipleSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getCategory, TEST_UUID, TEST_UNKNOWN, data);
    ASSERT_TRUE(m_db->updateByCategory(TEST_UUID, TEST_PHONE));

    std::string category;
    ASSERT_TRUE(m_db->getCategory(TEST_UUID, &category));
    ASSERT_THAT(category, Eq(TEST_PHONE));
}

/// Test getCategory with multiple rows, two of which contains UNKNOWN, one is updated to PHONE,
/// verify insertByMac preserves the category.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getCategoryWithMultipleInsertByMacSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getRetrieveValueHelper(&SQLiteBluetoothStorage::getCategory, TEST_UUID, TEST_UNKNOWN, data);
    ASSERT_TRUE(m_db->updateByCategory(TEST_UUID, TEST_PHONE));
    getRetrieveValueHelper(&SQLiteBluetoothStorage::getCategory, TEST_UUID, TEST_PHONE, data);
}

/// Test getCategory with no matching category for given uuid.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getCategoryNoMatchingFails) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::string category;
    ASSERT_FALSE(m_db->getCategory(TEST_UUID, &category));
}

/// Test getMacToUuid with one row.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getMacToUuidWithOneRowSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToUuid, data, data);
}

/// Test getMacToUuid with multiple expected.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getMacToUuidWithMultipleRowsSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToUuid, data, data);
}

/// Test getMacToUuid when empty.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getMacToUuidWithEmptySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::unordered_map<std::string, std::string> data;
    getRowsHelper(&SQLiteBluetoothStorage::getMacToUuid, data, data);
}

/// Test getUuidToMac with one row.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getUuidToMacWithOneRowSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_MAC}};

    getRowsHelper(&SQLiteBluetoothStorage::getUuidToMac, data, expected);
}

/// Test getUuidToMac with multiple expected.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getUuidToMacWithMultipleRowsSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_MAC}, {TEST_UUID_2, TEST_MAC_2}};
    getRowsHelper(&SQLiteBluetoothStorage::getUuidToMac, data, expected);
}

/// Test getUuidToMac when empty.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getUuidToMacWithEmptySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::unordered_map<std::string, std::string> data;
    getRowsHelper(&SQLiteBluetoothStorage::getUuidToMac, data, data);
}

/// Test getUuidToCategory with one row.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getUuidToCategoryWithOneRowSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_UNKNOWN}};

    getRowsHelper(&SQLiteBluetoothStorage::getUuidToCategory, data, expected);
}

/// Test getUuidToCategory with one row, updated category to PHONE.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getUuidToCategoryWithOneRowUpdateCategorySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_UNKNOWN}};

    const std::unordered_map<std::string, std::string> expectedUpdate{{TEST_UUID, TEST_PHONE}};

    getRowsHelper(&SQLiteBluetoothStorage::getUuidToCategory, data, expected);

    ASSERT_TRUE(m_db->updateByCategory(TEST_UUID, TEST_PHONE));

    getRowsHelper(&SQLiteBluetoothStorage::getUuidToCategory, data, expectedUpdate);
}

/// Test getUuidToCategory with multiple expected.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getUuidToCategoryWithMultipleRowsSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    const std::unordered_map<std::string, std::string> expected{{TEST_UUID, TEST_UNKNOWN}, {TEST_UUID_2, TEST_UNKNOWN}};

    getRowsHelper(&SQLiteBluetoothStorage::getUuidToCategory, data, expected);
}

/// Test getUuidToCategory when empty.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getUuidToCategoryWithEmptySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::unordered_map<std::string, std::string> data;
    getRowsHelper(&SQLiteBluetoothStorage::getUuidToCategory, data, data);
}

/// Test getMacToCategory with one row.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getMacToCategoryWithOneRowSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UNKNOWN}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToCategory, data, expected);
}

/// Test getMacToCategory with one row, updated category to PHONE.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getMacToCategoryWithOneRowUpdateCategorySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}};

    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UNKNOWN}};

    const std::unordered_map<std::string, std::string> expectedUpdate{{TEST_MAC, TEST_PHONE}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToCategory, data, expected);

    ASSERT_TRUE(m_db->updateByCategory(TEST_UUID, TEST_PHONE));

    getRowsHelper(&SQLiteBluetoothStorage::getMacToCategory, data, expectedUpdate);
}

/// Test getMacToCategory with multiple expected.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getMacToCategoryWithMultipleRowsSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> data{{TEST_MAC, TEST_UUID}, {TEST_MAC_2, TEST_UUID_2}};

    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UNKNOWN}, {TEST_MAC_2, TEST_UNKNOWN}};

    getRowsHelper(&SQLiteBluetoothStorage::getMacToCategory, data, expected);
}

/// Test getMacToCategory when empty.
TEST_P(SQLiteBluetoothStorageParameterizedTests, getMacToCategoryWithEmptySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::unordered_map<std::string, std::string> data;
    getRowsHelper(&SQLiteBluetoothStorage::getMacToCategory, data, data);
}

/// Test getOrderedMac and retrieve the macs in ascending insertion order (oldest first).
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getOrderedMacAscending) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    getOrderedMacHelper(true);
}

/// Test getOrderedMac and retrieve the macs in descending insertion order (newest first).
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_getOrderedMacDescending) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    getOrderedMacHelper(false);
}

/// Test updateByCategory succeeds.
TEST_P(SQLiteBluetoothStorageParameterizedTests, updateByCategorySucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->updateByCategory(TEST_UUID, TEST_PHONE));

    std::string category;
    ASSERT_TRUE(m_db->getCategory(TEST_UUID, &category));
    ASSERT_THAT(category, Eq(TEST_PHONE));
}

/// Test updateByCategory with no matching uuid.
TEST_P(SQLiteBluetoothStorageParameterizedTests, updateByCategoryNoMatchingFails) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    std::string category;
    ASSERT_FALSE(m_db->updateByCategory(TEST_UUID, TEST_PHONE));
}

/// Test insertByMac succeeds.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_insertByMacSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UUID}};

    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));
    ASSERT_THAT(rows, Eq(expected));
}

/// Test insertByMac existing fails.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_insertByMacDuplicateFails) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_FALSE(m_db->insertByMac(TEST_MAC, TEST_UUID, false));
}

/// Test insertByMac with overwrite succeeds.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_insertByMacOverwriteSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    const std::unordered_map<std::string, std::string> expected{{TEST_MAC, TEST_UUID}};

    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID_2));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));
    ASSERT_THAT(rows, Eq(expected));
}

/// Test remove succeeds.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_removeExistingSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->insertByMac(TEST_MAC, TEST_UUID));
    ASSERT_TRUE(m_db->remove(TEST_MAC));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));

    ASSERT_THAT(rows.size(), Eq(0U));
}

/// Test remove on non-existing record succeeds.
TEST_P(SQLiteBluetoothStorageParameterizedTests, test_removeNonExistingSucceeds) {
    ASSERT_TRUE(setupDatabase(GetParam()));
    ASSERT_TRUE(m_db->remove(TEST_MAC));

    std::unordered_map<std::string, std::string> rows;
    ASSERT_TRUE(m_db->getMacToUuid(&rows));

    ASSERT_THAT(rows.size(), Eq(0U));
}

}  // namespace test
}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
