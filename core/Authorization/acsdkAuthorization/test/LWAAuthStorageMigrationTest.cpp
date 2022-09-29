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
#include <thread>

#include <gtest/gtest.h>

#include <acsdk/PropertiesInterfaces/test/StubPropertiesFactory.h>
#include <acsdk/Properties/MiscStorageAdapter.h>
#include <acsdkAuthorization/private/LWA/LWAStorageDataMigration.h>
#include <acsdkAuthorization/private/LWA/LWAStorageConstants.h>
#include <acsdkAuthorization/private/Logging.h>
#include <SQLiteStorage/SQLiteDatabase.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {
namespace test {

using namespace ::alexaClientSDK::properties;
using namespace ::alexaClientSDK::propertiesInterfaces::test;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;
using namespace ::alexaClientSDK::storage::sqliteStorage;

/// Logging tag.
static const std::string TAG = "LWAAuthStorageMigrationTest";

/// Component name for the misc DB tables
static const std::string COMPONENT_NAME = "config";

/// Table name for the misc DB tables
static const std::string TABLE_NAME = "LWAAuthorizationStorage";

/// Full table name.
static const std::string PROPERTIES_URI = COMPONENT_NAME + "/" + TABLE_NAME;

/// Full table name for legacy refresh token.
static const std::string TABLE_NAME_REFRESH_TOKEN = "refreshToken";

/// Full table name for legacy user id.
static const std::string TABLE_NAME_USER_ID = "userId";

/**
 * Test harness for @c LWAAuthorizationStorage class.
 */
class LWAAuthStorageMigrationTest : public ::testing::Test {
public:
    /**
     * Set up the test harness for running a test.
     */
    void SetUp() override;

    /**
     * Clean up the test harness after running a test.
     */
    void TearDown() override;

protected:
    void createUserIdTable();
    void createRefreshTokenTable();

    /// The Misc DB storage instance
    std::shared_ptr<SQLiteMiscStorage> m_miscStorage;

    /// Properties factory instance
    std::shared_ptr<StubPropertiesFactory> m_propertiesFactory;
};

void LWAAuthStorageMigrationTest::SetUp() {
    ACSDK_INFO(LX("SetUp"));
    m_miscStorage = SQLiteMiscStorage::create("LWAAuthorizationStorageMigrationTest.db");
    ASSERT_NE(nullptr, m_miscStorage);
    ASSERT_TRUE(m_miscStorage->open() || m_miscStorage->createDatabase());
    m_propertiesFactory = StubPropertiesFactory::create();
    ASSERT_NE(nullptr, m_propertiesFactory);
    if (m_miscStorage->getDatabase().tableExists(TABLE_NAME_REFRESH_TOKEN)) {
        ASSERT_TRUE(m_miscStorage->getDatabase().clearTable(TABLE_NAME_REFRESH_TOKEN));
        ASSERT_TRUE(m_miscStorage->getDatabase().dropTable(TABLE_NAME_REFRESH_TOKEN));
    }
    if (m_miscStorage->getDatabase().tableExists(TABLE_NAME_USER_ID)) {
        ASSERT_TRUE(m_miscStorage->getDatabase().clearTable(TABLE_NAME_USER_ID));
        ASSERT_TRUE(m_miscStorage->getDatabase().dropTable(TABLE_NAME_USER_ID));
    }
    bool propsTableExists;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, TABLE_NAME, &propsTableExists));
    if (propsTableExists) {
        ASSERT_TRUE(m_miscStorage->clearTable(COMPONENT_NAME, TABLE_NAME));
        ASSERT_TRUE(m_miscStorage->deleteTable(COMPONENT_NAME, TABLE_NAME));
    }
}

void LWAAuthStorageMigrationTest::TearDown() {
    ACSDK_INFO(LX("TearDown"));
    m_miscStorage.reset();
    m_propertiesFactory.reset();
}

