/*
 * Copyright 2017-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <algorithm>
#include <memory>
#include <set>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <SpeakerManager/SpeakerManagerConstants.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "SpeakerManager/SpeakerManager.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace rapidjson;
using namespace ::testing;

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The @c MessageId identifer.
static const std::string MESSAGE_ID("messageId");

/// A @c SetVolume/AdjustVolume payload.
static const std::string VOLUME_PAYLOAD =
    "{"
    "\"volume\":" +
    std::to_string(AVS_SET_VOLUME_MAX) +
    ""
    "}";

/// A @c SetMute payload.
static const std::string MUTE_PAYLOAD =
    "{"
    "\"mute\":" +
    MUTE_STRING +
    ""
    "}";

/// A @c SetMute payload to unmute.
static const std::string UNMUTE_PAYLOAD =
    "{"
    "\"mute\":" +
    UNMUTE_STRING +
    ""
    "}";

#ifdef ENABLE_MAXVOLUME_SETTING
/// A valid value to be used as maximum volume limit.
static const int8_t VALID_MAXIMUM_VOLUME_LIMIT = AVS_SET_VOLUME_MAX - 10;

/// An invalid maximum volume limit value
static const int8_t INVALID_MAXIMUM_VOLUME_LIMIT = AVS_SET_VOLUME_MAX + 10;
#endif

/**
 * A mock object to test that the observer is being correctly notified.
 */
class MockObserver : public SpeakerManagerObserverInterface {
public:
    MOCK_METHOD3(
        onSpeakerSettingsChanged,
        void(const Source&, const SpeakerInterface::Type&, const SpeakerInterface::SpeakerSettings&));
};
struct MockMetricRecorder : public avsCommon::utils::metrics::MetricRecorderInterface {
    MOCK_METHOD1(recordMetric, void(std::shared_ptr<avsCommon::utils::metrics::MetricEvent>));
};

class SpeakerManagerTest : public ::testing::TestWithParam<std::vector<SpeakerInterface::Type>> {
public:
    /// SetUp before each test.
    void SetUp();

    /// TearDown after each test.
    void TearDown();

    /// CleanUp and reset the SpeakerManager.
    void cleanUp();

    /// Function to wait for @c m_wakeSetCompleteFuture to be set.
    void wakeOnSetCompleted();

    /// Helper function to get unique @c Type.
    std::set<SpeakerInterface::Type> getUniqueTypes(std::vector<std::shared_ptr<SpeakerInterface>>& speakers);

#ifdef ENABLE_MAXVOLUME_SETTING
    /**
     * Helper function for create and sent a directive
     *
     * @param directiveName The directive name. One of SetVolume or AdjustVolume.
     * @param volume The value of the volume files within the directive.
     */
    void createAndSendVolumeDirective(const std::string directiveName, const int8_t volume);
#endif

    /// A constructor which initializes the promises and futures needed for the test class.
    SpeakerManagerTest() :
            m_wakeSetCompletedPromise{},
            m_wakeSetCompletedFuture{m_wakeSetCompletedPromise.get_future()} {
    }

protected:
    /// Promise to synchronize directive handling through setCompleted.
    std::promise<void> m_wakeSetCompletedPromise;

    /// Future to synchronize directive handling through setCompleted.
    std::future<void> m_wakeSetCompletedFuture;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /*
     * Set this to a nice mock. The only instance of the mock being called is the setStateProvider member, which we
     * explicitly test.
     */
    std::shared_ptr<NiceMock<MockContextManager>> m_mockContextManager;

    /// A strict mock that allows the test to strictly monitor the messages sent.
    std::shared_ptr<StrictMock<MockMessageSender>> m_mockMessageSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// A mock to allow testing of the observer callback behavior.
    std::shared_ptr<NiceMock<MockObserver>> m_observer;

    /// A pointer to an instance of the SpeakerManager that will be instantiated per test.
    std::shared_ptr<SpeakerManager> m_speakerManager;
};

void SpeakerManagerTest::SetUp() {
    m_metricRecorder = std::make_shared<NiceMock<MockMetricRecorder>>();
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockMessageSender = std::make_shared<StrictMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_observer = std::make_shared<NiceMock<MockObserver>>();
}

void SpeakerManagerTest::TearDown() {
    if (m_speakerManager) {
        m_speakerManager->shutdown();
        m_speakerManager.reset();
    }
}

void SpeakerManagerTest::wakeOnSetCompleted() {
    m_wakeSetCompletedPromise.set_value();
}

/**
 * Helper function to get unique @c Type from a vector of speakers.
 */
std::set<SpeakerInterface::Type> SpeakerManagerTest::getUniqueTypes(
    std::vector<std::shared_ptr<SpeakerInterface>>& speakers) {
    std::set<SpeakerInterface::Type> types;
    for (auto speaker : speakers) {
        types.insert(speaker->getSpeakerType());
    }
    return types;
}

#ifdef ENABLE_MAXVOLUME_SETTING
void SpeakerManagerTest::createAndSendVolumeDirective(const std::string directiveName, const int8_t volume) {
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    static int id = 1;
    const std::string messageId = MESSAGE_ID + std::to_string(id++);
    std::string payload =
        "{"
        "\"volume\":" +
        std::to_string(volume) + "}";

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(SET_VOLUME.nameSpace, directiveName, messageId);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, payload, attachmentManager, "");

    m_speakerManager->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_speakerManager->CapabilityAgent::handleDirective(messageId);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

static int8_t getSpeakerVolume(std::shared_ptr<SpeakerInterface> speaker) {
    SpeakerInterface::SpeakerSettings speakerSettings;

    speaker->getSpeakerSettings(&speakerSettings);

    return speakerSettings.volume;
}
#endif

/// Helper function to generate the VolumeState in JSON for the ContextManager.
std::string generateVolumeStateJson(SpeakerInterface::SpeakerSettings settings) {
    rapidjson::Document state(rapidjson::kObjectType);
    state.AddMember(VOLUME_KEY, settings.volume, state.GetAllocator());
    state.AddMember(MUTED_KEY, settings.mute, state.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        return "";
    }

    return buffer.GetString();
}

/**
 * Tests creating the SpeakerManager with a null contextManager.
 */
TEST_F(SpeakerManagerTest, test_nullContextManager) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers{
        std::make_shared<MockSpeakerInterface>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)};

    m_speakerManager =
        SpeakerManager::create(speakers, m_metricRecorder, nullptr, m_mockMessageSender, m_mockExceptionSender);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with a null messageSender.
 */
TEST_F(SpeakerManagerTest, test_nullMessageSender) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers{
        std::make_shared<MockSpeakerInterface>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)};

    m_speakerManager =
        SpeakerManager::create(speakers, m_metricRecorder, m_mockContextManager, nullptr, m_mockExceptionSender);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with a null exceptionSender.
 */
TEST_F(SpeakerManagerTest, test_nullExceptionSender) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers{
        std::make_shared<MockSpeakerInterface>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)};

    m_speakerManager =
        SpeakerManager::create(speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, nullptr);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with no speakers.
 */
TEST_F(SpeakerManagerTest, test_noSpeakers) {
    m_speakerManager =
        SpeakerManager::create({}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    ASSERT_NE(m_speakerManager, nullptr);
}

/**
 * Tests that the SpeakerManager initially provides the state at constructor time.
 */
TEST_F(SpeakerManagerTest, test_contextManagerSetStateConstructor) {
    EXPECT_CALL(
        *m_mockContextManager,
        setState(VOLUME_STATE, generateVolumeStateJson(DEFAULT_SETTINGS), StateRefreshPolicy::NEVER, _))
        .Times(Exactly(1));
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    std::vector<std::shared_ptr<SpeakerInterface>> speakers{speaker};
    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
}

/*
 * Test setVolume with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeUnderBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(_)).Times(Exactly(0));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MIN - 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test setVolume with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeOverBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(_)).Times(Exactly(0));
    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX + 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeUnderBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, adjustVolume(_)).Times(Exactly(0));
    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MIN - 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeOverBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, adjustVolume(_)).Times(Exactly(0));
    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX + 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test setVolume when the speaker interfaces are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SPEAKER_VOLUME));
    EXPECT_CALL(*speaker2, setVolume(_)).WillRepeatedly(Return(true));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume when the speaker interfaces are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SPEAKER_VOLUME));
    EXPECT_CALL(*speaker2, adjustVolume(_)).WillRepeatedly(Return(true));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume when the adjusted volume is unchanged. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenAdjustVolumeUnchanged) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, adjustVolume(AVS_ADJUST_VOLUME_MIN)).Times(Exactly(0));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MIN, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MIN);
        ASSERT_TRUE(future.get());
    }
}

/*
 * Test setVolume when the new volume is unchanged. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenSetVolumeUnchanged) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MIN)).Times(Exactly(1));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MIN);
        ASSERT_TRUE(future.get());
    }
}

/*
 * Test setMute when the speaker interfaces are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setMuteOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SPEAKER_VOLUME));
    EXPECT_CALL(*speaker2, setMute(_)).WillRepeatedly(Return(true));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future = m_speakerManager->setMute(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, MUTE);
    ASSERT_FALSE(future.get());
}

/**
 * Test getSpeakerSettings when speakers are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_getSpeakerSettingsSpeakersOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SPEAKER_VOLUME));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager = SpeakerManager::create(
        speakers, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    SpeakerInterface::SpeakerSettings settings;
    std::future<bool> future =
        m_speakerManager->getSpeakerSettings(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_FALSE(future.get());
}

/**
 * Test getConfiguration and ensure that all directives are handled.
 */
TEST_F(SpeakerManagerTest, test_getConfiguration) {
    std::shared_ptr<SpeakerInterface> speaker =
        std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    auto configuration = m_speakerManager->getConfiguration();
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    ASSERT_EQ(configuration[SET_VOLUME], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[ADJUST_VOLUME], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[SET_MUTE], audioNonBlockingPolicy);
}

/**
 * Test that adding a null observer does not cause any errors in the SpeakerManager.
 */
TEST_F(SpeakerManagerTest, test_addNullObserver) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(nullptr);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(2));

    m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX).wait();
    m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX).wait();
    m_speakerManager->setMute(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, MUTE).wait();
}

/**
 * Test that removing an observer works correctly.
 */
TEST_F(SpeakerManagerTest, test_removeSpeakerManagerObserver) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(2));

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    m_speakerManager->removeSpeakerManagerObserver(m_observer);

    m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX).wait();
    m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX).wait();
    m_speakerManager->setMute(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, MUTE).wait();
}

/**
 * Test that removing a null observer does not cause any errors in the SpeakerManager.
 */
TEST_F(SpeakerManagerTest, test_removeNullObserver) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(2));

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
    m_speakerManager->removeSpeakerManagerObserver(nullptr);

    m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX).wait();
    m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX).wait();
    m_speakerManager->setMute(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, MUTE).wait();
}

/*
 * Test retryLogic for SetVolume on speaker type AVS_SPEAKER_VOLUME. Returning false once for speaker->setVolume()
 * triggers retry and when successful returns the future of value true.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsForSetVolume) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(_)).Times(Exactly(2)).WillOnce(Return(false));

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    std::future<bool> future =
        m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MIN);
    ASSERT_TRUE(future.get());
}

/*
 * Test retryLogic for AdjustVolume on speaker type AVS_SPEAKER_VOLUME. Returning false once for speaker->setVolume()
 * triggers retry and when successful returns the future of value true.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsForAdjustVolume) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(_)).Times(Exactly(2)).WillOnce(Return(false));

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MIN);
    ASSERT_TRUE(future.get());
}

/*
 * Test retryLogic for SetMute on speaker type AVS_SPEAKER_VOLUME. Returning false once for speaker->setMute()
 * triggers retry and when successful returns the future of value true.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsForSetMute) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setMute(_)).Times(Exactly(2)).WillOnce(Return(false));

    m_speakerManager = SpeakerManager::create(
        {speaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));

    std::future<bool> future = m_speakerManager->setMute(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, MUTE);
    ASSERT_TRUE(future.get());
}

#ifdef ENABLE_MAXVOLUME_SETTING
/**
 * Test that setting a maximum volume limit succeeds
 * and a local call to setVolume or adjustVolume will
 * completely fail.
 */

TEST_F(SpeakerManagerTest, test_setMaximumVolumeLimit) {
    auto avsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    auto alertsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);

    avsSpeaker->DelegateToReal();
    alertsSpeaker->DelegateToReal();

    avsSpeaker->setVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1);
    alertsSpeaker->setVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1);

    // Expect volumeChanged event.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    EXPECT_CALL(*avsSpeaker, setVolume(_)).Times(AtLeast(1));
    EXPECT_CALL(*alertsSpeaker, setVolume(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(0);

    m_speakerManager = SpeakerManager::create(
        {avsSpeaker, alertsSpeaker},
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    EXPECT_TRUE(m_speakerManager->setMaximumVolumeLimit(VALID_MAXIMUM_VOLUME_LIMIT).get());

    // Local change either with setVolume will set to limit but with adjustVolume will fail
    EXPECT_TRUE(
        m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, VALID_MAXIMUM_VOLUME_LIMIT + 1).get());
    EXPECT_FALSE(
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, VALID_MAXIMUM_VOLUME_LIMIT + 1)
            .get());

    // The volume went to upper limit.
    EXPECT_EQ(getSpeakerVolume(avsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);
    EXPECT_EQ(getSpeakerVolume(alertsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);

    // Increase the volume by 2, so end result will exceed the limit.
    EXPECT_TRUE(m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, 2).get());

    // Following the 2nd adjustVolume, the volume will change to the limit.
    EXPECT_EQ(getSpeakerVolume(alertsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);
}

/**
 * Test that if a new limit was set while the volume was higher
 * than the new limit, operation will succeed and the volume will be decreased.
 */

TEST_F(SpeakerManagerTest, testSetMaximumVolumeLimitWhileVolumeIsHigher) {
    auto avsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    auto alertsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);

    avsSpeaker->DelegateToReal();
    alertsSpeaker->DelegateToReal();

    EXPECT_TRUE(avsSpeaker->setVolume(VALID_MAXIMUM_VOLUME_LIMIT + 1));
    EXPECT_TRUE(alertsSpeaker->setVolume(VALID_MAXIMUM_VOLUME_LIMIT + 1));

    EXPECT_CALL(*avsSpeaker, setVolume(VALID_MAXIMUM_VOLUME_LIMIT)).Times(1);
    EXPECT_CALL(*alertsSpeaker, setVolume(VALID_MAXIMUM_VOLUME_LIMIT)).Times(1);

    // Expect volumeChanged event.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        {avsSpeaker, alertsSpeaker},
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    EXPECT_TRUE(m_speakerManager->setMaximumVolumeLimit(VALID_MAXIMUM_VOLUME_LIMIT).get());

    EXPECT_EQ(getSpeakerVolume(avsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);
    EXPECT_EQ(getSpeakerVolume(alertsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);
}

/**
 * Test that SetVolume directive with volume > limit
 * should set the volume to the limit
 */

TEST_F(SpeakerManagerTest, testAVSSetVolumeHigherThanLimit) {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);
    auto avsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    auto alertsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);

    avsSpeaker->DelegateToReal();
    alertsSpeaker->DelegateToReal();

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));

    EXPECT_TRUE(avsSpeaker->setVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1));
    EXPECT_TRUE(alertsSpeaker->setVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1));

    m_speakerManager = SpeakerManager::create(
        {avsSpeaker, alertsSpeaker},
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    EXPECT_TRUE(m_speakerManager->setMaximumVolumeLimit(VALID_MAXIMUM_VOLUME_LIMIT).get());

    createAndSendVolumeDirective(SET_VOLUME.name, VALID_MAXIMUM_VOLUME_LIMIT + 1);

    ASSERT_EQ(getSpeakerVolume(avsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);
    ASSERT_EQ(getSpeakerVolume(alertsSpeaker), VALID_MAXIMUM_VOLUME_LIMIT);
}

/**
 * Test that a call to @c setMaximumVolumeLimit with invalid value fails.
 */
TEST_F(SpeakerManagerTest, testSetMaximumVolumeLimitWithInvalidValue) {
    auto avsSpeaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);

    m_speakerManager = SpeakerManager::create(
        {avsSpeaker}, m_metricRecorder, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_FALSE(m_speakerManager->setMaximumVolumeLimit(INVALID_MAXIMUM_VOLUME_LIMIT).get());
}
#endif

/**
 * Create different combinations of @c Type for  parameterized tests (TEST_P).
 */
INSTANTIATE_TEST_CASE_P(
    Parameterized,
    SpeakerManagerTest,
    // clang-format off
    ::testing::Values(
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_SPEAKER_VOLUME
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_ALERTS_VOLUME
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            SpeakerInterface::Type::AVS_SPEAKER_VOLUME
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_ALERTS_VOLUME,
            SpeakerInterface::Type::AVS_ALERTS_VOLUME,
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            SpeakerInterface::Type::AVS_ALERTS_VOLUME,
            SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            SpeakerInterface::Type::AVS_ALERTS_VOLUME
        }));
    // clang-format off

