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

#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <SynchronizeStateSender/PostConnectSynchronizeStateSender.h>

namespace alexaClientSDK {
namespace synchronizeStateSender {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::json::jsonUtils;

/// String indicating the device's context.
static const std::string TEST_CONTEXT_VALUE = "{}";

/// String indicating the SynchronizeState event's expected namespace.
static const std::string EXPECTED_NAMESPACE = "System";

/// String indicating the SynchronizeState event's expected name.
static const std::string EXPECTED_NAME = "SynchronizeState";

/// String indicating the SynchronizeState event's expected payload.
static const std::string EXPECTED_PAYLOAD = "{}";

/// Request token used to mock getContext return value.
static const ContextRequestToken MOCK_CONTEXT_REQUEST_TOKEN = 1;

/// Number of retries used in tests.
static const int TEST_RETRY_COUNT = 3;
/**
 * Test harness for @c PostConnectSynchronizeStateSender class.
 */
class PostConnectSynchronizeStateSenderTest : public Test {
public:
    /// Setup the test harness for running the test.
    void SetUp() override;

    /// Clean up the test harness after running the test.
    void TearDown() override;

protected:
    /// The mock @c ContextManager used to test.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// The mock @c PostConnectSendMessage used to test.
    std::shared_ptr<MockMessageSender> m_mockPostConnectSendMessage;

    /// The mock @c ContextManager used to test.
    std::thread m_mockContextManagerThread;

    /// The executor thread to run @c MockPostConnectSendMessage.
    std::thread m_mockPostConnectSenderThread;

