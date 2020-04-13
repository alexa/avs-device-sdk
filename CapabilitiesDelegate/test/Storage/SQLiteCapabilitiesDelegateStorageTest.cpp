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
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace storage {
namespace test {

using namespace avsCommon::utils::configuration;
using namespace ::testing;

/// Test database file name.
static const std::string TEST_DATABASE_FILE_NAME = "SQLiteCapabilitiesDelegateStorageTest.db";

// clang-format off
static const std::string CAPABILITIES_DELEGATE_JSON = R"(
{
   "capabilitiesDelegate" : {
        "databaseFilePath":")" + TEST_DATABASE_FILE_NAME + R"("
     }
}
)";
// clang-format on

/// Test endpoint ID.
static const std::string TEST_ENDPOINT_ID_1 = "EndpointID1";

/// Test endpoint ID.
static const std::string TEST_ENDPOINT_ID_2 = "EndpointID2";

/// Test endpoint configuration.
static const std::string TEST_ENDPOINT_CONFIG_1 = "EndpointConfig1";

/// Test endpoint configuration.
static const std::string TEST_ENDPOINT_CONFIG_2 = "EndpointConfig2";

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

/**
 * Test harness for @c SQLiteCapabilitiesDelegateStorage.
 */
class SQLiteCapabilitiesDelegateStorageTest : public Test {
public:
    /// Setup before each test case.
    void SetUp() override;

    /// Teardown after each test case.
    void TearDown() override;

protected:
    /// Cleanup function to close.
    void closeAndDeleteDB();

    /// The database instance used for the tests.
    std::unique_ptr<SQLiteCapabilitiesDelegateStorage> m_db;
};

void SQLiteCapabilitiesDelegateStorageTest::closeAndDeleteDB() {
    if (m_db) {
        m_db->close();
    }
    m_db.reset();
    if (fileExists(TEST_DATABASE_FILE_NAME)) {
        remove(TEST_DATABASE_FILE_NAME.c_str());
    }
}

void SQLiteCapabilitiesDelegateStorageTest::SetUp() {
    /// Initialize Global ConfigurationNode with valid value.
    auto json = std::shared_ptr<std::stringstream>(new std::stringstream());
    *json << CAPABILITIES_DELEGATE_JSON;
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(json);
    ASSERT_TRUE(ConfigurationNode::initialize(jsonStream));

    m_db = SQLiteCapabilitiesDelegateStorage::create(ConfigurationNode::getRoot());

    ASSERT_THAT(m_db, NotNull());
    ASSERT_TRUE(m_db->createDatabase());
}

void SQLiteCapabilitiesDelegateStorageTest::TearDown() {
    ConfigurationNode::uninitialize();
    closeAndDeleteDB();
}

/**
 * Tests create method with invalid @c ConfigurationNode.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_createInvalidConfigurationRoot) {
    ConfigurationNode::uninitialize();
    std::vector<std::shared_ptr<std::istream>> empty;
    ConfigurationNode::initialize(empty);

    ASSERT_THAT(SQLiteCapabilitiesDelegateStorage::create(ConfigurationNode::getRoot()), IsNull());
}

/**
 * Tests create method with valid @c ConfigurationNode.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_createValidConfigurationRoot) {
    ASSERT_THAT(SQLiteCapabilitiesDelegateStorage::create(ConfigurationNode::getRoot()), NotNull());
}

/**
 * Tests if create existing database fails.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_createExistingDatabaseFails) {
    ASSERT_FALSE(m_db->createDatabase());
}

/**
 * Tests if the open existing database succeeds.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_openExistingDatabaseSucceeds) {
    m_db->close();
    ASSERT_TRUE(m_db->open());
}

/**
 * Tests if the store method works with a single endpoint.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_storeForEndpointWorks) {
    ASSERT_TRUE(m_db->store(TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1));
    std::string testString;
    ASSERT_TRUE(m_db->load(TEST_ENDPOINT_ID_1, &testString));

    ASSERT_EQ(testString, TEST_ENDPOINT_CONFIG_1);
}

/**
 * Tests if the store method works with an endpoint map.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_storeForEndpointMapWorks) {
    std::unordered_map<std::string, std::string> storeMap;
    storeMap.insert({TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1});
    storeMap.insert({TEST_ENDPOINT_ID_2, TEST_ENDPOINT_CONFIG_2});
    ASSERT_TRUE(m_db->store(storeMap));

    std::unordered_map<std::string, std::string> loadMap;
    ASSERT_TRUE(m_db->load(&loadMap));

    ASSERT_THAT(loadMap.size(), Eq(2U));

    auto it1 = loadMap.find(TEST_ENDPOINT_ID_1);
    ASSERT_NE(it1, loadMap.end());
    ASSERT_EQ(it1->second, TEST_ENDPOINT_CONFIG_1);

    auto it2 = loadMap.find(TEST_ENDPOINT_ID_2);
    ASSERT_NE(it2, loadMap.end());
    ASSERT_EQ(it2->second, TEST_ENDPOINT_CONFIG_2);
}

/**
 * Test if storing an existing entry in to the database replaces the previous value.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_storeForExistingEntry) {
    std::string storedValue;
    std::unordered_map<std::string, std::string> storeMap;
    storeMap.insert({TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1});
    storeMap.insert({TEST_ENDPOINT_ID_2, TEST_ENDPOINT_CONFIG_2});
    ASSERT_TRUE(m_db->store(storeMap));
    ASSERT_TRUE(m_db->load(TEST_ENDPOINT_ID_1, &storedValue));

    std::string TEST_CONFIG = "TEST_CONFIG";
    ASSERT_TRUE(m_db->store(TEST_ENDPOINT_ID_1, TEST_CONFIG));

    ASSERT_TRUE(m_db->load(TEST_ENDPOINT_ID_1, &storedValue));
    ASSERT_EQ(storedValue, TEST_CONFIG);
}

/**
 * Test if the load method with endpointId input works.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_loadForEndpointWorks) {
    std::unordered_map<std::string, std::string> storeMap;
    storeMap.insert({TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1});
    storeMap.insert({TEST_ENDPOINT_ID_2, TEST_ENDPOINT_CONFIG_2});
    ASSERT_TRUE(m_db->store(storeMap));

    std::string endpointConfig1, endpointConfig2;
    ASSERT_TRUE(m_db->load(TEST_ENDPOINT_ID_1, &endpointConfig1));
    ASSERT_TRUE(m_db->load(TEST_ENDPOINT_ID_2, &endpointConfig2));

    ASSERT_EQ(endpointConfig1, TEST_ENDPOINT_CONFIG_1);
    ASSERT_EQ(endpointConfig2, TEST_ENDPOINT_CONFIG_2);
}

/**
 * Test if the load method with endpointId input works.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_loadForNonExistingEndpoint) {
    std::unordered_map<std::string, std::string> storeMap;
    storeMap.insert({TEST_ENDPOINT_ID_2, TEST_ENDPOINT_CONFIG_2});
    ASSERT_TRUE(m_db->store(storeMap));

    std::string endpointConfig1, endpointConfig2;
    ASSERT_TRUE(m_db->load(TEST_ENDPOINT_ID_1, &endpointConfig1));
}

/**
 * Test if the erase method works for a given endpointId.
 */
TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_eraseWorks) {
    std::unordered_map<std::string, std::string> storeMap;
    storeMap.insert({TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1});
    storeMap.insert({TEST_ENDPOINT_ID_2, TEST_ENDPOINT_CONFIG_2});
    ASSERT_TRUE(m_db->store(storeMap));

    ASSERT_TRUE(m_db->erase(TEST_ENDPOINT_ID_1));

    std::unordered_map<std::string, std::string> loadMap;
    ASSERT_TRUE(m_db->load(&loadMap));

    ASSERT_THAT(loadMap.size(), Eq(1U));
    auto it = loadMap.find(TEST_ENDPOINT_ID_1);
    ASSERT_EQ(it, loadMap.end());

    it = loadMap.find(TEST_ENDPOINT_ID_2);
    ASSERT_NE(it, loadMap.end());
    ASSERT_EQ(it->second, TEST_ENDPOINT_CONFIG_2);
}

TEST_F(SQLiteCapabilitiesDelegateStorageTest, test_clearDatabaseWorks) {
    /// Store one item in the database.
    std::unordered_map<std::string, std::string> testMap;
    ASSERT_TRUE(m_db->store(TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1));
    ASSERT_TRUE(m_db->load(&testMap));
    ASSERT_THAT(testMap.size(), Eq(1U));

    /// Validate after clearing the database.
    ASSERT_TRUE(m_db->clearDatabase());
    testMap.clear();
    ASSERT_TRUE(m_db->load(&testMap));
    ASSERT_TRUE(testMap.empty());

    /// Store two items in the database.
    ASSERT_TRUE(m_db->store(TEST_ENDPOINT_ID_1, TEST_ENDPOINT_CONFIG_1));
    ASSERT_TRUE(m_db->store(TEST_ENDPOINT_ID_2, TEST_ENDPOINT_CONFIG_2));
    ASSERT_TRUE(m_db->load(&testMap));
    ASSERT_THAT(testMap.size(), Eq(2U));

    /// Validate after clearing the database.
    ASSERT_TRUE(m_db->clearDatabase());
    testMap.clear();
    ASSERT_TRUE(m_db->load(&testMap));
    ASSERT_TRUE(testMap.empty());

    /// Test clearing empty database.
    testMap.clear();
    ASSERT_TRUE(m_db->load(&testMap));
    ASSERT_TRUE(testMap.empty());
    ASSERT_TRUE(m_db->clearDatabase());
}

}  // namespace test
}  // namespace storage
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK