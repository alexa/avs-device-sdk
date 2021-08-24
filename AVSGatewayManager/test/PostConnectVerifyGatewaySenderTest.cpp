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

#include <future>
#include <memory>
#include <thread>

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSGatewayManager/PostConnectVerifyGatewaySender.h>

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::json::jsonUtils;

/// String indicating the VerifyGateway event's expected namespace.
static const std::string EXPECTED_NAMESPACE = "Alexa.ApiGateway";

/// String indicating the VerifyGateway event's expected name.
static const std::string EXPECTED_NAME = "VerifyGateway";

/// String indicating the VerifyGateway event's expected payload.
static const std::string EXPECTED_PAYLOAD = "{}";

/// Number of retries used in tests.
static const int TEST_RETRY_COUNT = 3;

/**
 * Test harness for @c PostConnectVerifyGatewaySender class.
 */
class PostConnectVerifyGatewaySenderTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /// flag that will be set when the callback method is called.
    bool m_callbackCheck;
    /**
     * Callback that will be called when the postConnectVerifyGatewaySender receives a successful response.
     *
     * @param postConnectVerifyGatewaySender The @c PostConnectVerifyGatewaySender instance.
     */
    void updateStateCallback(std::shared_ptr<PostConnectVerifyGatewaySender> postConnectVerifyGatewaySender);

    /// The instance of the @c PostConnectVerifyGatewaySender that will be used in the tests.
    std::shared_ptr<PostConnectVerifyGatewaySender> m_postConnectVerifyGatewaySender;

    /// The executor thread to run @c MockPostConnectSendMessage.
    std::thread m_mockPostConnectSenderThread;

    /// The mock @c PostConnectSendMessage used to test.
    std::shared_ptr<MockMessageSender> m_mockPostConnectSendMessage;
};

struct EventData {
    std::string namespaceString;
    std::string nameString;
    std::string payloadString;
};

bool parseEventJson(const std::string& eventJson, EventData* eventData) {
    std::string eventString;
    if (!retrieveValue(eventJson, "event", &eventString)) {
        return false;
    }

    std::string headerString;
    if (!retrieveValue(eventString, "header", &headerString)) {
        return false;
    }

    if (!retrieveValue(headerString, "namespace", &eventData->namespaceString)) {
        return false;
    }

    if (!retrieveValue(headerString, "name", &eventData->nameString)) {
        return false;
    }

    if (!retrieveValue(eventString, "payload", &eventData->payloadString)) {
        return false;
    }
    return true;
}

bool validateEvent(const std::string& eventJson) {
    EventData eventData;
    if (!parseEventJson(eventJson, &eventData)) {
        return false;
    }

    if (eventData.nameString != EXPECTED_NAME || eventData.namespaceString != EXPECTED_NAMESPACE ||
        eventData.payloadString != EXPECTED_PAYLOAD) {
        return false;
    }

    return true;
}

void PostConnectVerifyGatewaySenderTest::SetUp() {
    m_callbackCheck = false;
    m_mockPostConnectSendMessage = std::make_shared<NiceMock<MockMessageSender>>();
    auto callback = std::bind(&PostConnectVerifyGatewaySenderTest::updateStateCallback, this, std::placeholders::_1);
    m_postConnectVerifyGatewaySender = PostConnectVerifyGatewaySender::create(callback);
}

void PostConnectVerifyGatewaySenderTest::TearDown() {
    if (m_mockPostConnectSenderThread.joinable()) {
        m_mockPostConnectSenderThread.join();
    }
}

void PostConnectVerifyGatewaySenderTest::updateStateCallback(
    std::shared_ptr<PostConnectVerifyGatewaySender> postConnectVerifyGatewaySender) {
    if (postConnectVerifyGatewaySender == m_postConnectVerifyGatewaySender) {
        m_callbackCheck = true;
    }
}

/**
 * Test if the create method fails
 */
TEST_F(PostConnectVerifyGatewaySenderTest, test_creaetWithEmptyCallbackMethod) {
    std::function<void(const std::shared_ptr<PostConnectVerifyGatewaySender>&)> callback;
    auto instance = PostConnectVerifyGatewaySender::create(callback);
}

/**
 * Test happy path where performGateway returns true when VerifyGateway event is sent and 204 response is received.
 */
TEST_F(PostConnectVerifyGatewaySenderTest, test_performGatewayWhen204IsReceived) {
    auto sendMessageLambda = [this](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }

        m_mockPostConnectSenderThread = std::thread([request]() {
            EXPECT_TRUE(validateEvent(request->getJsonContent()));
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillOnce(Invoke(sendMessageLambda));
    ASSERT_TRUE(m_postConnectVerifyGatewaySender->performOperation(m_mockPostConnectSendMessage));
    ASSERT_TRUE(m_callbackCheck);
}

/**
 * Test if performGateway returns false when VerifyGateway event is sent and 200 response is received.
 */
TEST_F(PostConnectVerifyGatewaySenderTest, test_performGatewayWhen200IsReceived) {
    auto sendMessageLambda = [this](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }

        m_mockPostConnectSenderThread = std::thread([request]() {
            EXPECT_TRUE(validateEvent(request->getJsonContent()));
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillOnce(Invoke(sendMessageLambda));
    ASSERT_TRUE(m_postConnectVerifyGatewaySender->performOperation(m_mockPostConnectSendMessage));
    ASSERT_FALSE(m_callbackCheck);
}

/**
 * Test if performGateway returns false when VerifyGateway event is sent and 400 response is received.
 */
TEST_F(PostConnectVerifyGatewaySenderTest, test_performGatewayWhen400IsReceived) {
    auto sendMessageLambda = [this](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }

        m_mockPostConnectSenderThread = std::thread([request]() {
            EXPECT_TRUE(validateEvent(request->getJsonContent()));
            request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillOnce(Invoke(sendMessageLambda));
    ASSERT_FALSE(m_postConnectVerifyGatewaySender->performOperation(m_mockPostConnectSendMessage));
    ASSERT_FALSE(m_callbackCheck);
}

/**
 * Test if performGateway retries sending VerifyGateway event when a 5xx response is received.
 */
TEST_F(PostConnectVerifyGatewaySenderTest, test_performGatewayRetriesWhen503IsReceived) {
    std::promise<int> retryCountPromise;
    auto sendMessageLambda = [this, &retryCountPromise](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }

        m_mockPostConnectSenderThread = std::thread([this, request, &retryCountPromise]() {
            EXPECT_TRUE(validateEvent(request->getJsonContent()));
            request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            static int count = 0;
            count++;

            if (TEST_RETRY_COUNT == count) {
                retryCountPromise.set_value(count);

                std::thread localThread([this]() { m_postConnectVerifyGatewaySender->abortOperation(); });
                if (localThread.joinable()) {
                    localThread.join();
                }
            }
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillRepeatedly(Invoke(sendMessageLambda));
    ASSERT_FALSE(m_postConnectVerifyGatewaySender->performOperation(m_mockPostConnectSendMessage));
    EXPECT_EQ(retryCountPromise.get_future().get(), TEST_RETRY_COUNT);
    ASSERT_FALSE(m_callbackCheck);
}

/**
 * Test if performOperation wakes up from wait when it receives a wake up call while waiting to retry
 */
TEST_F(PostConnectVerifyGatewaySenderTest, test_performGatewayWakesUpAndWaitingToRetry) {
    std::promise<int> retryCountPromise;
    auto sendMessageLambda = [this, &retryCountPromise](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }

        m_mockPostConnectSenderThread = std::thread([this, request, &retryCountPromise]() {
            EXPECT_TRUE(validateEvent(request->getJsonContent()));

            static int count = 0;
            if (count == 0) {
                request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            }
            count++;
            if (TEST_RETRY_COUNT == count) {
                retryCountPromise.set_value(count);
                std::thread localThread([this, request]() { m_postConnectVerifyGatewaySender->wakeOperation(); });
                request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);

                if (localThread.joinable()) {
                    localThread.join();
                }
            }
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillRepeatedly(Invoke(sendMessageLambda));
    ASSERT_TRUE(m_postConnectVerifyGatewaySender->performOperation(m_mockPostConnectSendMessage));
    EXPECT_EQ(retryCountPromise.get_future().get(), TEST_RETRY_COUNT);
    ASSERT_TRUE(m_callbackCheck);
}

}  // namespace test
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
