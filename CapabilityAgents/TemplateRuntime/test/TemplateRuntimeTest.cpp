/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file TemplateRuntimeTest
#include <future>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MediaPropertiesInterface.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/TemplateRuntimeObserverInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "TemplateRuntime/TemplateRuntime.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace templateRuntime {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// Timeout when waiting for clearTemplateCard.
static std::chrono::milliseconds TEMPLATE_TIMEOUT(5000);

/// Timeout when waiting for clearTemplateCard not called.
static std::chrono::milliseconds TEMPLATE_NOT_CLEAR_TIMEOUT(2500);

/// Timeout when waiting for clearTemplateCard.
static std::chrono::milliseconds PLAYER_FINISHED_TIMEOUT(5000);

/// The namespace for this capability agent.
static const std::string NAMESPACE{"TemplateRuntime"};

/// An unknown directive signature.
static const std::string UNKNOWN_DIRECTIVE{"Unknown"};

/// The RenderTemplate directive signature.
static const NamespaceAndName TEMPLATE{NAMESPACE, "RenderTemplate"};

/// The RenderPlayerInfo directive signature.
static const NamespaceAndName PLAYER_INFO{NAMESPACE, "RenderPlayerInfo"};

/// The @c MessageId identifer.
static const std::string MESSAGE_ID("messageId");

/// An audioItemId for the RenderPlayerInfo directive.
static const std::string AUDIO_ITEM_ID("AudioItemId abcdefgh");

/// An audioItemId without a corresponding RenderPlayerInfo directive.
static const std::string AUDIO_ITEM_ID_1("AudioItemId 12345678");

/// A RenderTemplate directive payload.
// clang-format off
static const std::string TEMPLATE_PAYLOAD = "{"
    "\"token\":\"TOKEN1\","
    "\"type\":\"BodyTemplate1\","
    "\"title\":{"
        "\"mainTitle\":\"MAIN_TITLE\","
        "\"subTitle\":\"SUB_TITLE\""
    "}"
"}";
// clang-format on

/// A RenderPlayerInfo directive payload.
// clang-format off
static const std::string PLAYERINFO_PAYLOAD = "{"
    "\"audioItemId\":\"" + AUDIO_ITEM_ID + "\","
    "\"content\":{"
        "\"title\":\"TITLE\","
        "\"header\":\"HEADER\""
    "}"
"}";
// clang-format on

/// A malformed RenderPlayerInfo directive payload.
// clang-format off
static const std::string MALFORM_PLAYERINFO_PAYLOAD = "{"
    "\"audioItemId\"::::\"" + AUDIO_ITEM_ID + "\","
        "\"content\":{{{{"
        "\"title\":\"TITLE\","
        "\"header\":\"HEADER\""
    "}"
"}";
// clang-format on

class MockMediaPropertiesFetcher : public MediaPropertiesInterface {
public:
    MOCK_METHOD0(getAudioItemOffset, std::chrono::milliseconds());
};

class MockRenderInfoCardsPlayer : public RenderPlayerInfoCardsProviderInterface {
public:
    MOCK_METHOD1(
        setObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer));
};

class MockGui : public TemplateRuntimeObserverInterface {
public:
    MOCK_METHOD2(renderTemplateCard, void(const std::string& jsonPayload, avsCommon::avs::FocusState focusState));
    MOCK_METHOD0(clearTemplateCard, void());
    MOCK_METHOD3(
        renderPlayerInfoCard,
        void(
            const std::string& jsonPayload,
            TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo,
            avsCommon::avs::FocusState focusState));
    MOCK_METHOD0(clearPlayerInfoCard, void());
};

