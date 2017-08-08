/*
 * StateSynchronizerTest.cpp
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

/// @file StateSynchronizerTest

#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockStateSynchronizerObserver.h>

#include "System/StateSynchronizer.h"

using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::utils::json;

static const std::string MOCK_CONTEXT = "{"
        "\"context\":[{"
            "\"header\":{"
                "\"name\":\"SpeechState\","
                "\"namespace\":\"SpeechSynthesizer\""
            "},"
            "\"payload\":{"
                "\"playerActivity\":\"FINISHED\","
                "\"offsetInMilliseconds\":0,"
                "\"token\":\"\""
            "}"
        "}]}";

/**
 * Check if message request has errors.
 *
 * @param messageRequest The message requests to be checked.
 * @return @c true if parsing the JSON has no errors and the payload is empty, otherwise @c false.
 */
static bool checkMessageRequest(std::shared_ptr<MessageRequest> messageRequest) {
    rapidjson::Document jsonContent(rapidjson::kObjectType);
    if (jsonContent.Parse(messageRequest->getJsonContent()).HasParseError()) {
        return false;
    }
    rapidjson::Value::ConstMemberIterator eventNode;
    if (!jsonUtils::findNode(jsonContent, "event", &eventNode)) {
        return false;
    }
    rapidjson::Value::ConstMemberIterator payloadNode;
    if (!jsonUtils::findNode(eventNode->value, "payload", &payloadNode)) {
        return false;
    }
    // The payload should be an empty JSON Object.
    return payloadNode->value.ObjectEmpty();
}

class TestMessageSender : public MessageSenderInterface {
    void sendMessage(std::shared_ptr<MessageRequest> messageRequest) {
        EXPECT_TRUE(checkMessageRequest(messageRequest));
        messageRequest->onSendCompleted(MessageRequest::Status::SUCCESS);
    }
};

/// Test harness for @c StateSynchronizer class.
class StateSynchronizerTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    /// Mocked Context Manager. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;
    /// Mocked Message Sender. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<TestMessageSender> m_mockMessageSender;
    /// Mocked State Synchronizer Observer. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockStateSynchronizerObserver>> m_mockStateSynchronizerObserver;
};

void StateSynchronizerTest::SetUp() {
    m_mockContextManager = std::make_shared<StrictMock<MockContextManager>>();
    m_mockMessageSender = std::make_shared<TestMessageSender>();
    m_mockStateSynchronizerObserver = std::make_shared<StrictMock<MockStateSynchronizerObserver>>();
}

/**
 * This case tests if @c StateSynchronizer basic create function works properly
 */
TEST_F(StateSynchronizerTest, createSuccessfully) {
    ASSERT_NE(nullptr, StateSynchronizer::create(m_mockContextManager, m_mockMessageSender));
}

/**
 * This case tests if possible @c nullptr parameters passed to @c StateSynchronizer::create are handled properly.
 */
TEST_F(StateSynchronizerTest, createWithError) {
    ASSERT_EQ(nullptr, StateSynchronizer::create(m_mockContextManager, nullptr));
    ASSERT_EQ(nullptr, StateSynchronizer::create(nullptr, m_mockMessageSender));
    ASSERT_EQ(nullptr, StateSynchronizer::create(nullptr, nullptr));
}

/**
 * This case tests if @c onConnectionStatusChanged triggers @c getContext when connected.
 */
TEST_F(StateSynchronizerTest, connectedTriggersGetContext) {
    auto stateSynchronizer = StateSynchronizer::create(m_mockContextManager, m_mockMessageSender);
    ASSERT_NE(nullptr, stateSynchronizer);

    EXPECT_CALL(*m_mockContextManager, getContext(NotNull()));
    stateSynchronizer->onConnectionStatusChanged(
            ConnectionStatusObserverInterface::Status::CONNECTED,
            ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

}

/**
 * This case tests if @c onConnectionStatusChanged does not trigger @c getContext when disconnected.
 */
TEST_F(StateSynchronizerTest, noConnectedTriggersNothing) {
    auto strictMockMessageSender = std::make_shared<StrictMock<MockMessageSender>>();
    auto strictMockContextManager = std::make_shared<StrictMock<MockContextManager>>();
    auto stateSynchronizer = StateSynchronizer::create(strictMockContextManager, strictMockMessageSender);
    ASSERT_NE(nullptr, stateSynchronizer);

    stateSynchronizer->onConnectionStatusChanged(
            ConnectionStatusObserverInterface::Status::DISCONNECTED,
            ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

/**
 * This case tests if @c onContextReceived sends a message request to the message sender interface.
 */
TEST_F(StateSynchronizerTest, contextReceivedSendsMessage) {
    auto strictMockContextManager = std::make_shared<StrictMock<MockContextManager>>();
    auto stateSynchronizer = StateSynchronizer::create(strictMockContextManager, m_mockMessageSender);
    ASSERT_NE(nullptr, stateSynchronizer);

    stateSynchronizer->onContextAvailable(MOCK_CONTEXT);
}

/**
 * This case tests if @c onContextReceived sends a message request to the message sender interface.
 */
TEST_F(StateSynchronizerTest, contextReceivedSendsMessageAndNotifiesObserver) {
    auto stateSynchronizer = StateSynchronizer::create(m_mockContextManager, m_mockMessageSender);
    ASSERT_NE(nullptr, stateSynchronizer);

    EXPECT_CALL(
            *m_mockStateSynchronizerObserver,
            onStateChanged(StateSynchronizerObserverInterface::State::NOT_SYNCHRONIZED)).Times(1);
    stateSynchronizer->addObserver(m_mockStateSynchronizerObserver);

    EXPECT_CALL(*m_mockContextManager, getContext(NotNull()));
    stateSynchronizer->onConnectionStatusChanged(
            ConnectionStatusObserverInterface::Status::CONNECTED,
            ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);

    EXPECT_CALL(
            *m_mockStateSynchronizerObserver,
            onStateChanged(StateSynchronizerObserverInterface::State::SYNCHRONIZED)).Times(1);
    stateSynchronizer->onContextAvailable(MOCK_CONTEXT);
}

} // namespace test
} // namespace system
} // namespace capabilityAgents
} // namespace alexaClientSDK
