/*
 * PlaybackControllerTest.cpp
 *
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

/// @file PlaybackControllerTest
#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockPlaybackRouter.h>

#include "PlaybackController/PlaybackController.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::json;
using namespace testing;

/// String to identify the AVS namespace of the event we send.
static const std::string PLAYBACK_CONTROLLER_NAMESPACE = "PlaybackController";

/// String to identify the AVS name of the event on the 'Play' button pressed.
static const std::string PLAYBACK_PLAY_NAME = "PlayCommandIssued";
/// String to identify the AVS name of the event on the 'Pause' button pressed.
static const std::string PLAYBACK_PAUSE_NAME = "PauseCommandIssued";
/// String to identify the AVS name of the event on the 'Next' button pressed.
static const std::string PLAYBACK_NEXT_NAME = "NextCommandIssued";
/// String to identify the AVS name of the event on the 'Previous' button pressed.
static const std::string PLAYBACK_PREVIOUS_NAME = "PreviousCommandIssued";

/// String to test for MessageRequest exceptionReceived()
static const std::string TEST_EXCEPTION_TEXT = "Exception test";

/// A short period of time to wait for the m_contextTrigger or m_messageTrigger.
static const std::chrono::milliseconds TEST_RESULT_WAIT_PERIOD{100};

/// A mock context returned by MockContextManager.
// clang-format off
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
// clang-format on

/**
 * Check if message request has errors.
 *
 * @param messageRequest The message requests to be checked.
 * @return "ERROR" if parsing the JSON has any unexpected results and the payload is empty,
 * otherwise return the name of the message.
 */
static std::string checkMessageRequest(std::shared_ptr<MessageRequest> messageRequest) {
    const std::string error = "ERROR";
    rapidjson::Document jsonContent(rapidjson::kObjectType);
    if (jsonContent.Parse(messageRequest->getJsonContent()).HasParseError()) {
        return error;
    }
    rapidjson::Value::ConstMemberIterator eventNode;
    if (!jsonUtils::findNode(jsonContent, "event", &eventNode)) {
        return error;
    }

    // get payload
    rapidjson::Value::ConstMemberIterator payloadNode;
    if (!jsonUtils::findNode(eventNode->value, "payload", &payloadNode)) {
        return error;
    }

    // check payload is empty
    if (!payloadNode->value.ObjectEmpty()) {
        return error;
    }

    // get header
    rapidjson::Value::ConstMemberIterator headerIt;
    if (!jsonUtils::findNode(eventNode->value, "header", &headerIt)) {
        return error;
    }

    // verify namespace
    std::string avsNamespace;
    if (!jsonUtils::retrieveValue(headerIt->value, "namespace", &avsNamespace)) {
        return error;
    }
    if (avsNamespace != PLAYBACK_CONTROLLER_NAMESPACE) {
        return error;
    }

    // return name
    std::string avsName;
    if (!jsonUtils::retrieveValue(headerIt->value, "name", &avsName)) {
        return error;
    }
    return avsName;
}

/// Test harness for @c StateSynchronizer class.
class PlaybackControllerTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

protected:
    /**
     * Check if message contextRequester is a nullptr and notify the m_contextTrigger to continue execution of the test.
     *
     * @param func The callable object to execute the key press API.
     * @param expectedMessageName The string of the expected AVS message name sent by the key pressed.
     */
    void verifyButtonPressed(std::function<void()> func, const std::string& expectedMessageName);

    /**
     * Check if message request has errors and notify the g_messageTrigger to continue execution of the test.
     *
     * @param messageRequest The message requests to be checked.
     * @param sendException If this is true, then an exceptionReceived() will be sent, otherwise sendCompleted()
     * @param expected_name The string of the expected AVS message name sent.
     */
    void checkMessageRequestAndReleaseTrigger(
        std::shared_ptr<MessageRequest> messageRequest,
        bool sendException,
        const std::string& expected_name);

    /**
     * Check if message contextRequester is a nullptr and notify the m_contextTrigger to continue execution of the test.
     *
     * @param contextRequester The @c ContextRequesterInterface.
     */
    void checkGetContextAndReleaseTrigger(std::shared_ptr<ContextRequesterInterface> contextRequester);

    /// This holds the return status of sendMessage() calls.
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status m_messageStatus;

    /// Mocked Context Manager. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// Mocked Message Sender. Note that we make it a strict mock to ensure we test the flow completely.
    std::shared_ptr<StrictMock<MockMessageSender>> m_mockMessageSender;

    /// PlaybackController instance.
    std::shared_ptr<PlaybackController> m_playbackController;

    /// This is the condition variable to be used to control sending of a message in test cases.
    std::condition_variable m_messageTrigger;

    /// This is the condition variable to be used to control getting of a context in test cases.
    std::condition_variable m_contextTrigger;

    /// mutex for the conditional variables.
    std::mutex m_mutex;
};

