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

#include <SQLiteStorage/SQLiteMiscStorage.h>

#include <gtest/gtest.h>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {
namespace test {

using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::utils::configuration;

/// Component name for the misc DB tables
static const std::string COMPONENT_NAME = "SQLiteMiscStorageTest";

/// JSON text for miscDB config.
// clang-format off
static const std::string MISC_DB_CONFIG_JSON =
    "{"
      "\"miscDatabase\":{"
        "\"databaseFilePath\":\"miscDBSQLiteMiscStorageTest.db\""
        "}"
    "}";
// clang-format on

/**
 * Test harness for @c SQLiteMiscStorage class.
 */
class SQLiteMiscStorageTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    SQLiteMiscStorageTest();

    /**
     * Destructor.
     */
    ~SQLiteMiscStorageTest();

    /**
     * Set up the test harness for running a test.
     */
    void SetUp() override;

protected:
    /**
     * Creates a test table in MiscDB.
     *
     * @param tableName The table name to be created.
     * @param keyType The key column type.
     * @param valueType The value column type.
     */
    void createTestTable(
        const std::string& tableName,
        const SQLiteMiscStorage::KeyType keyType,
        const SQLiteMiscStorage::ValueType valueType);

    /**
     * Deletes a test table from MiscDB.
     *
     * @param tableName The table name to be created.
     */
    void deleteTestTable(const std::string& tableName);

    /// The Misc DB storage instance
    std::shared_ptr<SQLiteMiscStorage> m_miscStorage;
};

void SQLiteMiscStorageTest::createTestTable(
    const std::string& tableName,
    const SQLiteMiscStorage::KeyType keyType,
    const SQLiteMiscStorage::ValueType valueType) {
    if (m_miscStorage) {
        bool tableExists;
        m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists);
        if (tableExists) {
            m_miscStorage->clearTable(COMPONENT_NAME, tableName);
        } else {
            m_miscStorage->createTable(COMPONENT_NAME, tableName, keyType, valueType);
        }
    }
}

void SQLiteMiscStorageTest::deleteTestTable(const std::string& tableName) {
    if (m_miscStorage) {
        bool tableExists;
        m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists);
        if (tableExists) {
            m_miscStorage->clearTable(COMPONENT_NAME, tableName);
            m_miscStorage->deleteTable(COMPONENT_NAME, tableName);
        }
    }
}

SQLiteMiscStorageTest::SQLiteMiscStorageTest() {
    auto inString = std::shared_ptr<std::istringstream>(new std::istringstream(MISC_DB_CONFIG_JSON));
    AlexaClientSDKInit::initialize({inString});
    auto config = ConfigurationNode::getRoot();
    if (config) {
        m_miscStorage = SQLiteMiscStorage::create(config);
        if (m_miscStorage) {
            if (!m_miscStorage->open()) {
                m_miscStorage->createDatabase();
            }
        }
    }
}

SQLiteMiscStorageTest::~SQLiteMiscStorageTest() {
    if (m_miscStorage) {
        m_miscStorage->close();
    }
    AlexaClientSDKInit::uninitialize();
}

void SQLiteMiscStorageTest::SetUp() {
    ASSERT_NE(m_miscStorage, nullptr);
}

/// Tests with creating a string key - string value table
TEST_F(SQLiteMiscStorageTest, createStringKeyValueTable) {
    const std::string tableName = "SQLiteMiscStorageCreateTableTest";
    deleteTestTable(tableName);

    bool tableExists;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists));
    ASSERT_FALSE(tableExists);
    ASSERT_TRUE(m_miscStorage->createTable(
        COMPONENT_NAME, tableName, SQLiteMiscStorage::KeyType::STRING_KEY, SQLiteMiscStorage::ValueType::STRING_VALUE));
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists));
    ASSERT_TRUE(tableExists);

    deleteTestTable(tableName);
}

