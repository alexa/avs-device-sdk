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

#include <mutex>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "acsdkAlerts/AlertsCapabilityAgent.h"

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/AVS/AVSMessageHeader.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/SDKInterfaces/Audio/MockAlertsAudioFactory.h>
#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerManager.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/MockSetting.h>
#include <Settings/Types/AlarmVolumeRampTypes.h>

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

using namespace acsdkAlertsInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::audio::test;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::memory;
using namespace certifiedSender;
using namespace registrationManager;
using namespace renderer;
using namespace storage;
using namespace settings;
using namespace settings::test;
using namespace testing;

/// Maximum time to wait for operation result.
constexpr int MAX_WAIT_TIME_MS = 200;

/// Alerts.SetVolume Directive name.
constexpr char SET_VOLUME_DIRECTIVE_NAME[] = "SetVolume";

/// Alerts.SetAlarmVolumeRamp Directive name.
static constexpr char SET_ALARM_VOLUME_RAMP_DIRECTIVE_NAME[] = "SetAlarmVolumeRamp";

/// Alerts namespace.
static constexpr char ALERTS_NAMESPACE[] = "Alerts";

/// Alerts.SetVolume Namespace name.
constexpr char SET_VOLUME_NAMESPACE_NAME[] = "Alerts";

/// Crafted message ID.
constexpr char MESSAGE_ID[] = "1";

/// General test value for alerts volume.
constexpr int TEST_VOLUME_VALUE = 33;

/// Higher test volume value.
constexpr int HIGHER_VOLUME_VALUE = 100;

/// Lower test volume value.
constexpr int LOWER_VOLUME_VALUE = 50;

/// The timeout used throughout the tests.
static const auto TEST_TIMEOUT = std::chrono::seconds(5);

// clang-format off

/// General test directive payload.
static const std::string VOLUME_PAYLOAD =
        "{"
        R"("volume":)" +
        std::to_string(TEST_VOLUME_VALUE) +
        ""
        "}";

/// Test directive payload with volume too high.
static const std::string VOLUME_PAYLOAD_ABOVE_MAX =
        "{"
        R"("volume":)" +
        std::to_string(AVS_SET_VOLUME_MAX + 1) +
        ""
        "}";

/// Test directive payload with volume too low.
static const std::string VOLUME_PAYLOAD_BELOW_MIN =
        "{"
        R"("volume":)" +
        std::to_string(AVS_SET_VOLUME_MIN - 1) +
        ""
        "}";

/// Test directive payload for alarm volume ramp true.
static const std::string ALARM_VOLUME_RAMP_PAYLOAD_ENABLED = R"({"alarmVolumeRamp":"ASCENDING"})";

/// Test directive payload for alarm volume ramp with an invalid setting name.
static const std::string ALARM_VOLUME_RAMP_PAYLOAD_INVALID = R"({"ascendingAlarm":"ASCENDING"})";

/// Expected header for alarm volume ramp report.
static const std::string ALARM_VOLUME_RAMP_JSON_NAME = R"("name":"AlarmVolumeRampReport")";

/// The json string expected for TONE value.
static const std::string ALARM_VOLUME_RAMP_JSON_VALUE = R"("ASCENDING")";

// clang-format on

/**
 * Test @c AlertStorageInterface implementation to provide a valid instance for the initialization of other components.
 */
class StubAlertStorage : public AlertStorageInterface {
public:
    bool createDatabase() override {
        return true;
    }

    bool open() override {
        return true;
    }

    void close() override {
    }

    bool store(std::shared_ptr<Alert>) override {
        return true;
    }

    bool load(std::vector<std::shared_ptr<Alert>>*, std::shared_ptr<settings::DeviceSettingsManager>) override {
        return true;
    }

    bool modify(std::shared_ptr<Alert>) override {
        return true;
    }

    bool erase(std::shared_ptr<Alert>) override {
        return true;
    }

    bool clearDatabase() override {
        return true;
    }