void PlaybackControllerTest::SetUp() {
    m_mockContextManager = std::make_shared<StrictMock<MockContextManager>>();
    m_mockMessageSender = std::make_shared<StrictMock<MockMessageSender>>();

    // initialize m_contextTrigger to success
    m_messageStatus = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS;

    m_playbackController = PlaybackController::create(m_mockContextManager, m_mockMessageSender);
    ASSERT_NE(nullptr, m_playbackController);
}

void PlaybackControllerTest::TearDown() {
    if (m_playbackController) {
        m_playbackController->shutdown();
    }
}

void PlaybackControllerTest::verifyButtonPressed(std::function<void()> func, const std::string& expectedMessageName) {
    std::unique_lock<std::mutex> exitLock(m_mutex);

    EXPECT_CALL(*m_mockContextManager, getContext(_))
        .WillOnce(Invoke([this](std::shared_ptr<ContextRequesterInterface> contextRequester) {
            checkGetContextAndReleaseTrigger(contextRequester);
        }));
    func();
    m_contextTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([this, expectedMessageName](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            checkMessageRequestAndReleaseTrigger(request, false, expectedMessageName);
        }));
    m_playbackController->onContextAvailable(MOCK_CONTEXT);
    m_messageTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
}

void PlaybackControllerTest::checkGetContextAndReleaseTrigger(
    std::shared_ptr<ContextRequesterInterface> contextRequester) {
    m_contextTrigger.notify_one();
    EXPECT_THAT(contextRequester, NotNull());
}

void PlaybackControllerTest::checkMessageRequestAndReleaseTrigger(
    std::shared_ptr<MessageRequest> messageRequest,
    bool sendException,
    const std::string& expected_name) {
    auto returnValue = checkMessageRequest(messageRequest);
    m_messageTrigger.notify_one();
    if (sendException) {
        messageRequest->exceptionReceived(TEST_EXCEPTION_TEXT);
    } else {
        messageRequest->sendCompleted(m_messageStatus);
    }
    EXPECT_EQ(returnValue, expected_name);
}

/**
 * This case tests if @c StateSynchronizer basic create function works properly
 */
TEST_F(PlaybackControllerTest, createSuccessfully) {
    ASSERT_NE(nullptr, PlaybackController::create(m_mockContextManager, m_mockMessageSender));
}

/**
 * This case tests if possible @c nullptr parameters passed to @c StateSynchronizer::create are handled properly.
 */
TEST_F(PlaybackControllerTest, createWithError) {
    ASSERT_EQ(nullptr, PlaybackController::create(m_mockContextManager, nullptr));
    ASSERT_EQ(nullptr, PlaybackController::create(nullptr, m_mockMessageSender));
    ASSERT_EQ(nullptr, PlaybackController::create(nullptr, nullptr));
}

/**
 * This case tests if playButtonPressed will send the correct event message.
 */
TEST_F(PlaybackControllerTest, playButtonPressed) {
    PlaybackControllerTest::verifyButtonPressed(
        [this]() { m_playbackController->onButtonPressed(PlaybackButton::PLAY); }, PLAYBACK_PLAY_NAME);
}

/**
 * This case tests if pauseButtonPressed will send the correct event message.
 */
TEST_F(PlaybackControllerTest, pauseButtonPressed) {
    ASSERT_NE(nullptr, m_playbackController);

    PlaybackControllerTest::verifyButtonPressed(
        [this]() { m_playbackController->onButtonPressed(PlaybackButton::PAUSE); }, PLAYBACK_PAUSE_NAME);
}

