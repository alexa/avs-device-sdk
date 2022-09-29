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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "acsdk/APLCapabilityCommonInterfaces/PresentationToken.h"
#include "gmock/gmock-spec-builders.h"
#include <rapidjson/document.h>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <acsdk/APLCapabilityCommonInterfaces/MockAPLCapabilityAgent.h>
#include <acsdk/APLCapabilityCommonInterfaces/MockVisualStateProvider.h>
#include <acsdk/APLCapabilityCommonInterfaces/MockAPLCapabilityAgentObserver.h>

#include <acsdk/APLCapabilityCommon/BaseAPLCapabilityAgent.h>

namespace alexaClientSDK {
namespace aplCapabilityCommon {
namespace test {

using namespace alexaClientSDK;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;
using namespace avsCommon::avs::attachment::test;
using namespace alexaClientSDK::avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;
using namespace aplCapabilityCommonInterfaces::test;

/// String to identify log entries originating from this file.
#define TAG "BaseAPLCapabilityAgentTest"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

/// The key in our config file to find the root of APL CA configuration.
static std::string TESTPRESENTATIONAPL_CONFIGURATION_ROOT_KEY = "TestPresentationAPLCapabilityAgent";

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace registered for this capability agent.
static const std::string NAMESPACE = "Test.Presentation.APL";

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE = "Unknown";

/// The name for UserEvent event.
static const std::string USER_EVENT_EVENT = "UserEvent";

/// The name for LoadIndexListData event.
static const std::string LOAD_INDEX_LIST_DATA = "LoadIndexListData";

/// The name for LoadTokenListData event.
static const std::string LOAD_TOKEN_LIST_DATA = "LoadTokenListData";

/// The @c MessageId identifer.
static const std::string MESSAGE_ID = "messageId";

/// The @c MessageId identifer.
static const std::string MESSAGE_ID_2 = "messageId2";

/// Payload to be sent for UserEvent
static const std::string SAMPLE_USER_EVENT_PAYLOAD =
    R"({"presentationToken":"APL_TOKEN","arguments":{},"source":{},"components":{}})";

/// DynamicIndexList DataSource type
static const std::string DYNAMIC_INDEX_LIST = "dynamicIndexList";

/// Payload to be sent for ListDataSourceFetchRequest
static const std::string SAMPLE_INDEX_DATA_SOURCE_FETCH_REQUEST =
    R"({"correlationToken":"101","count":10.0,"listId":"vQdpOESlok","startIndex":1.0,"presentationToken":"APL_TOKEN"})";

/// Payload to be sent for ListDataSourceFetchRequest from APLCore
static const std::string SAMPLE_INDEX_DATA_SOURCE_FETCH_APL_REQUEST =
    R"({"correlationToken":"101","count":10.0,"listId":"vQdpOESlok","startIndex":1.0})";

/// DynamicTokenList DataSource type
static const std::string DYNAMIC_TOKEN_LIST = "dynamicTokenList";

/// Payload to be sent for TokenDataSourceFetchRequest
static const std::string SAMPLE_TOKEN_DATA_SOURCE_FETCH_REQUEST =
    R"({"correlationToken":"101","listId":"vQdpOESlok","pageToken":"nextPage","presentationToken":"APL_TOKEN"})";

/// Payload to be sent for TokenDataSourceFetchRequest from APLCore
static const std::string SAMPLE_TOKEN_DATA_SOURCE_FETCH_APL_REQUEST =
    R"({"correlationToken":"101","listId":"vQdpOESlok","pageToken":"nextPage"})";

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

/// The name for SendIndexListData directive.
static const char* SEND_INDEX_LIST_DATA = "SendIndexListData";

/// The name for UpdateIndexListData directive.
static const char* UPDATE_INDEX_LIST_DATA = "UpdateIndexListData";

/// The name for SendTokenListData directive.
static const char* SEND_TOKEN_LIST_DATA = "SendTokenListData";

/// The RenderDocument directive signature.
static const NamespaceAndName DOCUMENT{NAMESPACE, "RenderDocument"};

/// The ExecuteCommands directive signature.
static const NamespaceAndName COMMAND{NAMESPACE, "ExecuteCommands"};

/// The SendIndexListData directive signature.
static const NamespaceAndName INDEX_LIST_DATA{NAMESPACE, SEND_INDEX_LIST_DATA};

/// The UpdateIndexListData directive signature.
static const NamespaceAndName INDEX_LIST_UPDATE{NAMESPACE, UPDATE_INDEX_LIST_DATA};

/// The SendTokenListData directive signature.
static const NamespaceAndName TOKEN_LIST_DATA{NAMESPACE, SEND_TOKEN_LIST_DATA};

/// Identifier for the skilld in presentationSession
static const char SKILL_ID[] = "skillId";

/// Identifier for the id in presentationSession
static const char PRESENTATION_SESSION_ID[] = "id";

// clang-format off

const std::string makeDocumentAplPayload(const std::string& token, const std::string& timeoutType) {
    return "{"
             "\"presentationToken\":\"" + token + "\","
             "\"windowId\":\"WINDOW_ID\","
             "\"timeoutType\":\"" + timeoutType + "\","
             "\"document\":\"{}\""
             "}";
}

/// A RenderDocument directive with APL payload.
static const std::string DOCUMENT_APL_PAYLOAD = "{"
                                                "\"presentationToken\":\"APL_TOKEN\","
                                                "\"windowId\":\"WINDOW_ID\","
                                                "\"timeoutType\":\"TRANSIENT\","
                                                "\"document\":\"{}\""
                                                "}";

/// A 2nd RenderDocument directive with APL payload.
static const std::string DOCUMENT_APL_PAYLOAD_2 = "{"
                                                  "\"presentationToken\":\"APL_TOKEN_2\","
                                                  "\"windowId\":\"WINDOW_ID\","
                                                  "\"timeoutType\":\"TRANSIENT\","
                                                  "\"document\":\"{}\""
                                                  "}";

static const std::string DOCUMENT_APL_PAYLOAD_MISSING_TIMEOUTTYPE =
                                                "{"
                                                "\"presentationToken\":\"APL_TOKEN\","
                                                "\"windowId\":\"WINDOW_ID\","
                                                "\"document\":\"{}\""
                                                "}";

static const std::string DOCUMENT_APL_PAYLOAD_INVALID_TIMEOUTTYPE =
                                                "{"
                                                "\"presentationToken\":\"APL_TOKEN\","
                                                "\"windowId\":\"WINDOW_ID\","
                                                "\"timeoutType\":\"SNAKES\","
                                                "\"document\":\"{}\""
                                                "}";

/// A malformed RenderDocument directive with APL payload without presentationToken.
static const std::string DOCUMENT_APL_PAYLOAD_MALFORMED = "{"
                                                          "\"token\":\"APL_TOKEN\""
                                                          "}";

/// A malformed RenderDocument directive with APL payload without document.
static const std::string DOCUMENT_APL_PAYLOAD_MALFORMED_2 = "{"
                                                            "\"presentationToken\":\"APL_TOKEN\""
                                                            "}";

/// A malformed ExecuteCommand directive with APL payload without commands.
static const std::string EXECUTE_COMMAND_PAYLOAD_MALFORMED = "{"
                                                             "\"presentationToken\":\"APL_TOKEN\""
                                                             "}";

/// A malformed ExecuteCommand directive with APL payload without presentationToken.
static const std::string EXECUTE_COMMAND_PAYLOAD_MALFORMED_2 = "{"
                                                               "\"token\":\"APL_TOKEN\""
                                                               "}";

// Properly formed execute command
static const std::string EXECUTE_COMMAND_PAYLOAD = "{"
                                                   "\"presentationToken\":\"APL_TOKEN\","
                                                   "\"commands\": ["
                                                   " {\"type\": \"idleCommand\"} "
                                                   "]"
                                                   "}";

static const std::string SETTINGS_CONFIG = R"({"TestPresentationAPLCapabilityAgent":{
                                                    "minStateReportIntervalMs": 250,
                                                    "stateReportCheckIntervalMs": 300
                                                }})";

// clang-format on

/// Test window ID
static const std::string WINDOW_ID = "WINDOW_ID";

/// A visual state request token.
static const unsigned int STATE_REQUEST_TOKEN = 1;

/// A state request token for a proactive state request
static const unsigned int PROACTIVE_STATE_REQUEST_TOKEN = 0;

/// APL_TOKEN used in documents
static const std::string APL_TOKEN = "APL_TOKEN";

/// APL_TOKEN_2 used in documents
static const char* const APL_TOKEN_2 = "APL_TOKEN_2";

/// Metric name for the test CA
static const std::string TEST_METRIC_NAME = "APLCATest";

/// Metric recorder shared ptr
static std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder;

class APLCATest : public BaseAPLCapabilityAgent {
public:
    ~APLCATest() = default;
    /// @name BaseAPLCapabilityAgent template functions
    /// @{
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getAPLDirectiveConfiguration() const override {
        DirectiveHandlerConfiguration configuration;
        return configuration;
    }
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getAPLCapabilityConfigurations(const std::string& APLMaxVersion) override {
        std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> config;
        return config;
    }
    aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType getDirectiveType(
        std::shared_ptr<DirectiveInfo> info) override {
        if (!info) {
            return aplCapabilityCommon::BaseAPLCapabilityAgent::DirectiveType::UNKNOWN;
        }

        if (info->directive->getNamespace() == DOCUMENT.nameSpace && info->directive->getName() == DOCUMENT.name) {
            return BaseAPLCapabilityAgent::DirectiveType::RENDER_DOCUMENT;
        } else if (info->directive->getNamespace() == COMMAND.nameSpace && info->directive->getName() == COMMAND.name) {
            return BaseAPLCapabilityAgent::DirectiveType::EXECUTE_COMMAND;
        } else if (
            (info->directive->getNamespace() == INDEX_LIST_DATA.nameSpace &&
             info->directive->getName() == INDEX_LIST_DATA.name) ||
            (info->directive->getNamespace() == INDEX_LIST_UPDATE.nameSpace &&
             info->directive->getName() == INDEX_LIST_UPDATE.name)) {
            return BaseAPLCapabilityAgent::DirectiveType::DYNAMIC_INDEX_DATA_SOURCE_UPDATE;
        } else if (
            info->directive->getNamespace() == TOKEN_LIST_DATA.nameSpace &&
            info->directive->getName() == TOKEN_LIST_DATA.name) {
            return BaseAPLCapabilityAgent::DirectiveType::DYNAMIC_TOKEN_DATA_SOURCE_UPDATE;
        } else {
            return BaseAPLCapabilityAgent::DirectiveType::UNKNOWN;
        }
    }
    const std::string& getConfigurationRootKey() override {
        return TESTPRESENTATIONAPL_CONFIGURATION_ROOT_KEY;
    }
    const std::string& getMetricDataPointName(aplCapabilityCommon::BaseAPLCapabilityAgent::MetricEvent event) override {
        return TEST_METRIC_NAME;
    }
    const std::string& getMetricActivityName(
        aplCapabilityCommon::BaseAPLCapabilityAgent::MetricActivity activity) override {
        return TEST_METRIC_NAME;
    }
    aplCapabilityCommon::BaseAPLCapabilityAgent::PresentationSessionFieldNames getPresentationSessionFieldNames()
        override {
        aplCapabilityCommon::BaseAPLCapabilityAgent::PresentationSessionFieldNames fieldNames;
        fieldNames.skillId = SKILL_ID;
        fieldNames.presentationSessionId = PRESENTATION_SESSION_ID;
        return fieldNames;
    }
    const bool shouldPackPresentationSessionToAvsEvents() override {
        return false;
    }
    /// @}
    /**
     * Constructor.
     *
     * @param exceptionSender The object to send AVS Exception messages.
     * @param messageSender The object to send message to AVS.
     * @param contextManager The object to fetch the context of the system.
     * @param visualStateProvider The VisualStateProviderInterface object used to request visual context.
     */
    APLCATest(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::string aplVersion,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface>
            visualStateProvider = nullptr) :
            BaseAPLCapabilityAgent(
                NAMESPACE,
                exceptionSender,
                metricRecorder,
                messageSender,
                contextManager,
                aplVersion,
                visualStateProvider) {
    }
};

std::mutex m;
std::condition_variable conditionVariable;

/// Test harness for @c BaseAPLCapabilityAgent class.
class BaseAPLCapabilityAgentTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// A constructor which initializes the promises and futures needed for the test class.
    BaseAPLCapabilityAgentTest() {
    }

protected:
    /// This is the condition variable to be used to control getting of a context in test cases.
    std::condition_variable m_contextTrigger;

    /// mutex for the conditional variables.
    std::mutex m_mutex;

    /// A strict mock that allows the test to fetch context.
    std::shared_ptr<StrictMock<test::MockContextManager>> m_mockContextManager;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<test::MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// A strict mock to allow testing of the observer callback.
    std::shared_ptr<NiceMock<MockAPLCapabilityAgentObserver>> m_mockAPLObserver;

    /// A pointer to an instance of the @c APLCATest that will be instantiated per test.
    std::shared_ptr<APLCATest> m_APLCATest;

    /// A pointer to an instance of the @c APLCapabilityAgentInterface that will be instantiated per test.
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> m_aplCA;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<test::MockMessageSender>> m_mockMessageSender;

    /// A strict mock to allow testing for visual state provider.
    std::shared_ptr<StrictMock<MockVisualStateProvider>> m_mockVisualStateProvider;

    // The pointer into the @c Executor used by the tested object.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

/*
 * Utility function to generate @c ConfigurationNode from JSON string.
 */
static void setConfig() {
    auto stream = std::shared_ptr<std::stringstream>(new std::stringstream(SETTINGS_CONFIG));
    JSONStream jsonStream({stream});
    ConfigurationNode::initialize(jsonStream);
}

static aplCapabilityCommonInterfaces::aplEventPayload::UserEvent createUserEventPayload(
    aplCapabilityCommonInterfaces::PresentationToken token,
    std::string arguments,
    std::string source,
    std::string components) {
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    aplCapabilityCommonInterfaces::aplEventPayload::UserEvent payload(token, arguments, source, components);
#else
    aplCapabilityCommonInterfaces::aplEventPayload::UserEvent payload{
        .token = token, .arguments = arguments, .source = source, .components = components};
#endif
    return payload;
}

static aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch createDataSourceFetchPayload(
    aplCapabilityCommonInterfaces::PresentationToken token,
    std::string dataSourceType,
    std::string fetchPayload) {
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch payload(token, dataSourceType, fetchPayload);
#else
    aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch payload{
        .token = token, .dataSourceType = dataSourceType, .fetchPayload = fetchPayload};
#endif
    return payload;
}

static aplCapabilityCommonInterfaces::aplEventPayload::VisualContext createVisualContextPayload(
    aplCapabilityCommonInterfaces::PresentationToken token,
    std::string version,
    std::string visualContext,
    std::string datasourceContext) {
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    aplCapabilityCommonInterfaces::aplEventPayload::VisualContext payload(
        token, version, visualContext, datasourceContext);
#else
    aplCapabilityCommonInterfaces::aplEventPayload::VisualContext payload{
        .token = token, .version = version, .visualContext = visualContext, .datasourceContext = datasourceContext};
#endif
    return payload;
}

void BaseAPLCapabilityAgentTest::SetUp() {
    setConfig();
    m_mockContextManager = std::make_shared<StrictMock<test::MockContextManager>>();
    m_mockExceptionSender = std::make_shared<StrictMock<test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<test::MockDirectiveHandlerResult>>();
    m_mockAPLObserver = std::make_shared<NiceMock<test::MockAPLCapabilityAgentObserver>>();
    m_mockMessageSender = std::make_shared<StrictMock<test::MockMessageSender>>();
    m_mockVisualStateProvider = std::make_shared<StrictMock<test::MockVisualStateProvider>>();

    EXPECT_CALL(*m_mockContextManager, setStateProvider(_, _)).Times(Exactly(1));

    auto aplVersion = "";

    m_APLCATest = std::make_shared<APLCATest>(
        m_mockExceptionSender,
        metricRecorder,
        m_mockMessageSender,
        m_mockContextManager,
        aplVersion,
        m_mockVisualStateProvider);

    m_APLCATest->BaseAPLCapabilityAgent::initialize();

    m_aplCA = std::dynamic_pointer_cast<alexaClientSDK::aplCapabilityCommonInterfaces::APLCapabilityAgentInterface>(
        m_APLCATest);

    m_executor = std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>();
    m_APLCATest->setExecutor(m_executor);
    m_APLCATest->addObserver(m_mockAPLObserver);
}

void BaseAPLCapabilityAgentTest::TearDown() {
    EXPECT_CALL(*m_mockContextManager, removeStateProvider(_)).Times(Exactly(1));
    if (m_APLCATest) {
        m_APLCATest->removeObserver(m_mockAPLObserver);
        m_APLCATest->shutdown();
        m_APLCATest.reset();
    }
    if (m_mockAPLObserver) {
        m_mockAPLObserver.reset();
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
 * Tests timeout calculation
 */
TEST_F(BaseAPLCapabilityAgentTest, testTimeoutShort) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    auto aplDocument = makeDocumentAplPayload(APL_TOKEN, "SHORT");
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, aplDocument, attachmentManager, "");

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(
        *m_mockAPLObserver,
        onRenderDocument(
            _,
            _,
            APL_TOKEN,
            WINDOW_ID,
            aplCapabilityCommonInterfaces::APLTimeoutType::SHORT,
            NAMESPACE,
            _,
            _,
            _,
            m_aplCA))
        .Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();

    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();
}

TEST_F(BaseAPLCapabilityAgentTest, testTimeoutTransient) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    auto aplDocument = makeDocumentAplPayload(APL_TOKEN, "TRANSIENT");
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, aplDocument, attachmentManager, "");

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(
        *m_mockAPLObserver,
        onRenderDocument(
            _,
            _,
            APL_TOKEN,
            WINDOW_ID,
            aplCapabilityCommonInterfaces::APLTimeoutType::TRANSIENT,
            NAMESPACE,
            _,
            _,
            _,
            m_aplCA))
        .Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();

    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();
}

TEST_F(BaseAPLCapabilityAgentTest, testTimeoutLong) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    auto aplDocument = makeDocumentAplPayload(APL_TOKEN, "LONG");
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, aplDocument, attachmentManager, "");

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    EXPECT_CALL(
        *m_mockAPLObserver,
        onRenderDocument(
            _,
            _,
            APL_TOKEN,
            WINDOW_ID,
            aplCapabilityCommonInterfaces::APLTimeoutType::LONG,
            NAMESPACE,
            _,
            _,
            _,
            m_aplCA))
        .Times(Exactly(1));
    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();

    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();
}

/**
 * Tests unknown Directive. Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(BaseAPLCapabilityAgentTest, testUnknownDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE, UNKNOWN_DIRECTIVE, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, "", attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a RenderDocument Directive doesn't contain timeoutType.
 *  Verify that sendExceptionEncountered and setFailed are not called.
 *
 *  Some domains do not include timeout type.
 */
TEST_F(BaseAPLCapabilityAgentTest, testMissingTimeoutTypeRenderDocumentDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD_MISSING_TIMEOUTTYPE, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(0));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a RenderDocument Directive contains an invalid timeoutType.
 *  Expect that sendExceptionEncountered and setFailed will be called.
 */
TEST_F(BaseAPLCapabilityAgentTest, testInvalidTimeoutTypeRenderDocumentDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD_INVALID_TIMEOUTTYPE, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a malformed RenderDocument Directive (without presentationToken) is received.  Expect that the
 * sendExceptionEncountered and setFailed will be called.
 */
TEST_F(BaseAPLCapabilityAgentTest, testMalformedRenderDocumentDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD_MALFORMED, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a malformed RenderDocument Directive (without document) is received.  Expect that the
 * sendExceptionEncountered and setFailed will be called.
 */
TEST_F(BaseAPLCapabilityAgentTest, testMalformedRenderDocumentDirective2) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD_MALFORMED_2, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a malformed ExecuteCommands Directive is received (without presentationToken).  Expect that the
 * sendExceptionEncountered and setFailed will be called.
 */
TEST_F(BaseAPLCapabilityAgentTest, testMalformedExecuteCommandDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(COMMAND.nameSpace, COMMAND.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, EXECUTE_COMMAND_PAYLOAD_MALFORMED, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a malformed ExecuteCommands Directive (without commands) is received.  Expect that the
 * sendExceptionEncountered and setFailed will be called.
 */
TEST_F(BaseAPLCapabilityAgentTest, testMalformedExecuteCommandDirective2) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(COMMAND.nameSpace, COMMAND.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, EXECUTE_COMMAND_PAYLOAD_MALFORMED_2, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a ExecuteCommands Directive is received after an APL card is displayed. In this case the
 * ExecuteCommand should fail as presentationToken(APL rendered) != presentationToken(ExecuteCommand).
 */
TEST_F(BaseAPLCapabilityAgentTest, testExecuteCommandAfterMismatchedAPLCard) {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD_2, attachmentManager, "");

    EXPECT_CALL(*m_mockAPLObserver, onRenderDocument(_, _, APL_TOKEN_2, WINDOW_ID, _, NAMESPACE, _, _, _, m_aplCA))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_executor->waitForSubmittedTasks();

    m_APLCATest->processRenderDocumentResult(APL_TOKEN_2, true, "");
    m_executor->waitForSubmittedTasks();

    m_mockDirectiveHandlerResult = make_unique<StrictMock<test::MockDirectiveHandlerResult>>();

    // Create Directive.
    auto attachmentManager1 = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(COMMAND.nameSpace, COMMAND.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create("", avsMessageHeader1, EXECUTE_COMMAND_PAYLOAD, attachmentManager1, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive1, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_APLCATest->processExecuteCommandsResult(
        MESSAGE_ID, aplCapabilityCommonInterfaces::APLCommandExecutionEvent::RESOLVED, "");
    m_executor->waitForSubmittedTasks();
}

/**
 * Tests when a ExecuteCommands Directive is received after displaying an APL card with matching presentationToken.
 * The command should be successful.
 */
TEST_F(BaseAPLCapabilityAgentTest, testExecuteCommandAfterRightAPL) {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockAPLObserver, onRenderDocument(_, _, APL_TOKEN, WINDOW_ID, _, NAMESPACE, _, _, _, m_aplCA))
        .Times(Exactly(1));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();
    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();

    // Create Directive.
    auto attachmentManager1 = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(COMMAND.nameSpace, COMMAND.name, MESSAGE_ID_2);
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create("", avsMessageHeader1, EXECUTE_COMMAND_PAYLOAD, attachmentManager1, "");
    m_mockDirectiveHandlerResult = make_unique<StrictMock<test::MockDirectiveHandlerResult>>();

    EXPECT_CALL(*m_mockAPLObserver, onExecuteCommands(EXECUTE_COMMAND_PAYLOAD, APL_TOKEN)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive1, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID_2);
    m_APLCATest->processExecuteCommandsResult(
        APL_TOKEN, aplCapabilityCommonInterfaces::APLCommandExecutionEvent::RESOLVED, "");
    m_executor->waitForSubmittedTasks();
}

TEST_F(BaseAPLCapabilityAgentTest, testSendUserEvent) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    auto verifyEvent = [](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, USER_EVENT_EVENT, SAMPLE_USER_EVENT_PAYLOAD, NAMESPACE);
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1)).WillOnce(Invoke(verifyEvent));
    // Expect a call to getContext.
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _))
        .WillOnce(Invoke([this](
                             std::shared_ptr<ContextRequesterInterface> contextRequester,
                             const std::string&,
                             const std::chrono::milliseconds&) {
            m_contextTrigger.notify_one();
            return 0;
        }));

    auto payload = createUserEventPayload(APL_TOKEN, "{}", "{}", "{}");

    m_APLCATest->sendUserEvent(payload);
    // wait for getContext
    m_contextTrigger.wait_for(exitLock, TIMEOUT);
    m_APLCATest->onContextAvailable("");

    std::unique_lock<std::mutex> lk(m);
    conditionVariable.wait(lk);
}

TEST_F(BaseAPLCapabilityAgentTest, testIndexListDataSourceFetchRequestEvent) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    auto verifyEvent = [](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LOAD_INDEX_LIST_DATA, SAMPLE_INDEX_DATA_SOURCE_FETCH_REQUEST, NAMESPACE);
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1)).WillOnce(Invoke(verifyEvent));
    // Expect a call to getContext.
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _))
        .WillOnce(Invoke([this](
                             std::shared_ptr<ContextRequesterInterface> contextRequester,
                             const std::string&,
                             const std::chrono::milliseconds&) {
            m_contextTrigger.notify_one();
            return 0;
        }));

    auto fetchPayload =
        createDataSourceFetchPayload(APL_TOKEN, DYNAMIC_INDEX_LIST, SAMPLE_INDEX_DATA_SOURCE_FETCH_APL_REQUEST);

    m_APLCATest->sendDataSourceFetchRequestEvent(fetchPayload);
    // wait for getContext
    m_contextTrigger.wait_for(exitLock, TIMEOUT);
    m_APLCATest->onContextAvailable("");

    std::unique_lock<std::mutex> lk(m);
    conditionVariable.wait(lk);
}

TEST_F(BaseAPLCapabilityAgentTest, testTokenListDataSourceFetchRequestEvent) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    auto verifyEvent = [](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LOAD_TOKEN_LIST_DATA, SAMPLE_TOKEN_DATA_SOURCE_FETCH_REQUEST, NAMESPACE);
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1)).WillOnce(Invoke(verifyEvent));
    // Expect a call to getContext.
    EXPECT_CALL(*m_mockContextManager, getContext(_, _, _))
        .WillOnce(Invoke([this](
                             std::shared_ptr<ContextRequesterInterface> contextRequester,
                             const std::string&,
                             const std::chrono::milliseconds&) {
            m_contextTrigger.notify_one();
            return 0;
        }));

    auto fetchPayload =
        createDataSourceFetchPayload(APL_TOKEN, DYNAMIC_TOKEN_LIST, SAMPLE_TOKEN_DATA_SOURCE_FETCH_APL_REQUEST);

    m_APLCATest->sendDataSourceFetchRequestEvent(fetchPayload);
    // wait for getContext
    m_contextTrigger.wait_for(exitLock, TIMEOUT);
    m_APLCATest->onContextAvailable("");

    std::unique_lock<std::mutex> lk(m);
    conditionVariable.wait(lk);
}  // namespace test

TEST_F(BaseAPLCapabilityAgentTest, testAPLProactiveStateReportNotSentIfNotRendering) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Make sure that this is not called, because no APL is being presented
    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Exactly(0));

    m_contextTrigger.wait_for(exitLock, TIMEOUT);
    m_executor->waitForSubmittedTasks();
}

TEST_F(BaseAPLCapabilityAgentTest, testAPLProactiveStateReportSentIfRendering) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockAPLObserver, onRenderDocument(_, _, APL_TOKEN, WINDOW_ID, _, NAMESPACE, _, _, _, m_aplCA))
        .Times(Exactly(1))
        .WillRepeatedly(
            InvokeWithoutArgs([this] { m_APLCATest->recordRenderComplete(std::chrono::steady_clock::now()); }));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();

    m_aplCA->recordRenderComplete(std::chrono::steady_clock::now());
    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Exactly(1));
    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Exactly(1));
    m_APLCATest->provideState(DOCUMENT, STATE_REQUEST_TOKEN);
    m_executor->waitForSubmittedTasks();

    // Expect a state reponse for the original provideState request
    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).Times(Exactly(1));

    auto payload = createVisualContextPayload(APL_TOKEN, "test", "{ 1 }", "{}");

    m_APLCATest->onVisualContextAvailable(STATE_REQUEST_TOKEN, payload);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _)).Times(Exactly(1));
    // Now wait, and we should get a proactive state change following a different request
    m_contextTrigger.wait_for(exitLock, std::chrono::milliseconds(400));
    auto payload2 = createVisualContextPayload(APL_TOKEN, "test", "{ 2 }", "{}");

    m_APLCATest->onVisualContextAvailable(PROACTIVE_STATE_REQUEST_TOKEN, payload2);
    m_executor->waitForSubmittedTasks();
}

TEST_F(BaseAPLCapabilityAgentTest, testAPLProactiveStateReportSentIndependentOfProvideStateResponse) {
    std::unique_lock<std::mutex> exitLock(m_mutex);
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockAPLObserver, onRenderDocument(_, _, APL_TOKEN, WINDOW_ID, _, NAMESPACE, _, _, _, m_aplCA))
        .Times(Exactly(1))
        .WillRepeatedly(
            InvokeWithoutArgs([this] { m_APLCATest->recordRenderComplete(std::chrono::steady_clock::now()); }));

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();
    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Exactly(1));
    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();

    // At least one state request will come as a result of rendering, depending on timing a second one may be made
    // by the state reporter
    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Between(1, 2));
    m_APLCATest->provideState(DOCUMENT, STATE_REQUEST_TOKEN);
    m_executor->waitForSubmittedTasks();

    // Expect a state reponse for the original provideState request
    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).Times(Exactly(1));
    auto payload = createVisualContextPayload(APL_TOKEN, "test", "{ 1 }", "{}");
    m_APLCATest->onVisualContextAvailable(STATE_REQUEST_TOKEN, payload);
    m_executor->waitForSubmittedTasks();

    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _)).Times(Exactly(1));
    // Now wait, and we should get a proactive state change following a different request
    m_contextTrigger.wait_for(exitLock, std::chrono::milliseconds(400));

    auto payload2 = createVisualContextPayload(APL_TOKEN, "test", "{ 2 }", "{}");
    m_APLCATest->onVisualContextAvailable(PROACTIVE_STATE_REQUEST_TOKEN, payload2);
    m_executor->waitForSubmittedTasks();
}

TEST_F(BaseAPLCapabilityAgentTest, testAPLProactiveStateReportNotSentIfRenderingNotComplete) {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockAPLObserver, onRenderDocument(_, _, APL_TOKEN, WINDOW_ID, _, NAMESPACE, _, _, _, m_aplCA))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();

    m_APLCATest->processRenderDocumentResult(APL_TOKEN, true, "");
    m_executor->waitForSubmittedTasks();

    // At least one state request will come as a result of rendering, depending on timing a second one may be made
    // by the state reporter
    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, STATE_REQUEST_TOKEN)).Times(Exactly(1));
    EXPECT_CALL(*m_mockVisualStateProvider, provideState(_, PROACTIVE_STATE_REQUEST_TOKEN)).Times(Exactly(0));
    m_APLCATest->provideState(DOCUMENT, STATE_REQUEST_TOKEN);
    m_executor->waitForSubmittedTasks();

    // Expect a state reponse for the original provideState request
    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, STATE_REQUEST_TOKEN)).Times(Exactly(1));
    auto payload = createVisualContextPayload(APL_TOKEN, "test", "{ 1 }", "{}");
    m_APLCATest->onVisualContextAvailable(STATE_REQUEST_TOKEN, payload);
    m_executor->waitForSubmittedTasks();
}

/**
 * Checks that the timestamp for the directive is recorded and passed on correctly
 */
TEST_F(BaseAPLCapabilityAgentTest, testRenderDocumentReceivedTimestamp) {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(DOCUMENT.nameSpace, DOCUMENT.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, DOCUMENT_APL_PAYLOAD, attachmentManager, "");

    auto startTime = std::chrono::steady_clock::now();

    EXPECT_CALL(*m_mockAPLObserver, onRenderDocument(_, _, APL_TOKEN, WINDOW_ID, _, NAMESPACE, _, _, _, m_aplCA))
        .Times(1)
        .WillRepeatedly(Invoke(
            [startTime](
                const std::string& document,
                const std::string& datasource,
                const aplCapabilityCommonInterfaces::PresentationToken& token,
                const std::string& windowId,
                const aplCapabilityCommonInterfaces::APLTimeoutType timeoutType,
                const std::string& interfaceName,
                const std::string& supportedViewports,
                const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
                const std::chrono::steady_clock::time_point& receiveTime,
                std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> agent) {
                auto endTime = std::chrono::steady_clock::now();
                ASSERT_GE(receiveTime, startTime);
                ASSERT_LE(receiveTime, endTime);
            }));

    m_APLCATest->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_APLCATest->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_executor->waitForSubmittedTasks();
}

}  // namespace test
}  // namespace aplCapabilityCommon
}  // namespace alexaClientSDK
