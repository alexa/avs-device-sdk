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

#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockAVSEndpointAssigner.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <ADSL/DirectiveSequencer.h>

#include "System/EndpointHandler.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// This is a string for the namespace we are testing for.
static const std::string ENDPOINTING_NAMESPACE = "System";

/// This is a string for the correct name the endpointing directive uses.
static const std::string ENDPOINTING_NAME = "SetEndpoint";

/// This is a string for the wrong name the endpointing directive uses.
static const std::string ENDPOINTING_WRONG_NAME = "WrongEndpointer";

/// This string holds the key for the endpoint in the payload.
static const std::string ENDPOINT_PAYLOAD_KEY = "endpoint";

/// This is the string for the endpoint payload.
static const std::string ENDPOINT_PAYLOAD_VALUE = "https://avs-alexa-na.amazon.com";

/// This is the full payload expected to come from AVS.
static const std::string ENDPOINT_PAYLOAD = "{\"" + ENDPOINT_PAYLOAD_KEY + "\": \"" + ENDPOINT_PAYLOAD_VALUE + "\"}";

/// This is the string for the message ID used in the directive.
static const std::string ENDPOINTING_MESSAGE_ID = "ABC123DEF";

/// This is the condition variable to be used to control the exit of the test case.
std::condition_variable exitTrigger;

/**
 * Helper function to check if the incoming endpoint is the one we prescribed.
 *
 * @param endpoint The endpoint provided by @c EndpointHandler.
 * @return @c true if the output matches; otherwise @c false.
 */
static bool checkIncomingEndpoint(const std::string& endpoint) {
    std::cout << "Incoming endpoint: " << endpoint << std::endl;
    exitTrigger.notify_all();
    return ENDPOINT_PAYLOAD_VALUE == endpoint;
}

/// Test harness for @c EndpointHandler class.
class EndpointHandlerTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// Mocked AVS Endpoint Assigner. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockAVSEndpointAssigner>> m_mockAVSEndpointAssigner;
    /// Mocked Exception Encountered Sender. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionEncounteredSender;
};

void EndpointHandlerTest::SetUp() {
    m_mockAVSEndpointAssigner = std::make_shared<StrictMock<MockAVSEndpointAssigner>>();
    m_mockExceptionEncounteredSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
}

/**
 * This case tests if @c EndpointHandler basic create function works properly
 */
TEST_F(EndpointHandlerTest, createSuccessfully) {
    ASSERT_NE(nullptr, EndpointHandler::create(m_mockAVSEndpointAssigner, m_mockExceptionEncounteredSender));
}

/**
 * This case tests if possible @c nullptr parameters passed to @c EndpointHandler::create are handled properly.
 */
TEST_F(EndpointHandlerTest, createWithError) {
    ASSERT_EQ(nullptr, EndpointHandler::create(m_mockAVSEndpointAssigner, nullptr));
    ASSERT_EQ(nullptr, EndpointHandler::create(nullptr, m_mockExceptionEncounteredSender));
    ASSERT_EQ(nullptr, EndpointHandler::create(nullptr, nullptr));
}

/**
 * This case tests if a directive is handled properly.
 */
TEST_F(EndpointHandlerTest, handleDirectiveProperly) {
    auto endpointHandler = EndpointHandler::create(m_mockAVSEndpointAssigner, m_mockExceptionEncounteredSender);
    ASSERT_NE(nullptr, endpointHandler);

    auto directiveSequencer = adsl::DirectiveSequencer::create(m_mockExceptionEncounteredSender);
    directiveSequencer->addDirectiveHandler(endpointHandler);

    auto endpointDirectiveHeader =
        std::make_shared<AVSMessageHeader>(ENDPOINTING_NAMESPACE, ENDPOINTING_NAME, ENDPOINTING_MESSAGE_ID);
    auto attachmentManager = std::make_shared<StrictMock<attachment::test::MockAttachmentManager>>();
    std::shared_ptr<AVSDirective> endpointDirective =
        AVSDirective::create("", endpointDirectiveHeader, ENDPOINT_PAYLOAD, attachmentManager, "");

    std::mutex exitMutex;
    std::unique_lock<std::mutex> exitLock(exitMutex);
    EXPECT_CALL(*m_mockAVSEndpointAssigner, setAVSEndpoint(ResultOf(&checkIncomingEndpoint, Eq(true))));
    directiveSequencer->onDirective(endpointDirective);
    exitTrigger.wait_for(exitLock, std::chrono::seconds(2));
    directiveSequencer->shutdown();
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