    bool bulkErase(const std::list<std::shared_ptr<Alert>>&) override {
        return true;
    }
};

/**
 * Mock of @c AlertStorageInterface.
 */
class MockAlertStorage : public AlertStorageInterface {
public:
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD1(store, bool(std::shared_ptr<Alert>));
    MOCK_METHOD2(load, bool(std::vector<std::shared_ptr<Alert>>*, std::shared_ptr<settings::DeviceSettingsManager>));
    MOCK_METHOD1(modify, bool(std::shared_ptr<Alert>));
    MOCK_METHOD1(erase, bool(std::shared_ptr<Alert>));
    MOCK_METHOD1(bulkErase, bool(const std::list<std::shared_ptr<Alert>>&));
    MOCK_METHOD0(clearDatabase, bool());
};

class StubRenderer : public RendererInterface {
    void start(
        std::shared_ptr<acsdkAlerts::renderer::RendererObserverInterface> observer,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory,
        bool alarmVolumeRampEnabled,
        const std::vector<std::string>& urls,
        int loopCount,
        std::chrono::milliseconds loopPause,
        bool startWithPause) override {
    }
};

/**
 * Mock of @c RendererInterface.
 */
class MockRenderer : public RendererInterface {
public:
    MOCK_METHOD7(
        start,
        void(
            std::shared_ptr<acsdkAlerts::renderer::RendererObserverInterface> observer,
            std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>,
            bool,
            const std::vector<std::string>&,
            int,
            std::chrono::milliseconds,
            bool));
    MOCK_METHOD0(stop, void());
};

/**
 * Test @c MessageStorageInterface implementation to provide a valid instance for the initialization of other
 * components.
 */
class StubMessageStorage : public MessageStorageInterface {
public:
    bool createDatabase() override {
        return true;
    }

    bool open() override {
        return true;
    }

    void close() override {
    }

    bool store(const std::string& message, int* id) override {
        return true;
    }

    bool store(const std::string& message, const std::string& uriPathExtension, int* id) override {
        return true;
    }

    bool load(std::queue<StoredMessage>* messageContainer) override {
        return true;
    }

    bool erase(int messageId) override {
        return true;
    }

    bool clearDatabase() override {
        return true;
    }
};

/**
 * Test @c MessageSenderInterface implementation that allows tracking of messages sent.
 */
class TestMessageSender : public MessageSenderInterface {
public:
    void sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_nextMessagePromise) {
            m_nextMessagePromise->set_value(request);
            m_nextMessagePromise.reset();
        }
        lock.unlock();

        request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
    }

    /**
     * Wait for next message to be sent using this object. The message sent is then returned to the caller.
     * @return The last message sent using this object.
     */
    std::future<std::shared_ptr<avsCommon::avs::MessageRequest>> getNextMessage() {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_nextMessagePromise = std::make_shared<std::promise<std::shared_ptr<avsCommon::avs::MessageRequest>>>();
        return m_nextMessagePromise->get_future();
    }

private:
    /// Promise fulfilled when @c sendMessage is called.
    std::shared_ptr<std::promise<std::shared_ptr<avsCommon::avs::MessageRequest>>> m_nextMessagePromise;

    /// The mutex synchronizing access to @c m_nextMessagePromise
    std::mutex m_mutex;
};

class AlertsCapabilityAgentTest : public ::testing::Test {
protected:
    void testStartAlertWithContentVolume(
        int8_t speakerVolume,
        int8_t alertsVolume,
        const std::string& otherChannel,
        bool shouldResultInSetVolume);

    void SetUp() override;
    void TearDown() override;

    std::shared_ptr<AlertsCapabilityAgent> m_alertsCA;

    std::shared_ptr<CertifiedSender> m_certifiedSender;
    std::shared_ptr<TestMessageSender> m_mockMessageSender;
    std::shared_ptr<StubMessageStorage> m_messageStorage;
    std::shared_ptr<MockAVSConnectionManager> m_mockAVSConnectionManager;
    std::shared_ptr<MockFocusManager> m_mockFocusManager;
    std::shared_ptr<MockSpeakerManager> m_speakerManager;
    std::shared_ptr<MockExceptionEncounteredSender> m_exceptionSender;
    std::shared_ptr<MockContextManager> m_contextManager;
    std::shared_ptr<MockAlertStorage> m_alertStorage;
    std::shared_ptr<MockAlertsAudioFactory> m_alertsAudioFactory;
    std::shared_ptr<MockRenderer> m_renderer;
    std::shared_ptr<MockSetting<AlarmVolumeRampSetting::ValueType>> m_mockAlarmVolumeRampSetting;
    std::shared_ptr<CustomerDataManager> m_customerDataManager;
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;
    std::shared_ptr<settings::DeviceSettingsManager> m_settingsManager;
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    std::mutex m_mutex;
};

void AlertsCapabilityAgentTest::SetUp() {
    m_metricRecorder = std::make_shared<NiceMock<avsCommon::utils::metrics::test::MockMetricRecorder>>();
    m_mockMessageSender = std::make_shared<TestMessageSender>();
    m_mockAVSConnectionManager = std::make_shared<NiceMock<MockAVSConnectionManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_speakerManager = std::make_shared<NiceMock<MockSpeakerManager>>();
    m_exceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_contextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_alertStorage = std::make_shared<NiceMock<MockAlertStorage>>();
    m_alertsAudioFactory = std::make_shared<NiceMock<MockAlertsAudioFactory>>();
    m_renderer = std::make_shared<NiceMock<MockRenderer>>();
    m_customerDataManager = std::make_shared<CustomerDataManager>();
    m_messageStorage = std::make_shared<StubMessageStorage>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_settingsManager =
        std::make_shared<settings::DeviceSettingsManager>(std::make_shared<registrationManager::CustomerDataManager>());
    m_mockAlarmVolumeRampSetting =
        std::make_shared<MockSetting<AlarmVolumeRampSetting::ValueType>>(settings::types::getAlarmVolumeRampDefault());
    ASSERT_TRUE(m_settingsManager->addSetting<DeviceSettingsIndex::ALARM_VOLUME_RAMP>(m_mockAlarmVolumeRampSetting));

    ON_CALL(*(m_speakerManager.get()), getSpeakerSettings(_, _))
        .WillByDefault(Invoke([](ChannelVolumeInterface::Type, SpeakerInterface::SpeakerSettings*) {
            std::promise<bool> promise;
            promise.set_value(true);
            return promise.get_future();
        }));

    ON_CALL(*(m_speakerManager.get()), setVolume(_, _, _))
        .WillByDefault(
            Invoke([](ChannelVolumeInterface::Type, int8_t, const SpeakerManagerInterface::NotificationProperties&) {
                std::promise<bool> promise;
                promise.set_value(true);
                return promise.get_future();
            }));

    ON_CALL(*(m_alertStorage), createDatabase()).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), open()).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), store(_)).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), load(_, _)).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), modify(_)).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), erase(_)).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), bulkErase(_)).WillByDefault(Return(true));
    ON_CALL(*(m_alertStorage), clearDatabase()).WillByDefault(Return(true));

    m_certifiedSender = CertifiedSender::create(
        m_mockMessageSender, m_mockAVSConnectionManager, m_messageStorage, m_customerDataManager);

    m_alertsCA = AlertsCapabilityAgent::create(
        m_mockMessageSender,
        m_mockAVSConnectionManager,
        m_certifiedSender,
        m_mockFocusManager,
        m_speakerManager,
        m_contextManager,
        m_exceptionSender,
        m_alertStorage,
        m_alertsAudioFactory,
        m_renderer,
        m_customerDataManager,
        m_mockAlarmVolumeRampSetting,
        m_settingsManager,
        m_metricRecorder);

    std::static_pointer_cast<ConnectionStatusObserverInterface>(m_certifiedSender)
        ->onConnectionStatusChanged(
            ConnectionStatusObserverInterface::Status::CONNECTED,
            ConnectionStatusObserverInterface::ChangedReason::ACL_CLIENT_REQUEST);
}

void AlertsCapabilityAgentTest::TearDown() {
    m_certifiedSender->shutdown();
    m_certifiedSender.reset();
    m_alertsCA->shutdown();
    m_alertsCA.reset();
    m_mockMessageSender.reset();
    m_mockAVSConnectionManager.reset();
    m_mockFocusManager.reset();
    m_speakerManager.reset();
    m_exceptionSender.reset();
    m_contextManager.reset();
    m_alertStorage.reset();
    m_alertsAudioFactory.reset();
    m_renderer.reset();
    m_customerDataManager.reset();
    m_messageStorage.reset();
}

void AlertsCapabilityAgentTest::testStartAlertWithContentVolume(
    int8_t speakerVolume,
    int8_t alertsVolume,
    const std::string& otherChannel,
    bool shouldResultInSetVolume) {
    ON_CALL(*(m_speakerManager.get()), getSpeakerSettings(_, _))
        .WillByDefault(Invoke([speakerVolume, alertsVolume](
                                  ChannelVolumeInterface::Type type, SpeakerInterface::SpeakerSettings* settings) {
            if (type == ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME) {
                settings->volume = speakerVolume;
            } else {
                settings->volume = alertsVolume;
            }
            settings->mute = false;
            std::promise<bool> promise;
            promise.set_value(true);
            return promise.get_future();
        }));

    std::condition_variable waitCV;
    ON_CALL(*(m_speakerManager.get()), setVolume(_, _, _))
        .WillByDefault(Invoke(
            [&waitCV](ChannelVolumeInterface::Type, int8_t, const SpeakerManagerInterface::NotificationProperties&) {
                waitCV.notify_all();
                std::promise<bool> promise;
                promise.set_value(true);
                return promise.get_future();
            }));

    EXPECT_CALL(*(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, _, _))
        .Times(shouldResultInSetVolume ? 1 : 0);

    SpeakerInterface::SpeakerSettings speakerSettings{speakerVolume, false};
    speakerSettings.mute = false;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        speakerSettings);

    speakerSettings.volume = alertsVolume;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
        speakerSettings);

    // "Start" content
    m_alertsCA->onFocusChanged(otherChannel, avsCommon::avs::FocusState::BACKGROUND);

    // "Start" alert
    m_alertsCA->onAlertStateChange("", "", AlertObserverInterface::State::STARTED, "");

    std::unique_lock<std::mutex> ulock(m_mutex);
    waitCV.wait_for(ulock, std::chrono::milliseconds(MAX_WAIT_TIME_MS));
}

/**
 * Test local alert volume changes. Without alert sounding. Must send event.
 */
TEST_F(AlertsCapabilityAgentTest, test_localAlertVolumeChangeNoAlert) {
    SpeakerInterface::SpeakerSettings speakerSettings;
    speakerSettings.volume = TEST_VOLUME_VALUE;
    speakerSettings.mute = false;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
        speakerSettings);

    auto future = m_mockMessageSender->getNextMessage();

    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    std::string content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"VolumeChanged\"") != std::string::npos);
}

/**
 * Test local alert volume changes. With alert sounding. Must not send event, volume is treated as local.
 */
TEST_F(AlertsCapabilityAgentTest, testTimer_localAlertVolumeChangeAlertPlaying) {
    m_alertsCA->onAlertStateChange("", "", AlertObserverInterface::State::STARTED, "");

    // We have to wait for the alert state to be processed before updating speaker settings.
    auto future = m_mockMessageSender->getNextMessage();
    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    std::string content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"AlertStarted\"") != std::string::npos);

    SpeakerInterface::SpeakerSettings speakerSettings;
    speakerSettings.volume = TEST_VOLUME_VALUE;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME,
        speakerSettings);

    ASSERT_EQ(
        m_mockMessageSender->getNextMessage().wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)),
        std::future_status::timeout);
}

/**
 * Test volume changes originated from AVS
 */
TEST_F(AlertsCapabilityAgentTest, test_avsAlertVolumeChangeNoAlert) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_VOLUME_NAMESPACE_NAME, SET_VOLUME_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(
        *(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, TEST_VOLUME_VALUE, _))
        .Times(1);

    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)
        ->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)->handleDirective(MESSAGE_ID);

    auto future = m_mockMessageSender->getNextMessage();

    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    std::string content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"VolumeChanged\"") != std::string::npos);
}

/**
 * Test if AVS alerts volume directive results in a proper event when alert is already playing.
 */
TEST_F(AlertsCapabilityAgentTest, test_avsAlertVolumeChangeAlertPlaying) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_VOLUME_NAMESPACE_NAME, SET_VOLUME_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(
        *(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, TEST_VOLUME_VALUE, _))
        .Times(1);

    m_alertsCA->onAlertStateChange("", "", AlertObserverInterface::State::STARTED, "");
    auto future = m_mockMessageSender->getNextMessage();
    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    std::string content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"AlertStarted\"") != std::string::npos);

    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)
        ->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)->handleDirective(MESSAGE_ID);

    future = m_mockMessageSender->getNextMessage();

    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"VolumeChanged\"") != std::string::npos);
}

/**
 * Test use cases when alert is going to start when content is being played on Content channel with lower volume.
 */
TEST_F(AlertsCapabilityAgentTest, test_startAlertWithContentChannelLowerVolume) {
    testStartAlertWithContentVolume(
        LOWER_VOLUME_VALUE, HIGHER_VOLUME_VALUE, FocusManagerInterface::CONTENT_CHANNEL_NAME, false);
}

/**
 * Test use cases when alert is going to start when content is being played on Content channel with higher volume.
 */
TEST_F(AlertsCapabilityAgentTest, test_startAlertWithContentChannelHigherVolume) {
    testStartAlertWithContentVolume(
        HIGHER_VOLUME_VALUE, LOWER_VOLUME_VALUE, FocusManagerInterface::CONTENT_CHANNEL_NAME, true);
}

/**
 * Test use cases when alert is going to start when content is being played on Comms channel with lower volume.
 */
TEST_F(AlertsCapabilityAgentTest, test_startAlertWithCommsChannelLowerVolume) {
    testStartAlertWithContentVolume(
        LOWER_VOLUME_VALUE, HIGHER_VOLUME_VALUE, FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME, false);
}

/**
 * Test use cases when alert is going to start when content is being played on Comms channel with higher volume.
 */
TEST_F(AlertsCapabilityAgentTest, test_startAlertWithCommsChannelHigherVolume) {
    testStartAlertWithContentVolume(
        HIGHER_VOLUME_VALUE, LOWER_VOLUME_VALUE, FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME, true);
}

/**
 * Test use cases when alert is going to start when content is being played on Dialog channel with lower volume.
 */
TEST_F(AlertsCapabilityAgentTest, test_startAlertWithDialogChannelLowerVolume) {
    testStartAlertWithContentVolume(
        LOWER_VOLUME_VALUE, HIGHER_VOLUME_VALUE, FocusManagerInterface::DIALOG_CHANNEL_NAME, false);
}

/**
 * Test use cases when alert is going to start when content is being played on Dialog channel with higher volume.
 */
TEST_F(AlertsCapabilityAgentTest, test_startAlertWithDialogChannelHigherVolume) {
    testStartAlertWithContentVolume(
        HIGHER_VOLUME_VALUE, LOWER_VOLUME_VALUE, FocusManagerInterface::DIALOG_CHANNEL_NAME, false);
}

/**
 * Test invalid volume value handling.
 */
TEST_F(AlertsCapabilityAgentTest, test_invalidVolumeValuesMax) {
    EXPECT_CALL(
        *(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MAX, _))
        .Times(1);

    std::condition_variable waitCV;
    ON_CALL(
        *(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MAX, _))
        .WillByDefault(Invoke(
            [&waitCV](ChannelVolumeInterface::Type, int8_t, const SpeakerManagerInterface::NotificationProperties&) {
                waitCV.notify_all();
                std::promise<bool> promise;
                promise.set_value(true);
                return promise.get_future();
            }));

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_VOLUME_NAMESPACE_NAME, SET_VOLUME_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD_ABOVE_MAX, attachmentManager, "");

    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)
        ->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)->handleDirective(MESSAGE_ID);

    std::unique_lock<std::mutex> ulock(m_mutex);
    waitCV.wait_for(ulock, std::chrono::milliseconds(MAX_WAIT_TIME_MS));
}
/**
 * Test invalid volume value handling.
 */
TEST_F(AlertsCapabilityAgentTest, test_invalidVolumeValuesMin) {
    EXPECT_CALL(
        *(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MIN, _))
        .Times(1);

    std::condition_variable waitCV;
    ON_CALL(
        *(m_speakerManager.get()), setVolume(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MIN, _))
        .WillByDefault(Invoke(
            [&waitCV](ChannelVolumeInterface::Type, int8_t, const SpeakerManagerInterface::NotificationProperties&) {
                waitCV.notify_all();
                std::promise<bool> promise;
                promise.set_value(true);
                return promise.get_future();
            }));

    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_VOLUME_NAMESPACE_NAME, SET_VOLUME_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD_BELOW_MIN, attachmentManager, "");

    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)
        ->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    std::static_pointer_cast<CapabilityAgent>(m_alertsCA)->handleDirective(MESSAGE_ID);

    std::unique_lock<std::mutex> ulock(m_mutex);
    waitCV.wait_for(ulock, std::chrono::milliseconds(MAX_WAIT_TIME_MS));
}

/**
 * Test that alerts CA can correctly parse and apply alarm volume ramp value correctly.
 */
TEST_F(AlertsCapabilityAgentTest, test_SetAlarmVolumeRampDirective) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(ALERTS_NAMESPACE, SET_ALARM_VOLUME_RAMP_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, ALARM_VOLUME_RAMP_PAYLOAD_ENABLED, attachmentManager, "");

    // Set expectations.
    avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockAlarmVolumeRampSetting, setAvsChange(types::AlarmVolumeRampTypes::ASCENDING))
        .WillOnce(InvokeWithoutArgs([&waitEvent] {
            waitEvent.wakeUp();
            return true;
        }));

    auto alertsCA = std::static_pointer_cast<CapabilityAgent>(m_alertsCA);
    alertsCA->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    alertsCA->handleDirective(MESSAGE_ID);

    /// Wait till last expectation is met.
    ASSERT_TRUE(waitEvent.wait(TEST_TIMEOUT));
}

/**
 * Test that alerts CA will send exception for invalid SetAlarmVolumeRamp.
 */
TEST_F(AlertsCapabilityAgentTest, test_SetAlarmVolumeRampDirectiveInvalid) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(ALERTS_NAMESPACE, SET_ALARM_VOLUME_RAMP_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, ALARM_VOLUME_RAMP_PAYLOAD_INVALID, attachmentManager, "");

    // Expect exception to be sent.
    avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_exceptionSender, sendExceptionEncountered(_, _, _)).WillOnce(InvokeWithoutArgs([&waitEvent] {
        waitEvent.wakeUp();
    }));

    // Verify certified sender sends and exception.
    auto alertsCA = std::static_pointer_cast<CapabilityAgent>(m_alertsCA);
    alertsCA->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    alertsCA->handleDirective(MESSAGE_ID);

    EXPECT_TRUE(waitEvent.wait(TEST_TIMEOUT));
}

}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
