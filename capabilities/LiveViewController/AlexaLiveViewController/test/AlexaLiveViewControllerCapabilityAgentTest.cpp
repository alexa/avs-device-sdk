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

#include <acsdk/AlexaLiveViewController/private/AlexaLiveViewControllerCapabilityAgent.h>
#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerInterface.h>
#include <acsdk/AlexaLiveViewControllerInterfaces/LiveViewControllerObserverInterface.h>
#include <RTCSCNativeInterface/RtcscAppClientInterface.h>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/WaitEvent.h>

namespace alexaClientSDK {
namespace alexaLiveViewController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::memory;
using namespace ::testing;

using namespace alexaLiveViewController;
using namespace alexaLiveViewControllerInterfaces;

/// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.Camera.LiveViewController"};

/// The supported version
static const std::string INTERFACE_VERSION{"1.7"};

/// The name for StartLiveView directive
static const std::string NAME_STARTLIVEVIEW{"StartLiveView"};

/// The name for StopLiveView directive
static const std::string NAME_STOPLIVEVIEW{"StopLiveView"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The LiveViewStarted event signature.
static const std::string LIVE_VIEW_STARTED_EVENT{"LiveViewStarted"};

/// The LiveViewStopped event signature.
static const std::string LIVE_VIEW_STOPPED_EVENT{"LiveViewStopped"};

/// The test EndpointId
static const EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// MessageId key.
static const std::string MESSAGE_ID("messageId");

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Dialog request Id key.
static const std::string DIALOG_REQUEST_ID("dialogRequestId");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Correlation token key.
static const std::string CORRELATION_TOKEN("correlationToken");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

/// Event correlation token key.
static const std::string EVENT_CORRELATION_TOKEN("eventCorrelationToken");

/// Event correlation for testing.
static const std::string EVENT_CORRELATION_TOKEN_TEST("EventCorrelationToken_Test");

/// JSON key for the event section of a message.
static const std::string MESSAGE_EVENT_KEY = "event";

/// JSON key for the header section of a message.
static const std::string MESSAGE_HEADER_KEY = "header";

/// JSON key for the payload section of an message.
static const std::string MESSAGE_PAYLOAD_KEY = "payload";

/// JSON key for the namespace field of a message header.
static const std::string MESSAGE_NAMESPACE_KEY = "namespace";

/// JSON key for the name field of a message header.
static const std::string MESSAGE_NAME_KEY = "name";

/// Sample StartLiveView payload
static const std::string START_LIVE_VIEW_PAYLOAD = R"(
{
    "sessionId" : "session_id",
    "target" : {
        "type" : "ALEXA_ENDPOINT",
        "endpointId" : "endpoint_id of the viewing device"
    },
    "role" : "VIEWER",
    "participants" : {
        "viewers" : [
            {
                "name" : "name of the viewing device",
                "hasCameraControl" : true,
                "state" : "CONNECTING"
            }
        ],
        "camera" : {
            "name" : "name of camera streaming the live feed",
            "make" : "camera make name",
            "model" : "camera model name"
        }
    },
    "viewerExperience" : {
        "suggestedDisplay" : {
            "displayMode" : "FULL_SCREEN",
            "overlayType" : "NONE"
        },
        "audioProperties" : {
            "talkMode" : "PRESS_AND_HOLD",
            "concurrentTwoWayTalk" : "ENABLED",
            "microphoneState" : "MUTED",
            "speakerState" : "UNMUTED"
        },
        "liveViewTrigger" : "USER_ACTION",
        "idleTimeoutInMilliseconds" : 15000
    }
})";

/// Sample StopLiveView payload
static const std::string STOP_LIVE_VIEW_PAYLOAD = R"(
{
    "sessionId" : "session_id",
    "target" : {
        "type" : "ALEXA_ENDPOINT",
        "endpointId" : "endpoint_id of the viewing device"
    }
})";

/// Expected LiveViewStarted payload for test purposes
static const std::string EXPECTED_LIVE_VIEW_STARTED_PAYLOAD =
    R"({"sessionId":"session_id","target":{"endpointId":"endpoint_id of the viewing device","type":"ALEXA_ENDPOINT"}})";

/// Expected LiveViewStopped payload for test purposes
static const std::string EXPECTED_LIVE_VIEW_STOPPED_PAYLOAD =
    R"({"sessionId":"session_id","target":{"endpointId":"endpoint_id of the viewing device","type":"ALEXA_ENDPOINT"}})";

class MockAlexaLiveViewControllerInterface : public LiveViewControllerInterface {
public:
    MOCK_METHOD1(startMock, Response(StartLiveViewRequest*));
    MOCK_METHOD0(stopMock, Response());
    MOCK_METHOD1(setCameraStateMock, Response(CameraState cameraState));
    MOCK_METHOD0(getConfigurationMock, Configuration());
    MOCK_METHOD1(addObserver, bool(std::weak_ptr<LiveViewControllerObserverInterface> observer));
    MOCK_METHOD1(removeObserver, void(std::weak_ptr<LiveViewControllerObserverInterface> observer));

    Response start(std::unique_ptr<StartLiveViewRequest> request) {
        return startMock(request.get());
    }

    Response stop() {
        return stopMock();
    }

    Response setCameraState(CameraState cameraState) {
        return setCameraStateMock(cameraState);
    }

    Configuration getConfiguration() {
        return getConfigurationMock();
    }
};

class MockRtcscAppClient : public rtc::native::RtcscAppClientInterface {
public:
    MOCK_METHOD2(
        registerAppClientListener,
        rtc::native::RtcscErrorCode(
            const rtc::native::AppInfo& appInfo,
            rtc::native::RtcscAppClientListenerInterface* appClientListener));
    MOCK_METHOD1(unregisterAppClientListener, rtc::native::RtcscErrorCode(const rtc::native::AppInfo& appInfo));
    MOCK_METHOD2(
        registerMetricsPublisherListener,
        rtc::native::RtcscErrorCode(
            const rtc::native::AppInfo& appInfo,
            rtc::native::RtcscMetricsPublisherListenerInterface* metricsPublisherListener));
    MOCK_METHOD1(unregisterMetricsPublisherListener, rtc::native::RtcscErrorCode(const rtc::native::AppInfo& appInfo));
    MOCK_METHOD2(setLocalAudioState, rtc::native::RtcscErrorCode(const std::string& sessionId, bool audioEnabled));
    MOCK_METHOD2(setLocalVideoState, rtc::native::RtcscErrorCode(const std::string& sessionId, bool videoEnabled));
    MOCK_METHOD2(setRemoteAudioState, rtc::native::RtcscErrorCode(const std::string& sessionId, bool audioEnabled));
    MOCK_METHOD1(acceptSession, rtc::native::RtcscErrorCode(const std::string& sessionId));
    MOCK_METHOD2(
        disconnectSession,
        rtc::native::RtcscErrorCode(
            const std::string& sessionId,
            rtc::native::RtcscAppDisconnectCode rtcscAppDisconnectCode));
    MOCK_METHOD2(
        switchCamera,
        rtc::native::RtcscErrorCode(const std::string& sessionId, const std::string& cameraName));
    MOCK_METHOD1(signalReadyForSession, rtc::native::RtcscErrorCode(const std::string& sessionId));
    MOCK_METHOD3(
        setVideoEffect,
        rtc::native::RtcscErrorCode(
            const std::string& sessionId,
            const rtc::native::VideoEffect& videoEffect,
            int videoEffectDurationMs));
    MOCK_METHOD2(
        registerDataChannelListener,
        bool(const std::string& sessionId, rtc::native::RtcscDataChannelListenerInterface* dataChannelListener));
    MOCK_METHOD1(unregisterDataChannelListener, bool(const std::string& sessionId));
    MOCK_METHOD4(
        sendData,
        bool(const std::string& sessionId, const std::string& label, const std::string& data, bool binary));
    MOCK_METHOD3(
        registerSurfaceConsumer,
        void(
            const std::string& sessionId,
            std::shared_ptr<rtc::native::RtcscSurfaceConsumerInterface> surfaceConsumer,
            rtc::native::MediaSide side));
    MOCK_METHOD2(unregisterSurfaceConsumer, void(const std::string& sessionId, rtc::native::MediaSide side));
};

class AlexaLiveViewControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaLiveViewControllerCapabilityAgentTest() {
    }

protected:
    /// A mock @c LiveViewControllerInterface object.
    std::shared_ptr<StrictMock<MockAlexaLiveViewControllerInterface>> m_mockLiveViewController;

    /// A context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock message sender.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockMessageSender>> m_mockMessageSender;

    /// The mock response sender.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    /// The mock @c RtcscAppClientInterface.
    std::shared_ptr<StrictMock<MockRtcscAppClient>> m_mockRtcscAppClient;

    /// A pointer to an instance of the LiveViewController that will be instantiated per test.
    std::shared_ptr<AlexaLiveViewControllerCapabilityAgent> m_liveViewControllerCapabilityAgent;
};

void AlexaLiveViewControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockLiveViewController = std::make_shared<StrictMock<MockAlexaLiveViewControllerInterface>>();
    m_mockMessageSender = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockMessageSender>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();

    m_liveViewControllerCapabilityAgent = AlexaLiveViewControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockLiveViewController,
        m_mockMessageSender,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender);

    m_mockRtcscAppClient = std::make_shared<StrictMock<MockRtcscAppClient>>();
    m_liveViewControllerCapabilityAgent->setRtcscAppClient(m_mockRtcscAppClient);
}

void AlexaLiveViewControllerCapabilityAgentTest::TearDown() {
}

/**
 * Verify the request sent to AVS is as expected.
 *
 * @param request Request for the event that is being sent to the cloud.
 * @param expectedEventName Name of the event being sent.
 * @param expectedPayload Payload of the event being sent.
 * @param expectedNameSpace Namespace of the event being sent.
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
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    if (directiveName == NAME_STARTLIVEVIEW) {
        return AVSDirective::create(
            "", avsMessageHeader, START_LIVE_VIEW_PAYLOAD, attachmentManager, "", avsMessageEndpoint);
    } else if (directiveName == NAME_STOPLIVEVIEW) {
        return AVSDirective::create(
            "", avsMessageHeader, STOP_LIVE_VIEW_PAYLOAD, attachmentManager, "", avsMessageEndpoint);
    } else {
        return AVSDirective::create("", avsMessageHeader, "", attachmentManager, "", avsMessageEndpoint);
    }
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(AlexaLiveViewControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaLiveViewControllerCapabilityAgent::create(
            "",
            m_mockLiveViewController,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaLiveViewControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            nullptr,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockResponseSender,
            m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaLiveViewControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockLiveViewController,
            m_mockMessageSender,
            nullptr,
            m_mockResponseSender,
            m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaLiveViewControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockLiveViewController,
            m_mockMessageSender,
            m_mockContextManager,
            nullptr,
            m_mockExceptionSender),
        IsNull());
    ASSERT_THAT(
        AlexaLiveViewControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockLiveViewController,
            m_mockMessageSender,
            m_mockContextManager,
            m_mockResponseSender,
            nullptr),
        IsNull());
}

/**
 * Test successful handling of StartLiveView and StopLiveView directives.
 */
TEST_F(AlexaLiveViewControllerCapabilityAgentTest, test_liveViewDirectives_successCase) {
    avsCommon::utils::WaitEvent waitEvent, waitEvent2;
    ON_CALL(*m_mockRtcscAppClient, registerAppClientListener(_, _))
        .WillByDefault(Return(rtc::native::RtcscErrorCode::SUCCESS));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(*m_mockRtcscAppClient, registerAppClientListener(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockLiveViewController, setCameraStateMock(_)).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response();
    }));
    EXPECT_CALL(*m_mockLiveViewController, startMock(_)).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response();
    }));
    auto liveViewStartedVerifyEvent = [&waitEvent](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LIVE_VIEW_STARTED_EVENT, EXPECTED_LIVE_VIEW_STARTED_PAYLOAD, NAMESPACE);
        waitEvent.wakeUp();
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).WillOnce(Invoke(liveViewStartedVerifyEvent));
    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _)).Times(Exactly(1));

    ASSERT_THAT(m_liveViewControllerCapabilityAgent, NotNull());

    // Simulate StartLiveView directive
    std::shared_ptr<AVSDirective> startLiveViewdirective = buildAVSDirective(NAME_STARTLIVEVIEW);
    m_liveViewControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        startLiveViewdirective, std::move(m_mockDirectiveHandlerResult));
    m_liveViewControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    // Re-initializing the unique pointer after the move above.
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(*m_mockRtcscAppClient, disconnectSession(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockLiveViewController, stopMock()).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response();
    }));
    auto liveViewStoppedVerifyEvent = [&waitEvent2](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LIVE_VIEW_STOPPED_EVENT, EXPECTED_LIVE_VIEW_STOPPED_PAYLOAD, NAMESPACE);
        waitEvent2.wakeUp();
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).WillOnce(Invoke(liveViewStoppedVerifyEvent));
    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _)).Times(Exactly(1));

    // Simulate StopLiveView directive
    std::shared_ptr<AVSDirective> stopLiveViewdirective = buildAVSDirective(NAME_STOPLIVEVIEW);
    m_liveViewControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        stopLiveViewdirective, std::move(m_mockDirectiveHandlerResult));
    m_liveViewControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent2.wait(TIMEOUT));
    m_liveViewControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of StartLiveView directive.
 */
TEST_F(AlexaLiveViewControllerCapabilityAgentTest, test_startLiveViewDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    ON_CALL(*m_mockRtcscAppClient, registerAppClientListener(_, _))
        .WillByDefault(Return(rtc::native::RtcscErrorCode::SUCCESS));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(*m_mockRtcscAppClient, registerAppClientListener(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockLiveViewController, setCameraStateMock(_)).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response(
            LiveViewControllerInterface::Response::Type::FAILED_INTERNAL_ERROR, "InternalError");
    }));
    EXPECT_CALL(*m_mockLiveViewController, startMock(_)).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response(
            LiveViewControllerInterface::Response::Type::FAILED_INTERNAL_ERROR, "InternalError");
    }));
    auto verifyEvent = [&waitEvent](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LIVE_VIEW_STARTED_EVENT, EXPECTED_LIVE_VIEW_STARTED_PAYLOAD, NAMESPACE);
        waitEvent.wakeUp();
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1)).WillOnce(Invoke(verifyEvent));
    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(_, _, _, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, _))
        .Times(Exactly(1));

    ASSERT_THAT(m_liveViewControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_STARTLIVEVIEW);
    m_liveViewControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_liveViewControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    m_liveViewControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of StopLiveView directive.
 */
TEST_F(AlexaLiveViewControllerCapabilityAgentTest, test_stopLiveViewDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent, waitEvent2;
    ON_CALL(*m_mockRtcscAppClient, registerAppClientListener(_, _))
        .WillByDefault(Return(rtc::native::RtcscErrorCode::SUCCESS));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(*m_mockRtcscAppClient, registerAppClientListener(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockLiveViewController, setCameraStateMock(_)).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response();
    }));
    EXPECT_CALL(*m_mockLiveViewController, startMock(_)).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response();
    }));
    auto liveViewStartedVerifyEvent = [&waitEvent](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LIVE_VIEW_STARTED_EVENT, EXPECTED_LIVE_VIEW_STARTED_PAYLOAD, NAMESPACE);
        waitEvent.wakeUp();
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).WillOnce(Invoke(liveViewStartedVerifyEvent));
    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _)).Times(Exactly(1));

    ASSERT_THAT(m_liveViewControllerCapabilityAgent, NotNull());

    // Simulate StartLiveView directive
    std::shared_ptr<AVSDirective> startLiveViewdirective = buildAVSDirective(NAME_STARTLIVEVIEW);
    m_liveViewControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        startLiveViewdirective, std::move(m_mockDirectiveHandlerResult));
    m_liveViewControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));

    // Re-initializing the unique pointer after the move above.
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).Times(Exactly(1));
    EXPECT_CALL(*m_mockRtcscAppClient, disconnectSession(_, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockLiveViewController, stopMock()).WillOnce(InvokeWithoutArgs([] {
        return LiveViewControllerInterface::Response(
            LiveViewControllerInterface::Response::Type::FAILED_INTERNAL_ERROR, "InternalError");
    }));

    auto liveViewStoppedVerifyEvent = [&waitEvent2](std::shared_ptr<avsCommon::avs::MessageRequest> request) {
        verifySendMessage(request, LIVE_VIEW_STOPPED_EVENT, EXPECTED_LIVE_VIEW_STOPPED_PAYLOAD, NAMESPACE);
        waitEvent2.wakeUp();
    };
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).WillOnce(Invoke(liveViewStoppedVerifyEvent));
    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(_, _, _, AlexaInterfaceMessageSenderInterface::ErrorResponseType::INTERNAL_ERROR, _))
        .Times(Exactly(1));

    // Simulate StopLiveView directive
    std::shared_ptr<AVSDirective> stopLiveViewdirective = buildAVSDirective(NAME_STOPLIVEVIEW);
    m_liveViewControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        stopLiveViewdirective, std::move(m_mockDirectiveHandlerResult));
    m_liveViewControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent2.wait(TIMEOUT));
    m_liveViewControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaLiveViewControllerCapabilityAgentTest, test_unknownDirective) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    ASSERT_THAT(m_liveViewControllerCapabilityAgent, NotNull());

    std::shared_ptr<AVSDirective> directive = buildAVSDirective(UNKNOWN_DIRECTIVE);
    m_liveViewControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    m_liveViewControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    m_liveViewControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace alexaLiveViewController
}  // namespace alexaClientSDK
