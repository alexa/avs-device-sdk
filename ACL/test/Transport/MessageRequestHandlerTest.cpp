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

#include <gtest/gtest.h>

#include <ACL/Transport/MessageRequestHandler.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestInterface.h>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {
static const std::string AUTHORIZATION_HEADER = "Authorization: Bearer ";
static const std::string AUTH_TOKEN = "authToken";

using namespace ::testing;

class MessageRequestHandlerTest : public Test {};

class MockExchangeHandlerContext : public ExchangeHandlerContextInterface {
public:
    class HTTP2Request : public avsCommon::utils::http2::HTTP2RequestInterface {
    public:
        bool cancel() override {
            return false;
        }
        std::string getId() const override {
            return "TestId";
        }
    };
    void onDownchannelConnected() override {
    }
    void onDownchannelFinished() override {
    }
    void onMessageRequestSent(const std::shared_ptr<avsCommon::avs::MessageRequest>& request) override {
    }
    void onMessageRequestTimeout() override {
    }
    void onMessageRequestAcknowledged(const std::shared_ptr<avsCommon::avs::MessageRequest>& request) override {
    }
    void onMessageRequestFinished() override {
    }
    void onPingRequestAcknowledged(bool success) override {
    }
    void onPingTimeout() override {
    }
    void onActivity() override {
    }
    void onForbidden(const std::string& authToken) override {
    }
    std::shared_ptr<avsCommon::utils::http2::HTTP2RequestInterface> createAndSendRequest(
        const avsCommon::utils::http2::HTTP2RequestConfig& cfg) override {
        return std::make_shared<HTTP2Request>();
    }
    std::string getAVSGateway() override {
        return "";
    }
};

/**
 * Test if extra headers are preserved in message request handler
 */
TEST_F(MessageRequestHandlerTest, test_headers) {
    auto messageRequest = std::shared_ptr<avsCommon::avs::MessageRequest>(
        new avsCommon::avs::MessageRequest("{}", true, "", {{"k1", "v1"}, {"k2", "v2"}}));

    auto classUnderTest = MessageRequestHandler::create(
        std::make_shared<MockExchangeHandlerContext>(),
        AUTH_TOKEN,
        messageRequest,
        std::shared_ptr<MessageConsumerInterface>(),
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface>(),
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>());

    const std::vector<std::string> actual = classUnderTest->getRequestHeaderLines();
    std::vector<std::string> expected{AUTHORIZATION_HEADER + AUTH_TOKEN, "k1: v1", "k2: v2"};
    EXPECT_EQ(actual, expected);
}
}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK