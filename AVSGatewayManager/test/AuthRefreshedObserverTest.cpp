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
#include <string>
#include <gmock/gmock.h>
#include <AVSGatewayManager/AuthRefreshedObserver.h>
#include "AVSCommon/SDKInterfaces/AuthObserverInterface.h"

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

class AuthRefreshedObserverTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /// The instance of the @c AVSGatewayManagerStorage that will be used in the tests.
    std::shared_ptr<AuthRefreshedObserver> m_authRefreshedObserver;

    /// The callback function provided to the observer
    MockFunction<void()> m_mockCallback;
};

void AuthRefreshedObserverTest::SetUp() {
    m_authRefreshedObserver = AuthRefreshedObserver::create(m_mockCallback.AsStdFunction());
}

void AuthRefreshedObserverTest::TearDown() {
    // Nothing to do
}

TEST_F(AuthRefreshedObserverTest, onAuthStatusChangeToRefreshed) {
    EXPECT_CALL(m_mockCallback, Call()).Times(1);
    m_authRefreshedObserver->onAuthStateChange(
        AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
}

TEST_F(AuthRefreshedObserverTest, onAuthStatusChangeToRefreshedExpired) {
    EXPECT_CALL(m_mockCallback, Call()).Times(0);
    m_authRefreshedObserver->onAuthStateChange(
        AuthObserverInterface::State::EXPIRED, AuthObserverInterface::Error::SUCCESS);
}

TEST_F(AuthRefreshedObserverTest, onAuthStatusChangeToUnitialized) {
    EXPECT_CALL(m_mockCallback, Call()).Times(0);
    m_authRefreshedObserver->onAuthStateChange(
        AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS);
}

TEST_F(AuthRefreshedObserverTest, onAuthStatusChangeToUnrecoverable) {
    EXPECT_CALL(m_mockCallback, Call()).Times(0);
    m_authRefreshedObserver->onAuthStateChange(
        AuthObserverInterface::State::UNRECOVERABLE_ERROR, AuthObserverInterface::Error::SUCCESS);
}

}  // namespace test
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
