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

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/MockDeviceSettingStorage.h>

#include "DoNotDisturbCA/DoNotDisturbCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace registrationManager;
using namespace settings::storage;
using namespace settings::storage::test;
using namespace settings;

/// Amount of time for the test to wait for event to be sent.
static const std::chrono::seconds WAIT_TIMEOUT(2);

/// A sample Directive JSON string for the purposes of creating an AVSDirective object.
static const std::string SETDNDMODE_DIRECTIVE_VALID_JSON_STRING = R"delim(
    {
        "directive": {
            "header": {
                "namespace": "DoNotDisturb",
                "name": "SetDoNotDisturb",
                "messageId": "12345"
            },
            "payload": {
                "enabled": true
            }
        }
    })delim";

/// "Report" event for DoNotDisturb API
static const std::string DND_REPORT_EVENT = "ReportDoNotDisturb";
/// "Changed" event for DoNotDisturb API
static const std::string DND_CHANGE_EVENT = "DoNotDisturbChanged";

/// Test harness for @c DoNotDisturbCapabilityAgent class.
class DoNotDisturbCapabilityAgentTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

protected:
    void TearDown() override;

public:
    /// Helper method to set up mocks for an upcoming event.
    bool expectEventSend(
        const std::string& eventName,
        MessageRequestObserverInterface::Status statusReported,
        std::function<void()> triggerOperation);

protected:
    /// The InteractionModelCapabilityAgent instance to be tested
    std::shared_ptr<DoNotDisturbCapabilityAgent> m_dndCA;

    /// Message sender mock to track messages being sent.
    std::shared_ptr<MockMessageSender> m_messageSender;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;

    /// Customer data manager to handle the factory reset logic.
    std::shared_ptr<CustomerDataManager> m_customerDataManager;

    /// An instace of @c DeviceSettingsManager,
    std::shared_ptr<DeviceSettingsManager> m_settingsManager;

    /// Storage for the settings.
    std::shared_ptr<MockDeviceSettingStorage> m_settingsStorage;
};

void DoNotDisturbCapabilityAgentTest::SetUp() {
    m_customerDataManager = std::make_shared<CustomerDataManager>();

    m_messageSender = std::make_shared<MockMessageSender>();

    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();

    m_settingsManager = std::make_shared<DeviceSettingsManager>();

    m_settingsStorage = std::make_shared<MockDeviceSettingStorage>();
    EXPECT_CALL(*m_settingsStorage, storeSetting(_, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_settingsStorage, updateSettingStatus(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_settingsStorage, loadSetting(_))
        .WillRepeatedly(Return(std::make_pair(SettingStatus::SYNCHRONIZED, "true")));

    m_dndCA = DoNotDisturbCapabilityAgent::create(
        m_customerDataManager, m_mockExceptionEncounteredSender, m_messageSender, m_settingsManager, m_settingsStorage);

    ASSERT_NE(m_dndCA, nullptr);
}

bool DoNotDisturbCapabilityAgentTest::expectEventSend(
    const std::string& eventName,
    MessageRequestObserverInterface::Status statusReported,
    std::function<void()> triggerOperation) {
    std::promise<bool> eventPromise;
    EXPECT_CALL(*m_messageSender, sendMessage(_))
        .Times(1)
        .WillOnce(Invoke([statusReported, &eventName, &eventPromise](std::shared_ptr<MessageRequest> request) {
            if (request->getJsonContent().find(eventName) != std::string::npos) {
                request->sendCompleted(statusReported);
                eventPromise.set_value(true);
                return;
            }
            // Unexpected event
            eventPromise.set_value(false);
        }));

    triggerOperation();

    auto future = eventPromise.get_future();
    bool isFutureReady = future.wait_for(WAIT_TIMEOUT) == std::future_status::ready;
    EXPECT_TRUE(isFutureReady);

    if (!isFutureReady) {
        return false;
    }
    return future.get();
}

void DoNotDisturbCapabilityAgentTest::TearDown() {
    if (m_dndCA) {
        m_dndCA->shutdown();
    }
}

TEST_F(DoNotDisturbCapabilityAgentTest, givenNullCustomerDataManager_create_shouldFail) {
    auto dndCA = DoNotDisturbCapabilityAgent::create(
        nullptr, m_mockExceptionEncounteredSender, m_messageSender, m_settingsManager, m_settingsStorage);
    EXPECT_THAT(dndCA, IsNull());
}

TEST_F(DoNotDisturbCapabilityAgentTest, givenNullExceptionSender_create_shouldFail) {
    auto dndCA = DoNotDisturbCapabilityAgent::create(
        m_customerDataManager, nullptr, m_messageSender, m_settingsManager, m_settingsStorage);
    EXPECT_THAT(dndCA, IsNull());
}

TEST_F(DoNotDisturbCapabilityAgentTest, givenNullMessageSender_create_shouldFail) {
    auto dndCA = DoNotDisturbCapabilityAgent::create(
        m_customerDataManager, m_mockExceptionEncounteredSender, nullptr, m_settingsManager, m_settingsStorage);
    EXPECT_THAT(dndCA, IsNull());
}

TEST_F(DoNotDisturbCapabilityAgentTest, givenNullSettingsManager_create_shouldFail) {
    auto dndCA = DoNotDisturbCapabilityAgent::create(
        m_customerDataManager, m_mockExceptionEncounteredSender, m_messageSender, nullptr, m_settingsStorage);
    EXPECT_THAT(dndCA, IsNull());
}

TEST_F(DoNotDisturbCapabilityAgentTest, givenNullSettingsStorage_create_shouldFail) {
    auto dndCA = DoNotDisturbCapabilityAgent::create(
        m_customerDataManager, m_mockExceptionEncounteredSender, m_messageSender, m_settingsManager, nullptr);
    EXPECT_THAT(dndCA, IsNull());
}

TEST_F(DoNotDisturbCapabilityAgentTest, givenValidSetDNDDirective_handleDirective_shouldSucceed) {
    // Become online
    bool initialReportSent =
        expectEventSend(DND_REPORT_EVENT, MessageRequestObserverInterface::Status::SUCCESS, [this]() {
            m_dndCA->onConnectionStatusChanged(
                ConnectionStatusObserverInterface::Status::CONNECTED,
                ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
        });

    ASSERT_TRUE(initialReportSent);

    auto directivePair = AVSDirective::create(SETDNDMODE_DIRECTIVE_VALID_JSON_STRING, nullptr, "");
    std::shared_ptr<AVSDirective> directive = std::move(directivePair.first);

    bool directiveResponseEventSent =
        expectEventSend(DND_REPORT_EVENT, MessageRequestObserverInterface::Status::SUCCESS, [this, &directive]() {
            m_dndCA->handleDirectiveImmediately(directive);
        });

    ASSERT_TRUE(directiveResponseEventSent);
}

TEST_F(DoNotDisturbCapabilityAgentTest, beingOnline_applyLocalChange_shouldSendReport) {
    bool initialReportSent =
        expectEventSend(DND_REPORT_EVENT, MessageRequestObserverInterface::Status::SUCCESS, [this]() {
            m_dndCA->onConnectionStatusChanged(
                ConnectionStatusObserverInterface::Status::CONNECTED,
                ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
        });

    ASSERT_TRUE(initialReportSent);

    bool changeEventSent =
        expectEventSend(DND_CHANGE_EVENT, MessageRequestObserverInterface::Status::SUCCESS, [this]() {
            m_dndCA->sendChangedEvent("true");
        });

    ASSERT_TRUE(changeEventSent);
}

TEST_F(DoNotDisturbCapabilityAgentTest, beingOffline_applyLocalChangeAndBecomeOnline_shouldSendChanged) {
    // Apply change while offline
    m_dndCA->sendChangedEvent("true");

    bool changeEventSent =
        expectEventSend(DND_CHANGE_EVENT, MessageRequestObserverInterface::Status::SUCCESS, [this]() {
            m_dndCA->onConnectionStatusChanged(
                ConnectionStatusObserverInterface::Status::CONNECTED,
                ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
        });

    ASSERT_TRUE(changeEventSent);
}

TEST_F(DoNotDisturbCapabilityAgentTest, whileSendingChangedEvent_sendChangedFail_shouldSendReport) {
    // Become online and ignore the first "report" event.

    bool initialReportSent =
        expectEventSend(DND_REPORT_EVENT, MessageRequestObserverInterface::Status::SUCCESS, [this]() {
            m_dndCA->onConnectionStatusChanged(
                ConnectionStatusObserverInterface::Status::CONNECTED,
                ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
        });

    ASSERT_TRUE(initialReportSent);

    int eventMask = 0;

    std::promise<bool> eventPromise;
    EXPECT_CALL(*m_messageSender, sendMessage(_))
        .Times(2)
        .WillRepeatedly(Invoke([&eventMask, &eventPromise](std::shared_ptr<MessageRequest> request) {
            if (request->getJsonContent().find(DND_CHANGE_EVENT) != std::string::npos) {
                eventMask <<= 1;
                eventMask |= 1;
                request->sendCompleted(MessageRequestObserverInterface::Status::INTERNAL_ERROR);
                return;
            } else if (request->getJsonContent().find(DND_REPORT_EVENT) != std::string::npos) {
                eventMask <<= 1;
                eventPromise.set_value(true);
                request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
                return;
            }

            // Unexpected event
            eventPromise.set_value(false);
        }));

    m_dndCA->sendChangedEvent("true");

    auto future = eventPromise.get_future();
    bool isFutureReady = future.wait_for(WAIT_TIMEOUT) == std::future_status::ready;
    EXPECT_TRUE(isFutureReady);
    // Check the order and existence of the events: Changed first, Report second, both happened.
    EXPECT_EQ(eventMask, 2);
}

}  // namespace test
}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
