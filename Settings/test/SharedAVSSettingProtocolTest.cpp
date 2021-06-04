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

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <Settings/SharedAVSSettingProtocol.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingObserverInterface.h>

#include "Settings/MockSettingEventSender.h"
#include "Settings/MockDeviceSettingStorage.h"

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace testing;
using namespace storage::test;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::metrics::test;

/// A dummy setting metadata.
const SettingEventMetadata METADATA = {"namespace", "ChangedName", "ReportName", "setting"};

/// Constant representing a valid database value.
static const std::string DB_VALUE = R"("db-value")";

/// Constant representing a default value.
static const std::string DEFAULT_VALUE = R"("default-value")";

/// Constant representing a valid new value.
static const std::string NEW_VALUE = R"("new-value")";

/// Empty string used to represent invalid value by the protocol.
static const std::string INVALID_VALUE = "";

/// The database key to be used by the protocol given the @c METADATA object.
static const std::string key = METADATA.eventNamespace + "::" + METADATA.settingName;

/// The timeout used throughout the tests.
static const auto TEST_TIMEOUT = std::chrono::seconds(5);

/**
 * Object that allow us to mock the callback functions.
 */
class MockCallbacks {
public:
    MOCK_METHOD0(applyChange, std::pair<bool, std::string>());
    MOCK_METHOD0(revertChange, std::string());
    MOCK_METHOD1(applyDbChange, std::pair<bool, std::string>(const std::string& dbValue));
    MOCK_METHOD1(notifyObservers, void(SettingNotifications));
};

/**
 * The test class.
 */
class SharedAVSSettingProtocolTest : public Test {
protected:
    /**
     * Create protocol object and dependencies mocks.
     */
    void SetUp() override;

    /**
     * A helper method to change the value of the setting.
     *
     * @param value The string value of the setting.
     * @param isLocal True means the setting changed is done locally, otherwise, it is a cloud-driven change.
     */
    void modifySetting(std::string value, bool isLocal);

    /**
     * A helper method to test multiple value changes of the setting.
     *
     */
    void testMultipleChanges(bool isLocal);

    /// Mock event sender.
    std::shared_ptr<MockSettingEventSender> m_senderMock;

    /// Mock setting storage
    std::shared_ptr<MockDeviceSettingStorage> m_storageMock;

    /// Pointer to a mock protocol.
    std::unique_ptr<SharedAVSSettingProtocol> m_protocol;

    /// Mock Metric Recorder.
    std::shared_ptr<MockMetricRecorder> m_metricRecorder;

    /// Mock callbacks.
    StrictMock<MockCallbacks> m_callbacksMock;

    /// Mock connection manager;
    std::shared_ptr<MockAVSConnectionManager> m_mockConnectionManager;
};

void SharedAVSSettingProtocolTest::SetUp() {
    m_senderMock = std::make_shared<StrictMock<MockSettingEventSender>>();
    m_storageMock = std::make_shared<StrictMock<MockDeviceSettingStorage>>();
    m_mockConnectionManager = std::make_shared<NiceMock<MockAVSConnectionManager>>();
    m_metricRecorder = std::make_shared<NiceMock<MockMetricRecorder>>();
    m_protocol = SharedAVSSettingProtocol::create(
        METADATA, m_senderMock, m_storageMock, m_mockConnectionManager, m_metricRecorder);
}

void SharedAVSSettingProtocolTest::modifySetting(std::string value, bool isLocal) {
    auto executeSet = [value] { return std::make_pair(true, value); };
    auto revertChange = [] { return INVALID_VALUE; };
    auto notifyObservers = [](SettingNotifications notification) {};

    if (isLocal) {
        m_protocol->localChange(executeSet, revertChange, notifyObservers);
    } else {
        m_protocol->avsChange(executeSet, revertChange, notifyObservers);
    }
}

void SharedAVSSettingProtocolTest::testMultipleChanges(bool isLocal) {
    // The number of setting value changes.
    const int numValuesToSet = 10;

    // Event notification that all the setting changes has been requested.
    WaitEvent lastValueSet;

    // Event notification when the AVS event for the last setting change has been sent.
    WaitEvent lastValueSentEvent;

    // Make all storage operations successful.
    EXPECT_CALL(*m_storageMock, storeSetting(key, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(_, _)).WillRepeatedly(Return(true));

    // It is expected that one of the intermediate values may be sent or it may not be sent at all, since they are
    // replaced by the last value.
    if (isLocal) {
        EXPECT_CALL(*m_senderMock, sendChangedEvent(_))
            .Times(Between(0, 1))
            .WillRepeatedly(InvokeWithoutArgs([&lastValueSet] {
                // Simulate a delay in sending the event by blocking until all setting changes has been set.
                // This cause any pending events to merge.
                lastValueSet.wait(TEST_TIMEOUT);

                std::promise<bool> retPromise;
                retPromise.set_value(true);
                return retPromise.get_future();
            }));

    } else {
        EXPECT_CALL(*m_senderMock, sendReportEvent(_))
            .Times(Between(0, 1))
            .WillRepeatedly(InvokeWithoutArgs([&lastValueSet] {
                // Simulate a delay in sending the event by blocking until all setting changes has been set.
                // This cause any pending events to merge.
                lastValueSet.wait(TEST_TIMEOUT);

                std::promise<bool> retPromise;
                retPromise.set_value(true);
                return retPromise.get_future();
            }));
    }

    // Last value should be sent.
    if (isLocal) {
        EXPECT_CALL(*m_senderMock, sendChangedEvent(std::to_string(numValuesToSet)))
            .WillOnce(InvokeWithoutArgs([&lastValueSentEvent] {
                lastValueSentEvent.wakeUp();
                std::promise<bool> retPromise;
                retPromise.set_value(true);
                return retPromise.get_future();
            }));
    } else {
        EXPECT_CALL(*m_senderMock, sendReportEvent(std::to_string(numValuesToSet)))
            .WillOnce(InvokeWithoutArgs([&lastValueSentEvent] {
                lastValueSentEvent.wakeUp();
                std::promise<bool> retPromise;
                retPromise.set_value(true);
                return retPromise.get_future();
            }));
    }

    // Change the setting value multiple times.
    for (int i = 1; i <= numValuesToSet; i++) {
        modifySetting(std::to_string(i), isLocal);
    }

    lastValueSet.wakeUp();

    // This expectation verifies the merging action.
    EXPECT_TRUE(lastValueSentEvent.wait(TEST_TIMEOUT));

    // Below is an extra sanity test to verify that we can still send another event after sending a merged event.

    // Send another value and see if it still can be sent.
    const int newValue = numValuesToSet + 1;

    WaitEvent sentEvent;

    if (isLocal) {
        EXPECT_CALL(*m_senderMock, sendChangedEvent(std::to_string(newValue))).WillOnce(InvokeWithoutArgs([&sentEvent] {
            sentEvent.wakeUp();
            std::promise<bool> retPromise;
            retPromise.set_value(true);
            return retPromise.get_future();
        }));
    } else {
        EXPECT_CALL(*m_senderMock, sendReportEvent(std::to_string(newValue))).WillOnce(InvokeWithoutArgs([&sentEvent] {
            sentEvent.wakeUp();
            std::promise<bool> retPromise;
            retPromise.set_value(true);
            return retPromise.get_future();
        }));
    }

    modifySetting(std::to_string(newValue), isLocal);

    EXPECT_TRUE(sentEvent.wait(TEST_TIMEOUT));
}

/// Test create with null event sender.
TEST_F(SharedAVSSettingProtocolTest, test_nullEventSender) {
    ASSERT_FALSE(
        SharedAVSSettingProtocol::create(METADATA, nullptr, m_storageMock, m_mockConnectionManager, m_metricRecorder));
}

/// Test create with null storage.
TEST_F(SharedAVSSettingProtocolTest, test_nullStorage) {
    ASSERT_FALSE(
        SharedAVSSettingProtocol::create(METADATA, m_senderMock, nullptr, m_mockConnectionManager, m_metricRecorder));
}

/// Test restore when value is not available in the database.
TEST_F(SharedAVSSettingProtocolTest, test_restoreValueNotAvailable) {
    WaitEvent settingsUpdatedEvent;

    EXPECT_CALL(*m_storageMock, loadSetting(key)).WillOnce(Return(std::make_pair(SettingStatus::NOT_AVAILABLE, "")));
    EXPECT_CALL(*m_storageMock, storeSetting(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(_, _)).WillOnce(InvokeWithoutArgs([&settingsUpdatedEvent] {
        settingsUpdatedEvent.wakeUp();
        return true;
    }));

    EXPECT_CALL(m_callbacksMock, applyDbChange(INVALID_VALUE)).WillOnce(Return(std::make_pair(true, DEFAULT_VALUE)));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE));
    EXPECT_CALL(*m_senderMock, sendChangedEvent(_)).WillOnce(InvokeWithoutArgs([&]() {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    m_protocol->restoreValue(
        std::bind(&MockCallbacks::applyDbChange, &m_callbacksMock, std::placeholders::_1),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    settingsUpdatedEvent.wait(TEST_TIMEOUT);
}

/// Test restore when value is not available in the database for a setting that use cloud authoritative default value.
TEST_F(SharedAVSSettingProtocolTest, test_restoreValueNotAvailableCloudAuthoritative) {
    WaitEvent settingsUpdatedEvent;

    EXPECT_CALL(*m_storageMock, loadSetting(key)).WillOnce(Return(std::make_pair(SettingStatus::NOT_AVAILABLE, "")));
    EXPECT_CALL(*m_storageMock, storeSetting(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(_, _)).WillOnce(InvokeWithoutArgs([&settingsUpdatedEvent] {
        settingsUpdatedEvent.wakeUp();
        return true;
    }));

    EXPECT_CALL(m_callbacksMock, applyDbChange(INVALID_VALUE)).WillOnce(Return(std::make_pair(true, DEFAULT_VALUE)));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_senderMock, sendReportEvent(_)).WillOnce(InvokeWithoutArgs([&]() {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    auto protocol = SharedAVSSettingProtocol::create(
        METADATA, m_senderMock, m_storageMock, m_mockConnectionManager, m_metricRecorder, true);

    protocol->restoreValue(
        std::bind(&MockCallbacks::applyDbChange, &m_callbacksMock, std::placeholders::_1),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    settingsUpdatedEvent.wait(TEST_TIMEOUT);
}

/// Test restore when the value is available and synchronized.
TEST_F(SharedAVSSettingProtocolTest, test_restoreSynchronized) {
    EXPECT_CALL(*m_storageMock, loadSetting(key))
        .WillOnce(Return(std::make_pair(SettingStatus::SYNCHRONIZED, DB_VALUE)));
    EXPECT_CALL(m_callbacksMock, applyDbChange(DB_VALUE)).WillOnce(Return(std::make_pair(true, DB_VALUE)));

    m_protocol->restoreValue(
        std::bind(&MockCallbacks::applyDbChange, &m_callbacksMock, std::placeholders::_1),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));
}

/// Test success flow for AVS request.
TEST_F(SharedAVSSettingProtocolTest, test_AVSChangeRequest) {
    WaitEvent event;
    InSequence inSequence;

    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_senderMock, sendReportEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([&] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(key, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_protocol->avsChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test set value failed for AVS request.
TEST_F(SharedAVSSettingProtocolTest, test_AVSChangeRequestSetFailed) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(false, DB_VALUE)));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_FAILED));
    EXPECT_CALL(*m_senderMock, sendReportEvent(DB_VALUE)).WillOnce(InvokeWithoutArgs([&] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(key, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_protocol->avsChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test store value failed. Revert should be called and AVS notified of previous value.
TEST_F(SharedAVSSettingProtocolTest, test_AVSChangeRequestStoreFailed) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(false));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_FAILED));
    EXPECT_CALL(m_callbacksMock, revertChange()).WillOnce(Return(DEFAULT_VALUE));
    EXPECT_CALL(*m_senderMock, sendReportEvent(DEFAULT_VALUE)).WillOnce(InvokeWithoutArgs([&] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(key, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_protocol->avsChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test send event failed for avs request.
TEST_F(SharedAVSSettingProtocolTest, test_AVSChangeRequestSendEventFailed) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_senderMock, sendReportEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([&event] {
        event.wakeUp();
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        return retPromise.get_future();
    }));

    m_protocol->avsChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test success flow for local request.
TEST_F(SharedAVSSettingProtocolTest, test_localRequest) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::LOCAL_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE));
    EXPECT_CALL(*m_senderMock, sendChangedEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([&]() {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(key, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_protocol->localChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test set value failed during local request.
TEST_F(SharedAVSSettingProtocolTest, test_localRequestSetFailed) {
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(false, DB_VALUE)));

    WaitEvent event;
    auto notifyObservers = [&event](SettingNotifications notification) {
        switch (notification) {
            case SettingNotifications::AVS_CHANGE_IN_PROGRESS:
            case SettingNotifications::LOCAL_CHANGE:
            case SettingNotifications::AVS_CHANGE:
            case SettingNotifications::AVS_CHANGE_FAILED:
            case SettingNotifications::LOCAL_CHANGE_CANCELLED:
            case SettingNotifications::AVS_CHANGE_CANCELLED:
                FAIL() << notification;
                break;
            case SettingNotifications::LOCAL_CHANGE_IN_PROGRESS:
                break;
            case SettingNotifications::LOCAL_CHANGE_FAILED:
                event.wakeUp();
                break;
        }
    };

    m_protocol->localChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        notifyObservers);

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test store value to database failed for local request.
TEST_F(SharedAVSSettingProtocolTest, test_localRequestStoreFailed) {
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::LOCAL_CHANGE_IN_PROGRESS))
        .WillOnce(Return(false));
    EXPECT_CALL(m_callbacksMock, revertChange()).WillOnce(Return(DEFAULT_VALUE));

    WaitEvent event;
    auto notifyObservers = [&event](SettingNotifications notification) {
        switch (notification) {
            case SettingNotifications::AVS_CHANGE_IN_PROGRESS:
            case SettingNotifications::LOCAL_CHANGE:
            case SettingNotifications::AVS_CHANGE:
            case SettingNotifications::AVS_CHANGE_FAILED:
            case SettingNotifications::LOCAL_CHANGE_CANCELLED:
            case SettingNotifications::AVS_CHANGE_CANCELLED:
                FAIL() << notification;
                break;
            case SettingNotifications::LOCAL_CHANGE_IN_PROGRESS:
                break;
            case SettingNotifications::LOCAL_CHANGE_FAILED:
                event.wakeUp();
                break;
        }
    };

    m_protocol->localChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        notifyObservers);

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Test send changed event failed for local request. In this case, we should not update the database with SYNCHRONIZED.
TEST_F(SharedAVSSettingProtocolTest, test_localRequestSendEventFailed) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::LOCAL_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE));
    EXPECT_CALL(*m_senderMock, sendChangedEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([&event] {
        event.wakeUp();
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        return retPromise.get_future();
    }));

    m_protocol->localChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Verify that after network disconnect / reconnect will send out setting events that was changed locally while
/// disconnected.
TEST_F(SharedAVSSettingProtocolTest, test_localChangeSettingOfflineSynchronization) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::LOCAL_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::LOCAL_CHANGE));

    // Return false to simulate a disconnected network.
    EXPECT_CALL(*m_senderMock, sendChangedEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([&event] {
        event.wakeUp();
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        return retPromise.get_future();
    }));

    m_protocol->localChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
    event.reset();

    EXPECT_CALL(*m_storageMock, loadSetting(key))
        .WillOnce(Return(std::make_pair(SettingStatus::LOCAL_CHANGE_IN_PROGRESS, NEW_VALUE)));

    EXPECT_CALL(*m_senderMock, sendChangedEvent(_)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    EXPECT_CALL(*m_storageMock, updateSettingStatus(key, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_protocol->connectionStatusChangeCallback(true);

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
    event.reset();
}

/// Verify that after network disconnect / reconnect will send out setting events that was changed through AVS
/// directive.
TEST_F(SharedAVSSettingProtocolTest, test_avsChangeSettingOfflineSynchronization) {
    WaitEvent event;
    InSequence inSequence;
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(m_callbacksMock, applyChange()).WillOnce(Return(std::make_pair(true, NEW_VALUE)));
    EXPECT_CALL(*m_storageMock, storeSetting(key, NEW_VALUE, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(m_callbacksMock, notifyObservers(SettingNotifications::AVS_CHANGE));

    // Return false to simulate a disconnected network.
    EXPECT_CALL(*m_senderMock, sendReportEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([&event] {
        event.wakeUp();
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        return retPromise.get_future();
    }));

    m_protocol->avsChange(
        std::bind(&MockCallbacks::applyChange, &m_callbacksMock),
        std::bind(&MockCallbacks::revertChange, &m_callbacksMock),
        std::bind(&MockCallbacks::notifyObservers, &m_callbacksMock, std::placeholders::_1));

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
    event.reset();

    EXPECT_CALL(*m_storageMock, loadSetting(key))
        .WillOnce(Return(std::make_pair(SettingStatus::AVS_CHANGE_IN_PROGRESS, NEW_VALUE)));

    EXPECT_CALL(*m_senderMock, sendReportEvent(NEW_VALUE)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    EXPECT_CALL(*m_storageMock, updateSettingStatus(key, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_protocol->connectionStatusChangeCallback(true);

    EXPECT_TRUE(event.wait(TEST_TIMEOUT));
}

/// Verify that multiple AVS setting changes will be merged.
/// The setting will be changed multiple times immediately one after the other.
/// It's expected that only at most 2 settings are sent, that is, at least 1 has been canceled and the last value sent
/// is the latest one.
TEST_F(SharedAVSSettingProtocolTest, test_multipleAVSChanges) {
    testMultipleChanges(false);
}

/// Verify that multiple local setting changes will be merged.
/// The setting will be changed multiple times immediately one after the other.
/// It's expected that only at most 2 settings are sent, that is, at least 1 has been canceled and the last value sent
/// is the latest one.
TEST_F(SharedAVSSettingProtocolTest, test_multipleLocalChanges) {
    testMultipleChanges(true);
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
