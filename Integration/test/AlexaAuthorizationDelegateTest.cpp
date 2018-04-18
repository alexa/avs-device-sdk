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

#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "Integration/AuthDelegateTestContext.h"
#include "Integration/AuthObserver.h"
#include "Integration/SDKTestContext.h"

/// Path to the AlexaClientSDKConfig.json file (from command line arguments).
static std::string g_configPath;

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace avsCommon::avs::initialization;
using namespace avsCommon::utils::configuration;
using namespace authorization::cblAuthDelegate;
using namespace registrationManager;
using namespace ::testing;

/// Timeout in seconds for AuthDelegate to wait for LWA response.
static const std::chrono::seconds TIME_OUT(60);

/**
 * Define test fixture for AuthDelegate integration test.
 */
class AlexaAuthorizationDelegateTest : public ::testing::Test {
public:
    /**
     * Destructor
     */
    ~AlexaAuthorizationDelegateTest();
};

AlexaAuthorizationDelegateTest::~AlexaAuthorizationDelegateTest() {
    AlexaClientSDKInit::uninitialize();
}

/**
 * Test AuthDelegate could get token refreshed with valid configurations
 *
 * If valid clientID, refreshToken, clientSecret are provided in the AuthDelegate.config file, then AuthDelegate should
 * be able to retrieve the valid refresh token (get authorized).
 */
TEST_F(AlexaAuthorizationDelegateTest, refreshAuthToken) {
    auto authDelegateContext = AuthDelegateTestContext::create(g_configPath);
    ASSERT_TRUE(authDelegateContext);

    auto authDelegate = authDelegateContext->getAuthDelegate();
    ASSERT_TRUE(authDelegate);
    auto authObserver = std::make_shared<AuthObserver>();
    authDelegate->addAuthObserver(authObserver);

    ASSERT_TRUE(authObserver->waitFor(AuthObserver::State::REFRESHED, TIME_OUT))
        << "Refreshing the auth token timed out.";
}

/**
 * Test when sending invalid refresh token, CBLAuthDelegate notifies the observer of the error.
 *
 * If an invalid client_id is sent to @c LWA, a "InvalidValue" response will be sent back and we should
 * notify the observer of the unrecoverable error.
 */
TEST_F(AlexaAuthorizationDelegateTest, invalidClientIdWithUnrecoverableError) {
    auto authDelegateContext =
        AuthDelegateTestContext::create(g_configPath, R"({ "deviceInfo" : { "clientId" : "InvalidClientId" } })");
    ASSERT_TRUE(authDelegateContext);

    auto authDelegate = authDelegateContext->getAuthDelegate();
    ASSERT_TRUE(authDelegate);
    auto authObserver = std::make_shared<AuthObserver>();
    authDelegate->addAuthObserver(authObserver);

    ASSERT_TRUE(authObserver->waitFor(AuthObserver::State::UNRECOVERABLE_ERROR, TIME_OUT))
        << "Waiting for UNRECOVERABLE_ERROR timed out";
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
