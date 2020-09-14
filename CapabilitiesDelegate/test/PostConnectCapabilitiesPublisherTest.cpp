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

#include <memory>
#include <thread>
#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h>

#include "MockAuthDelegate.h"
#include "MockDiscoveryEventSender.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json::jsonUtils;

/**
 * Test harness for @c PostConnectCapabilitiesPublisher class.
 */
class PostConnectCapabilitiesPublisherTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /// The mock @c PostConnectSendMessage used in the tests.
    std::shared_ptr<MockMessageSender> m_mockPostConnectSendMessage;

    /// The instance of the @c DiscoveryEventSender used in the tests.
    std::shared_ptr<MockDiscoveryEventSender> m_mockDiscoveryEventSender;

    /// The instance of the @c PostConnectCapabilitiesPublisher to be tested.
    std::shared_ptr<PostConnectCapabilitiesPublisher> m_postConnectCapabilitiesPublisher;
};

void PostConnectCapabilitiesPublisherTest::SetUp() {
    m_mockPostConnectSendMessage = std::make_shared<StrictMock<MockMessageSender>>();
    m_mockDiscoveryEventSender = std::make_shared<StrictMock<MockDiscoveryEventSender>>();

    m_postConnectCapabilitiesPublisher = PostConnectCapabilitiesPublisher::create(m_mockDiscoveryEventSender);
}

void PostConnectCapabilitiesPublisherTest::TearDown() {
    // PostConnectCapabilitiesPublisher calls stop on DiscoveryEventSender when destructed.
    EXPECT_CALL(*m_mockDiscoveryEventSender, stop()).Times(1);
}

/**
 * Test create fails with invalid parameters.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_createWithInvalidParams) {
    auto instance = PostConnectCapabilitiesPublisher::create(nullptr);
    ASSERT_EQ(instance, nullptr);
}

/**
 * Test getPriority method returns expected value.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_getPostConnectOperationPriority) {
    ASSERT_EQ(
        static_cast<unsigned int>(PostConnectOperationInterface::ENDPOINT_DISCOVERY_PRIORITY),
        m_postConnectCapabilitiesPublisher->getOperationPriority());
}

/**
 * Test that performOperation fails if the PostConnectSendMessage is invalid.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationWithInvalidPostConnectSender) {
    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(nullptr));
}

/**
 * Test performOperation sends Discovery events.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationSendsDiscoveryEvents) {
    EXPECT_CALL(*m_mockDiscoveryEventSender, sendDiscoveryEvents(_)).WillOnce(Return(true));

    ASSERT_TRUE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test second call to performOperation fails when executed twice.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationFailsWhenCalledTwice) {
    EXPECT_CALL(*m_mockDiscoveryEventSender, sendDiscoveryEvents(_)).WillOnce(Return(true));

    ASSERT_TRUE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test when sending Discovery events fails.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_performOperationFailsWhenSendDiscoveryEventsFails) {
    EXPECT_CALL(*m_mockDiscoveryEventSender, sendDiscoveryEvents(_)).WillOnce(Return(false));

    ASSERT_FALSE(m_postConnectCapabilitiesPublisher->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test abortOperation stops DiscoveryEventSender.
 */
TEST_F(PostConnectCapabilitiesPublisherTest, test_abortStopsSendDiscoveryEvents) {
    /// Stop is called on destruction as well as abort.
    EXPECT_CALL(*m_mockDiscoveryEventSender, stop()).Times(1);
    m_postConnectCapabilitiesPublisher->abortOperation();
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
