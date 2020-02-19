/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <gmock/gmock.h>

#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSCommon/SDKInterfaces/MockAVSGatewayAssigner.h>
#include <AVSCommon/SDKInterfaces/MockAVSGatewayObserver.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <RegistrationManager/CustomerDataManager.h>

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace test {

using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::configuration;
using namespace ::testing;

/// Test AVS Gateway
static const std::string TEST_AVS_GATEWAY = "www.test-avs-gateway.com";

/// Default AVS Gateway
static const std::string DEFAULT_AVS_GATEWAY = "https://alexa.na.gateway.devices.a2z.com";

/// Stored AVS Gateway
static const std::string STORED_AVS_GATEWAY = "www.avs-gatewa-from-storage.com";

// clang-format off
static const std::string AVS_GATEWAY_MANAGER_JSON = R"(
{
   "avsGatewayManager" : {
        "avsGateway":")" + TEST_AVS_GATEWAY + R"("
    }
}
)";
// clang-format on

// clang-format off
static const std::string AVS_GATEWAY_MANAGER_JSON_NO_GATEWAY = R"(
{
   "avsGatewayManager" : {
    }
}
)";
// clang-format on

// clang-format off
static const std::string AVS_GATEWAY_MANAGER_JSON_EMPTY_GATEWAY = R"(
{
   "avsGatewayManager" : {
        "avsGateway":""
    }
}
)";
// clang-format on

class MockAVSGatewayManagerStorage : public storage::AVSGatewayManagerStorageInterface {
public:
    MOCK_METHOD0(init, bool());
    MOCK_METHOD1(loadState, bool(GatewayVerifyState*));
    MOCK_METHOD1(saveState, bool(const GatewayVerifyState&));
    MOCK_METHOD0(clear, void());
};

/**
 * Test harness for @c AVSGatewayManager class.
 */
class AVSGatewayManagerTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /// Initializes the @c ConfigurationRoot.
    void initializeConfigRoot(const std::string& configJson);

    /// Creates the @c AVSGatewayManager used in the tests.
    void createAVSGatewayManager();

    /// The @c CustomerDataManager.
    std::shared_ptr<registrationManager::CustomerDataManager> m_customerDataManager;

    /// The mock @c AVSGatewayAssigner.
    std::shared_ptr<MockAVSGatewayAssigner> m_mockAVSGatewayAssigner;

    /// The mock @c AVSGatewayObserver.
    std::shared_ptr<MockAVSGatewayObserver> m_mockAVSGatewayObserver;

    /// The mock @c AVSGatewayManagerStorageInterface.
    std::shared_ptr<MockAVSGatewayManagerStorage> m_mockAVSGatewayManagerStorage;

    /// The instance of the @c AVSGatewayManagerStorage that will be used in the tests.
    std::shared_ptr<AVSGatewayManager> m_avsGatewayManager;
};

void AVSGatewayManagerTest::SetUp() {
    m_mockAVSGatewayManagerStorage = std::make_shared<NiceMock<MockAVSGatewayManagerStorage>>();
    m_mockAVSGatewayAssigner = std::make_shared<NiceMock<MockAVSGatewayAssigner>>();
    m_mockAVSGatewayObserver = std::make_shared<NiceMock<MockAVSGatewayObserver>>();
    m_customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
}

void AVSGatewayManagerTest::TearDown() {
    ConfigurationNode::uninitialize();
}

void AVSGatewayManagerTest::initializeConfigRoot(const std::string& configJson) {
    auto json = std::shared_ptr<std::stringstream>(new std::stringstream());
    *json << configJson;
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(json);
    ASSERT_TRUE(ConfigurationNode::initialize(jsonStream));
}

void AVSGatewayManagerTest::createAVSGatewayManager() {
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, init()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, loadState(_)).Times(1).WillOnce(Return(true));

    m_avsGatewayManager =
        AVSGatewayManager::create(m_mockAVSGatewayManagerStorage, m_customerDataManager, ConfigurationNode::getRoot());

    ASSERT_NE(m_avsGatewayManager, nullptr);
}

TEST_F(AVSGatewayManagerTest, test_createAVSGatewayManagerWithInvalidParameters) {
    auto instance = AVSGatewayManager::create(nullptr, m_customerDataManager, ConfigurationNode::getRoot());
    ASSERT_EQ(instance, nullptr);
    instance = AVSGatewayManager::create(m_mockAVSGatewayManagerStorage, nullptr, ConfigurationNode::getRoot());
    ASSERT_EQ(instance, nullptr);
}

/**
 * Test if a call to setAVSGatewayAssigner uses the gateway URL from the configuration file.
 */
TEST_F(AVSGatewayManagerTest, test_defaultAVSGatewayFromConfigFile) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();

    EXPECT_CALL(*m_mockAVSGatewayAssigner, setAVSGateway(_)).WillOnce(Invoke([](const std::string& gatewayURL) {
        ASSERT_EQ(gatewayURL, TEST_AVS_GATEWAY);
    }));
    m_avsGatewayManager->setAVSGatewayAssigner(m_mockAVSGatewayAssigner);
}

/**
 * Test if a call to setAVSGatewayAssigner uses the default gateway URL if configuration is not specified.
 */
TEST_F(AVSGatewayManagerTest, test_defaultAVSGatewayFromConfigFileWithNoGateway) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON_NO_GATEWAY);
    createAVSGatewayManager();

    EXPECT_CALL(*m_mockAVSGatewayAssigner, setAVSGateway(_)).WillOnce(Invoke([](const std::string& gatewayURL) {
        ASSERT_EQ(gatewayURL, DEFAULT_AVS_GATEWAY);
    }));
    m_avsGatewayManager->setAVSGatewayAssigner(m_mockAVSGatewayAssigner);
}

