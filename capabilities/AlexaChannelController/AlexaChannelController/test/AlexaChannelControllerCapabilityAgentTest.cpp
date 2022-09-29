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

#include <acsdk/AlexaChannelController/private/AlexaChannelControllerCapabilityAgent.h>
#include <acsdk/AlexaChannelControllerInterfaces/ChannelControllerInterface.h>
#include <acsdk/AlexaChannelControllerInterfaces/ChannelControllerObserverInterface.h>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockAlexaInterfaceMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimePoint.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace alexaChannelController {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::memory;
using namespace ::testing;

using namespace alexaChannelController;
using namespace alexaChannelControllerInterfaces;
using namespace alexaChannelControllerTypes;

// For readability.
using EndpointIdentifier = endpoints::EndpointIdentifier;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// Time of sample used for testing
static const std::string TIME_OF_SAMPLE_TEST("2017-02-03T16:20:50.523Z");

/// The namespace for capability agent.
static const std::string NAMESPACE{"Alexa.ChannelController"};

/// The supported version
static const std::string INTERFACE_VERSION{"3"};

/// The name for ChangeChannel directive
static const std::string NAME_CHANGECHANNEL{"ChangeChannel"};

/// The name for SkipChannels directive
static const std::string NAME_SKIPCHANNELS{"SkipChannels"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The test EndpointId
static const EndpointIdentifier TEST_ENDPOINT_ID("testEndpointId");

/// Event key.
static const std::string EVENT("event");

/// MessageId for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// DialogRequestId for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogRequestId_Test");

/// Correlation token for testing.
static const std::string CORRELATION_TOKEN_TEST("CorrelationToken_Test");

/// Channel number for testing
static const std::string TEST_CHANNEL_NUMBER{"256"};

/// Channel call sign for testing
static const std::string TEST_CHANNEL_CALLSIGN{"PBS"};

/// Channel affiliate callsign for testing
static const std::string TEST_CHANNEL_AFFILIATE_CALLSIGN{"affiliateCallSign"};

/// Channel metadata name for testing
static const std::string TEST_CHANNEL_METADATA_NAME{"Sample Channel"};

/// Channel image URL for testing
static const std::string TEST_CHANNEL_METADATA_IMAGE{"<URI>"};

// Sample ChangeChannel payload
static const std::string CHANGECHANNEL_PAYLOAD = R"({
                                                  "channel": {
                                                    "number": "9",
                                                    "callSign": "PBS",
                                                    "affiliateCallSign": "KCTS",
                                                    "uri": "<channel uri>"
                                                  },
                                                  "channelMetadata": {
                                                    "name": "Alternate channel name",
                                                    "image": "<url for image>"
                                                  }
                                                })";

// Sample SkipChannels increment payload
static const std::string SKIPCHANNELS_INCREMENT_PAYLOAD = R"({
                                              "channelCount" : 1
                                            })";

// Sample SkipChannels decrement payload
static const std::string SKIPCHANNELS_DECREMENT_PAYLOAD = R"({
                                              "channelCount" : -1
                                            })";

/// The name of channel property
static const std::string CHANNELSTATE_PROPERTY_NAME{"channel"};

class MockAlexaChannelControllerInterface : public ChannelControllerInterface {
public:
    MOCK_METHOD1(changeMock, Response(Channel* channel));
    MOCK_METHOD0(incrementChannelMock, Response());
    MOCK_METHOD0(decrementChannelMock, Response());
    MOCK_METHOD0(getCurrentChannelMock, std::unique_ptr<Channel>());
    MOCK_METHOD1(addObserverMock, bool(std::weak_ptr<ChannelControllerObserverInterface> observer));
    MOCK_METHOD1(removeObserverMock, void(std::weak_ptr<ChannelControllerObserverInterface> observer));

    Response change(std::unique_ptr<Channel> channel) {
        return changeMock(channel.get());
    }

    Response incrementChannel() {
        return incrementChannelMock();
    }

    Response decrementChannel() {
        return decrementChannelMock();
    }

    std::unique_ptr<Channel> getCurrentChannel() {
        return getCurrentChannelMock();
    }

    bool addObserver(std::weak_ptr<ChannelControllerObserverInterface> observer) {
        return addObserverMock(observer);
    }

    void removeObserver(std::weak_ptr<ChannelControllerObserverInterface> observer) {
        removeObserverMock(observer);
    }
};

class AlexaChannelControllerCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test case.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Constructor.
    AlexaChannelControllerCapabilityAgentTest() :
            m_channel{Channel::Create(
                TEST_CHANNEL_NUMBER,
                TEST_CHANNEL_CALLSIGN,
                TEST_CHANNEL_AFFILIATE_CALLSIGN,
                "",
                TEST_CHANNEL_METADATA_NAME,
                TEST_CHANNEL_METADATA_IMAGE)},
            m_channelState{Channel::Create(
                TEST_CHANNEL_NUMBER,
                TEST_CHANNEL_CALLSIGN,
                TEST_CHANNEL_AFFILIATE_CALLSIGN,
                "",
                TEST_CHANNEL_METADATA_NAME,
                TEST_CHANNEL_METADATA_IMAGE)} {
    }

protected:
    /**
     * Function to create AlexaChannelControllerCapabilityAgent based on the reportable flags
     * and sets required ContextManager and ChannelController mocks.
     */
    std::shared_ptr<AlexaChannelControllerCapabilityAgent> createCapabilityAgentAndSetExpects(
        bool proactivelyReported,
        bool retrievable);

    /// A mock @c AlexaChannelControllerHandlerInterface object.
    std::shared_ptr<StrictMock<MockAlexaChannelControllerInterface>> m_mockChannelController;

    /// A mock @c AlexaChannelControllerObserverInterface object.
    std::weak_ptr<ChannelControllerObserverInterface> m_observer;

    // a context manager
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>> m_mockContextManager;

    /// The mock @c MessageSenderInterface.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>> m_mockResponseSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>
        m_mockDirectiveHandlerResult;

    // Channel object used for ChangeChannel directive
    std::unique_ptr<Channel> m_channel;

    /// The test @c ChannelState
    std::unique_ptr<Channel> m_channelState;
};

void AlexaChannelControllerCapabilityAgentTest::SetUp() {
    m_mockExceptionSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult =
        make_unique<StrictMock<avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult>>();
    m_mockChannelController = std::make_shared<StrictMock<MockAlexaChannelControllerInterface>>();
    m_mockContextManager = std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockContextManager>>();
    m_mockResponseSender =
        std::make_shared<StrictMock<avsCommon::sdkInterfaces::test::MockAlexaInterfaceMessageSender>>();

    auto timePoint = avsCommon::utils::timing::TimePoint();
    timePoint.setTime_ISO_8601(TIME_OF_SAMPLE_TEST);
}

void AlexaChannelControllerCapabilityAgentTest::TearDown() {
}

std::shared_ptr<AlexaChannelControllerCapabilityAgent> AlexaChannelControllerCapabilityAgentTest::
    createCapabilityAgentAndSetExpects(bool proactivelyReported, bool retrievable) {
    // ContextManager Mocks
    if (retrievable) {
        // Expects a non-null CA reference during create.
        EXPECT_CALL(*m_mockContextManager, addStateProvider(_, ::testing::NotNull()));

        // Expects a null when CA is shutdown.
        EXPECT_CALL(*m_mockContextManager, removeStateProvider(_));
    }

    if (proactivelyReported) {
        // addObserver Called during the initialization.
        EXPECT_CALL(*m_mockChannelController, addObserverMock(_))
            .WillOnce(WithArg<0>(Invoke([this](std::weak_ptr<ChannelControllerObserverInterface> observer) {
                m_observer = observer;
                return true;
            })));

        // removeObserver is called during the shutdown
        EXPECT_CALL(*m_mockChannelController, removeObserverMock(_))
            .WillOnce(WithArg<0>(
                Invoke([this](std::weak_ptr<ChannelControllerObserverInterface> observer) { m_observer.reset(); })));
    }

    auto channelControllerCapabilityAgent = AlexaChannelControllerCapabilityAgent::create(
        TEST_ENDPOINT_ID,
        m_mockChannelController,
        m_mockContextManager,
        m_mockResponseSender,
        m_mockExceptionSender,
        proactivelyReported,
        retrievable);
    return channelControllerCapabilityAgent;
}

std::shared_ptr<AVSDirective> buildAVSDirective(std::string directiveName, std::string payload) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<avsCommon::avs::attachment::test::MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NAMESPACE, directiveName, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST, CORRELATION_TOKEN_TEST, INTERFACE_VERSION);
    auto avsMessageEndpoint = AVSMessageEndpoint(TEST_ENDPOINT_ID);

    return AVSDirective::create("", avsMessageHeader, payload, attachmentManager, "", avsMessageEndpoint);
}

/// Test that create() returns a nullptr if called with invalid arguments.
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_givenInvalidParameters_create_shouldFail) {
    ASSERT_THAT(
        AlexaChannelControllerCapabilityAgent::create(
            "", m_mockChannelController, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaChannelControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, nullptr, m_mockContextManager, m_mockResponseSender, m_mockExceptionSender, true, true),
        IsNull());
    ASSERT_THAT(
        AlexaChannelControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockChannelController,
            nullptr,
            m_mockResponseSender,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    ASSERT_THAT(
        AlexaChannelControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID,
            m_mockChannelController,
            m_mockContextManager,
            nullptr,
            m_mockExceptionSender,
            true,
            true),
        IsNull());
    ASSERT_THAT(
        AlexaChannelControllerCapabilityAgent::create(
            TEST_ENDPOINT_ID, m_mockChannelController, m_mockContextManager, m_mockResponseSender, nullptr, true, true),
        IsNull());
}

/**
 * Test successful handling of ChangeChannel directive.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_changeChannelDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockChannelController, changeMock(_)).WillOnce(Invoke([](Channel* channel) {
        return ChannelControllerInterface::Response{};
    }));

    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([this]() {
        return Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_CHANGECHANNEL, CHANGECHANNEL_PAYLOAD);
    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of ChangeChannel directive.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_changeChannelDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockChannelController, changeMock(_)).WillOnce(Invoke([](Channel* channel) {
        return ChannelControllerInterface::Response{
            ChannelControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE, "Unreachable Endpoint"};
    }));

    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([this]() {
        return Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_CHANGECHANNEL, CHANGECHANNEL_PAYLOAD);
    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}
/**
 * Test successful handling of SkipChannels directive with Increment Channel payload.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_incrementChannelsDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockChannelController, incrementChannelMock()).WillOnce(InvokeWithoutArgs([] {
        return ChannelControllerInterface::Response{};
    }));

    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([this]() {
        return Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SKIPCHANNELS, SKIPCHANNELS_INCREMENT_PAYLOAD);
    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Test successful handling of SkipChannels directive with Decrement payload.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_decrementChannelsDirective_successCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockChannelController, decrementChannelMock()).WillOnce(InvokeWithoutArgs([] {
        return ChannelControllerInterface::Response{};
    }));

    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([this]() {
        return Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
    }));

    EXPECT_CALL(*m_mockResponseSender, sendResponseEvent(_, _, _, _));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SKIPCHANNELS, SKIPCHANNELS_DECREMENT_PAYLOAD);
    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Test error path of SkipChannels directive.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_skipChannelsDirective_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    EXPECT_CALL(*m_mockChannelController, incrementChannelMock()).WillOnce(InvokeWithoutArgs([] {
        return ChannelControllerInterface::Response{
            ChannelControllerInterface::Response::Type::FAILED_ENDPOINT_UNREACHABLE, "Unreachable Endpoint"};
    }));

    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([this]() {
        return Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
    }));

    EXPECT_CALL(
        *m_mockResponseSender,
        sendErrorResponseEvent(
            _, _, _, Matcher<AlexaInterfaceMessageSenderInterface::ErrorResponseType>(testing::_), _));
    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    // Simulate directive
    std::shared_ptr<AVSDirective> directive = buildAVSDirective(NAME_SKIPCHANNELS, SKIPCHANNELS_INCREMENT_PAYLOAD);
    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        directive, std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_unknownDirective) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, ""), std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent call to the
 * ContextManager to build the context.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_reportStateChange_successCase) {
    avsCommon::utils::WaitEvent waitEvent;
    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockChannelController, changeMock(_)).WillOnce(Invoke([this](Channel* channel) {
        auto m_observer_shared = m_observer.lock();
        std::unique_ptr<Channel> channelState = Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
        m_observer_shared->onChannelChanged(std::move(channelState));
        return ChannelControllerInterface::Response{};
    }));

    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([channelControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            channelControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, CHANNELSTATE_PROPERTY_NAME, TEST_ENDPOINT_ID), stateRequestToken);
        }));

    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([this]() {
        return Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
    }));

    EXPECT_CALL(*m_mockContextManager, provideStateResponse(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    m_mockChannelController->changeMock(m_channel.get());

    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Test triggering of reportStateChange and subsequent failure to get the getChannelState and
 * call to the ContextManager to report the failure.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_reportStateChange_errorCase) {
    avsCommon::utils::WaitEvent waitEvent;
    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    EXPECT_CALL(*m_mockChannelController, changeMock(_)).WillOnce(Invoke([this](Channel* channel) {
        auto m_observer_shared = m_observer.lock();
        std::unique_ptr<Channel> channelState = Channel::Create(
            m_channelState->getNumber(),
            m_channelState->getCallSign(),
            m_channelState->getAffiliateCallSign(),
            m_channelState->getURI(),
            m_channelState->getName(),
            m_channelState->getImageURL());
        m_observer_shared->onChannelChanged(std::move(channelState));
        return ChannelControllerInterface::Response{};
    }));
    EXPECT_CALL(*m_mockContextManager, reportStateChange(_, _, _))
        .WillOnce(InvokeWithoutArgs([channelControllerCapabilityAgent] {
            const unsigned int stateRequestToken = 1;
            channelControllerCapabilityAgent->provideState(
                CapabilityTag(NAMESPACE, CHANNELSTATE_PROPERTY_NAME, TEST_ENDPOINT_ID), stateRequestToken);
        }));
    EXPECT_CALL(*m_mockChannelController, getCurrentChannelMock()).WillOnce(InvokeWithoutArgs([]() {
        return Channel::Create("", "", "", "", "", "");
    }));
    EXPECT_CALL(*m_mockContextManager, provideStateUnavailableResponse(_, _, _))
        .WillOnce(InvokeWithoutArgs([&waitEvent]() { waitEvent.wakeUp(); }));
    m_mockChannelController->changeMock(m_channel.get());
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable set.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableTrue) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(true, true);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, ""), std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

/**
 * Tests unknown Directive with both proactively reported and retrievable as false.
 * Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(AlexaChannelControllerCapabilityAgentTest, test_unknownDirectiveWithProactivelyReportedAndRetrievableFalse) {
    avsCommon::utils::WaitEvent waitEvent;

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_)).WillOnce(InvokeWithoutArgs([&waitEvent]() {
        waitEvent.wakeUp();
    }));

    auto channelControllerCapabilityAgent = createCapabilityAgentAndSetExpects(false, false);
    ASSERT_THAT(channelControllerCapabilityAgent, NotNull());

    channelControllerCapabilityAgent->CapabilityAgent::preHandleDirective(
        buildAVSDirective(UNKNOWN_DIRECTIVE, ""), std::move(m_mockDirectiveHandlerResult));
    channelControllerCapabilityAgent->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
    EXPECT_TRUE(waitEvent.wait(TIMEOUT));
    channelControllerCapabilityAgent->shutdown();
}

}  // namespace test
}  // namespace alexaChannelController
}  // namespace alexaClientSDK