/**
 * Parameterized test for setVolume. One event should be sent if an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_setVolume) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(1));
        speakers.push_back(speaker);
    }

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings)).Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MAX, false, SpeakerManagerObserverInterface::Source::DIRECTIVE);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for adjustVolume. One event should be sent if an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_adjustVolume) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(1));
        speakers.push_back(speaker);
    }

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MAX, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings)).Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MAX, false, SpeakerManagerObserverInterface::Source::DIRECTIVE);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for setMute. One event should be sent if an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_setMute) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, setMute(MUTE)).Times(Exactly(1));
        speakers.push_back(speaker);
    }

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings)).Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setMute(type, MUTE, false, SpeakerManagerObserverInterface::Source::DIRECTIVE);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for getSpeakerSettings. Operation should succeed with default speaker settings.
 */
TEST_P(SpeakerManagerTest, test_getSpeakerSettings) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : uniqueTypes) {
        SpeakerInterface::SpeakerSettings settings;
        std::future<bool> future = m_speakerManager->getSpeakerSettings(type, &settings);
        ASSERT_TRUE(future.get());
        ASSERT_EQ(settings.volume, DEFAULT_SETTINGS.volume);
        ASSERT_EQ(settings.mute, DEFAULT_SETTINGS.mute);
    }
}

/**
 * Tests SetVolume Directive. Expect that the volume is unmuted and set, as well at most one
 * event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_setVolumeDirective) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        SpeakerInterface::SpeakerSettings temp;
        speaker->getSpeakerSettings(&temp);
        if (temp.mute) {
            EXPECT_CALL(*speaker, setMute(UNMUTE)).Times(Exactly(timesCalled));
        }
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    // Creation expectations based on type.
    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, expectedSettings)).Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(SET_VOLUME.nameSpace, SET_VOLUME.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD, attachmentManager, "");

    m_speakerManager->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_speakerManager->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests AdjustVolume Directive. Expect that the volume is unmuted and adjusted, as well at most one
 * event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_adjustVolumeDirective) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        SpeakerInterface::SpeakerSettings temp;
        speaker->getSpeakerSettings(&temp);
        if (temp.mute) {
            EXPECT_CALL(*speaker, setMute(UNMUTE)).Times(Exactly(timesCalled));
        }
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    // Creation expectations based on type.
    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, expectedSettings)).Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(ADJUST_VOLUME.nameSpace, ADJUST_VOLUME.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD, attachmentManager, "");

    m_speakerManager->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_speakerManager->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Tests SetMute Directive. Expect that the volume is muted, as well at most one
 * event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_setMuteDirective) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        EXPECT_CALL(*speaker, setMute(MUTE)).Times(Exactly(timesCalled));

        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    // Creation expectations based on type.
    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, expectedSettings)).Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(SET_MUTE.nameSpace, SET_MUTE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, MUTE_PAYLOAD, attachmentManager, "");

    m_speakerManager->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_speakerManager->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test setVolume when unmute Directive sent. Setup test by setting volume to 0 and mute to true.
 * Expect that the volume is unmuted and set to MIN_UNMUTE_VOLUME, as well at most one
 * event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_setVolumeDirectiveWhenMuted) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MIN)).Times(1);
        EXPECT_CALL(*speaker, setMute(MUTE)).Times(1);

        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SPEAKER_VOLUME) {
            EXPECT_CALL(*speaker, setMute(UNMUTE)).Times(1);
            EXPECT_CALL(*speaker, setVolume(MIN_UNMUTE_VOLUME)).Times(1);
        }
        speakers.push_back(speaker);
    }

    m_speakerManager = SpeakerManager::create(
        speakers,
        m_metricRecorder,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        m_speakerManager->setVolume(type, AVS_SET_VOLUME_MIN, true);
    }

    for (auto type : getUniqueTypes(speakers)) {
        std::future<bool> future = m_speakerManager->setMute(type, MUTE, true);
    }

    // Check to see if AVS_SPEAKER_VOLUME speakers exist and set EXPECT_CALL accordingly
    auto uniqueTypes = getUniqueTypes(speakers);
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings unMuteSettings{MIN_UNMUTE_VOLUME, UNMUTE};

    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SPEAKER_VOLUME, unMuteSettings)).Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(unMuteSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(0);
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(0);
    }

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(eventsSent);
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    // Create Directive to unmute the device.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(SET_MUTE.nameSpace, SET_MUTE.name, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, UNMUTE_PAYLOAD, attachmentManager, "");

    m_speakerManager->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_speakerManager->CapabilityAgent::handleDirective(MESSAGE_ID);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

}  // namespace test
}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK