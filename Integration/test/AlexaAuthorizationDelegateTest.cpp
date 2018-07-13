/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <fstream>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AuthDelegate/AuthDelegate.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>

#include "Integration/AuthObserver.h"

using namespace alexaClientSDK::avsCommon::avs::initialization;
using namespace alexaClientSDK::authDelegate;
using namespace ::testing;

namespace {

/// Path to the AlexaClientSDKConfig.json file
std::string g_configPath;

/// Timeout in seconds for AuthDelegate to wait for LWA response.
const int TIME_OUT_IN_SECONDS = 60;

}  // namespace

namespace alexaClientSDK {
namespace integration {
namespace test {

/// Define test fixture for AuthDelegate integration test.
class AlexaAuthorizationDelegateTest : public ::testing::Test {
protected:
    /// Initialize test dependencies
    AlexaAuthorizationDelegateTest() {
        m_authObserver = std::make_shared<AuthObserver>();
    }

    /// Stub certain mock objects with default actions
    virtual void SetUp() {
        std::ifstream infile(g_configPath);
        ASSERT_TRUE(infile.good());
        ASSERT_TRUE(AlexaClientSDKInit::initialize({&infile}));
    }

    /// Release resources
    virtual void TearDown() {
        AlexaClientSDKInit::uninitialize();
    }

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
    auto authDelegate = AuthDelegate::create();
    authDelegate->addAuthObserver(m_authObserver);
    bool tokenRefreshed =
        m_authObserver->waitFor(AuthObserver::State::REFRESHED, std::chrono::seconds(TIME_OUT_IN_SECONDS));
    ASSERT_TRUE(tokenRefreshed) << "Refreshing the auth token timed out.";
}

/**
 * Test when sending invalid refresh token, AuthDelegate notifies the observer of the error.
 *
 * Currently if an invalid refresh token is sent to LWA, a "invalid_grant" response will be sent back and we should
 * notify the observer of the error.
 */
TEST_F(AlexaAuthorizationDelegateTest, invalidRefreshTokenWithUnrecoverableError) {
    AlexaClientSDKInit::uninitialize();
    std::ifstream infile(g_configPath);
    ASSERT_TRUE(infile.good());
    std::stringstream override;
    override << R"({
            "authDelegate" : {
                "refreshToken" : "InvalidRefreshToken"
            }
        })";
    ASSERT_TRUE(AlexaClientSDKInit::initialize({&infile, &override}));
    auto authDelegate = AuthDelegate::create();
    authDelegate->addAuthObserver(m_authObserver);
    bool gotUnrecoverableError =
        m_authObserver->waitFor(AuthObserver::State::UNRECOVERABLE_ERROR, std::chrono::seconds(TIME_OUT_IN_SECONDS));
    ASSERT_TRUE(gotUnrecoverableError) << "Waiting for UNRECOVERABLE_ERROR timed out";
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        std::cerr << "USAGE: " << std::string(argv[0]) << " <path to AlexaClientSDKConfig.json>" << std::endl;
        return 1;
    } else {
        g_configPath = std::string(argv[1]);
    }
    return RUN_ALL_TESTS();
}
