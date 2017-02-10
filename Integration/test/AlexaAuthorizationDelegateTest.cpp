/*
 * AlexaAuthorizationDelegateTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AuthDelegate/AuthDelegate.h"
#include "AuthDelegate/MockConfig.h"
#include "AVSUtils/Initialization/AlexaClientSDKInit.h"
#include "Integration/FileConfig.h"
#include "Integration/AuthObserver.h"


using namespace alexaClientSDK::avsUtils::initialization;
using namespace alexaClientSDK::authDelegate;
using namespace ::testing;

namespace {

/// Path to the AuthDelegate.config file
std::string g_configPath;

/// Timeout in seconds for AuthDelegate to wait for LWA response.
const int TIME_OUT_IN_SECONDS = 60;

} // namespace

namespace alexaClientSDK {
namespace integration {

/// Define test fixture for AuthDelegate integration test.
class AlexaAuthorizationDelegateTest : public ::testing::Test {
protected:

    /// Initialize test dependencies
    AlexaAuthorizationDelegateTest() {
        m_authObserver = std::make_shared<AuthObserver>();
        m_config = std::make_shared<FileConfig>(g_configPath);
        m_mockConfig = std::make_shared<NiceMock<MockConfig>>();
    }

    /// Stub certain mock objects with default actions
    virtual void SetUp() {
        ASSERT_TRUE(AlexaClientSDKInit::initialize());

        ON_CALL(*m_mockConfig, getClientId()).WillByDefault(Return(m_config->getClientId()));
        ON_CALL(*m_mockConfig, getClientSecret()).WillByDefault(Return(m_config->getClientSecret()));
        ON_CALL(*m_mockConfig, getRefreshToken()).WillByDefault(Return(m_config->getRefreshToken()));
        ON_CALL(*m_mockConfig, getLwaUrl()).WillByDefault(Return(m_config->getLwaUrl()));
    }

    /// Release resources
    virtual void TearDown() {
        AlexaClientSDKInit::uninitialize();
    }

    /// Config object that contains valid information needed by LWA for authorization
    std::shared_ptr<Config> m_config;

    /// Mock object of @c Config which could be stubbed with desired behaviors
    std::shared_ptr<NiceMock<MockConfig>> m_mockConfig;

    /// AuthObserver that implements AuthObserverInterface
    std::shared_ptr<AuthObserver> m_authObserver;
};

/**
 * Test AuthDelegate could get token refreshed with valid configurations
 *
 * If valid clientID, refreshToken, clientSecret are provided in the AuthDelegate.config file, then AuthDelegate should
 * be able to retrieve the valid refresh token (get authorized).
 */
TEST_F(AlexaAuthorizationDelegateTest, refreshAuthToken) {
    auto authDelegate = AuthDelegate::create(m_mockConfig);
    authDelegate->setAuthObserver(m_authObserver);
    bool tokenRefreshed = m_authObserver->waitFor(AuthObserver::State::REFRESHED,
        std::chrono::seconds(TIME_OUT_IN_SECONDS));
    ASSERT_TRUE(tokenRefreshed) << "Refreshing the auth token timed out.";
}

/**
 * Test when sending invalid refresh token, AuthDelegate notifies the observer of the error.
 *
 * Currently if an invalid refresh token is sent to LWA, a "invalid_grant" response will be sent back and we should
 * notify the observer of the error.
 */
TEST_F(AlexaAuthorizationDelegateTest, invalidRefreshTokenWithUnrecoverableError) {
    ON_CALL(*m_mockConfig, getRefreshToken())
            .WillByDefault(Return("InvalidRefreshToken"));
    auto authDelegate = AuthDelegate::create(m_mockConfig);
    authDelegate->setAuthObserver(m_authObserver);
    bool gotUnrecoverableError = m_authObserver->waitFor(AuthObserver::State::UNRECOVERABLE_ERROR,
         std::chrono::seconds(TIME_OUT_IN_SECONDS));
    ASSERT_TRUE(gotUnrecoverableError) << "Waiting for UNRECOVERABLE_ERROR timed out";
}

}   // namespace integration
}   // namespace alexaClientSDK

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        std::cerr << "USAGE: AlexaAuthorizationDelegateTest <path_to_auth_delgate_config>" << std::endl;
    } else {
        g_configPath = std::string(argv[1]);
    }
    return RUN_ALL_TESTS();
}