/// Tests with table entry add, remove, update, put
TEST_F(SQLiteMiscStorageTest, tableEntryTests) {
    const std::string tableName = "SQLiteMiscStorageTableEntryTest";
    const std::string tableEntryKey = "tableEntryTestsKey";
    const std::string tableEntryAddedValue = "tableEntryAddedValue";
    const std::string tableEntryPutValue = "tableEntryPutValue";
    const std::string tableEntryAnotherPutValue = "tableEntryAnotherPutValue";
    const std::string tableEntryUpdatedValue = "tableEntryUpdatedValue";
    std::string tableEntryValue;
    deleteTestTable(tableName);

    createTestTable(tableName, SQLiteMiscStorage::KeyType::STRING_KEY, SQLiteMiscStorage::ValueType::STRING_VALUE);

    /// Entry doesn't exist at first
    bool tableEntryExists;
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_FALSE(tableEntryExists);

    /// Ensure that add entry works
    ASSERT_TRUE(m_miscStorage->add(COMPONENT_NAME, tableName, tableEntryKey, tableEntryAddedValue));
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_TRUE(tableEntryExists);
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryValue));
    ASSERT_EQ(tableEntryValue, tableEntryAddedValue);

    /// Ensure that update entry works
    ASSERT_TRUE(m_miscStorage->update(COMPONENT_NAME, tableName, tableEntryKey, tableEntryUpdatedValue));
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_TRUE(tableEntryExists);
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryValue));
    ASSERT_EQ(tableEntryValue, tableEntryUpdatedValue);

    /// Ensure that remove entry works
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_TRUE(tableEntryExists);
    ASSERT_TRUE(m_miscStorage->remove(COMPONENT_NAME, tableName, tableEntryKey));
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_FALSE(tableEntryExists);

    /// Ensure that put entry works
    /// Try with a new entry for key
    ASSERT_TRUE(m_miscStorage->put(COMPONENT_NAME, tableName, tableEntryKey, tableEntryPutValue));
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_TRUE(tableEntryExists);
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryValue));
    ASSERT_EQ(tableEntryValue, tableEntryPutValue);
    /// Try with an existing entry for key
    ASSERT_TRUE(m_miscStorage->put(COMPONENT_NAME, tableName, tableEntryKey, tableEntryAnotherPutValue));
    ASSERT_TRUE(m_miscStorage->tableEntryExists(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryExists));
    ASSERT_TRUE(tableEntryExists);
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, tableName, tableEntryKey, &tableEntryValue));
    ASSERT_EQ(tableEntryValue, tableEntryAnotherPutValue);

    deleteTestTable(tableName);
}

/// Tests with loading and clearing table entries
TEST_F(SQLiteMiscStorageTest, loadAndClear) {
    const std::string tableName = "SQLiteMiscStorageLoadClearTest";
    size_t numOfEntries = 3;
    std::string keyPrefix = "key";
    std::string valuePrefix = "value";
    std::unordered_map<std::string, std::string> valuesContainer;
    deleteTestTable(tableName);

    createTestTable(tableName, SQLiteMiscStorage::KeyType::STRING_KEY, SQLiteMiscStorage::ValueType::STRING_VALUE);

    /// Add entries
    for (size_t entryIndx = 1; entryIndx <= numOfEntries; entryIndx++) {
        std::string key = keyPrefix + std::to_string(entryIndx);
        std::string value = valuePrefix + std::to_string(entryIndx);
        ASSERT_TRUE(m_miscStorage->add(COMPONENT_NAME, tableName, key, value));
    }

    /// Ensure that load works
    ASSERT_TRUE(m_miscStorage->load(COMPONENT_NAME, tableName, &valuesContainer));
    ASSERT_EQ(valuesContainer.size(), numOfEntries);
    for (size_t entryIndx = 1; entryIndx <= numOfEntries; entryIndx++) {
        std::string keyExpected = keyPrefix + std::to_string(entryIndx);
        std::string valueExpected = valuePrefix + std::to_string(entryIndx);
        auto mapIterator = valuesContainer.find(keyExpected);
        ASSERT_NE(mapIterator, valuesContainer.end());
        ASSERT_EQ(mapIterator->first, keyExpected);
        ASSERT_EQ(mapIterator->second, valueExpected);
    }

    /// Ensure that clear works
    valuesContainer.clear();
    ASSERT_TRUE(m_miscStorage->clearTable(COMPONENT_NAME, tableName));
    m_miscStorage->load(COMPONENT_NAME, tableName, &valuesContainer);
    ASSERT_TRUE(valuesContainer.empty());

    deleteTestTable(tableName);
}

/// Tests with creating and deleting tables
TEST_F(SQLiteMiscStorageTest, createDeleteTable) {
    const std::string tableName = "SQLiteMiscStorageCreateDeleteTest";
    deleteTestTable(tableName);

    /// Ensure that create works
    bool tableExists;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists));
    ASSERT_FALSE(tableExists);
    ASSERT_TRUE(m_miscStorage->createTable(
        COMPONENT_NAME, tableName, SQLiteMiscStorage::KeyType::STRING_KEY, SQLiteMiscStorage::ValueType::STRING_VALUE));
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists));
    ASSERT_TRUE(tableExists);

    /// Ensure that delete doesnt work on a non-empty table
    ASSERT_TRUE(m_miscStorage->add(COMPONENT_NAME, tableName, "randomKey", "randomValue"));
    ASSERT_FALSE(m_miscStorage->deleteTable(COMPONENT_NAME, tableName));
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists));
    ASSERT_TRUE(tableExists);

    /// Ensure that delete works on an empty table
    ASSERT_TRUE(m_miscStorage->clearTable(COMPONENT_NAME, tableName));
    ASSERT_TRUE(m_miscStorage->deleteTable(COMPONENT_NAME, tableName));
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, tableName, &tableExists));
    ASSERT_FALSE(tableExists);
}

}  // namespace test
}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK
