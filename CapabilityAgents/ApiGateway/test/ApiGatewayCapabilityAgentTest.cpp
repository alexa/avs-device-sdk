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
#include <gmock/gmock.h>

#include <ApiGateway/ApiGatewayCapabilityAgent.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/SDKInterfaces/MockAVSGatewayManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace apiGateway {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;

using namespace ::testing;

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alexa.ApiGateway";

/// The @c SetGateway directive signature.
static const NamespaceAndName SET_GATEWAY_REQUEST{NAMESPACE, "SetGateway"};

/// A sample gateway URL.
static const std::string TEST_GATEWAY_URL = "https://avs-alexa-na.amazon.com";

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string SET_GATEWAY_DIRECTIVE_JSON_STRING = R"(
    {
        "directive": {
            "header": {
                "namespace": ")" + NAMESPACE + R"(",
                "name": ")" + SET_GATEWAY_REQUEST.name +
                                                             R"(",
                "messageId": "12345"
            },
            "payload": {
                "gateway": ")" + TEST_GATEWAY_URL + R"("
            }
        }
    })";

/// An invalid NewDialogRequest directive with an incorrect name
static const std::string UNKNOWN_DIRECTIVE_JSON_STRING = R"(
    {
        "directive": {
            "header": {
                "namespace": ")" + NAMESPACE + R"(",
                "name": "NewDialogRequest1",
                "messageId": "12345"
            },
            "payload": {
                "gateway": ")" + TEST_GATEWAY_URL +
                                                         R"("
            }
        }
    })";

/// An invalid NewDialogRequest directive with no payload
static const std::string NO_PAYLOAD_SET_GATEWAY_DIRECTIVE_JSON_STRING = R"(
    {
        "directive": {
            "header": {
                "namespace": ")" + NAMESPACE + R"(",
                "name": ")" + SET_GATEWAY_REQUEST.name +
                                                                        R"(",
                "messageId": "12345"
            },
            "payload": {

            }
        }
    })";

/// An invalid NewDialogRequest with invalid dialogRequestID format
static const std::string INVALID_PAYLOAD_SET_GATEWAY_DIRECTIVE_JSON_STRING = R"(
    {
        "directive": {
            "header": {
                "namespace": ")" + NAMESPACE + R"(",
                "name": ")" + SET_GATEWAY_REQUEST.name +
                                                                             R"(",
                "messageId": "12345"
            },
            "payload": {
                "gateway": 2
            }
        }
    })";

/// Timeout when waiting for future to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/**
 * Test harness for @c ApiGatewayCapabilityAgent class.
 */
class ApiGatewayCapabilityAgentTest : public Test {
public:
    /// Setup the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness for running a test.
    void TearDown() override;

    /// Function to set the promise and wake @ m_wakeSetCompleteFuture.
    void wakeOnSetCompleted();

    /**
     * Constructor.
     */
    ApiGatewayCapabilityAgentTest() :
            m_wakeSetCompletedPromise{},
            m_wakeSetCompletedFuture{m_wakeSetCompletedPromise.get_future()} {
    }

protected:
    /// Promise to synchronize directive handling through setCompleted.
    std::promise<void> m_wakeSetCompletedPromise;

    /// Future to synchronize directive handling through setCompleted.
    std::future<void> m_wakeSetCompletedFuture;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// A strict mock that allows the test to strictly monitor calls to @c AVSGatewayManager.
    std::shared_ptr<StrictMock<MockAVSGatewayManager>> m_mockAVSGatewayManager;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A pointer to an instance of @c ApiGatewayCapabilityAgent that will be instantiated per test.
    std::shared_ptr<ApiGatewayCapabilityAgent> m_apiGatewayCA;
};

void ApiGatewayCapabilityAgentTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockAVSGatewayManager = std::make_shared<StrictMock<MockAVSGatewayManager>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();

    m_apiGatewayCA = ApiGatewayCapabilityAgent::create(m_mockAVSGatewayManager, m_mockExceptionSender);

    ASSERT_NE(m_apiGatewayCA, nullptr);
}

void ApiGatewayCapabilityAgentTest::TearDown() {
    if (m_apiGatewayCA) {
        m_apiGatewayCA->shutdown();
        m_apiGatewayCA.reset();
    }
}

void ApiGatewayCapabilityAgentTest::wakeOnSetCompleted() {
    m_wakeSetCompletedPromise.set_value();
}

/**
 * Tests if create method returns null with null @c AVSGatewayManager.
 */
TEST_F(ApiGatewayCapabilityAgentTest, createNoGatewayManager) {
    auto apiGatewayCA = ApiGatewayCapabilityAgent::create(nullptr, m_mockExceptionSender);
    ASSERT_EQ(apiGatewayCA, nullptr);
}

/**
 * Test if create method return null with null @c ExceptionEncounteredSender.
 */
TEST_F(ApiGatewayCapabilityAgentTest, createNoAVSGatewayManager) {
    auto apiGatewayCA = ApiGatewayCapabilityAgent::create(m_mockAVSGatewayManager, nullptr);
    ASSERT_EQ(apiGatewayCA, nullptr);
}

/**
 * Tests that @c AVSGatewayManager does not get called if the directive is null.
 */
TEST_F(ApiGatewayCapabilityAgentTest, testNullDirective) {
    EXPECT_CALL(*m_mockAVSGatewayManager, setGatewayURL(_)).Times(Exactly(0));
    m_apiGatewayCA->handleDirectiveImmediately(nullptr);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test if sendExceptionEncountered and setFailed will be called for invalid directives.
 */
TEST_F(ApiGatewayCapabilityAgentTest, testValidUnknownDirective) {
    auto directivePair = AVSDirective::create(UNKNOWN_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    EXPECT_CALL(*m_mockAVSGatewayManager, setGatewayURL(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& unparsedDirective,
                            ExceptionErrorType errorType,
                            const std::string& errorDescription) {
            EXPECT_EQ(errorType, ExceptionErrorType::UNSUPPORTED_OPERATION);
        }));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &ApiGatewayCapabilityAgentTest::wakeOnSetCompleted));

    m_apiGatewayCA->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_apiGatewayCA->CapabilityAgent::handleDirective(directive->getMessageId());
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test if sendExceptionEncountered and setFailed will be called for SetGateway directives with no payload.
 */
TEST_F(ApiGatewayCapabilityAgentTest, testValidDirectiveWithNoPayload) {
    auto directivePair = AVSDirective::create(NO_PAYLOAD_SET_GATEWAY_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    EXPECT_CALL(*m_mockAVSGatewayManager, setGatewayURL(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& unparsedDirective,
                            ExceptionErrorType errorType,
                            const std::string& errorDescription) {
            EXPECT_EQ(errorType, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        }));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &ApiGatewayCapabilityAgentTest::wakeOnSetCompleted));

    m_apiGatewayCA->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_apiGatewayCA->CapabilityAgent::handleDirective(directive->getMessageId());
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test if sendExceptionEncountered and setFailed will be called for SetGateway directives with invalid payload.
 */
TEST_F(ApiGatewayCapabilityAgentTest, testValidDirectiveWithInvalidPayload) {
    auto directivePair = AVSDirective::create(INVALID_PAYLOAD_SET_GATEWAY_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    EXPECT_CALL(*m_mockAVSGatewayManager, setGatewayURL(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& unparsedDirective,
                            ExceptionErrorType errorType,
                            const std::string& errorDescription) {
            EXPECT_EQ(errorType, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        }));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &ApiGatewayCapabilityAgentTest::wakeOnSetCompleted));

    m_apiGatewayCA->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_apiGatewayCA->CapabilityAgent::handleDirective(directive->getMessageId());
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 *  Test if setGatewayURL and setCompleted get called for valid SetGateway directives.
 */
TEST_F(ApiGatewayCapabilityAgentTest, testValidSetGatewayDirective) {
    auto directivePair = AVSDirective::create(SET_GATEWAY_DIRECTIVE_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    EXPECT_CALL(*m_mockAVSGatewayManager, setGatewayURL(_))
        .Times(Exactly(1))
        .WillOnce(Invoke([](const std::string& gatewayURL) {
            EXPECT_EQ(gatewayURL, TEST_GATEWAY_URL);
            return true;
        }));
    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &ApiGatewayCapabilityAgentTest::wakeOnSetCompleted));

    m_apiGatewayCA->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_apiGatewayCA->CapabilityAgent::handleDirective(directive->getMessageId());
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

}  // namespace test
}  // namespace apiGateway
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
