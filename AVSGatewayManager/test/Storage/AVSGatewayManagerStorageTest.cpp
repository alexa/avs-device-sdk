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

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/Storage/StubMiscStorage.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace storage {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::storage;
using namespace avsCommon::sdkInterfaces::storage::test;

/// Component name for Misc DB.
static const std::string COMPONENT_NAME = "avsGatewayManager";

/// Misc DB table for Verification State.
static const std::string VERIFICATION_STATE_TABLE = "verificationState";

/// Key for state in Misc DB table.
static const std::string VERIFICATION_STATE_KEY = "state";

/// Test URL used in the unit tests.
static const std::string TEST_URL = "www.amazon.com";

/// Second Test URL used in the unit tests.
static const std::string SECOND_TEST_URL = "www.avs.amazon.com";

/// Test gateway verification state string stored in the database.
static const std::string STORED_STATE = R"({"gatewayURL":")" + TEST_URL + R"(","isVerified":false})";

/// Test gateway verification state string stored in the database.
static const std::string SECOND_STORED_STATE = R"({"gatewayURL":")" + SECOND_TEST_URL + R"(","isVerified":true})";

/**
 * Test harness for @c AVSGatewayManagerStorage class.
 */
class AVSGatewayManagerStorageTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /// The @c StubMiscStorage Interface used for the test.
    std::shared_ptr<StubMiscStorage> m_miscStorage;

    /// The instance of the @c AVSGatewayManagerStorage that will be used in the tests.
    std::shared_ptr<AVSGatewayManagerStorage> m_avsGatewayManagerStorage;
};

void AVSGatewayManagerStorageTest::SetUp() {
    m_miscStorage = StubMiscStorage::create();
    m_avsGatewayManagerStorage = AVSGatewayManagerStorage::create(m_miscStorage);
}

void AVSGatewayManagerStorageTest::TearDown() {
    m_avsGatewayManagerStorage->clear();
}

TEST_F(AVSGatewayManagerStorageTest, test_createWithNullMiscStorage) {
    ASSERT_EQ(AVSGatewayManagerStorage::create(nullptr), nullptr);
}

/**
 * Test initialization creates verify gateway table.
 */
TEST_F(AVSGatewayManagerStorageTest, test_init) {
    /// Before
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_FALSE(tableExists);

    ASSERT_TRUE(m_avsGatewayManagerStorage->init());

    /// After
    tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_TRUE(tableExists);
}

/**
 * Test store gateway state into empty storage.
 */
TEST_F(AVSGatewayManagerStorageTest, test_storeGatewayState) {
    /// Validate MiscStorage is empty.
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_FALSE(tableExists);

    ASSERT_TRUE(m_avsGatewayManagerStorage->init());

    GatewayVerifyState gatewayVerifyState = {TEST_URL, false};
    ASSERT_TRUE(m_avsGatewayManagerStorage->saveState(gatewayVerifyState));

    std::string stateString;
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, VERIFICATION_STATE_TABLE, VERIFICATION_STATE_KEY, &stateString));
    ASSERT_EQ(stateString, STORED_STATE);
}

/**
 * Test store gateway into a previously used storage.
 */
TEST_F(AVSGatewayManagerStorageTest, test_storeSameValue) {
    /// Validate MiscStorage is empty.
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_FALSE(tableExists);

    ASSERT_TRUE(m_avsGatewayManagerStorage->init());

    GatewayVerifyState gatewayVerifyState = {TEST_URL, false};
    ASSERT_TRUE(m_avsGatewayManagerStorage->saveState(gatewayVerifyState));

    std::string stateString;
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, VERIFICATION_STATE_TABLE, VERIFICATION_STATE_KEY, &stateString));
    ASSERT_EQ(stateString, STORED_STATE);

    gatewayVerifyState = {SECOND_TEST_URL, true};
    ASSERT_TRUE(m_avsGatewayManagerStorage->saveState(gatewayVerifyState));

    stateString = "";
    ASSERT_TRUE(m_miscStorage->get(COMPONENT_NAME, VERIFICATION_STATE_TABLE, VERIFICATION_STATE_KEY, &stateString));
    ASSERT_EQ(stateString, SECOND_STORED_STATE);
}

/**
 * Test load gateway data from storage.
 */
TEST_F(AVSGatewayManagerStorageTest, test_loadGatewayState) {
    /// Validate MiscStorage is empty.
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_FALSE(tableExists);

    ASSERT_TRUE(m_avsGatewayManagerStorage->init());

    GatewayVerifyState gatewayVerifyState = {TEST_URL, true};
    ASSERT_TRUE(m_avsGatewayManagerStorage->saveState(gatewayVerifyState));

    GatewayVerifyState stateFromStorage = {"", false};
    ASSERT_TRUE(m_avsGatewayManagerStorage->loadState(&stateFromStorage));

    ASSERT_EQ(stateFromStorage.avsGatewayURL, TEST_URL);
    ASSERT_EQ(stateFromStorage.isVerified, true);
}

/**
 * Test load gateway data from empty storage.
 */
TEST_F(AVSGatewayManagerStorageTest, test_loadGatewayStateFromEmptyStorage) {
    /// Validate MiscStorage is empty.
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_FALSE(tableExists);

    ASSERT_TRUE(m_avsGatewayManagerStorage->init());

    GatewayVerifyState stateFromStorage = {"", false};

    ASSERT_FALSE(m_avsGatewayManagerStorage->loadState(&stateFromStorage));

    ASSERT_EQ(stateFromStorage.avsGatewayURL, "");
    ASSERT_EQ(stateFromStorage.isVerified, false);
}

/**
 * Test clear gateway data.
 */
TEST_F(AVSGatewayManagerStorageTest, test_clearState) {
    /// Validate MiscStorage is empty.
    bool tableExists = false;
    ASSERT_TRUE(m_miscStorage->tableExists(COMPONENT_NAME, VERIFICATION_STATE_TABLE, &tableExists));
    ASSERT_FALSE(tableExists);

    ASSERT_TRUE(m_avsGatewayManagerStorage->init());

    GatewayVerifyState gatewayVerifyState = {TEST_URL, true};
    ASSERT_TRUE(m_avsGatewayManagerStorage->saveState(gatewayVerifyState));

    GatewayVerifyState stateFromStorage = {"", false};
    ASSERT_TRUE(m_avsGatewayManagerStorage->loadState(&stateFromStorage));

    ASSERT_EQ(stateFromStorage.avsGatewayURL, TEST_URL);
    ASSERT_EQ(stateFromStorage.isVerified, true);

    m_avsGatewayManagerStorage->clear();

    stateFromStorage = {"", false};
    ASSERT_FALSE(m_avsGatewayManagerStorage->loadState(&stateFromStorage));

    ASSERT_EQ(stateFromStorage.avsGatewayURL, "");
    ASSERT_EQ(stateFromStorage.isVerified, false);
}

}  // namespace test
}  // namespace storage
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