void LWAAuthStorageMigrationTest::createRefreshTokenTable() {
    ASSERT_TRUE(m_miscStorage->getDatabase().performQuery("DROP TABLE IF EXISTS refreshToken"));
    ASSERT_TRUE(m_miscStorage->getDatabase().performQuery("CREATE TABLE refreshToken (refreshToken TEXT)"));
    ASSERT_TRUE(m_miscStorage->getDatabase().tableExists(TABLE_NAME_REFRESH_TOKEN));
    ASSERT_TRUE(m_miscStorage->getDatabase().performQuery("INSERT INTO refreshToken VALUES('refreshTokenValue')"));
}

void LWAAuthStorageMigrationTest::createUserIdTable() {
    ASSERT_TRUE(m_miscStorage->getDatabase().performQuery("DROP TABLE IF EXISTS userId"));
    ASSERT_TRUE(m_miscStorage->getDatabase().performQuery("CREATE TABLE userId (userId TEXT)"));
    ASSERT_TRUE(m_miscStorage->getDatabase().tableExists(TABLE_NAME_USER_ID));
    ASSERT_TRUE(m_miscStorage->getDatabase().performQuery("INSERT INTO userId VALUES('userIdValue')"));
}

/// Tests with creating a string key - string value table
TEST_F(LWAAuthStorageMigrationTest, test_migrateEmptyDatabase) {
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, TABLE_NAME, &tableExists));
    ASSERT_FALSE(tableExists);

    LWAStorageDataMigration{m_miscStorage, m_propertiesFactory}.upgradeStorage();

    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, TABLE_NAME, &tableExists));
    ASSERT_FALSE(tableExists);
}

TEST_F(LWAAuthStorageMigrationTest, test_migrateRefreshToken) {
    createRefreshTokenTable();
    LWAStorageDataMigration{m_miscStorage, m_propertiesFactory}.upgradeStorage();

    auto properties = m_propertiesFactory->getProperties(PROPERTIES_URI);
    ASSERT_NE(nullptr, properties);

    std::string refreshToken;
    ASSERT_TRUE(properties->getString(REFRESH_TOKEN_PROPERTY_NAME, refreshToken));
    ASSERT_EQ("refreshTokenValue", refreshToken);
}

TEST_F(LWAAuthStorageMigrationTest, test_migrateUserId) {
    createUserIdTable();

    LWAStorageDataMigration{m_miscStorage, m_propertiesFactory}.upgradeStorage();

    ASSERT_FALSE(m_miscStorage->getDatabase().tableExists(TABLE_NAME_USER_ID));

    auto properties = m_propertiesFactory->getProperties(PROPERTIES_URI);
    ASSERT_NE(nullptr, properties);

    std::string userId;
    ASSERT_TRUE(properties->getString(USER_ID_PROPERTY_NAME, userId));
    ASSERT_EQ("userIdValue", userId);
}

TEST_F(LWAAuthStorageMigrationTest, test_verifyMigrationForSameDatabase) {
    createRefreshTokenTable();
    createUserIdTable();

    auto propertiesFactory = createPropertiesFactory(m_miscStorage);

    bool propsTableExists;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, TABLE_NAME, &propsTableExists));
    ASSERT_FALSE(propsTableExists);

    LWAStorageDataMigration{m_miscStorage, propertiesFactory}.upgradeStorage();

    ASSERT_FALSE(m_miscStorage->getDatabase().tableExists(TABLE_NAME_USER_ID));
    ASSERT_FALSE(m_miscStorage->getDatabase().tableExists(TABLE_NAME_REFRESH_TOKEN));
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, TABLE_NAME, &propsTableExists));
    ASSERT_TRUE(propsTableExists);

    auto properties = propertiesFactory->getProperties(PROPERTIES_URI);
    ASSERT_NE(nullptr, properties);

    std::string refreshToken;
    ASSERT_TRUE(properties->getString(REFRESH_TOKEN_PROPERTY_NAME, refreshToken));
    ASSERT_EQ("refreshTokenValue", refreshToken);

    std::string userId;
    ASSERT_TRUE(properties->getString(USER_ID_PROPERTY_NAME, userId));
    ASSERT_EQ("userIdValue", userId);
}

}  // namespace test
}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