/**
 * Test if a call to setAVSGatewayAssigner uses the default gateway URL if avsGateway in configuration is empty.
 */
TEST_F(AVSGatewayManagerTest, test_defaultAVSGatewayFromConfigFileWithEmptyGateway) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON_EMPTY_GATEWAY);
    createAVSGatewayManager();

    EXPECT_CALL(*m_mockAVSGatewayAssigner, setAVSGateway(_)).WillOnce(Invoke([](const std::string& gatewayURL) {
        ASSERT_EQ(gatewayURL, DEFAULT_AVS_GATEWAY);
    }));
    m_avsGatewayManager->setAVSGatewayAssigner(m_mockAVSGatewayAssigner);
}

/**
 * Test if a call to setAVSGatewayAssigner uses the gateway URL from storage.
 */
TEST_F(AVSGatewayManagerTest, test_avsGatewayFromStorage) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, init()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, loadState(_)).WillOnce(Invoke([](GatewayVerifyState* state) {
        state->avsGatewayURL = STORED_AVS_GATEWAY;
        state->isVerified = true;
        return true;
    }));

    m_avsGatewayManager =
        AVSGatewayManager::create(m_mockAVSGatewayManagerStorage, m_customerDataManager, ConfigurationNode::getRoot());

    ASSERT_NE(m_avsGatewayManager, nullptr);

    EXPECT_CALL(*m_mockAVSGatewayAssigner, setAVSGateway(_)).WillOnce(Invoke([](const std::string& gatewayURL) {
        ASSERT_EQ(gatewayURL, STORED_AVS_GATEWAY);
    }));
    m_avsGatewayManager->setAVSGatewayAssigner(m_mockAVSGatewayAssigner);
}

/**
 * Test if a call to setGatewayURL() with a new URL calls
 *  - calls the setAVSGateway() method on @c AVSGatewayAssigner
 *  - calls the storeState() method on @c AVSGatewayManagerStorage
 *  - calls the onGatewayChanged() method on @c AVSGatewayObserver.
 */
TEST_F(AVSGatewayManagerTest, test_setAVSGatewayURLWithNewURL) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();

    EXPECT_CALL(*m_mockAVSGatewayAssigner, setAVSGateway(_)).Times(2);
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, saveState(_)).Times(1);
    EXPECT_CALL(*m_mockAVSGatewayObserver, onAVSGatewayChanged(_)).Times(1);

    m_avsGatewayManager->addObserver(m_mockAVSGatewayObserver);
    m_avsGatewayManager->setAVSGatewayAssigner(m_mockAVSGatewayAssigner);
    m_avsGatewayManager->setGatewayURL(DEFAULT_AVS_GATEWAY);
}

/**
 * Test if a call to setGatewayURL with the same URL does not trigger calls to @c AVSGatewayAssigner,
 * @c AVSGatewayObserver and @c AVSGatewayManagerStorage.
 */
TEST_F(AVSGatewayManagerTest, test_setAVSGatewayURLWithSameURL) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();
    EXPECT_CALL(*m_mockAVSGatewayAssigner, setAVSGateway(_)).Times(1);
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, saveState(_)).Times(0);
    EXPECT_CALL(*m_mockAVSGatewayObserver, onAVSGatewayChanged(_)).Times(0);
    m_avsGatewayManager->addObserver(m_mockAVSGatewayObserver);
    m_avsGatewayManager->setAVSGatewayAssigner(m_mockAVSGatewayAssigner);
    m_avsGatewayManager->setGatewayURL(TEST_AVS_GATEWAY);
}

/**
 * Test if AVSGatewayManager gracefully handles adding a nullObserver.
 */
TEST_F(AVSGatewayManagerTest, test_addNullObserver) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();
    m_avsGatewayManager->addObserver(nullptr);
}

/**
 * Test if AVSGatewayManager gracefully handles removing a nullObserver.
 */
TEST_F(AVSGatewayManagerTest, test_removeNullObserver) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();
    m_avsGatewayManager->removeObserver(nullptr);
}

/**
 * Test removing a previously added observer.
 */
TEST_F(AVSGatewayManagerTest, test_removeAddedObserver) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();

    EXPECT_CALL(*m_mockAVSGatewayObserver, onAVSGatewayChanged(_)).Times(1);
    m_avsGatewayManager->addObserver(m_mockAVSGatewayObserver);
    m_avsGatewayManager->setGatewayURL(DEFAULT_AVS_GATEWAY);

    EXPECT_CALL(*m_mockAVSGatewayObserver, onAVSGatewayChanged(_)).Times(0);
    m_avsGatewayManager->removeObserver(m_mockAVSGatewayObserver);
    m_avsGatewayManager->setGatewayURL(TEST_AVS_GATEWAY);
}

/**
 * Test removing an observer that is not previously added is handled gracefully.
 */
TEST_F(AVSGatewayManagerTest, test_removeObserverNotAddedPreviously) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();

    EXPECT_CALL(*m_mockAVSGatewayObserver, onAVSGatewayChanged(_)).Times(0);
    m_avsGatewayManager->setGatewayURL(DEFAULT_AVS_GATEWAY);

    m_avsGatewayManager->removeObserver(m_mockAVSGatewayObserver);
}

/**
 * Test if a call to clearData() invokes call to clear() on the storage.
 */
TEST_F(AVSGatewayManagerTest, test_clearData) {
    initializeConfigRoot(AVS_GATEWAY_MANAGER_JSON);
    createAVSGatewayManager();
    EXPECT_CALL(*m_mockAVSGatewayManagerStorage, clear()).Times(1);
    m_avsGatewayManager->clearData();
}

}  // namespace test
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
