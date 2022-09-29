/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include <acsdk/AlexaPresentation/private/AlexaPresentation.h>

namespace alexaClientSDK {
namespace alexaPresentation {
namespace test {

using namespace alexaClientSDK;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace alexaClientSDK::avsCommon::utils::memory;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;
using namespace avsCommon::avs::attachment::test;
using namespace rapidjson;
using namespace ::testing;

/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The second namespace registered for this capability agent.
static const std::string NAMESPACE{"Alexa.Presentation"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The name for Dismissed event.
static const std::string PRESENTATION_DISMISSED_EVENT{"Dismissed"};

/// The @c MessageId identifer.
static const std::string MESSAGE_ID("messageId");

/// Expected payload to be sent with Dismissed event
static const std::string EXPECTED_PRESENTATION_DISMISSED_PAYLOAD = R"({"presentationToken":"TOKEN"})";

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the namespace field of a message header.
static const std::string MESSAGE_NAMESPACE_KEY = "namespace";

/// JSON key for the name field of a message header.
static const std::string MESSAGE_NAME_KEY = "name";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the payload section of an message.
static const std::string MESSAGE_PAYLOAD_KEY = "payload";

/// Presentation Token used by presentations
static const aplCapabilityCommonInterfaces::PresentationToken PRESENTATION_TOKEN = "TOKEN";

std::mutex m;
std::condition_variable conditionVariable;

/// Test harness for @c AlexaPresentationTest class.
class AlexaPresentationTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaPresentationTest() {
    }

protected:
    /// A strict mock that allows the test to fetch context.
    std::shared_ptr<StrictMock<test::MockContextManager>> m_mockContextManager;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<test::MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<test::MockMessageSender>> m_mockMessageSender;

    /// mutex for the conditional variables.
    std::mutex m_mutex;

    /// This is the condition variable to be used to control getting of a context in test cases.
    std::condition_variable m_contextTrigger;

    /// A pointer to an instance of the @c AlexaPresentationCapabilityAgentInterface
    std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> m_AlexaCA;

    /// A pointer to an instance of the @c AlexaPresentation that will be instantiated per test.
    std::shared_ptr<AlexaPresentation> m_AlexaPresentation;

    /// The pointer into the @c Executor used by the tested object.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

void AlexaPresentationTest::SetUp() {
    m_mockContextManager = std::make_shared<StrictMock<test::MockContextManager>>();
    m_mockExceptionSender = std::make_shared<StrictMock<test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<test::MockDirectiveHandlerResult>>();

    m_mockMessageSender = std::make_shared<StrictMock<test::MockMessageSender>>();

    m_AlexaPresentation = AlexaPresentation::create(m_mockExceptionSender, m_mockMessageSender, m_mockContextManager);

    m_AlexaCA = std::dynamic_pointer_cast<
        alexaClientSDK::alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface>(m_AlexaPresentation);

    m_executor = std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>();
    m_AlexaPresentation->setExecutor(m_executor);
}

void AlexaPresentationTest::TearDown() {
    if (m_AlexaPresentation) {
        m_AlexaPresentation->shutdown();
        m_AlexaPresentation.reset();
    }
}

/**
 * Verify the request sent to AVS is as expected.
 */
static void verifySendMessage(
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    const std::string expectedEventName,
    const std::string expectedPayload,
    const std::string expectedNameSpace) {
    rapidjson::Document document;
    document.Parse(request->getJsonContent());
    auto event = document.FindMember(MESSAGE_EVENT_KEY);
    EXPECT_NE(event, document.MemberEnd());

    auto header = event->value.FindMember(MESSAGE_HEADER_KEY);
    EXPECT_NE(header, event->value.MemberEnd());
    auto payload = event->value.FindMember(MESSAGE_PAYLOAD_KEY);
    EXPECT_NE(payload, event->value.MemberEnd());
    EXPECT_EQ(header->value.FindMember(MESSAGE_NAMESPACE_KEY)->value.GetString(), expectedNameSpace);
    EXPECT_EQ(header->value.FindMember(MESSAGE_NAME_KEY)->value.GetString(), expectedEventName);
    EXPECT_NE(header->value.FindMember(MESSAGE_ID)->value.GetString(), "");

    std::string messagePayload;
    avsCommon::utils::json::jsonUtils::convertToValue(payload->value, &messagePayload);
    EXPECT_EQ(messagePayload, expectedPayload);
    EXPECT_EQ(request->attachmentReadersCount(), 0);

    conditionVariable.notify_all();
}

/**
 * Tests unknown Directive. Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaPresentationTest, testUnknownDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE, UNKNOWN_DIRECTIVE, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, "", attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_AlexaPresentation->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_AlexaPresentation->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();
}

/**
 * Verify the Presentation Dismissed Event
 */
TEST_F(AlexaPresentationTest, testPresentationDismissed) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    auto verifyEvent = [](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, PRESENTATION_DISMISSED_EVENT, EXPECTED_PRESENTATION_DISMISSED_PAYLOAD, NAMESPACE);
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1)).WillOnce(Invoke(verifyEvent));
    /// Expect a call to getContext as part of sending DISMISSED event.
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _))
        .WillOnce(Invoke([this](
                             std::shared_ptr<ContextRequesterInterface> contextRequester,
                             const std::string&,
                             const std::chrono::milliseconds&) {
            m_contextTrigger.notify_one();
            return 0;
        }));
    m_contextTrigger.wait_for(exitLock, TIMEOUT);
    /// wait for getContext
    m_contextTrigger.wait_for(exitLock, TIMEOUT);
    m_AlexaCA->onPresentationDismissed(PRESENTATION_TOKEN);
    m_executor->waitForSubmittedTasks();
    m_AlexaPresentation->onContextAvailable("");

    std::unique_lock<std::mutex> lk(m);
    conditionVariable.wait_for(lk, TIMEOUT);
}

}  // namespace test
}  // namespace alexaPresentation
}  // namespace alexaClientSDK