/// Test harness for @c TemplateRuntime class.
class TemplateRuntimeTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Function to set the promise and wake @c m_wakeSetCompleteFuture.
    void wakeOnSetCompleted();

    /// Function to set the promise and wake @c m_wakeRenderTemplateCardFuture.
    void wakeOnRenderTemplateCard();

    /// Function to set the promise and wake @c m_wakeRenderPlayerInfoCardFuture.
    void wakeOnRenderPlayerInfoCard();

    /// Function to set the promise and wake @c m_wakeClearTemplateCardFuture.
    void wakeOnClearTemplateCard();

    /// Function to set the promise and wake @c m_wakeClearPlayerInfoCardFuture.
    void wakeOnClearPlayerInfoCard();

    /// Function to set the promise and wake @c m_wakeReleaseChannelFuture.
    void wakeOnReleaseChannel();

    /// A constructor which initializes the promises and futures needed for the test class.
    TemplateRuntimeTest() :
            m_wakeSetCompletedPromise{},
            m_wakeSetCompletedFuture{m_wakeSetCompletedPromise.get_future()},
            m_wakeRenderTemplateCardPromise{},
            m_wakeRenderTemplateCardFuture{m_wakeRenderTemplateCardPromise.get_future()},
            m_wakeRenderPlayerInfoCardPromise{},
            m_wakeRenderPlayerInfoCardFuture{m_wakeRenderPlayerInfoCardPromise.get_future()},
            m_wakeClearTemplateCardPromise{},
            m_wakeClearTemplateCardFuture{m_wakeClearTemplateCardPromise.get_future()},
            m_wakeClearPlayerInfoCardPromise{},
            m_wakeClearPlayerInfoCardFuture{m_wakeClearPlayerInfoCardPromise.get_future()},
            m_wakeReleaseChannelPromise{},
            m_wakeReleaseChannelFuture{m_wakeReleaseChannelPromise.get_future()} {
    }

protected:
    /// Promise to synchronize directive handling through setCompleted.
    std::promise<void> m_wakeSetCompletedPromise;

    /// Future to synchronize directive handling through setCompleted.
    std::future<void> m_wakeSetCompletedFuture;

    /// Promise to synchronize directive handling with RenderTemplateCard callback.
    std::promise<void> m_wakeRenderTemplateCardPromise;

    /// Future to synchronize directive handling with RenderTemplateCard callback.
    std::future<void> m_wakeRenderTemplateCardFuture;

    /// Promise to synchronize directive handling with RenderPlayerInfoCard callback.
    std::promise<void> m_wakeRenderPlayerInfoCardPromise;

    /// Future to synchronize directive handling with RenderPlayerInfoCard callback.
    std::future<void> m_wakeRenderPlayerInfoCardFuture;

    /// Promise to synchronize ClearTemplateCard callback.
    std::promise<void> m_wakeClearTemplateCardPromise;

    /// Future to synchronize ClearTemplateCard callback.
    std::future<void> m_wakeClearTemplateCardFuture;

    /// Promise to synchronize ClearPlayerInfoCard callback.
    std::promise<void> m_wakeClearPlayerInfoCardPromise;

    /// Future to synchronize ClearPlayerInfoCard callback.
    std::future<void> m_wakeClearPlayerInfoCardFuture;

    /// Promise to synchronize releaseChannel calls.
    std::promise<void> m_wakeReleaseChannelPromise;

    /// Future to synchronize releaseChannel calls.
    std::future<void> m_wakeReleaseChannelFuture;

    /// A nice mock for the RenderInfoCardsInterface calls.
    std::shared_ptr<NiceMock<MockRenderInfoCardsPlayer>> m_mockRenderPlayerInfoCardsProvider;

    /// A nice mock for the MediaPropertiesInterface calls.
    std::shared_ptr<MockMediaPropertiesFetcher> m_mediaPropertiesFetcher;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// @c FocusManager to request focus to the Visual channel.
    std::shared_ptr<MockFocusManager> m_mockFocusManager;

    /// A strict mock to allow testing of the observer callback.
    std::shared_ptr<StrictMock<MockGui>> m_mockGui;

    /// A pointer to an instance of the TemplateRuntime that will be instantiated per test.
    std::shared_ptr<TemplateRuntime> m_templateRuntime;
};

void TemplateRuntimeTest::SetUp() {
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mediaPropertiesFetcher = std::make_shared<NiceMock<MockMediaPropertiesFetcher>>();
    m_mockRenderPlayerInfoCardsProvider = std::make_shared<NiceMock<MockRenderInfoCardsPlayer>>();
    m_mockGui = std::make_shared<StrictMock<MockGui>>();
    m_templateRuntime =
        TemplateRuntime::create({m_mockRenderPlayerInfoCardsProvider}, m_mockFocusManager, m_mockExceptionSender);
    m_templateRuntime->addObserver(m_mockGui);

    ON_CALL(*m_mockFocusManager, acquireChannel(_, _, _)).WillByDefault(InvokeWithoutArgs([this] {
        m_templateRuntime->onFocusChanged(avsCommon::avs::FocusState::FOREGROUND);
        return true;
    }));

    ON_CALL(*m_mockFocusManager, releaseChannel(_, _)).WillByDefault(InvokeWithoutArgs([this] {
        auto releaseChannelSuccess = std::make_shared<std::promise<bool>>();
        std::future<bool> returnValue = releaseChannelSuccess->get_future();
        m_templateRuntime->onFocusChanged(avsCommon::avs::FocusState::NONE);
        releaseChannelSuccess->set_value(true);
        return returnValue;
    }));
}

void TemplateRuntimeTest::TearDown() {
    if (m_templateRuntime) {
        m_templateRuntime->shutdown();
        m_templateRuntime.reset();
    }
}

void TemplateRuntimeTest::wakeOnSetCompleted() {
    m_wakeSetCompletedPromise.set_value();
}

void TemplateRuntimeTest::wakeOnRenderTemplateCard() {
    m_wakeRenderTemplateCardPromise.set_value();
}

void TemplateRuntimeTest::wakeOnRenderPlayerInfoCard() {
    m_wakeRenderPlayerInfoCardPromise.set_value();
}

void TemplateRuntimeTest::wakeOnClearTemplateCard() {
    m_wakeClearTemplateCardPromise.set_value();
}

void TemplateRuntimeTest::wakeOnClearPlayerInfoCard() {
    m_wakeClearPlayerInfoCardPromise.set_value();
}

void TemplateRuntimeTest::wakeOnReleaseChannel() {
    m_wakeReleaseChannelPromise.set_value();
}

/**
 * Tests creating the TemplateRuntime with a null audioPlayerInterface.
 */
TEST_F(TemplateRuntimeTest, test_nullAudioPlayerInterface) {
    auto templateRuntime = TemplateRuntime::create({nullptr}, m_mockFocusManager, m_mockExceptionSender);
    ASSERT_EQ(templateRuntime, nullptr);
}

/**
 * Tests creating the TemplateRuntime with a null focusManagerInterface.
 */
TEST_F(TemplateRuntimeTest, test_nullFocusManagerInterface) {
    auto templateRuntime =
        TemplateRuntime::create({m_mockRenderPlayerInfoCardsProvider}, nullptr, m_mockExceptionSender);
    ASSERT_EQ(templateRuntime, nullptr);
}

/**
 * Tests creating the TemplateRuntime with a null exceptionSender.
 */
TEST_F(TemplateRuntimeTest, test_nullExceptionSender) {
    auto templateRuntime = TemplateRuntime::create({m_mockRenderPlayerInfoCardsProvider}, m_mockFocusManager, nullptr);
    ASSERT_EQ(templateRuntime, nullptr);
}

/**
 * Tests that the TemplateRuntime successfully add itself with the RenderInfoCardsPlayers at constructor time, and
 * successfully remove itself with the RenderPlayerInfoCardsPlayers during shutdown.
 */
TEST_F(TemplateRuntimeTest, test_renderInfoCardsPlayersAddRemoveObserver) {
    auto mockRenderInfoCardsProvider1 = std::make_shared<NiceMock<MockRenderInfoCardsPlayer>>();
    auto mockRenderInfoCardsProvider2 = std::make_shared<NiceMock<MockRenderInfoCardsPlayer>>();
    auto mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    auto mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();

    Expectation setObserver1 = EXPECT_CALL(*mockRenderInfoCardsProvider1, setObserver(NotNull())).Times(Exactly(1));
    EXPECT_CALL(*mockRenderInfoCardsProvider1, setObserver(IsNull())).Times(Exactly(1)).After(setObserver1);
    Expectation setObserver2 = EXPECT_CALL(*mockRenderInfoCardsProvider2, setObserver(NotNull())).Times(Exactly(1));
    EXPECT_CALL(*mockRenderInfoCardsProvider2, setObserver(IsNull())).Times(Exactly(1)).After(setObserver2);

    auto templateRuntime = TemplateRuntime::create(
        {mockRenderInfoCardsProvider1, mockRenderInfoCardsProvider2}, mockFocusManager, mockExceptionSender);
    templateRuntime->shutdown();
}

/**
 * Tests unknown Directive. Expect that the sendExceptionEncountered and setFailed will be called.
 */
TEST_F(TemplateRuntimeTest, test_unknownDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE, UNKNOWN_DIRECTIVE, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive = AVSDirective::create("", avsMessageHeader, "", attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests RenderTemplate Directive. Expect that the renderTemplateCard callback will be called and clearTemplateCard will
 * be called after 2s after DialogUXState is changed to IDLE state.
 */
TEST_F(TemplateRuntimeTest, testSlow_renderTemplateDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(TEMPLATE.nameSpace, TEMPLATE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderTemplateCard(TEMPLATE_PAYLOAD, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderTemplateCard));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockGui, clearTemplateCard())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnClearTemplateCard));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);
    m_templateRuntime->onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_wakeClearTemplateCardFuture.wait_for(TEMPLATE_TIMEOUT);
}

/**
 * Tests RenderTemplate Directive. Expect that the renderTemplateCard callback will be called and clearTemplateCard will
 * not be called if DialogUXState goes to IDLE state and then goes EXPECTING and SPEAKING state.
 */
TEST_F(
    TemplateRuntimeTest,
    testRenderTemplateDirectiveWillNotClearCardAfterGoingToExpectingStateAfterGoingToIDLESlowTest) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(TEMPLATE.nameSpace, TEMPLATE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderTemplateCard(TEMPLATE_PAYLOAD, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderTemplateCard));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockGui, clearTemplateCard()).Times(Exactly(0));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);

    // first test IDLE->EXPECTING transition
    m_templateRuntime->onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_templateRuntime->onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::EXPECTING);
    EXPECT_EQ(m_wakeClearTemplateCardFuture.wait_for(TEMPLATE_NOT_CLEAR_TIMEOUT), std::future_status::timeout);

    // now test IDLE->SPEAKING transition
    m_templateRuntime->onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE);
    m_templateRuntime->onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::SPEAKING);
    EXPECT_EQ(m_wakeClearTemplateCardFuture.wait_for(TEMPLATE_NOT_CLEAR_TIMEOUT), std::future_status::timeout);
}

/**
 * Tests RenderTemplate Directive using the handleDirectiveImmediately. Expect that the renderTemplateCard
 * callback will be called.
 */
TEST_F(TemplateRuntimeTest, test_handleDirectiveImmediately) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(TEMPLATE.nameSpace, TEMPLATE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderTemplateCard(TEMPLATE_PAYLOAD, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderTemplateCard));

    m_templateRuntime->handleDirectiveImmediately(directive);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);
}

/**
 * Tests RenderTemplate Directive received before the corresponding AudioPlayer call. Expect
 * that the renderTemplateCard callback will be called and clearPlayerInfoCard will be called after 2s after Audio State
 * is changed to FINISHED state.
 */
TEST_F(TemplateRuntimeTest, testSlow_renderPlayerInfoDirectiveBefore) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PLAYERINFO_PAYLOAD, attachmentManager, "");

    ::testing::InSequence s;
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockGui, renderTemplateCard(_, _)).Times(Exactly(0));

    // do not expect renderPlayerInfo card call until AudioPlayer notify with the correct audioItemId
    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(_, _, _)).Times(Exactly(0));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(2))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderPlayerInfoCard))
        .WillOnce(InvokeWithoutArgs([] {}));

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID;
    context.offset = TIMEOUT;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);

    m_wakeRenderPlayerInfoCardFuture.wait_for(TIMEOUT);

    EXPECT_CALL(*m_mockGui, clearPlayerInfoCard())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnClearPlayerInfoCard));

    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::FINISHED, context);
    m_wakeClearPlayerInfoCardFuture.wait_for(PLAYER_FINISHED_TIMEOUT);
}

/**
 * Tests RenderTemplate Directive received after the corresponding AudioPlayer call. Expect
 * that the renderTemplateCard callback will be called.
 */
TEST_F(TemplateRuntimeTest, test_renderPlayerInfoDirectiveAfter) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PLAYERINFO_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderPlayerInfoCard));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID;
    context.offset = TIMEOUT;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);
    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_wakeRenderPlayerInfoCardFuture.wait_for(TIMEOUT);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests RenderTemplate Directive received without an audioItemId. Expect that the
 * sendExceptionEncountered and setFailed will be called.
 */
TEST_F(TemplateRuntimeTest, test_renderPlayerInfoDirectiveWithoutAudioItemId) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests when a malformed RenderTemplate Directive is received.  Expect that the
 * sendExceptionEncountered and setFailed will be called.
 */
TEST_F(TemplateRuntimeTest, test_malformedRenderPlayerInfoDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, MALFORM_PLAYERINFO_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockExceptionSender, sendExceptionEncountered(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests AudioPlayer notified the handling of AUDIO_ITEM_ID_1, and then RenderTemplate Directive with
 * AUDIO_ITEM_ID is received.  Expect that the renderTemplateCard callback will not be called until
 * the AudioPlayer notified the handling of AUDIO_ITEM_ID later.
 */
TEST_F(TemplateRuntimeTest, test_renderPlayerInfoDirectiveDifferentAudioItemId) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PLAYERINFO_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID_1;
    context.offset = TIMEOUT;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);
    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderPlayerInfoCard));

    context.audioItemId = AUDIO_ITEM_ID;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);

    m_wakeRenderPlayerInfoCardFuture.wait_for(TIMEOUT);
}

/**
 * Tests Provider notified the handling of AUDIO_ITEM_ID_1, and another provider notified the handling of
 * AUDIO_ITEM_ID, and then RenderTemplate Directive with AUDIO_ITEM_ID is received.  Expect that the
 * renderTemplateCard callback will be called and the correct getAudioItemOffset is called.
 */
TEST_F(TemplateRuntimeTest, test_renderPlayerInfoDirectiveWithTwoProviders) {
    auto anotherMediaPropertiesFetcher = std::make_shared<NiceMock<MockMediaPropertiesFetcher>>();

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PLAYERINFO_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderPlayerInfoCard));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));

    EXPECT_CALL(*anotherMediaPropertiesFetcher, getAudioItemOffset())
        .Times(Exactly(1))
        .WillOnce(Return(std::chrono::milliseconds::zero()));
    EXPECT_CALL(*m_mediaPropertiesFetcher, getAudioItemOffset()).Times(Exactly(0));

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID_1;
    context.offset = TIMEOUT;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);

    RenderPlayerInfoCardsObserverInterface::Context context1;
    context1.mediaProperties = anotherMediaPropertiesFetcher;
    context1.audioItemId = AUDIO_ITEM_ID;
    context1.offset = TIMEOUT;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context1);

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);

    m_wakeRenderPlayerInfoCardFuture.wait_for(TIMEOUT);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests AudioPlayer callbacks will trigger the correct renderPlayerInfoCard callbacks. Expect
 * the payload, audioPlayerState and offset to match to the ones passed in by the
 * RenderPlayerInfoCardsObserverInterface.
 */
TEST_F(TemplateRuntimeTest, test_renderPlayerInfoDirectiveAudioStateUpdate) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PLAYERINFO_PAYLOAD, attachmentManager, "");

    ::testing::InSequence s;
    // Send a directive first to TemplateRuntime
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID;

    // Test onAudioPlayed() callback with 100ms offset
    std::promise<void> wakePlayPromise;
    std::future<void> wakePlayFuture = wakePlayPromise.get_future();
    context.offset = std::chrono::milliseconds(100);
    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&wakePlayPromise, context](
                             const std::string& jsonPayload,
                             TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo,
                             avsCommon::avs::FocusState focusState) {
            EXPECT_EQ(audioPlayerInfo.audioPlayerState, avsCommon::avs::PlayerActivity::PLAYING);
            EXPECT_EQ(audioPlayerInfo.offset, context.offset);
            wakePlayPromise.set_value();
        }));
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);
    wakePlayFuture.wait_for(TIMEOUT);

    // Test onAudioPaused() callback with 200ms offset
    std::promise<void> wakePausePromise;
    std::future<void> wakePauseFuture = wakePausePromise.get_future();
    context.offset = std::chrono::milliseconds(200);
    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&wakePausePromise, context](
                             const std::string& jsonPayload,
                             TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo,
                             avsCommon::avs::FocusState focusState) {
            EXPECT_EQ(audioPlayerInfo.audioPlayerState, avsCommon::avs::PlayerActivity::PAUSED);
            EXPECT_EQ(audioPlayerInfo.offset, context.offset);
            wakePausePromise.set_value();
        }));
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PAUSED, context);
    wakePauseFuture.wait_for(TIMEOUT);

    // Test onAudioStopped() callback with 300ms offset
    std::promise<void> wakeStopPromise;
    std::future<void> wakeStopFuture = wakeStopPromise.get_future();
    context.offset = std::chrono::milliseconds(300);
    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&wakeStopPromise, context](
                             const std::string& jsonPayload,
                             TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo,
                             avsCommon::avs::FocusState focusState) {
            EXPECT_EQ(audioPlayerInfo.audioPlayerState, avsCommon::avs::PlayerActivity::STOPPED);
            EXPECT_EQ(audioPlayerInfo.offset, context.offset);
            wakeStopPromise.set_value();
        }));
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::STOPPED, context);
    wakeStopFuture.wait_for(TIMEOUT);

    // Test onAudioFinished() callback with 400ms offset
    std::promise<void> wakeFinishPromise;
    std::future<void> wakeFinishFuture = wakeFinishPromise.get_future();
    context.offset = std::chrono::milliseconds(400);
    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(Invoke([&wakeFinishPromise, context](
                             const std::string& jsonPayload,
                             TemplateRuntimeObserverInterface::AudioPlayerInfo audioPlayerInfo,
                             avsCommon::avs::FocusState focusState) {
            EXPECT_EQ(audioPlayerInfo.audioPlayerState, avsCommon::avs::PlayerActivity::FINISHED);
            EXPECT_EQ(audioPlayerInfo.offset, context.offset);
            wakeFinishPromise.set_value();
        }));
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::FINISHED, context);
    wakeFinishFuture.wait_for(TIMEOUT);
}

/**
 * Tests that if focus is changed to none, the clearTemplateCard() will be called.
 */
TEST_F(TemplateRuntimeTest, test_focusNone) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(TEMPLATE.nameSpace, TEMPLATE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderTemplateCard(TEMPLATE_PAYLOAD, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderTemplateCard));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockGui, clearTemplateCard())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnClearTemplateCard));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);
    m_templateRuntime->onFocusChanged(FocusState::NONE);
    m_wakeClearTemplateCardFuture.wait_for(TIMEOUT);
}

/**
 * Tests that if displayCardCleared() is called, the clearTemplateCard() will not be called.
 */
TEST_F(TemplateRuntimeTest, test_displayCardCleared) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(TEMPLATE.nameSpace, TEMPLATE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderTemplateCard(TEMPLATE_PAYLOAD, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderTemplateCard));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    EXPECT_CALL(*m_mockGui, clearTemplateCard()).Times(Exactly(0));
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(_, _)).Times(Exactly(1)).WillOnce(InvokeWithoutArgs([this] {
        auto releaseChannelSuccess = std::make_shared<std::promise<bool>>();
        std::future<bool> returnValue = releaseChannelSuccess->get_future();
        m_templateRuntime->onFocusChanged(avsCommon::avs::FocusState::NONE);
        releaseChannelSuccess->set_value(true);
        wakeOnReleaseChannel();
        return returnValue;
    }));

    m_templateRuntime->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);
    m_templateRuntime->displayCardCleared();
    m_wakeReleaseChannelFuture.wait_for(TIMEOUT);
}

/**
 * Tests that if another displayCard event is sent before channel's focus is set to none, the state machine would
 * transition to REACQUIRING state and acquireChannel again to display the card.
 */
TEST_F(TemplateRuntimeTest, test_reacquireChannel) {
    // Create RenderPlayerInfo Directive and wait until PlayerInfo card is displayed.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, PLAYERINFO_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderPlayerInfoCard));

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID;
    context.offset = TIMEOUT;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);
    m_templateRuntime->handleDirectiveImmediately(directive);
    m_wakeRenderPlayerInfoCardFuture.wait_for(TIMEOUT);

    // Send displayCardCleared() to clear card, before setting focus to NONE, send another TemplateCard.
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(_, _)).Times(Exactly(1)).WillOnce(InvokeWithoutArgs([this] {
        auto releaseChannelSuccess = std::make_shared<std::promise<bool>>();
        std::future<bool> returnValue = releaseChannelSuccess->get_future();
        releaseChannelSuccess->set_value(true);
        wakeOnReleaseChannel();
        return returnValue;
    }));
    m_templateRuntime->displayCardCleared();
    m_wakeReleaseChannelFuture.wait_for(TIMEOUT);

    // Create RenderTemplate Directive and see if channel is reacquire correctly.
    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(TEMPLATE.nameSpace, TEMPLATE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create("", avsMessageHeader1, TEMPLATE_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*m_mockGui, renderTemplateCard(TEMPLATE_PAYLOAD, _))
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnRenderTemplateCard));

    m_templateRuntime->handleDirectiveImmediately(directive1);
    m_templateRuntime->onFocusChanged(avsCommon::avs::FocusState::NONE);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);
}

/**
 * Test that we should skip rendering a player info card if the audio has already changed.
 */
TEST_F(TemplateRuntimeTest, testTimer_RenderPlayerInfoAfterPlayerActivityChanged) {
    // Create Directive1.
    const std::string messageId1{"messageId1"};
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader1 = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, messageId1);
    std::shared_ptr<AVSDirective> directive1 =
        AVSDirective::create("", avsMessageHeader1, PLAYERINFO_PAYLOAD, attachmentManager, "");

    RenderPlayerInfoCardsObserverInterface::Context context;
    context.mediaProperties = m_mediaPropertiesFetcher;
    context.audioItemId = AUDIO_ITEM_ID;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);

    ::testing::InSequence s;
    EXPECT_CALL(*m_mockFocusManager, acquireChannel(_, _, _)).WillOnce(Return(true));
    // Send a directive first to TemplateRuntime
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(Exactly(1))
        .WillOnce(InvokeWithoutArgs(this, &TemplateRuntimeTest::wakeOnSetCompleted));
    m_templateRuntime->CapabilityAgent::preHandleDirective(directive1, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(messageId1);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    // Test onAudioPlayed() callback with 100ms offset
    std::promise<void> wakePlayPromise;
    std::future<void> wakePlayFuture = wakePlayPromise.get_future();
    context.offset = std::chrono::milliseconds(100);
    EXPECT_CALL(*m_mockGui, renderPlayerInfoCard(PLAYERINFO_PAYLOAD, _, _)).Times(0);
    EXPECT_CALL(*m_mockFocusManager, releaseChannel(_, _)).Times(Exactly(1)).WillOnce(InvokeWithoutArgs([this] {
        auto releaseChannelSuccess = std::make_shared<std::promise<bool>>();
        std::future<bool> returnValue = releaseChannelSuccess->get_future();
        m_templateRuntime->onFocusChanged(avsCommon::avs::FocusState::NONE);
        releaseChannelSuccess->set_value(true);
        wakeOnReleaseChannel();
        return returnValue;
    }));

    // Create Directive2.
    const std::string messageId2{"messageId2"};
    auto avsMessageHeader2 = std::make_shared<AVSMessageHeader>(PLAYER_INFO.nameSpace, PLAYER_INFO.name, messageId2);
    auto mockDirectiveHandlerResult1 = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    std::shared_ptr<AVSDirective> directive2 =
        AVSDirective::create("", avsMessageHeader1, PLAYERINFO_PAYLOAD, attachmentManager, "");
    m_templateRuntime->CapabilityAgent::preHandleDirective(directive2, std::move(m_mockDirectiveHandlerResult));
    m_templateRuntime->CapabilityAgent::handleDirective(messageId2);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    m_wakeRenderTemplateCardFuture.wait_for(TIMEOUT);
    m_templateRuntime->displayCardCleared();
    m_wakeReleaseChannelFuture.wait_for(TIMEOUT);
    context.audioItemId = AUDIO_ITEM_ID_1;
    m_templateRuntime->onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity::PLAYING, context);
    m_templateRuntime->onFocusChanged(avsCommon::avs::FocusState::FOREGROUND);
    m_templateRuntime->displayCardCleared();
    m_wakeReleaseChannelFuture.wait_for(TIMEOUT);
}

}  // namespace test
}  // namespace templateRuntime
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
