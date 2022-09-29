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

#include <cstdio>
#include <thread>
#include <gtest/gtest.h>

#include <acsdk/PropertiesInterfaces/test/StubPropertiesFactory.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationStorage.h>
#include <acsdkAuthorization/private/LWA/LWAStorageConstants.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {
namespace test {

using namespace ::alexaClientSDK::propertiesInterfaces::test;
using namespace ::alexaClientSDK::avsCommon::avs::initialization;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;
using namespace ::alexaClientSDK::storage::sqliteStorage;

/// Component name for the misc DB tables.
static const std::string COMPONENT_NAME = "config";

/// Table name for the misc DB tables.
static const std::string TABLE_NAME = "LWAAuthorizationStorage";

/// Test value for refresh token.
static const std::string REFRESH_TOKEN_VALUE = "refreshTokenValue";

/// Test value for user id.
static const std::string USER_ID_VALUE = "userIdValue";

/// JSON text for miscDB config.
// clang-format off
static const std::string MISC_DB_CONFIG_JSON = R"(
{
    "lwaAuthorization": {
        "databaseFilePath":"LWAAuthorizationStorageTest.db"
    }
}
)";
// clang-format on

/**
 * Test harness for @c LWAAuthorizationStorage class.
 */
class LWAAuthorizationStorageTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    LWAAuthorizationStorageTest();

    /**
     * Destructor.
     */
    ~LWAAuthorizationStorageTest();

protected:
    /**
     * Deletes a test table from MiscDB.
     */
    void cleanupTestDatabase();
};

void LWAAuthorizationStorageTest::cleanupTestDatabase() {
    std::remove("LWAAuthorizationStorageTest.db");
}

LWAAuthorizationStorageTest::LWAAuthorizationStorageTest() {
    auto inString = std::shared_ptr<std::istringstream>(new std::istringstream(MISC_DB_CONFIG_JSON));
    AlexaClientSDKInit::initialize({inString});
}

LWAAuthorizationStorageTest::~LWAAuthorizationStorageTest() {
    AlexaClientSDKInit::uninitialize();
}

TEST_F(LWAAuthorizationStorageTest, test_createFromEmptyStorage) {
    auto propertiesFactory = StubPropertiesFactory::create();
    ASSERT_NE(nullptr, propertiesFactory);
    auto storage = LWAAuthorizationStorage::createStorage(propertiesFactory);
    ASSERT_NE(nullptr, storage);

    ASSERT_TRUE(storage->open());

    std::string refreshToken;
    std::string userId;
    ASSERT_FALSE(storage->getRefreshToken(&refreshToken));
    ASSERT_FALSE(storage->getUserId(&userId));

    storage->setRefreshToken(REFRESH_TOKEN_VALUE);
    ASSERT_TRUE(storage->getRefreshToken(&refreshToken));
    ASSERT_EQ(REFRESH_TOKEN_VALUE, refreshToken);

    ASSERT_TRUE(storage->setUserId(USER_ID_VALUE));
    ASSERT_TRUE(storage->getUserId(&userId));
    ASSERT_EQ(USER_ID_VALUE, userId);
}

TEST_F(LWAAuthorizationStorageTest, test_createFromNonEmptyStorage) {
    auto propertiesFactory = StubPropertiesFactory::create();
    ASSERT_NE(nullptr, propertiesFactory);
    auto properties = propertiesFactory->getProperties(CONFIG_URI);
    ASSERT_NE(nullptr, properties);

    properties->putString(REFRESH_TOKEN_PROPERTY_NAME, REFRESH_TOKEN_VALUE);
    properties->putString(USER_ID_PROPERTY_NAME, USER_ID_VALUE);

    auto storage = LWAAuthorizationStorage::createStorage(propertiesFactory);
    ASSERT_NE(nullptr, storage);

    ASSERT_TRUE(storage->open());

    std::string refreshToken;
    std::string userId;

    ASSERT_TRUE(storage->getRefreshToken(&refreshToken));
    ASSERT_EQ(REFRESH_TOKEN_VALUE, refreshToken);
    ASSERT_TRUE(storage->getUserId(&userId));
    ASSERT_EQ(USER_ID_VALUE, userId);
}

TEST_F(LWAAuthorizationStorageTest, test_createFromEmptyDatabase) {
    cleanupTestDatabase();

    auto node = std::make_shared<ConfigurationNode>(ConfigurationNode::getRoot());
    auto storage = LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(node, "", nullptr, nullptr);
    ASSERT_NE(nullptr, storage);

    ASSERT_TRUE(storage->open());

    std::string refreshToken;
    std::string userId;
    ASSERT_FALSE(storage->getRefreshToken(&refreshToken));
    ASSERT_FALSE(storage->getUserId(&userId));
}

TEST_F(LWAAuthorizationStorageTest, test_createsTableAfterPutCloseAndReopen) {
    cleanupTestDatabase();

    auto node = std::make_shared<ConfigurationNode>(ConfigurationNode::getRoot());
    auto storage = LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(node, "", nullptr, nullptr);
    ASSERT_NE(nullptr, storage);
    ASSERT_TRUE(storage->open());

    ASSERT_TRUE(storage->setUserId(USER_ID_VALUE));
    ASSERT_TRUE(storage->setRefreshToken(REFRESH_TOKEN_VALUE));

    std::string refreshToken;
    std::string userId;
    ASSERT_TRUE(storage->getRefreshToken(&refreshToken));
    ASSERT_EQ(REFRESH_TOKEN_VALUE, refreshToken);
    ASSERT_TRUE(storage->getUserId(&userId));
    ASSERT_EQ(USER_ID_VALUE, userId);

    storage.reset();

    storage = LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(node, "", nullptr, nullptr);
    ASSERT_NE(nullptr, storage);
    ASSERT_TRUE(storage->open());

    refreshToken.clear();
    userId.clear();
    ASSERT_TRUE(storage->getRefreshToken(&refreshToken));
    ASSERT_EQ(REFRESH_TOKEN_VALUE, refreshToken);
    ASSERT_TRUE(storage->getUserId(&userId));
    ASSERT_EQ(USER_ID_VALUE, userId);
}

}  // namespace test
}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
