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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/MessageRequest.h"
#include "AVSCommon/AVS/EditableMessageRequest.h"

using namespace ::testing;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/// Valid test event.
// clang-format off
static const std::string VALID_TEST_EVENT = R"({
    "event": {
        "header": {
            "namespace": "test_namespace",
            "name": "test_name",
            "messageId": "test_messageId",
            "dialogRequestId": "test_dialogRequestId"
        },
        "payload": {}
    }
})";
// clang-format on

/// Partially valid test event.
// clang-format off
static const std::string PARTIALLY_VALID_TEST_EVENT = R"({
    "event": {
        "header": {
            "namespace": "test_namespace",
            "messageId": "test_messageId",
            "dialogRequestId": "test_dialogRequestId"
        },
        "payload": {}
    }
})";
// clang-format on

/// Incorrectly formatted test event.
// clang-format off
static const std::string INCORRECTLY_FORMATTED_TEST_EVENT = R"({
    "event": {
        "namespace": "test_namespace",
        "messageId": "test_messageId",
        "dialogRequestId": "test_dialogRequestId",
        "payload": {}
    }
})";
// clang-format on

/// Invalid test event.
// clang-format off
static const std::string INVALID_TEST_EVENT = R"({
    "event": {
        "header": {
            "messageId": "test_messageId",
            "dialogRequestId": "test_dialogRequestId"
        },
        "payload": {}
    }
})";
// clang-format on

/// Test event namespace header value.
static const std::string TEST_NAMESPACE = "test_namespace";

/// Test event name header value.
static const std::string TEST_NAME = "test_name";

class MockAttachmentReader : public attachment::AttachmentReader {
public:
    MOCK_METHOD4(
        read,
        std::size_t(
            void* buf,
            std::size_t numBytes,
            attachment::AttachmentReader::ReadStatus* readStatus,
            std::chrono::milliseconds timeoutMs));
    MOCK_METHOD1(seek, bool(uint64_t offset));
    MOCK_METHOD0(getNumUnreadBytes, uint64_t());
    MOCK_METHOD1(close, void(attachment::AttachmentReader::ClosePoint closePoint));
};

class MockMessageRequestObserver : public avsCommon::sdkInterfaces::MessageRequestObserverInterface {
public:
    MOCK_METHOD1(onResponseStatusReceived, void(MessageRequestObserverInterface::Status status));
    MOCK_METHOD1(onSendCompleted, void(MessageRequestObserverInterface::Status status));
    MOCK_METHOD1(onExceptionReceived, void(const std::string& exceptionMessage));
};

class MessageRequestTest : public ::testing::Test {};

/// Test copy constructor
TEST_F(MessageRequestTest, test_copyConstructor) {
    std::string jsonContent = "{\"name\": \"value\"}";
    std::shared_ptr<attachment::AttachmentReader> attachmentReader = std::make_shared<MockAttachmentReader>();
    std::string uri = "/test/uri";
    MockFunction<bool(const std::shared_ptr<EditableMessageRequest>&, std::string)> mockResolverFunc;
    auto mockMessageRequestObserver = std::make_shared<MockMessageRequestObserver>();
    MessageRequest request{
        jsonContent, true, uri, std::vector<std::pair<std::string, std::string>>{}, mockResolverFunc.AsStdFunction()};
    request.addAttachmentReader("reader", attachmentReader);
    request.addObserver(mockMessageRequestObserver);

    auto copiedReq = request;

    EXPECT_EQ(jsonContent, copiedReq.getJsonContent());
    EXPECT_EQ(uri, copiedReq.getUriPathExtension());
    EXPECT_FALSE(copiedReq.isResolved());
    EXPECT_EQ(1, copiedReq.attachmentReadersCount());
    EXPECT_EQ(request.getAttachmentReader(0), copiedReq.getAttachmentReader(0));

    // Verify that observers are not copied in copy constructor.
    EXPECT_CALL(*mockMessageRequestObserver, onSendCompleted(_)).Times(1);
    request.sendCompleted(sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
    copiedReq.sendCompleted(sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
}

/**
 * Test functionality of adding extra headers
 */
TEST_F(MessageRequestTest, test_extraHeaders) {
    std::vector<std::pair<std::string, std::string>> expected({{"k1", "v1"}, {"k2", "v2"}});
    MessageRequest messageRequest("{}", true, "", expected);
    auto actual = messageRequest.getHeaders();

    EXPECT_EQ(expected, actual);
}

TEST_F(MessageRequestTest, test_eventHeaders) {
    auto expectedEventHeaders = MessageRequest::EventHeaders(TEST_NAMESPACE, TEST_NAME);
    MessageRequest messageRequest(VALID_TEST_EVENT, true, "", {});
    auto actualEventHeaders = messageRequest.retrieveEventHeaders();

    EXPECT_EQ(expectedEventHeaders.eventNamespace, actualEventHeaders.eventNamespace);
    EXPECT_EQ(expectedEventHeaders.eventName, actualEventHeaders.eventName);
}

TEST_F(MessageRequestTest, test_partialEventHeaders) {
    auto expectedEventHeaders = MessageRequest::EventHeaders(TEST_NAMESPACE, "");
    MessageRequest messageRequest(PARTIALLY_VALID_TEST_EVENT, true, "", {});
    auto actualEventHeaders = messageRequest.retrieveEventHeaders();

    EXPECT_EQ(expectedEventHeaders.eventNamespace, actualEventHeaders.eventNamespace);
    EXPECT_EQ(expectedEventHeaders.eventName, actualEventHeaders.eventName);
}

TEST_F(MessageRequestTest, test_incorrectlyFormattedEventHeaders) {
    auto expectedEventHeaders = MessageRequest::EventHeaders();
    MessageRequest messageRequest(INCORRECTLY_FORMATTED_TEST_EVENT, true, "", {});
    auto actualEventHeaders = messageRequest.retrieveEventHeaders();

    EXPECT_EQ(expectedEventHeaders.eventNamespace, actualEventHeaders.eventNamespace);
    EXPECT_EQ(expectedEventHeaders.eventName, actualEventHeaders.eventName);
}

TEST_F(MessageRequestTest, test_emptyEventHeaders) {
    auto expectedEventHeaders = MessageRequest::EventHeaders();
    MessageRequest messageRequest(INVALID_TEST_EVENT, true, "", {});
    auto actualEventHeaders = messageRequest.retrieveEventHeaders();

    EXPECT_EQ(expectedEventHeaders.eventNamespace, actualEventHeaders.eventNamespace);
    EXPECT_EQ(expectedEventHeaders.eventName, actualEventHeaders.eventName);
}

TEST_F(MessageRequestTest, test_isResolved) {
    MessageRequest resolvedReq("{}", true, "");
    EXPECT_TRUE(resolvedReq.isResolved());

    MockFunction<bool(const std::shared_ptr<EditableMessageRequest>&, std::string)> mockResolverFunc;
    MessageRequest unresolvedReq{
        "{}", true, "", std::vector<std::pair<std::string, std::string>>{}, mockResolverFunc.AsStdFunction()};
    EXPECT_FALSE(unresolvedReq.isResolved());
}

TEST_F(MessageRequestTest, test_resolveRequestFails) {
    MessageRequest resolvedReq("{}", true, "");
    EXPECT_TRUE(nullptr == resolvedReq.resolveRequest(""));
}

TEST_F(MessageRequestTest, test_resolveRequest) {
    MockFunction<bool(const std::shared_ptr<EditableMessageRequest>&, std::string)> mockResolverFunc;
    std::string resolvedJson = "resolvedJson";
    MessageRequest unresolvedReq{
        "{}", true, "", std::vector<std::pair<std::string, std::string>>{}, mockResolverFunc.AsStdFunction()};
    EXPECT_CALL(mockResolverFunc, Call(_, _))
        .WillOnce(Invoke([&](const std::shared_ptr<EditableMessageRequest>& req, const std::string& resolveKey) {
            req->setJsonContent(resolvedJson);
            return true;
        }));
    auto resolvedReq = unresolvedReq.resolveRequest("");
    EXPECT_TRUE(resolvedReq->isResolved());
    EXPECT_TRUE(resolvedReq != nullptr);
    EXPECT_EQ(resolvedJson, resolvedReq->getJsonContent());
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
