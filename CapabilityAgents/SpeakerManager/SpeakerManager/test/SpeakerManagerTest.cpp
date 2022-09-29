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

#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>
#include <vector>

#include <acsdk/SpeakerManager/private/SpeakerManagerConstants.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageInterface.h>
#include <acsdk/SpeakerManager/private/SpeakerManager.h>
#include <acsdk/SpeakerManager/test/MockSpeakerManagerConfig.h>
#include <acsdk/SpeakerManager/test/MockSpeakerManagerObserver.h>
#include <acsdk/SpeakerManager/test/MockSpeakerManagerStorage.h>
#include <acsdk/Test/GmockExtensions.h>
#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>

namespace alexaClientSDK {
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

/// @addtogroup Test_acsdkSpeakerManager
/// @{

/// Timeout when waiting for futures to be set.
static std::chrono::milliseconds TIMEOUT(1000);

/// The @c MessageId identifer.
static const auto MESSAGE_ID = "messageId";

/// A @c SetVolume/AdjustVolume payload.
static const auto VOLUME_PAYLOAD = R"(
{
  "volume":100
}
)";

/// A @c SetMute payload.
static const auto MUTE_PAYLOAD = R"(
{
  "mute": true
}
)";

/// A @c SetMute payload to unmute.
static const auto UNMUTE_PAYLOAD = R"(
{
  "mute": false
}
)";

#ifdef ENABLE_MAXVOLUME_SETTING
/// A valid value to be used as maximum volume limit.
static const int8_t VALID_MAXIMUM_VOLUME_LIMIT = AVS_SET_VOLUME_MAX - 10;

/// An invalid maximum volume limit value
static const int8_t INVALID_MAXIMUM_VOLUME_LIMIT = AVS_SET_VOLUME_MAX + 10;
#endif

/// A valid delta to adjust the volume.
static const int8_t VALID_VOLUME_ADJUSTMENT = 10;

/**
 * @brief Extend MockSpeakerManagerStorage with helpers.
 */
class MockSpeakerManagerStorageWithHelpers : public MockSpeakerManagerStorage {
public:
    /**
     * Constructs object with default values and configures methods to return success.
     */
    MockSpeakerManagerStorageWithHelpers() {
        setDefaults();
        setSuccessMode();
    }

    /**
     * @brief Sets default values for cached channels' values.
     */
    void setDefaults() {
        m_state = {{AVS_SET_VOLUME_MIN, UNMUTE}, {AVS_SET_VOLUME_MIN, UNMUTE}};
    }

    /**
     * @brief Configures mocked methods to load/store cached values.
     */
    void setSuccessMode() {
        ON_CALL(*this, loadState(_)).WillByDefault(Invoke([this](SpeakerManagerStorageState& state) {
            state = this->m_state;
            return true;
        }));
        ON_CALL(*this, saveState(_)).WillByDefault(Invoke([this](const SpeakerManagerStorageState& state) {
            this->m_state = state;
            return true;
        }));
    }

    /**
     * @brief Configures mocked methods to fail.
     */
    void setFailureMode() {
        ON_CALL(*this, loadState(_)).WillByDefault(Return(false));
        ON_CALL(*this, saveState(_)).WillByDefault(Return(false));
    }

    /// @brief Cached values for channels.
    SpeakerManagerStorageState m_state;
};

/**
 * @brief Test fixture for SpeakerManager unit tests.
 */
class SpeakerManagerTest : public ::testing::TestWithParam<std::vector<ChannelVolumeInterface::Type>> {
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
    std::set<ChannelVolumeInterface::Type> getUniqueTypes(std::vector<std::shared_ptr<ChannelVolumeInterface>>& groups);

#ifdef ENABLE_MAXVOLUME_SETTING
    /**
     * Helper function for create and sent a directive
     *
     * @param directiveName The directive name. One of SetVolume or AdjustVolume.
     * @param volume The value of the volume files within the directive.
     */
    void createAndSendVolumeDirective(const std::string directiveName, const int8_t volume);
#endif

    std::vector<std::shared_ptr<ChannelVolumeInterface>> createChannelVolumeInterfaces() {
        auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
        channelVolumeInterface->DelegateToReal();
        return {channelVolumeInterface};
    }

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

    /**
     * Set this to a nice mock. The only instance of the mock being called is the setStateProvider member, which we
     * explicitly test.
     */
    std::shared_ptr<NiceMock<MockContextManager>> m_mockContextManager;

    /// Configuration interface mock.
    std::shared_ptr<MockSpeakerManagerConfig> m_mockConfig;

    /// Storage interface mock.
    std::shared_ptr<MockSpeakerManagerStorageWithHelpers> m_mockStorage;

    /// A strict mock that allows the test to strictly monitor the messages sent.
    std::shared_ptr<StrictMock<MockMessageSender>> m_mockMessageSender;

    /// A strict mock that allows the test to strictly monitor the exceptions being sent.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionSender;

    /// A strict mock that allows the test to strictly monitor the handling of directives.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    /// A mock to allow testing of the observer callback behavior.
    std::shared_ptr<NiceMock<MockSpeakerManagerObserver>> m_observer;

    /// A pointer to an instance of the SpeakerManager that will be instantiated per test.
    std::shared_ptr<SpeakerManager> m_speakerManager;
};

/// @brief Test fixture for SpeakerManager.
void SpeakerManagerTest::SetUp() {
    m_mockConfig = std::make_shared<NiceMock<MockSpeakerManagerConfig>>();
    m_mockStorage = std::make_shared<NiceMock<MockSpeakerManagerStorageWithHelpers>>();

    m_metricRecorder = std::make_shared<NiceMock<avsCommon::utils::metrics::test::MockMetricRecorder>>();
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockMessageSender = std::make_shared<StrictMock<MockMessageSender>>();
    m_mockExceptionSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_observer = std::make_shared<NiceMock<MockSpeakerManagerObserver>>();
}

/// @}

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
std::set<ChannelVolumeInterface::Type> SpeakerManagerTest::getUniqueTypes(
    std::vector<std::shared_ptr<ChannelVolumeInterface>>& groups) {
    std::set<ChannelVolumeInterface::Type> types;
    for (auto item : groups) {
        types.insert(item->getSpeakerType());
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

static int8_t getSpeakerVolume(std::shared_ptr<ChannelVolumeInterface> channelVolumeInterface) {
    SpeakerInterface::SpeakerSettings speakerSettings;
    channelVolumeInterface->getSpeakerSettings(&speakerSettings);
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
    auto channelVolumeInterfaces = createChannelVolumeInterfaces();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaces,
        nullptr,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with a null messageSender.
 */
TEST_F(SpeakerManagerTest, test_nullMessageSender) {
    auto channelVolumeInterfaces = createChannelVolumeInterfaces();
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaces,
        m_mockContextManager,
        nullptr,
        m_mockExceptionSender,
        m_metricRecorder);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with a null exceptionSender.
 */
TEST_F(SpeakerManagerTest, test_nullExceptionSender) {
    auto channelVolumeInterfaces = createChannelVolumeInterfaces();
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaces,
        m_mockContextManager,
        m_mockMessageSender,
        nullptr,
        m_metricRecorder);

    ASSERT_EQ(m_speakerManager, nullptr);
}

/**
 * Tests creating the SpeakerManager with no channelVolumeInterfaces.
 */
TEST_F(SpeakerManagerTest, test_noChannelVolumeInterfaces) {
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    ASSERT_NE(m_speakerManager, nullptr);
}

/**
 * Tests that adding a channel volume interface does not overwrite existing default volume settings when persistent
 * storage is enabled.
 */
TEST_F(SpeakerManagerTest, test_addChannelVolumeInterfaceDoesNotOverwriteDefaults) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    channelVolumeInterface->setUnduckedVolume(AVS_SET_VOLUME_MAX);
    channelVolumeInterface->setMute(MUTE);

    SpeakerInterface::SpeakerSettings settings;
    ASSERT_TRUE(channelVolumeInterface->getSpeakerSettings(&settings));
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MAX);
    ASSERT_EQ(settings.mute, MUTE);

    m_speakerManager->addChannelVolumeInterface(channelVolumeInterface);
    auto future = m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_TRUE(future.get());
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MIN);
    ASSERT_EQ(settings.mute, UNMUTE);
}

/**
 * Tests that the SpeakerManager initially provides the state at constructor time.
 */
TEST_F(SpeakerManagerTest, test_contextManagerSetStateConstructor) {
    EXPECT_CALL(
        *m_mockContextManager,
        setState(VOLUME_STATE, generateVolumeStateJson(DEFAULT_SETTINGS), StateRefreshPolicy::NEVER, _))
        .Times(Exactly(1));
    auto groups = createChannelVolumeInterfaces();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groups,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
}

/**
 * Test setVolume with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeUnderBounds) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));

    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->setVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MIN - 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test setVolume with a value that's under the bounds with persistent storage enabled. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeUnderBoundsWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));

    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->setVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MIN - 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test setVolume with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeOverBounds) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->setVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX + 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test setVolume with a value that's over the bounds with persistent storage enabled. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_setVolumeOverBoundsWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->setVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX + 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test adjustVolume with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeUnderBounds) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MIN - 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test adjustVolume with a value that's under the bounds with persistent storage enabled. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeUnderBoundsWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MIN - 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test adjustVolume with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeOverBounds) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX + 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test adjustVolume with a value that's over the bounds with persistent storage enabled. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_adjustVolumeOverBoundsWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX + 1, properties);
    ASSERT_FALSE(future.get());
}

/**
 * Test if one speaker is out of sync, getSpeakingSettings should return the cached value correctly.
 */
TEST_F(SpeakerManagerTest, test_getCachedSettings) {
    // Prepare two speakers with the same type AVS_SPEAKER_VOLUME
    auto channelVolumeInterface1 = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    auto channelVolumeInterface2 = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface1->DelegateToReal();
    channelVolumeInterface2->DelegateToReal();
    // Get speaker settings from the first speaker of each type during initialization.
    EXPECT_CALL(*channelVolumeInterface1, getSpeakerSettings(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface1, channelVolumeInterface2},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // If a speaker changes its volume and is out of sync with the rest speakers of the same type, querying speaker
    // settings from SpeakerManager should return the cached volume correctly.
    channelVolumeInterface2->setUnduckedVolume(AVS_SET_VOLUME_MAX);
    channelVolumeInterface2->setMute(MUTE);
    SpeakerInterface::SpeakerSettings settings;
    std::future<bool> future =
        m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_TRUE(future.get());
    ASSERT_EQ(settings.volume, DEFAULT_SETTINGS.volume);
    ASSERT_EQ(settings.mute, DEFAULT_SETTINGS.mute);

    ASSERT_TRUE(channelVolumeInterface2->getSpeakerSettings(&settings));
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MAX);
    ASSERT_EQ(settings.mute, MUTE);
}

/**
 * Test adjustVolume when the adjusted volume is unchanged. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenAdjustVolumeUnchanged) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    auto groupVec = std::vector<std::shared_ptr<ChannelVolumeInterface>>{channelVolumeInterface};

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MIN, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MIN, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Test adjustVolume when the adjusted volume is unchanged with persistent storage enabled. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenAdjustVolumeUnchangedWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    auto groupVec = std::vector<std::shared_ptr<ChannelVolumeInterface>>{channelVolumeInterface};

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MIN, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MIN, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Test setVolume when the new volume is unchanged. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenSetVolumeUnchanged) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(AVS_SET_VOLUME_MIN)).Times(Exactly(1));

    auto groupVec = std::vector<std::shared_ptr<ChannelVolumeInterface>>{channelVolumeInterface};
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MIN, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Test setVolume when the new volume is unchanged with persistent storage enabled. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenSetVolumeUnchangedWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(AVS_SET_VOLUME_MIN)).Times(Exactly(2));

    auto groupVec = std::vector<std::shared_ptr<ChannelVolumeInterface>>{channelVolumeInterface};
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MIN, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Test getConfiguration and ensure that all directives are handled.
 */
TEST_F(SpeakerManagerTest, test_getConfiguration) {
    auto channelVolumeInterfaceVec = createChannelVolumeInterfaces();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaceVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    auto configuration = m_speakerManager->getConfiguration();
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);
    ASSERT_EQ(configuration[SET_VOLUME], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[ADJUST_VOLUME], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[SET_MUTE], neitherNonBlockingPolicy);
}

/**
 * Test that adding duplicated ChannelVolumeInterface instances in the SpeakerManager works correctly.
 */
TEST_F(SpeakerManagerTest, test_addDuplicatedChannelVolumeInterfaces) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    std::vector<std::shared_ptr<ChannelVolumeInterface>> channelVolumeInterfaceVec = {channelVolumeInterface,
                                                                                      channelVolumeInterface};
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaceVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX, properties);
    ASSERT_TRUE(future.get());
}

/**
 * Test that adding a null observer does not cause any errors in the SpeakerManager.
 */
TEST_F(SpeakerManagerTest, test_addNullObserver) {
    auto channelVolumeInterfaceVec = createChannelVolumeInterfaces();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaceVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
    m_speakerManager->addSpeakerManagerObserver(nullptr);
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(2));
    SpeakerManagerInterface::NotificationProperties properties;

    m_speakerManager->setVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX, properties)
        .wait();
    m_speakerManager->adjustVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX, properties)
        .wait();
    m_speakerManager->setMute(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, MUTE, properties).wait();
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MAX, MUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test that removing an observer works correctly.
 */
TEST_F(SpeakerManagerTest, test_removeSpeakerManagerObserver) {
    auto channelVolumeInterfaceVec = createChannelVolumeInterfaces();

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(2));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaceVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    m_speakerManager->removeSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    m_speakerManager->setVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX, properties)
        .wait();
    m_speakerManager->adjustVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX, properties)
        .wait();
    m_speakerManager->setMute(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, MUTE, properties).wait();
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MAX, MUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test that removing a null observer does not cause any errors in the SpeakerManager.
 */
TEST_F(SpeakerManagerTest, test_removeNullObserver) {
    auto channelVolumeInterfaceVec = createChannelVolumeInterfaces();

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(2));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        channelVolumeInterfaceVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
    m_speakerManager->removeSpeakerManagerObserver(nullptr);
    SpeakerManagerInterface::NotificationProperties properties;

    m_speakerManager->setVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MAX, properties)
        .wait();
    m_speakerManager->adjustVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_ADJUST_VOLUME_MAX, properties)
        .wait();
    m_speakerManager->setMute(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, MUTE, properties).wait();
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MAX, MUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);
}

/**
 * Test retry logic for SetVolume on speaker type AVS_SPEAKER_VOLUME. Returning false once for speaker->setVolume()
 * triggers retry and when successful returns the future of value true.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsForSetVolume) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    auto retryTimes = 0;
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).WillRepeatedly(InvokeWithoutArgs([&retryTimes] {
        return retryTimes++ > 0;
    }));

    SpeakerManagerInterface::NotificationProperties properties;
    std::future<bool> future =
        m_speakerManager->setVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, AVS_SET_VOLUME_MIN, properties);
    ASSERT_TRUE(future.get());
}

/**
 * Test retry logic for AdjustVolume on speakers of type AVS_SPEAKER_VOLUME. Return false once for the second speaker
 * during adjustVolume() to trigger a retry. The delta should not be applied again to the first speaker during retry.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsForAdjustVolume) {
    auto channelVolumeInterface1 = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    auto channelVolumeInterface2 = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface1->DelegateToReal();
    channelVolumeInterface2->DelegateToReal();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface1, channelVolumeInterface2},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    auto retryTimes = 0;
    EXPECT_CALL(*channelVolumeInterface2, setUnduckedVolume(_)).WillRepeatedly(InvokeWithoutArgs([&retryTimes] {
        return retryTimes++ > 0;
    }));

    // Expect volumeChanged event.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    std::future<bool> future = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        VALID_VOLUME_ADJUSTMENT,
        SpeakerManagerInterface::NotificationProperties());
    ASSERT_TRUE(future.get());

    SpeakerInterface::SpeakerSettings settings1;
    ASSERT_TRUE(channelVolumeInterface1->getSpeakerSettings(&settings1));
    ASSERT_EQ(settings1.volume, DEFAULT_SETTINGS.volume + VALID_VOLUME_ADJUSTMENT);

    SpeakerInterface::SpeakerSettings speakerSettings;
    std::future<bool> settingsFuture =
        m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &speakerSettings);
    ASSERT_TRUE(settingsFuture.get());
    ASSERT_EQ(speakerSettings.volume, DEFAULT_SETTINGS.volume + VALID_VOLUME_ADJUSTMENT);
}

/**
 * Test retry logic for SetMute on speaker type AVS_SPEAKER_VOLUME. Returning false once for speaker->setMute()
 * triggers retry and when successful returns the future of value true.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsForSetMute) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    auto retryTimes = 0;
    EXPECT_CALL(*channelVolumeInterface, setMute(_)).WillRepeatedly(InvokeWithoutArgs([&retryTimes] {
        return retryTimes++ > 0;
    }));

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    SpeakerManagerInterface::NotificationProperties properties;

    std::future<bool> future =
        m_speakerManager->setMute(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, MUTE, properties);
    ASSERT_TRUE(future.get());
}

/**
 * Test retryAndApplySettings() failure for setVolume, adjustVolume and setMute on speaker type AVS_SPEAKER_VOLUME.
 * Repeatedly returning false for adjustVolume() and setMute() to trigger retries. After retrying maximum times,
 * returning the future of false.
 */
TEST_F(SpeakerManagerTest, test_retryAndApplySettingsFails) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*channelVolumeInterface, setMute(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(0);

    std::future<bool> setVolumeResult = m_speakerManager->setVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        AVS_SET_VOLUME_MIN,
        SpeakerManagerInterface::NotificationProperties());
    ASSERT_FALSE(setVolumeResult.get());

    std::future<bool> adjustVolumeResult = m_speakerManager->adjustVolume(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        VALID_VOLUME_ADJUSTMENT,
        SpeakerManagerInterface::NotificationProperties());
    ASSERT_FALSE(adjustVolumeResult.get());

    std::future<bool> setMuteResult = m_speakerManager->setMute(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, MUTE, SpeakerManagerInterface::NotificationProperties());
    ASSERT_FALSE(setMuteResult.get());

    SpeakerInterface::SpeakerSettings speakerSettings;
    std::future<bool> settingsFuture =
        m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &speakerSettings);
    ASSERT_TRUE(settingsFuture.get());
    ASSERT_EQ(speakerSettings.volume, DEFAULT_SETTINGS.volume);
    ASSERT_EQ(speakerSettings.mute, DEFAULT_SETTINGS.mute);
}

#ifdef ENABLE_MAXVOLUME_SETTING
/**
 * Test that setting a maximum volume limit succeeds and a local call to setVolume or adjustVolume will
 * completely fail.
 */
TEST_F(SpeakerManagerTest, test_setMaximumVolumeLimit) {
    auto avsChannelVolumeInterface =
        std::make_shared<NiceMock<MockChannelVolumeInterface>>(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);
    avsChannelVolumeInterface->DelegateToReal();
    auto alertsChannelVolumeInterface =
        std::make_shared<NiceMock<MockChannelVolumeInterface>>(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);
    alertsChannelVolumeInterface->DelegateToReal();

    avsChannelVolumeInterface->setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1);
    alertsChannelVolumeInterface->setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1);

    // Expect volumeChanged event.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    EXPECT_CALL(*avsChannelVolumeInterface, setUnduckedVolume(_)).Times(AtLeast(1));
    EXPECT_CALL(*alertsChannelVolumeInterface, setUnduckedVolume(_)).Times(AtLeast(1));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(0);
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {avsChannelVolumeInterface, alertsChannelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
    SpeakerManagerInterface::NotificationProperties properties;

    EXPECT_TRUE(m_speakerManager->setMaximumVolumeLimit(VALID_MAXIMUM_VOLUME_LIMIT).get());

    // Local change either with setVolume will set to limit but with adjustVolume will fail
    EXPECT_TRUE(
        m_speakerManager
            ->setVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, VALID_MAXIMUM_VOLUME_LIMIT + 1, properties)
            .get());
    EXPECT_FALSE(
        m_speakerManager
            ->adjustVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, VALID_MAXIMUM_VOLUME_LIMIT + 1, properties)
            .get());

    // The volume went to upper limit.
    EXPECT_EQ(getSpeakerVolume(avsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);
    EXPECT_EQ(getSpeakerVolume(alertsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);

    // Increase the volume by 2, so end result will exceed the limit.
    EXPECT_TRUE(m_speakerManager->adjustVolume(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, 2, properties).get());

    // Following the 2nd adjustVolume, the volume will change to the limit.
    EXPECT_EQ(getSpeakerVolume(alertsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);
}

/**
 * Test that if a new limit was set while the volume was higher than the new limit, operation will succeed and the
 * volume will be decreased.
 */

TEST_F(SpeakerManagerTest, testSetMaximumVolumeLimitWhileVolumeIsHigher) {
    auto avsChannelVolumeInterface =
        std::make_shared<NiceMock<MockChannelVolumeInterface>>(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);
    auto alertsChannelVolumeInterface =
        std::make_shared<NiceMock<MockChannelVolumeInterface>>(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);

    avsChannelVolumeInterface->DelegateToReal();
    alertsChannelVolumeInterface->DelegateToReal();

    EXPECT_TRUE(avsChannelVolumeInterface->setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT + 1));
    EXPECT_TRUE(alertsChannelVolumeInterface->setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT + 1));

    EXPECT_CALL(*avsChannelVolumeInterface, setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT)).Times(1);
    EXPECT_CALL(*alertsChannelVolumeInterface, setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT)).Times(1);

    // Expect volumeChanged event.
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));

    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {avsChannelVolumeInterface, alertsChannelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    EXPECT_TRUE(m_speakerManager->setMaximumVolumeLimit(VALID_MAXIMUM_VOLUME_LIMIT).get());

    EXPECT_EQ(getSpeakerVolume(avsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);
    EXPECT_EQ(getSpeakerVolume(alertsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);
}

/**
 * Test that SetVolume directive with volume > limit should set the volume to the limit
 */

TEST_F(SpeakerManagerTest, testAVSSetVolumeHigherThanLimit) {
    avsCommon::utils::logger::getConsoleLogger()->setLevel(avsCommon::utils::logger::Level::DEBUG9);
    auto avsChannelVolumeInterface =
        std::make_shared<NiceMock<MockChannelVolumeInterface>>(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);
    auto alertsChannelVolumeInterface =
        std::make_shared<NiceMock<MockChannelVolumeInterface>>(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME);

    avsChannelVolumeInterface->DelegateToReal();
    alertsChannelVolumeInterface->DelegateToReal();

    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));

    EXPECT_TRUE(avsChannelVolumeInterface->setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1));
    EXPECT_TRUE(alertsChannelVolumeInterface->setUnduckedVolume(VALID_MAXIMUM_VOLUME_LIMIT - 1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {avsChannelVolumeInterface, alertsChannelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    EXPECT_TRUE(m_speakerManager->setMaximumVolumeLimit(VALID_MAXIMUM_VOLUME_LIMIT).get());

    createAndSendVolumeDirective(SET_VOLUME.name, VALID_MAXIMUM_VOLUME_LIMIT + 1);

    ASSERT_EQ(getSpeakerVolume(avsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);
    ASSERT_EQ(getSpeakerVolume(alertsChannelVolumeInterface), VALID_MAXIMUM_VOLUME_LIMIT);
}

/**
 * Test that a call to @c setMaximumVolumeLimit with invalid value fails.
 */
TEST_F(SpeakerManagerTest, testSetMaximumVolumeLimitWithInvalidValue) {
    auto avsChannelVolumeInterface = createChannelVolumeInterfaces();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        avsChannelVolumeInterface,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

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
        std::vector<ChannelVolumeInterface::Type>{
            ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME
        },
        std::vector<ChannelVolumeInterface::Type>{
            ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME
        },
        std::vector<ChannelVolumeInterface::Type>{
            ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
            ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME
        },
        std::vector<ChannelVolumeInterface::Type>{
            ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
            ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
        },
        std::vector<ChannelVolumeInterface::Type>{
            ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
            ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
            ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
            ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME
        }));
// clang-format on

/**
 * Parameterized test for setVolume. One event should be sent if an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_setVolume) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        EXPECT_CALL(*group, setUnduckedVolume(_)).Times(Exactly(0));
        EXPECT_CALL(*group, setUnduckedVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(1));
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(SpeakerManagerObserverInterface::Source::DIRECTIVE);

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MAX, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for setVolume with persistent storage enabled. One event should be sent if an AVS_SPEAKER_VOLUME
 * typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_setVolumeWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        EXPECT_CALL(*group, setUnduckedVolume(_)).Times(Exactly(1));
        EXPECT_CALL(*group, setUnduckedVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(1));
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(SpeakerManagerObserverInterface::Source::DIRECTIVE);

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setVolume(type, AVS_SET_VOLUME_MAX, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for onExternalSpeakerSettingsUpdate. One event should be sent if an AVS_SPEAKER_VOLUME typed
 * speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_onExternalSpeakerSettingsUpdate) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        m_speakerManager->onExternalSpeakerSettingsUpdate(type, expectedSettings, properties);
        m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    }
}

/**
 * Parameterized test for onExternalSpeakerSettingsUpdate with persistent storage enabled. One event should be sent if
 * an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_onExternalSpeakerSettingsUpdateWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        m_speakerManager->onExternalSpeakerSettingsUpdate(type, expectedSettings, properties);
        m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    }
}

/**
 * Test onExternalSpeakerSettingsUpdate when the new volume is unchanged. Should not send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenOnExternalSpeakerSettingsUpdateUnchanged) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    // expect call during initialization
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(AVS_SET_VOLUME_MIN)).Times(Exactly(0));

    auto groupVec = std::vector<std::shared_ptr<ChannelVolumeInterface>>{channelVolumeInterface};
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        m_speakerManager->onExternalSpeakerSettingsUpdate(type, expectedSettings, properties);
        m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    }
}

/**
 * Test onExternalSpeakerSettingsUpdate when the new volume is unchanged with persistent storage enabled. Should not
 * send an event.
 */
TEST_F(SpeakerManagerTest, test_eventNotSentWhenOnExternalSpeakerSettingsUpdateUnchangedWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();
    // expect call during initialization
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(AVS_SET_VOLUME_MIN)).Times(Exactly(1));

    auto groupVec = std::vector<std::shared_ptr<ChannelVolumeInterface>>{channelVolumeInterface};
    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MIN, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::LOCAL_API, type, expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(0));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        m_speakerManager->onExternalSpeakerSettingsUpdate(type, expectedSettings, properties);
        m_wakeSetCompletedFuture.wait_for(TIMEOUT);
    }
}

/**
 * Test onExternalSpeakerSettingsUpdate with a value that's under the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_onExternalSpeakerSettingsUpdateUnderBounds) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect calls with volume set to minimum
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MIN - 1, MUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    SpeakerInterface::SpeakerSettings settings;
    // Query SpeakerMananger for speaker settings
    auto future = m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_TRUE(future.get());
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MIN);
    ASSERT_EQ(settings.mute, MUTE);
}

/**
 * Test onExternalSpeakerSettingsUpdate with a value that's under the bounds with persistent storage enabled.
 * The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_onExternalSpeakerSettingsUpdateUnderBoundsWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect calls with volume set to minimum
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MIN - 1, MUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    SpeakerInterface::SpeakerSettings settings;
    // Query SpeakerMananger for speaker settings
    auto future = m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_TRUE(future.get());
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MIN);
    ASSERT_EQ(settings.mute, MUTE);
}

/**
 * Test onExternalSpeakerSettingsUpdate with a value that's over the bounds. The operation should fail.
 */
TEST_F(SpeakerManagerTest, test_onExternalSpeakerSettingsUpdateOverBounds) {
    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(0));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MAX + 1, UNMUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    SpeakerInterface::SpeakerSettings settings;
    // Query SpeakerMananger for speaker settings
    auto future = m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_TRUE(future.get());
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MAX);
    ASSERT_EQ(settings.mute, UNMUTE);
}

/**
 * Test onExternalSpeakerSettingsUpdate with a value that's over the bounds with persistent storage enabled. The
 * operation should fail.
 */
TEST_F(SpeakerManagerTest, test_onExternalSpeakerSettingsUpdateOverBoundsWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    auto channelVolumeInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    channelVolumeInterface->DelegateToReal();

    // Expect call on initialization.
    EXPECT_CALL(*channelVolumeInterface, setUnduckedVolume(_)).Times(Exactly(1));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        {channelVolumeInterface},
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // Expect no more calls.
    EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));
    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(1));
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties;
    m_speakerManager->onExternalSpeakerSettingsUpdate(
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, {AVS_SET_VOLUME_MAX + 1, UNMUTE}, properties);
    m_wakeSetCompletedFuture.wait_for(TIMEOUT);

    SpeakerInterface::SpeakerSettings settings;
    // Query SpeakerMananger for speaker settings
    auto future = m_speakerManager->getSpeakerSettings(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, &settings);
    ASSERT_TRUE(future.get());
    ASSERT_EQ(settings.volume, AVS_SET_VOLUME_MAX);
    ASSERT_EQ(settings.mute, UNMUTE);
}

/**
 * Parameterized test for adjustVolume. One event should be sent if an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_adjustVolume) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MAX, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(SpeakerManagerObserverInterface::Source::DIRECTIVE);

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(0));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MAX, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for adjustVolume with persistent storage enabled. One event should be sent if an
 * AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_adjustVolumeWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    // The test adjusts the volume by AVS_ADJUST_VOLUME_MAX, which results in the lowest volume possible.
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(SpeakerManagerObserverInterface::Source::DIRECTIVE);

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(*m_mockStorage, saveState(_)).Times(Exactly(1));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->adjustVolume(type, AVS_ADJUST_VOLUME_MAX, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for setMute. One event should be sent if an AVS_SPEAKER_VOLUME typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_setMute) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        EXPECT_CALL(*group, setMute(_)).Times(Exactly(0));
        EXPECT_CALL(*group, setMute(MUTE)).Times(Exactly(1));
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(SpeakerManagerObserverInterface::Source::DIRECTIVE);

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setMute(type, MUTE, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for setMute with persistent storage enabled. One event should be sent if an AVS_SPEAKER_VOLUME
 * typed speaker is modified.
 */
TEST_P(SpeakerManagerTest, test_setMuteWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        EXPECT_CALL(*group, setMute(_)).Times(Exactly(1));
        EXPECT_CALL(*group, setMute(MUTE)).Times(Exactly(1));
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};
    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(SpeakerManagerObserverInterface::Source::DIRECTIVE);

    for (auto type : getUniqueTypes(groupVec)) {
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(SpeakerManagerObserverInterface::Source::DIRECTIVE, type, expectedSettings))
            .Times(Exactly(1));
        if (ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
            EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(1));
            EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _))
                .Times(AnyNumber());
            EXPECT_CALL(
                *m_mockContextManager,
                setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
                .Times(Exactly(1));
        }

        std::future<bool> future = m_speakerManager->setMute(type, MUTE, properties);
        ASSERT_TRUE(future.get());
    }
}

/**
 * Parameterized test for getSpeakerSettings. Operation should succeed with default speaker settings.
 */
TEST_P(SpeakerManagerTest, test_getSpeakerSettings) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    std::set<ChannelVolumeInterface::Type> uniqueTypes;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();

        // There should be one call to getSpeakerSettings for the first speaker of each type.
        if (uniqueTypes.find(typeOfSpeaker) == uniqueTypes.end()) {
            EXPECT_CALL(*group, getSpeakerSettings(_)).Times(AtLeast(1));
            uniqueTypes.insert(typeOfSpeaker);
        }

        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto speaker : groupVec) {
        // SpeakerManager attempts to cache speaker settings initially. No getSpeakerSettings() call should be made to
        // each speaker.
        auto mockSpeaker = std::dynamic_pointer_cast<NiceMock<MockChannelVolumeInterface>>(speaker);
        ASSERT_TRUE(mockSpeaker);
        EXPECT_CALL(*mockSpeaker, getSpeakerSettings(_)).Times(0);
    }

    for (auto type : uniqueTypes) {
        SpeakerInterface::SpeakerSettings settings;
        // Query SpeakerMananger for speaker settings, value should be cached and not queried from each speaker.
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
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        SpeakerInterface::SpeakerSettings temp;
        group->getSpeakerSettings(&temp);
        if (temp.mute) {
            EXPECT_CALL(*group, setMute(_)).Times(Exactly(1));
            EXPECT_CALL(*group, setMute(UNMUTE)).Times(Exactly(timesCalled));
        }
        EXPECT_CALL(*group, setUnduckedVolume(_)).Times(Exactly(0));
        EXPECT_CALL(*group, setUnduckedVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        groupVec.push_back(group);
    }

    auto uniqueTypes = getUniqueTypes(groupVec);

    // Creation expectations based on type.
    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
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
 * Tests SetVolume Directive with persistent storage enabled. Expect that the volume is unmuted and set, as well at most
 * one event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_setVolumeDirectiveWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        SpeakerInterface::SpeakerSettings temp;
        group->getSpeakerSettings(&temp);
        if (temp.mute) {
            EXPECT_CALL(*group, setMute(_)).Times(Exactly(1));
            EXPECT_CALL(*group, setMute(UNMUTE)).Times(Exactly(timesCalled));
        }
        EXPECT_CALL(*group, setUnduckedVolume(_)).Times(Exactly(1));
        EXPECT_CALL(*group, setUnduckedVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        groupVec.push_back(group);
    }

    auto uniqueTypes = getUniqueTypes(groupVec);

    // Creation expectations based on type.
    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
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
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        SpeakerInterface::SpeakerSettings temp;
        group->getSpeakerSettings(&temp);
        if (temp.mute) {
            EXPECT_CALL(*group, setMute(_)).Times(Exactly(1));
            EXPECT_CALL(*group, setMute(UNMUTE)).Times(Exactly(timesCalled));
        }
        EXPECT_CALL(*group, setUnduckedVolume(_)).Times(Exactly(0));
        EXPECT_CALL(*group, setUnduckedVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        groupVec.push_back(group);
    }

    auto uniqueTypes = getUniqueTypes(groupVec);

    // Creation expectations based on type.
    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
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
 * Tests AdjustVolume Directive with persistent storage enabled. Expect that the volume is unmuted and adjusted, as well
 * at most one event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_adjustVolumeDirectiveWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{AVS_SET_VOLUME_MAX, UNMUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        SpeakerInterface::SpeakerSettings temp;
        group->getSpeakerSettings(&temp);
        if (temp.mute) {
            EXPECT_CALL(*group, setMute(_)).Times(Exactly(1));
            EXPECT_CALL(*group, setMute(UNMUTE)).Times(Exactly(timesCalled));
        }
        EXPECT_CALL(*group, setUnduckedVolume(_)).Times(Exactly(1));
        EXPECT_CALL(*group, setUnduckedVolume(AVS_SET_VOLUME_MAX)).Times(Exactly(timesCalled));

        groupVec.push_back(group);
    }

    auto uniqueTypes = getUniqueTypes(groupVec);

    // Creation expectations based on type.
    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }
    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
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
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        EXPECT_CALL(*group, setMute(_)).Times(Exactly(0));
        EXPECT_CALL(*group, setMute(MUTE)).Times(Exactly(timesCalled));

        groupVec.push_back(group);
    }

    auto uniqueTypes = getUniqueTypes(groupVec);

    // Creation expectations based on type.
    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
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
 * Tests SetMute Directive with persistent storage enabled. Expect that the volume is muted, as well at most one
 * event is sent. In the event there are no AVS_SPEAKER_VOLUME speakers registered, no event will be sent.
 * In addition, only AVS_SPEAKER_VOLUME speakers should be affected.
 */
TEST_P(SpeakerManagerTest, test_setMuteDirectiveWithPersistentStorage) {
    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings expectedSettings{DEFAULT_SETTINGS.volume, MUTE};

    // Create Speaker objects.
    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        int timesCalled = 0;
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            timesCalled = 1;
        }

        EXPECT_CALL(*group, setMute(_)).Times(Exactly(1));
        EXPECT_CALL(*group, setMute(MUTE)).Times(Exactly(timesCalled));

        groupVec.push_back(group);
    }

    auto uniqueTypes = getUniqueTypes(groupVec);

    // Creation expectations based on type.
    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        eventsSent = 1;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                expectedSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(expectedSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
    } else {
        EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(Exactly(0));
    }

    EXPECT_CALL(*m_mockMessageSender, sendMessage(_)).Times(Exactly(eventsSent));
    EXPECT_CALL(*(m_mockDirectiveHandlerResult.get()), setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &SpeakerManagerTest::wakeOnSetCompleted));

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);
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
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();
        groupVec.push_back(group);
    }

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    for (auto& group : groupVec) {
        auto mockGroup = std::dynamic_pointer_cast<NiceMock<MockChannelVolumeInterface>>(group);
        EXPECT_CALL(*mockGroup, setUnduckedVolume(AVS_SET_VOLUME_MIN)).Times(1);
        EXPECT_CALL(*mockGroup, setMute(MUTE)).Times(1);
        auto typeOfSpeaker = mockGroup->getSpeakerType();
        if (typeOfSpeaker == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
            EXPECT_CALL(*mockGroup, setMute(UNMUTE)).Times(1);
            EXPECT_CALL(*mockGroup, setUnduckedVolume(MIN_UNMUTE_VOLUME)).Times(1);
        }
    }

    m_speakerManager->addSpeakerManagerObserver(m_observer);
    SpeakerManagerInterface::NotificationProperties properties(
        SpeakerManagerObserverInterface::Source::LOCAL_API, false, false);

    for (auto type : getUniqueTypes(groupVec)) {
        m_speakerManager->setVolume(type, AVS_SET_VOLUME_MIN, properties);
    }

    for (auto type : getUniqueTypes(groupVec)) {
        std::future<bool> future = m_speakerManager->setMute(type, MUTE, properties);
    }

    // Check to see if AVS_SPEAKER_VOLUME speakers exist and set EXPECT_CALL accordingly
    auto uniqueTypes = getUniqueTypes(groupVec);
    int eventsSent = 0;
    SpeakerInterface::SpeakerSettings unMuteSettings{MIN_UNMUTE_VOLUME, UNMUTE};

    if (uniqueTypes.count(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME)) {
        // 2 events : {MIN_UNMUTE_VOLUME, MUTE} followed by {MIN_UNMUTE_VOLUME, UNMUTE}
        eventsSent = 2;

        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                SpeakerInterface::SpeakerSettings{MIN_UNMUTE_VOLUME, MUTE}))
            .Times(Exactly(1));
        EXPECT_CALL(
            *m_observer,
            onSpeakerSettingsChanged(
                SpeakerManagerObserverInterface::Source::DIRECTIVE,
                ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
                unMuteSettings))
            .Times(Exactly(1));
        EXPECT_CALL(*m_mockContextManager, setState(VOLUME_STATE, _, StateRefreshPolicy::NEVER, _)).Times(AnyNumber());
        EXPECT_CALL(
            *m_mockContextManager,
            setState(VOLUME_STATE, generateVolumeStateJson(unMuteSettings), StateRefreshPolicy::NEVER, _))
            .Times(Exactly(1));
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

/**
 * Parameterized test for getSpeakerSettings. Operation should succeed with default speaker settings.
 */
TEST_P(SpeakerManagerTest, test_getSpeakerConfigDefaults) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    std::set<ChannelVolumeInterface::Type> uniqueTypes;

    // Enable Persistent Storage Setting.
    EXPECT_CALL(*m_mockConfig, getPersistentStorage(_)).Times(1).WillOnce(Invoke([](bool& persistentStorage) {
        persistentStorage = true;
        return true;
    }));

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();

        // There should be one call to getSpeakerSettings for the first speaker of each type.
        if (uniqueTypes.find(typeOfSpeaker) == uniqueTypes.end()) {
            EXPECT_CALL(*group, getSpeakerSettings(_)).Times(AtLeast(1));
            uniqueTypes.insert(typeOfSpeaker);
        }

        groupVec.push_back(group);
    }

    m_mockStorage->setFailureMode();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto speaker : groupVec) {
        // SpeakerManager attempts to cache speaker settings initially. No getSpeakerSettings() call should be made to
        // each speaker.
        auto mockSpeaker = std::dynamic_pointer_cast<NiceMock<MockChannelVolumeInterface>>(speaker);
        ASSERT_TRUE(mockSpeaker);
        EXPECT_CALL(*mockSpeaker, getSpeakerSettings(_)).Times(0);
    }

    for (auto type : uniqueTypes) {
        SpeakerInterface::SpeakerSettings settings;
        // Query SpeakerMananger for speaker settings, value should be cached and not queried from each speaker.
        std::future<bool> future = m_speakerManager->getSpeakerSettings(type, &settings);
        ASSERT_TRUE(future.get());

        switch (type) {
            case avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME:
                EXPECT_EQ(settings.volume, alexaClientSDK::avsCommon::avs::speakerConstants::DEFAULT_SPEAKER_VOLUME);
                break;
            case avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME:
                EXPECT_EQ(settings.volume, alexaClientSDK::avsCommon::avs::speakerConstants::DEFAULT_ALERTS_VOLUME);
                break;
        }
        ASSERT_EQ(settings.mute, false);
    }
}

/**
 * Parameterized test for getSpeakerSettings. Operation should succeed with speaker settings from storage.
 */
TEST_P(SpeakerManagerTest, test_getSpeakerConfigFromStorage) {
    std::vector<std::shared_ptr<ChannelVolumeInterface>> groupVec;
    std::set<ChannelVolumeInterface::Type> uniqueTypes;

    for (auto& typeOfSpeaker : GetParam()) {
        auto group = std::make_shared<NiceMock<MockChannelVolumeInterface>>(typeOfSpeaker);
        group->DelegateToReal();

        // There should be one call to getSpeakerSettings for the first speaker of each type.
        if (uniqueTypes.find(typeOfSpeaker) == uniqueTypes.end()) {
            EXPECT_CALL(*group, getSpeakerSettings(_)).Times(AtLeast(1));
            uniqueTypes.insert(typeOfSpeaker);
        }

        groupVec.push_back(group);
    }

    m_mockStorage->setDefaults();

    m_speakerManager = SpeakerManager::create(
        m_mockConfig,
        m_mockStorage,
        groupVec,
        m_mockContextManager,
        m_mockMessageSender,
        m_mockExceptionSender,
        m_metricRecorder);

    EXPECT_CALL(*m_observer, onSpeakerSettingsChanged(_, _, _)).Times(Exactly(0));
    m_speakerManager->addSpeakerManagerObserver(m_observer);

    for (auto speaker : groupVec) {
        // SpeakerManager attempts to cache speaker settings initially. No getSpeakerSettings() call should be made to
        // each speaker.
        auto mockSpeaker = std::dynamic_pointer_cast<NiceMock<MockChannelVolumeInterface>>(speaker);
        ASSERT_TRUE(mockSpeaker);
        EXPECT_CALL(*mockSpeaker, getSpeakerSettings(_)).Times(0);
    }

    for (auto type : uniqueTypes) {
        SpeakerInterface::SpeakerSettings settings;
        // Query SpeakerMananger for speaker settings, value should be cached and not queried from each speaker.
        std::future<bool> future = m_speakerManager->getSpeakerSettings(type, &settings);
        ASSERT_TRUE(future.get());

        EXPECT_EQ(settings.volume, AVS_SET_VOLUME_MIN);
        EXPECT_EQ(settings.mute, false);
    }
}

}  // namespace test
}  // namespace speakerManager
}  // namespace alexaClientSDK
