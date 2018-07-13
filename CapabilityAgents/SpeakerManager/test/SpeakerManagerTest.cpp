/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include "SpeakerManager/SpeakerManagerConstants.h"
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

/// Value for mute.
static const bool MUTE(true);

/// String value for mute.
static const std::string MUTE_STRING("true");

/// Value for unmute.
static const bool UNMUTE(false);

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

static const SpeakerInterface::SpeakerSettings DEFAULT_SETTINGS{AVS_SET_VOLUME_MIN, UNMUTE};

class MockSpeaker : public SpeakerInterface {
public:
    bool setVolume(int8_t volume) {
        m_settings.volume = volume;
        return true;
    }

    bool adjustVolume(int8_t delta) {
        int8_t curVolume = m_settings.volume + delta;
        curVolume = std::min(curVolume, AVS_SET_VOLUME_MAX);
        curVolume = std::max(curVolume, AVS_SET_VOLUME_MIN);
        m_settings.volume = curVolume;
        return true;
    }

    bool setMute(bool mute) {
        m_settings.mute = mute;
        return true;
    }

    bool getSpeakerSettings(SpeakerInterface::SpeakerSettings* settings) {
        if (!settings) {
            return false;
        }

        settings->volume = m_settings.volume;
        settings->mute = m_settings.mute;

        return true;
    }

    SpeakerInterface::Type getSpeakerType() {
        return m_type;
    }

    MockSpeaker(SpeakerInterface::Type type) : m_type{type} {
        m_settings = DEFAULT_SETTINGS;
    }

private:
    SpeakerInterface::Type m_type;
    SpeakerInterface::SpeakerSettings m_settings;
};

class MockSpeakerInterface : public SpeakerInterface {
public:
    MOCK_METHOD1(setVolume, bool(int8_t));
    MOCK_METHOD1(adjustVolume, bool(int8_t));
    MOCK_METHOD1(setMute, bool(bool));
    MOCK_METHOD1(getSpeakerSettings, bool(SpeakerInterface::SpeakerSettings*));
    MOCK_METHOD0(getSpeakerType, SpeakerInterface::Type());

    void DelegateToReal() {
        ON_CALL(*this, setVolume(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::setVolume));
        ON_CALL(*this, adjustVolume(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::adjustVolume));
        ON_CALL(*this, setMute(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::setMute));
        ON_CALL(*this, getSpeakerSettings(_)).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::getSpeakerSettings));
        ON_CALL(*this, getSpeakerType()).WillByDefault(Invoke(&m_speaker, &SpeakerInterface::getSpeakerType));
    }

    MockSpeakerInterface(SpeakerInterface::Type type) : m_speaker{type} {
    }

private:
    // Implementation of Speaker object to handle calls.
    MockSpeaker m_speaker;
};

/**
 * A mock object to test that the observer is being correctly notified.
 */
class MockObserver : public SpeakerManagerObserverInterface {
public:
    MOCK_METHOD3(
        onSpeakerSettingsChanged,
        void(const Source&, const SpeakerInterface::Type&, const SpeakerInterface::SpeakerSettings&));
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
TEST_F(SpeakerManagerTest, testNullContextManager) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers{
        std::make_shared<MockSpeakerInterface>(SpeakerInterface::Type::AVS_SYNCED)};

    m_speakerManager = SpeakerManager::create(speakers, nullptr, m_mockMessageSender, m_mockExceptionSender);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with a null messageSender.
 */
TEST_F(SpeakerManagerTest, testNullMessageSender) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers{
        std::make_shared<MockSpeakerInterface>(SpeakerInterface::Type::AVS_SYNCED)};

    m_speakerManager = SpeakerManager::create(speakers, m_mockContextManager, nullptr, m_mockExceptionSender);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with a null exceptionSender.
 */
TEST_F(SpeakerManagerTest, testNullExceptionSender) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers{
        std::make_shared<MockSpeakerInterface>(SpeakerInterface::Type::AVS_SYNCED)};

    m_speakerManager = SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, nullptr);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with no speakers.
 */
TEST_F(SpeakerManagerTest, testNoSpeakers) {
    m_speakerManager = SpeakerManager::create({}, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests that the SpeakerManager initially provides the state at constructor time.
 */
TEST_F(SpeakerManagerTest, testContextManagerSetStateConstructor) {
    EXPECT_CALL(
        *m_mockContextManager,
        setState(VOLUME_STATE, generateVolumeStateJson(DEFAULT_SETTINGS), StateRefreshPolicy::NEVER, _))
        .Times(Exactly(1));
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    std::vector<std::shared_ptr<SpeakerInterface>> speakers{speaker};
    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
}

/*
 * Test setVolume with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testSetVolumeUnderBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(_)).Times(Exactly(0));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future = m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_SET_VOLUME_MIN - 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test setVolume with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testSetVolumeOverBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, setVolume(_)).Times(Exactly(0));
    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future = m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_SET_VOLUME_MAX + 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testAdjustVolumeUnderBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, adjustVolume(_)).Times(Exactly(0));
    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_ADJUST_VOLUME_MIN - 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testAdjustVolumeOverBounds) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();
    EXPECT_CALL(*speaker, adjustVolume(_)).Times(Exactly(0));
    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_ADJUST_VOLUME_MAX + 1);
    ASSERT_FALSE(future.get());
}

/*
 * Test setVolume when the speaker interfaces are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testSetVolumeOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SYNCED));
    EXPECT_CALL(*speaker2, setVolume(_)).WillRepeatedly(Return(true));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future = m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_SET_VOLUME_MAX);
    ASSERT_FALSE(future.get());
}

/*
 * Test adjustVolume when the speaker interfaces are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testAdjustVolumeOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SYNCED));
    EXPECT_CALL(*speaker2, adjustVolume(_)).WillRepeatedly(Return(true));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future =
        m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_ADJUST_VOLUME_MAX);
    ASSERT_FALSE(future.get());
}

/*
 * Test setMute when the speaker interfaces are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testSetMuteOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SYNCED));
    EXPECT_CALL(*speaker2, setMute(_)).WillRepeatedly(Return(true));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    std::future<bool> future = m_speakerManager->setMute(SpeakerInterface::Type::AVS_SYNCED, MUTE);
    ASSERT_FALSE(future.get());
}

/**
 * Test getSpeakerSettings when speakers are out of sync. The operation should fail.
 */
TEST_F(SpeakerManagerTest, testGetSpeakerSettingsSpeakersOutOfSync) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    auto speaker2 = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    EXPECT_CALL(*speaker2, getSpeakerType()).WillRepeatedly(Return(SpeakerInterface::Type::AVS_SYNCED));
    // Set speaker to be out of sync.
    EXPECT_CALL(*speaker2, getSpeakerSettings(_)).WillRepeatedly(Return(false));

    std::vector<std::shared_ptr<SpeakerInterface>> speakers = {speaker, speaker2};

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    SpeakerInterface::SpeakerSettings settings;
    std::future<bool> future = m_speakerManager->getSpeakerSettings(SpeakerInterface::Type::AVS_SYNCED, &settings);
    ASSERT_FALSE(future.get());
}

/**
 * Test getConfiguration and ensure that all directives are handled.
 */
TEST_F(SpeakerManagerTest, testGetConfiguration) {
    std::shared_ptr<SpeakerInterface> speaker =
        std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);

    m_speakerManager =
        SpeakerManager::create({speaker}, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    auto configuration = m_speakerManager->getConfiguration();
    ASSERT_EQ(configuration[SET_VOLUME], BlockingPolicy::NON_BLOCKING);
    ASSERT_EQ(configuration[ADJUST_VOLUME], BlockingPolicy::NON_BLOCKING);
    ASSERT_EQ(configuration[SET_MUTE], BlockingPolicy::NON_BLOCKING);
}

/**
 * Test that adding a null observer does not cause any errors in the SpeakerManager.
 */
TEST_F(SpeakerManagerTest, testAddNullObserver) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    m_speakerManager =
        SpeakerManager::create({speaker}, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(nullptr);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(3));

    m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_SET_VOLUME_MAX).wait();
    m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_ADJUST_VOLUME_MAX).wait();
    m_speakerManager->setMute(SpeakerInterface::Type::AVS_SYNCED, MUTE).wait();
}

/**
 * Test that removing an observer works correctly.
 */
TEST_F(SpeakerManagerTest, testRemoveSpeakerManagerObserver) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(3));

    m_speakerManager =
        SpeakerManager::create({speaker}, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    m_speakerManager->removeSpeakerManagerObserver(m_observer);

    m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_SET_VOLUME_MAX).wait();
    m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_ADJUST_VOLUME_MAX).wait();
    m_speakerManager->setMute(SpeakerInterface::Type::AVS_SYNCED, MUTE).wait();
}

/**
 * Test that removing a null observer does not cause any errors in the SpeakerManager.
 */
TEST_F(SpeakerManagerTest, testRemoveNullObserver) {
    auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SYNCED);
    speaker->DelegateToReal();

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(3));

    m_speakerManager =
        SpeakerManager::create({speaker}, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
    m_speakerManager->removeSpeakerManagerObserver(nullptr);

    m_speakerManager->setVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_SET_VOLUME_MAX).wait();
    m_speakerManager->adjustVolume(SpeakerInterface::Type::AVS_SYNCED, AVS_ADJUST_VOLUME_MAX).wait();
    m_speakerManager->setMute(SpeakerInterface::Type::AVS_SYNCED, MUTE).wait();
}

/**
 * Create different combinations of @c Type for  parameterized tests (TEST_P).
 */
INSTANTIATE_TEST_CASE_P(
    Parameterized,
    SpeakerManagerTest,
    // clang-format off
    ::testing::Values(
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_SYNCED
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::LOCAL
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_SYNCED,
            SpeakerInterface::Type::AVS_SYNCED
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::LOCAL,
            SpeakerInterface::Type::LOCAL,
        },
        std::vector<SpeakerInterface::Type>{
            SpeakerInterface::Type::AVS_SYNCED,
            SpeakerInterface::Type::LOCAL,
            SpeakerInterface::Type::AVS_SYNCED,
            SpeakerInterface::Type::LOCAL
        }));
    // clang-format off

/**
 * Parameterized test for setVolume. One event should be sent if an AVS_SYNCED typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, testSetVolume) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(1));
        speakers.push_back(speaker);
    }

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings)).Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SYNCED == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MAX);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for adjustVolume. One event should be sent if an AVS_SYNCED typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, testAdjustVolume) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, adjustVolume(AVS_ADJUST_VOLUME_MIN)).Times(Exactly(1));
        speakers.push_back(speaker);
    }

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MIN, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings)).Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SYNCED == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MIN);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for setMute. One event should be sent if an AVS_SYNCED typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, testSetMute) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        EXPECT_CALL(*speaker, setMute(MUTE)).Times(Exactly(1));
        speakers.push_back(speaker);
    }

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto type : getUniqueTypes(speakers)) {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings)).Times(Exactly(1));
        if (SpeakerInterface::Type::AVS_SYNCED == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _)).Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setMute(type, MUTE);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for getSpeakerSettings. Operation should succeed with default speaker settings.
 */
TEST_P(SpeakerManagerTest, testGetSpeakerSettings) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;

    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        // There are other calls to getSpeakerSettings(), such as when we initially provide the Context.
        EXPECT_CALL(*speaker, getSpeakerSettings(_)).Times(AtLeast(1));
        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);

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
 * event is sent. In the event there are no AVS_SYNCED speakers registered, no event will be sent.
 * In addition, only AVS_SYNCED speakers should be affected.
 */
TEST_P(SpeakerManagerTest, testSetVolumeDirective) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SYNCED) {
            timesCalled = 1;
        }

        EXPECT_CALL(*speaker, setMute(UNMUTE)).Times(Exactly(timesCalled));
        EXPECT_CALL(*speaker, setVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    // Creation expectations based on type.
    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SYNCED)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SYNCED, expectedSettings)).Times(Exactly(1));
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

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
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
 * event is sent. In the event there are no AVS_SYNCED speakers registered, no event will be sent.
 * In addition, only AVS_SYNCED speakers should be affected.
 */
TEST_P(SpeakerManagerTest, testAdjustVolumeDirective) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SYNCED) {
            timesCalled = 1;
        }

        EXPECT_CALL(*speaker, setMute(UNMUTE)).Times(Exactly(timesCalled));
        EXPECT_CALL(*speaker, adjustVolume(AVS_ADJUST_VOLUME_MAX)).Times(Exactly(timesCalled));

        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    // Creation expectations based on type.
    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SYNCED)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SYNCED, expectedSettings)).Times(Exactly(1));
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

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
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
 * event is sent. In the event there are no AVS_SYNCED speakers registered, no event will be sent.
 * In addition, only AVS_SYNCED speakers should be affected.
 */
TEST_P(SpeakerManagerTest, testSetMuteDirective) {
    std::vector<std::shared_ptr<SpeakerInterface>> speakers;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto speaker = std::make_shared<NiceMock<MockSpeakerInterface>>(typeOfSpeaker);
        speaker->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == SpeakerInterface::Type::AVS_SYNCED) {
            timesCalled = 1;
        }

        EXPECT_CALL(*speaker, setMute(MUTE)).Times(Exactly(timesCalled));

        speakers.push_back(speaker);
    }

    auto uniqueTypes = getUniqueTypes(speakers);

    // Creation expectations based on type.
    if (uniqueTypes.count(SpeakerInterface::Type::AVS_SYNCED)) {
        eventsSent = 1;

        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, SpeakerInterface::Type::AVS_SYNCED, expectedSettings)).Times(Exactly(1));
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

    m_speakerManager =
        SpeakerManager::create(speakers, m_mockContextManager, m_mockMessageSender, m_mockExceptionSender);
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

}  // namespace test
}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