    /// The executor thread to run @c MockPostConnectSendMessage.
    std::shared_ptr<PostConnectSynchronizeStateSender> m_postConnectSynchronizeStateSender;
};

struct EventData {
    std::string contextString;
    std::string namespaceString;
    std::string nameString;
    std::string payloadString;
};

bool parseEventJson(const std::string& eventJson, EventData* eventData) {
    if (!retrieveValue(eventJson, "context", &eventData->contextString)) {
        return false;
    }
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

    if (eventData.contextString != TEST_CONTEXT_VALUE || eventData.nameString != EXPECTED_NAME ||
        eventData.namespaceString != EXPECTED_NAMESPACE || eventData.payloadString != EXPECTED_PAYLOAD) {
        return false;
    }

    return true;
}

void PostConnectSynchronizeStateSenderTest::SetUp() {
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockPostConnectSendMessage = std::make_shared<NiceMock<MockMessageSender>>();

    m_postConnectSynchronizeStateSender = PostConnectSynchronizeStateSender::create(m_mockContextManager);
}

void PostConnectSynchronizeStateSenderTest::TearDown() {
    if (m_mockContextManagerThread.joinable()) {
        m_mockContextManagerThread.join();
    }

    if (m_mockPostConnectSenderThread.joinable()) {
        m_mockPostConnectSenderThread.join();
    }
}

/**
 * Test create with null @c ContextManager.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_createWithNullContextManager) {
    auto instance = PostConnectSynchronizeStateSender::create(nullptr);
    ASSERT_EQ(instance, nullptr);
}

/**
 * Test create with @c MetricRecorder.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_createWithMetricRecorder) {
    ASSERT_THAT(PostConnectSynchronizeStateSender::create(m_mockContextManager, nullptr), NotNull());
    ASSERT_THAT(
        PostConnectSynchronizeStateSender::create(
            m_mockContextManager, std::make_shared<avsCommon::utils::metrics::test::MockMetricRecorder>()),
        NotNull());
}

/**
 * Test GetPriority method for @c PostConnectSynchronizeStateSender.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_getOperationPriority) {
    ASSERT_EQ(
        m_postConnectSynchronizeStateSender->getOperationPriority(),
        static_cast<unsigned int>(PostConnectOperationInterface::SYNCHRONIZE_STATE_PRIORITY));
}

/**
 * Test happy case for performOperation method. SynchronizeState event is sent and performOperation() returns true.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_perfromOperationSendsSynchronizeStateEvent) {
    auto getContextLambda = [this](
                                std::shared_ptr<ContextRequesterInterface> contextRequester,
                                const std::string& endpointId,
                                const std::chrono::milliseconds& timeout) {
        if (m_mockContextManagerThread.joinable()) {
            m_mockContextManagerThread.join();
        }
        m_mockContextManagerThread =
            std::thread([contextRequester]() { contextRequester->onContextAvailable(TEST_CONTEXT_VALUE); });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillOnce(Invoke(getContextLambda));

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

    ASSERT_TRUE(m_postConnectSynchronizeStateSender->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test performOperation() method retries sending SynchronizeState event on context fetch failure.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_performOperationRetriesOnContextFailure) {
    std::promise<int> retryCountPromise;
    auto getContextLambda = [this, &retryCountPromise](
                                std::shared_ptr<ContextRequesterInterface> contextRequester,
                                const std::string& endpointId,
                                const std::chrono::milliseconds& timeout) {
        if (m_mockContextManagerThread.joinable()) {
            m_mockContextManagerThread.join();
        }
        m_mockContextManagerThread = std::thread([this, contextRequester, &retryCountPromise]() {
            contextRequester->onContextFailure(ContextRequestError::STATE_PROVIDER_TIMEDOUT);
            static int count = 0;
            count++;
            /// abort operation after 3 retries.
            if (count == TEST_RETRY_COUNT) {
                retryCountPromise.set_value(count);
                m_postConnectSynchronizeStateSender->abortOperation();
            }
        });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillRepeatedly(Invoke(getContextLambda));
    /// Synchronize State event not sent.
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).Times(0);
    /// Abort after 3 retries.
    EXPECT_FALSE(m_postConnectSynchronizeStateSender->performOperation(m_mockPostConnectSendMessage));
    EXPECT_EQ(retryCountPromise.get_future().get(), TEST_RETRY_COUNT);
}

/**
 * Test performOperation() method retries sending SynchronizeState event on unsuccessful response to SynchronizeState
 * event.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_testPerfromOperationRetriesOnUnsuccessfulResponse) {
    std::promise<int> retryCountPromise;
    auto getContextLambda = [this](
                                std::shared_ptr<ContextRequesterInterface> contextRequester,
                                const std::string& endpointId,
                                const std::chrono::milliseconds& timeout) {
        if (m_mockContextManagerThread.joinable()) {
            m_mockContextManagerThread.join();
        }
        m_mockContextManagerThread =
            std::thread([contextRequester]() { contextRequester->onContextAvailable(TEST_CONTEXT_VALUE); });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillRepeatedly(Invoke(getContextLambda));

    auto sendMessageLambda = [this, &retryCountPromise](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }
        m_mockPostConnectSenderThread = std::thread([this, request, &retryCountPromise]() {
            ASSERT_TRUE(validateEvent(request->getJsonContent()));
            request->sendCompleted(MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            static int count = 0;
            count++;
            /// abort operation after 3 retries.
            if (count == TEST_RETRY_COUNT) {
                retryCountPromise.set_value(count);

                std::thread localThread([this]() { m_postConnectSynchronizeStateSender->abortOperation(); });
                if (localThread.joinable()) {
                    localThread.join();
                }
            }
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillRepeatedly(Invoke(sendMessageLambda));
    /// Abort after 3 retries.
    EXPECT_FALSE(m_postConnectSynchronizeStateSender->performOperation(m_mockPostConnectSendMessage));
    EXPECT_EQ(retryCountPromise.get_future().get(), TEST_RETRY_COUNT);
}

/**
 * Test abortOperation() triggers performOperation() to return when context fetch is in progress.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_abortOperationWhenContextRequestInProgress) {
    auto getContextLambda = [this](
                                std::shared_ptr<ContextRequesterInterface> contextRequester,
                                const std::string& endpointId,
                                const std::chrono::milliseconds& timeout) {
        if (m_mockContextManagerThread.joinable()) {
            m_mockContextManagerThread.join();
        }
        m_mockContextManagerThread = std::thread([this]() {
            /// Wait for 500 ms and spin a thread to abort the post connect operation.
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::thread localThread([this]() { m_postConnectSynchronizeStateSender->abortOperation(); });
            if (localThread.joinable()) {
                localThread.join();
            }
        });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillRepeatedly(Invoke(getContextLambda));
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).Times(0);

    EXPECT_FALSE(m_postConnectSynchronizeStateSender->performOperation(m_mockPostConnectSendMessage));
}

/**
 * Test abortOperation() triggers performOperation() to return when SynchronizeState event send is in progress.
 */
TEST_F(PostConnectSynchronizeStateSenderTest, test_abortOperationWhenSendMessageInProgress) {
    auto getContextLambda = [this](
                                std::shared_ptr<ContextRequesterInterface> contextRequester,
                                const std::string& endpointId,
                                const std::chrono::milliseconds& timeout) {
        if (m_mockContextManagerThread.joinable()) {
            m_mockContextManagerThread.join();
        }
        m_mockContextManagerThread =
            std::thread([contextRequester]() { contextRequester->onContextAvailable(TEST_CONTEXT_VALUE); });
        return MOCK_CONTEXT_REQUEST_TOKEN;
    };
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _)).WillRepeatedly(Invoke(getContextLambda));

    auto sendMessageLambda = [this](std::shared_ptr<MessageRequest> request) {
        if (m_mockPostConnectSenderThread.joinable()) {
            m_mockPostConnectSenderThread.join();
        }
        m_mockPostConnectSenderThread = std::thread([this, request]() {
            ASSERT_TRUE(validateEvent(request->getJsonContent()));
            /// Wait for 500 ms and spin a thread to abort the post connect operation.
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::thread localThread([this]() { m_postConnectSynchronizeStateSender->abortOperation(); });
            if (localThread.joinable()) {
                localThread.join();
            }
        });
    };
    EXPECT_CALL(*m_mockPostConnectSendMessage, sendMessage(_)).WillOnce(Invoke(sendMessageLambda));
    EXPECT_FALSE(m_postConnectSynchronizeStateSender->performOperation(m_mockPostConnectSendMessage));
}

}  // namespace test
}  // namespace synchronizeStateSender
}  // namespace alexaClientSDK