/**
 * This case tests if nextButtonPressed will send the correct event message.
 */
TEST_F(PlaybackControllerTest, nextButtonPressed) {
    PlaybackControllerTest::verifyButtonPressed(
        [this]() { m_playbackController->onButtonPressed(PlaybackButton::NEXT); }, PLAYBACK_NEXT_NAME);
}

/**
 * This case tests if previousButtonPressed will send the correct event message.
 */
TEST_F(PlaybackControllerTest, previousButtonPressed) {
    PlaybackControllerTest::verifyButtonPressed(
        [this]() { m_playbackController->onButtonPressed(PlaybackButton::PREVIOUS); }, PLAYBACK_PREVIOUS_NAME);
}

/**
 * This case tests if getContext() returns failure, the button on the top of the queue will be dropped and getContext
 * will be called for the next button on the queue.
 */
TEST_F(PlaybackControllerTest, getContextFailure) {
    std::unique_lock<std::mutex> exitLock(m_mutex);

    EXPECT_CALL(*m_mockContextManager, getContext(_))
        .WillOnce(Invoke([this](std::shared_ptr<ContextRequesterInterface> contextRequester) {
            checkGetContextAndReleaseTrigger(contextRequester);
        }));
    // queue two button presses
    m_playbackController->onButtonPressed(PlaybackButton::PLAY);
    m_playbackController->onButtonPressed(PlaybackButton::PAUSE);
    // wait for first call of getContext
    m_contextTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);

    // expect no call of sendMessage for any button
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(0);

    // expect a call to getContext again when onContextFailure is received
    EXPECT_CALL(*m_mockContextManager, getContext(_))
        .WillOnce(Invoke([this](std::shared_ptr<ContextRequesterInterface> contextRequester) {
            checkGetContextAndReleaseTrigger(contextRequester);
        }));
    m_playbackController->onContextFailure(avsCommon::sdkInterfaces::ContextRequestError::BUILD_CONTEXT_ERROR);
    m_contextTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);

    // now expect call of sendMessage for the pause button
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            checkMessageRequestAndReleaseTrigger(request, false, PLAYBACK_PAUSE_NAME);
        }));
    m_playbackController->onContextAvailable(MOCK_CONTEXT);
    m_messageTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
}

/**
 * This case tests if sendMessage() returns failure, an error log should be logged with the button pressed and reason
 * for failure.
 */
TEST_F(PlaybackControllerTest, sendMessageFailure) {
    std::unique_lock<std::mutex> exitLock(m_mutex);

    m_messageStatus = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR;
    EXPECT_CALL(*m_mockContextManager, getContext(_))
        .WillOnce(Invoke([this](std::shared_ptr<ContextRequesterInterface> contextRequester) {
            checkGetContextAndReleaseTrigger(contextRequester);
        }));
    m_playbackController->onButtonPressed(PlaybackButton::NEXT);
    m_contextTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            checkMessageRequestAndReleaseTrigger(request, false, PLAYBACK_NEXT_NAME);
        }));

    m_playbackController->onContextAvailable(MOCK_CONTEXT);
    m_messageTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
}

/**
 * This case tests if exceptionReceived() is received, an error log should be logged with with the exception
 * description.
 */
TEST_F(PlaybackControllerTest, sendMessageException) {
    std::unique_lock<std::mutex> exitLock(m_mutex);

    m_messageStatus = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR;
    EXPECT_CALL(*m_mockContextManager, getContext(_))
        .WillOnce(Invoke([this](std::shared_ptr<ContextRequesterInterface> contextRequester) {
            checkGetContextAndReleaseTrigger(contextRequester);
        }));
    m_playbackController->onButtonPressed(PlaybackButton::NEXT);
    m_contextTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_))
        .WillOnce(Invoke([this](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
            checkMessageRequestAndReleaseTrigger(request, true, PLAYBACK_NEXT_NAME);
        }));

    m_playbackController->onContextAvailable(MOCK_CONTEXT);
    m_messageTrigger.wait_for(exitLock, TEST_RESULT_WAIT_PERIOD);
}

}  // namespace test
}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
