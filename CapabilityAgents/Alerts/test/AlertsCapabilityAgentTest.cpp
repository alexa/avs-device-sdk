/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Alerts/AlertsCapabilityAgent.h"

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

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace test {

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
using namespace testing;

/// Maximum time to wait for operaiton result.
constexpr int MAX_WAIT_TIME_MS = 200;

/// Alerts.SetVolume Directive name
constexpr char SET_VOLUME_DIRECTIVE_NAME[] = "SetVolume";

/// Alerts.SetVolume Namespace name
constexpr char SET_VOLUME_NAMESPACE_NAME[] = "Alerts";

/// Crafted message ID
constexpr char MESSAGE_ID[] = "1";

/// General test value for alerts volume
constexpr int TEST_VOLUME_VALUE = 33;

/// Higher test volume value
constexpr int HIGHER_VOLUME_VALUE = 100;

/// Lower test volume value
constexpr int LOWER_VOLUME_VALUE = 50;

// clang-format off

/// General test directive payload
static const std::string VOLUME_PAYLOAD =
        "{"
        R"("volume":)" +
        std::to_string(TEST_VOLUME_VALUE) +
        ""
        "}";

/// Test directive payload with volume too high
static const std::string VOLUME_PAYLOAD_ABOVE_MAX =
        "{"
        R"("volume":)" +
        std::to_string(AVS_SET_VOLUME_MAX + 1) +
        ""
        "}";

/// Test directive payload with volume too low
static const std::string VOLUME_PAYLOAD_BELOW_MIN =
        "{"
        R"("volume":)" +
        std::to_string(AVS_SET_VOLUME_MIN - 1) +
        ""
        "}";

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

    bool store(std::shared_ptr<Alert> alert) override {
        return true;
    }

    bool load(std::vector<std::shared_ptr<Alert>>* alertContainer) override {
        return true;
    }

    bool modify(std::shared_ptr<Alert> alert) override {
        return true;
    }

    bool erase(std::shared_ptr<Alert> alert) override {
        return true;
    }

    bool clearDatabase() override {
        return true;
    }

    bool bulkErase(const std::list<std::shared_ptr<Alert>>& alertList) override {
        return true;
    }
};

/**
 * Test @c RendererInterface implementation to provide a valid instance for the initialization of other components.
 */
class StubRenderer : public RendererInterface {
    void start(
        std::shared_ptr<capabilityAgents::alerts::renderer::RendererObserverInterface> observer,
        std::function<std::unique_ptr<std::istream>()> audioFactory,
        const std::vector<std::string>& urls,
        int loopCount,
        std::chrono::milliseconds loopPause) override {
    }

    void stop() override {
    }
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
        if (m_nextMessagePromise) {
            m_nextMessagePromise->set_value(request);
            m_nextMessagePromise.reset();
        }
        request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
    }

    /**
     * Wait for next message to be sent using this object. The message sent is then returned to the caller.
     * @return The last message sent using this object.
     */
    std::future<std::shared_ptr<avsCommon::avs::MessageRequest>> getNextMessage() {
        m_nextMessagePromise = std::make_shared<std::promise<std::shared_ptr<avsCommon::avs::MessageRequest>>>();
        return m_nextMessagePromise->get_future();
    }

private:
    std::shared_ptr<std::promise<std::shared_ptr<avsCommon::avs::MessageRequest>>> m_nextMessagePromise;
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
    std::shared_ptr<StubAlertStorage> m_alertStorage;
    std::shared_ptr<MockAlertsAudioFactory> m_alertsAudioFactory;
    std::shared_ptr<StubRenderer> m_renderer;
    std::shared_ptr<CustomerDataManager> m_customerDataManager;
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;

    std::mutex m_mutex;
};

void AlertsCapabilityAgentTest::SetUp() {
    m_mockMessageSender = std::make_shared<TestMessageSender>();
    m_mockAVSConnectionManager = std::make_shared<NiceMock<MockAVSConnectionManager>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_speakerManager = std::make_shared<NiceMock<MockSpeakerManager>>();
    m_exceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_contextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_alertStorage = std::make_shared<StubAlertStorage>();
    m_alertsAudioFactory = std::make_shared<NiceMock<MockAlertsAudioFactory>>();
    m_renderer = std::make_shared<StubRenderer>();
    m_customerDataManager = std::make_shared<CustomerDataManager>();
    m_messageStorage = std::make_shared<StubMessageStorage>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();

    ON_CALL(*(m_speakerManager.get()), getSpeakerSettings(_, _))
        .WillByDefault(Invoke([](avsCommon::sdkInterfaces::SpeakerInterface::Type,
                                 avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings*) {
            std::promise<bool> promise;
            promise.set_value(true);
            return promise.get_future();
        }));

    ON_CALL(*(m_speakerManager.get()), setVolume(_, _, _))
        .WillByDefault(Invoke([](avsCommon::sdkInterfaces::SpeakerInterface::Type, int8_t, bool) {
            std::promise<bool> promise;
            promise.set_value(true);
            return promise.get_future();
        }));

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
        m_customerDataManager);

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
        .WillByDefault(Invoke(
            [speakerVolume, alertsVolume](SpeakerInterface::Type type, SpeakerInterface::SpeakerSettings* settings) {
                if (type == SpeakerInterface::Type::AVS_SPEAKER_VOLUME) {
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
        .WillByDefault(Invoke([&waitCV](SpeakerInterface::Type, int8_t, bool) {
            waitCV.notify_all();
            std::promise<bool> promise;
            promise.set_value(true);
            return promise.get_future();
        }));

    EXPECT_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, _, _))
        .Times(shouldResultInSetVolume ? 1 : 0);

    SpeakerInterface::SpeakerSettings speakerSettings{speakerVolume, false};
    speakerSettings.mute = false;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API,
        SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        speakerSettings);

    speakerSettings.volume = alertsVolume;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API, SpeakerInterface::Type::AVS_ALERTS_VOLUME, speakerSettings);

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
TEST_F(AlertsCapabilityAgentTest, localAlertVolumeChangeNoAlert) {
    SpeakerInterface::SpeakerSettings speakerSettings;
    speakerSettings.volume = TEST_VOLUME_VALUE;
    speakerSettings.mute = false;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API, SpeakerInterface::Type::AVS_ALERTS_VOLUME, speakerSettings);

    auto future = m_mockMessageSender->getNextMessage();

    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    std::string content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"VolumeChanged\"") != std::string::npos);
}

/**
 * Test local alert volume changes. With alert sounding. Must not send event, volume is treated as local.
 */
TEST_F(AlertsCapabilityAgentTest, localAlertVolumeChangeAlertPlaying) {
    m_alertsCA->onAlertStateChange("", "", AlertObserverInterface::State::STARTED, "");

    // We have to wait for the alert state to be processed before updating speaker settings.
    auto future = m_mockMessageSender->getNextMessage();
    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)), std::future_status::ready);

    std::string content = future.get()->getJsonContent();
    ASSERT_TRUE(content.find("\"name\":\"AlertStarted\"") != std::string::npos);

    SpeakerInterface::SpeakerSettings speakerSettings;
    speakerSettings.volume = TEST_VOLUME_VALUE;
    m_alertsCA->onSpeakerSettingsChanged(
        SpeakerManagerObserverInterface::Source::LOCAL_API, SpeakerInterface::Type::AVS_ALERTS_VOLUME, speakerSettings);

    ASSERT_EQ(
        m_mockMessageSender->getNextMessage().wait_for(std::chrono::milliseconds(MAX_WAIT_TIME_MS)),
        std::future_status::timeout);
}

/**
 * Test volume changes originated from AVS
 */
TEST_F(AlertsCapabilityAgentTest, avsAlertVolumeChangeNoAlert) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_VOLUME_NAMESPACE_NAME, SET_VOLUME_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, TEST_VOLUME_VALUE, _))
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
TEST_F(AlertsCapabilityAgentTest, avsAlertVolumeChangeAlertPlaying) {
    // Create Directive.
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_VOLUME_NAMESPACE_NAME, SET_VOLUME_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, VOLUME_PAYLOAD, attachmentManager, "");

    EXPECT_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, TEST_VOLUME_VALUE, _))
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
TEST_F(AlertsCapabilityAgentTest, startAlertWithContentChannelLowerVolume) {
    testStartAlertWithContentVolume(
        LOWER_VOLUME_VALUE, HIGHER_VOLUME_VALUE, FocusManagerInterface::CONTENT_CHANNEL_NAME, false);
}

/**
 * Test use cases when alert is going to start when content is being played on Content channel with higher volume.
 */
TEST_F(AlertsCapabilityAgentTest, startAlertWithContentChannelHigherVolume) {
    testStartAlertWithContentVolume(
        HIGHER_VOLUME_VALUE, LOWER_VOLUME_VALUE, FocusManagerInterface::CONTENT_CHANNEL_NAME, true);
}

/**
 * Test use cases when alert is going to start when content is being played on Comms channel with lower volume.
 */
TEST_F(AlertsCapabilityAgentTest, startAlertWithCommsChannelLowerVolume) {
    testStartAlertWithContentVolume(
        LOWER_VOLUME_VALUE, HIGHER_VOLUME_VALUE, FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME, false);
}

/**
 * Test use cases when alert is going to start when content is being played on Comms channel with higher volume.
 */
TEST_F(AlertsCapabilityAgentTest, startAlertWithCommsChannelHigherVolume) {
    testStartAlertWithContentVolume(
        HIGHER_VOLUME_VALUE, LOWER_VOLUME_VALUE, FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME, true);
}

/**
 * Test use cases when alert is going to start when content is being played on Dialog channel with lower volume.
 */
TEST_F(AlertsCapabilityAgentTest, startAlertWithDialogChannelLowerVolume) {
    testStartAlertWithContentVolume(
        LOWER_VOLUME_VALUE, HIGHER_VOLUME_VALUE, FocusManagerInterface::DIALOG_CHANNEL_NAME, false);
}

/**
 * Test use cases when alert is going to start when content is being played on Dialog channel with higher volume.
 */
TEST_F(AlertsCapabilityAgentTest, startAlertWithDialogChannelHigherVolume) {
    testStartAlertWithContentVolume(
        HIGHER_VOLUME_VALUE, LOWER_VOLUME_VALUE, FocusManagerInterface::DIALOG_CHANNEL_NAME, false);
}

/**
 * Test invalid volume value handling.
 */
TEST_F(AlertsCapabilityAgentTest, invalidVolumeValuesMax) {
    EXPECT_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MAX, _))
        .Times(1);

    std::condition_variable waitCV;
    ON_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MAX, _))
        .WillByDefault(Invoke([&waitCV](SpeakerInterface::Type, int8_t, bool) {
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
TEST_F(AlertsCapabilityAgentTest, invalidVolumeValuesMin) {
    EXPECT_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MIN, _))
        .Times(1);

    std::condition_variable waitCV;
    ON_CALL(*(m_speakerManager.get()), setVolume(SpeakerInterface::Type::AVS_ALERTS_VOLUME, AVS_SET_VOLUME_MIN, _))
        .WillByDefault(Invoke([&waitCV](SpeakerInterface::Type, int8_t, bool) {
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

}  // namespace test
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
