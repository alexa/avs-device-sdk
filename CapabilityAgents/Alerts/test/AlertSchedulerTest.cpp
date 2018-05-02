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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Alerts/AlertScheduler.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace test {

/// Tokens for alerts.
static const std::string ALERT1_TOKEN = "token1";
static const std::string ALERT2_TOKEN = "token2";
static const std::string ALERT3_TOKEN = "token3";

/// Test alert type
static const std::string ALERT_TYPE = "TEST_ALERT_TYPE";

/// A schedule instant in the past for alerts.
static const std::string PAST_INSTANT = "2000-01-01T12:34:56+0000";

/// A schedule instant in the future for alerts.
static const std::string FUTURE_INSTANT = "2030-01-01T12:34:56+0000";

/// Amount of time that the alert observer should wait for a task to finish.
static const std::chrono::milliseconds TEST_TIMEOUT{100};

/// Alert past due time limit.
static const std::chrono::seconds ALERT_PAST_DUE_TIME_LIMIT{10};

class MockRenderer : public renderer::RendererInterface {
public:
    MOCK_METHOD1(
        setObserver,
        void(std::shared_ptr<capabilityAgents::alerts::renderer::RendererObserverInterface> observer));
    MOCK_METHOD4(
        start,
        void(
            std::function<std::unique_ptr<std::istream>()> audioFactory,
            const std::vector<std::string>& urls,
            int loopCount,
            std::chrono::milliseconds loopPause));
    MOCK_METHOD0(stop, void());
};

class TestAlert : public Alert {
public:
    TestAlert() :
            Alert(defaultAudioFactory, shortAudioFactory),
            m_alertType{ALERT_TYPE},
            m_renderer{std::make_shared<MockRenderer>()} {
        this->setRenderer(m_renderer);
    }

    TestAlert(const std::string& token, const std::string& schedTime) :
            Alert(defaultAudioFactory, shortAudioFactory),
            m_alertType{ALERT_TYPE},
            m_renderer{std::make_shared<MockRenderer>()} {
        this->setRenderer(m_renderer);

        // clang-format off
        const std::string payloadJson =
            "{"
                "\"token\": \"" + token + "\","
                "\"type\": \"" + m_alertType + "\","
                "\"scheduledTime\": \"" + schedTime + "\""
            "}";
        // clang-format on

        std::string errorMessage;
        rapidjson::Document payload;
        payload.Parse(payloadJson);

        this->parseFromJson(payload, &errorMessage);
    }

    std::string getTypeName() const override {
        return m_alertType;
    }

private:
    static std::unique_ptr<std::istream> defaultAudioFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream("default audio"));
    }

    static std::unique_ptr<std::istream> shortAudioFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream("short audio"));
    }

    const std::string m_alertType;
    std::shared_ptr<MockRenderer> m_renderer;
};

class MockAlertStorage : public storage::AlertStorageInterface {
public:
    MockAlertStorage() {
        m_createDatabaseRetVal = true;
        m_openRetVal = true;
        m_isOpenRetVal = true;
        m_alertExistsRetVal = true;
        m_storeRetVal = true;
        m_loadRetVal = true;
        m_eraseRetVal = true;
    }

    bool createDatabase() {
        return m_createDatabaseRetVal;
    }
    bool open() {
        return m_openRetVal;
    }
    bool isOpen() {
        return m_isOpenRetVal;
    }
    void close() {
    }
    bool alertExists(const std::string& token) {
        return m_alertExistsRetVal;
    }
    bool store(std::shared_ptr<Alert> alert) {
        return m_storeRetVal;
    }
    bool load(std::vector<std::shared_ptr<Alert>>* alertContainer) {
        if (m_loadRetVal) {
            alertContainer->clear();
            for (std::shared_ptr<Alert> alertToAdd : m_alertsInStorage) {
                alertContainer->push_back(alertToAdd);
            }
        }
        return m_loadRetVal;
    }
    bool erase(const std::vector<int>& alertDbIds) {
        return m_eraseRetVal;
    }
    void setCreateDatabaseRetVal(bool retVal) {
        m_createDatabaseRetVal = retVal;
    }
    void setOpenRetVal(bool retVal) {
        m_openRetVal = retVal;
    }
    void setIsOpenRetVal(bool retVal) {
        m_isOpenRetVal = retVal;
    }
    void setAlertExistsRetVal(bool retVal) {
        m_alertExistsRetVal = retVal;
    }
    void setStoreRetVal(bool retVal) {
        m_storeRetVal = retVal;
    }
    void setLoadRetVal(bool retVal) {
        m_loadRetVal = retVal;
    }
    void setEraseRetVal(bool retVal) {
        m_eraseRetVal = retVal;
    }
    void setAlerts(std::vector<std::shared_ptr<TestAlert>> alertsToAdd) {
        m_alertsInStorage.clear();
        for (std::shared_ptr<TestAlert> alertToAdd : alertsToAdd) {
            m_alertsInStorage.push_back(alertToAdd);
        }
    }

    MOCK_METHOD1(erase, bool(std::shared_ptr<Alert>));
    MOCK_METHOD1(modify, bool(std::shared_ptr<Alert>));
    MOCK_METHOD0(clearDatabase, bool());

private:
    std::vector<std::shared_ptr<Alert>> m_alertsInStorage;
    bool m_createDatabaseRetVal;
    bool m_openRetVal;
    bool m_isOpenRetVal;
    bool m_alertExistsRetVal;
    bool m_storeRetVal;
    bool m_loadRetVal;
    bool m_eraseRetVal;
};

class TestAlertObserver : public AlertObserverInterface {
public:
    bool waitFor(AlertScheduler::State newState) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_conditionVariable.wait_for(lock, TEST_TIMEOUT, [this, newState] { return m_state == newState; });
    }

    void onAlertStateChange(const std::string& alertToken, AlertScheduler::State newState, const std::string& reason) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = newState;
        m_conditionVariable.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    AlertScheduler::State m_state;
};

class AlertSchedulerTest : public ::testing::Test {
public:
    AlertSchedulerTest();
    void SetUp() override;
    std::shared_ptr<TestAlert> doSimpleTestSetup(bool activateAlert = false, bool initWithAlertObserver = false);

protected:
    std::shared_ptr<MockAlertStorage> m_alertStorage;
    std::shared_ptr<MockRenderer> m_alertRenderer;
    std::chrono::seconds m_alertPastDueTimeLimit;
    std::shared_ptr<AlertScheduler> m_alertScheduler;
    std::shared_ptr<TestAlertObserver> m_testAlertObserver;
};

AlertSchedulerTest::AlertSchedulerTest() :
        m_alertStorage{std::make_shared<MockAlertStorage>()},
        m_alertRenderer{std::make_shared<MockRenderer>()},
        m_alertPastDueTimeLimit{ALERT_PAST_DUE_TIME_LIMIT},
        m_alertScheduler{std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit)},
        m_testAlertObserver{std::make_shared<TestAlertObserver>()} {
}

void AlertSchedulerTest::SetUp() {
    m_alertStorage->setOpenRetVal(true);
}

/**
 * Helper method that does a basic setup for tests.
 * The method performs the following functions:
 * 1 - Creates a test alert and adds it to be scheduled
 * 2 - If initializing with alert scheduler observer, then creates alert scheduler observer
 * 3 - Initializes the alert scheduler
 * 4 - Based on the argument, may initialize the alert
 *
 * The method will return the scheduled alert
 */
std::shared_ptr<TestAlert> AlertSchedulerTest::doSimpleTestSetup(bool activateAlert, bool initWithAlertObserver) {
    std::vector<std::shared_ptr<TestAlert>> alertToAdd;
    std::shared_ptr<TestAlert> alert = std::make_shared<TestAlert>(ALERT1_TOKEN, FUTURE_INSTANT);
    alertToAdd.push_back(alert);
    m_alertStorage->setAlerts(alertToAdd);

    if (initWithAlertObserver) {
        m_alertScheduler->initialize(m_testAlertObserver);
    } else {
        std::shared_ptr<AlertScheduler> alertSchedulerObs{
            std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit)};
        m_alertScheduler->initialize(alertSchedulerObs);
    }

    if (activateAlert) {
        alert->activate();
        m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);
    }

    return alert;
}

/**
 * Test initializing AlertScheduler
 */
TEST_F(AlertSchedulerTest, initialize) {
    /// check if init fails if scheduler is not available
    ASSERT_FALSE(m_alertScheduler->initialize(nullptr));

    /// check if init fails if a database for alerts cant be created
    m_alertStorage->setOpenRetVal(false);
    m_alertStorage->setCreateDatabaseRetVal(false);
    ASSERT_FALSE(m_alertScheduler->initialize(m_alertScheduler));

    /// check if init succeeds. Pass in 3 alerts of which 1 is expired. Only 2 should actually remain in the end.
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit)};
    m_alertStorage->setOpenRetVal(true);
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// past alert
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, PAST_INSTANT);
    alertsToAdd.push_back(alert1);

    /// future active alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, FUTURE_INSTANT);
    alert2->activate();
    alert2->setStateActive();
    alertsToAdd.push_back(alert2);

    /// future inactive alert
    std::shared_ptr<TestAlert> alert3 = std::make_shared<TestAlert>(ALERT3_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert3);
    m_alertStorage->setAlerts(alertsToAdd);

    /// past alert should get removed
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);

    /// active alert should get modified
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(1);

    ASSERT_TRUE(m_alertScheduler->initialize(alertSchedulerObs));

    const unsigned int expectedRemainingAlerts = 2;

    /// only the 2 future alerts remain
    ASSERT_EQ(m_alertScheduler->getContextInfo().scheduledAlerts.size(), expectedRemainingAlerts);
}

/**
 * Test AlertScheduler getting focus
 */
TEST_F(AlertSchedulerTest, updateGetFocus) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup();

    /// check if focus changes to foreground
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::FOREGROUND);
    ASSERT_EQ(m_alertScheduler->getFocusState(), avsCommon::avs::FocusState::FOREGROUND);

    /// check if focus changes to background
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);
    ASSERT_EQ(m_alertScheduler->getFocusState(), avsCommon::avs::FocusState::BACKGROUND);

    /// check alert state change if focus is gone
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::NONE);
    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
}

/**
 * Test scheduling alerts
 */
TEST_F(AlertSchedulerTest, scheduleAlert) {
    /// check that a future alert is scheduled
    std::shared_ptr<TestAlert> alert1 = doSimpleTestSetup(true);
    ASSERT_TRUE(m_alertScheduler->scheduleAlert(alert1));

    /// check that a future alert is not scheduled if you cant store the alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, FUTURE_INSTANT);
    m_alertStorage->setStoreRetVal(false);
    ASSERT_FALSE(m_alertScheduler->scheduleAlert(alert2));

    /// check that past alerts cant be scheduled
    std::shared_ptr<TestAlert> alert3 = std::make_shared<TestAlert>(ALERT3_TOKEN, PAST_INSTANT);
    m_alertStorage->setStoreRetVal(true);
    ASSERT_TRUE(m_alertScheduler->scheduleAlert(alert2));
    ASSERT_FALSE(m_alertScheduler->scheduleAlert(alert3));
}

/**
 * Test snoozing alerts
 */
TEST_F(AlertSchedulerTest, snoozeAlert) {
    doSimpleTestSetup(true);

    /// check that a random alert token is ignored
    ASSERT_FALSE(m_alertScheduler->snoozeAlert(ALERT2_TOKEN, FUTURE_INSTANT));

    /// check that we succeed if the correct token is available
    ASSERT_TRUE(m_alertScheduler->snoozeAlert(ALERT1_TOKEN, FUTURE_INSTANT));
}

/**
 * Test deleting alerts
 */
TEST_F(AlertSchedulerTest, deleteAlert) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert1);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs);
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);

    /// if active alert and the token matches, ensure that we dont delete it (we deactivate the alert actually)
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(0);
    ASSERT_TRUE(m_alertScheduler->deleteAlert(ALERT1_TOKEN));

    /// check that a random alert token is ignored
    ASSERT_FALSE(m_alertScheduler->deleteAlert(ALERT2_TOKEN));

    /// if inactive alert, then check that we succeed if the correct token is available
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs);
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    ASSERT_TRUE(m_alertScheduler->deleteAlert(ALERT2_TOKEN));
}

/**
 * Test method that checks if an alert is active
 */
TEST_F(AlertSchedulerTest, isAlertActive) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// active alert
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert1);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs);
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);

    /// inactive alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);

    /// Success expected for active alert
    ASSERT_TRUE(m_alertScheduler->isAlertActive(alert1));

    /// Failure expected for inactive alert
    ASSERT_FALSE(m_alertScheduler->isAlertActive(alert2));
}

/**
 * Test to see if the correct context about the scheduler is obtained
 */
TEST_F(AlertSchedulerTest, getContextInfo) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// Schedule 2 alerts one of which is active.
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert1);
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, FUTURE_INSTANT);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs);
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);

    AlertScheduler::AlertsContextInfo resultContextInfo = m_alertScheduler->getContextInfo();

    const unsigned int expectedRemainingScheduledAlerts = 2;
    const unsigned int expectedRemainingActiveAlerts = 1;

    /// Check that 2 alerts were scheduled, one of which is active.
    ASSERT_EQ(resultContextInfo.scheduledAlerts.size(), expectedRemainingScheduledAlerts);
    ASSERT_EQ(resultContextInfo.activeAlerts.size(), expectedRemainingActiveAlerts);
}

/**
 * Test local stop on AlertScheduler
 */
TEST_F(AlertSchedulerTest, onLocalStop) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup(true);

    m_alertScheduler->onLocalStop();

    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(alert->getStopReason(), Alert::StopReason::LOCAL_STOP);
}

/**
 * Test if AlertScheduler clears data
 */
TEST_F(AlertSchedulerTest, clearData) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup(true);
    EXPECT_CALL(*(m_alertStorage.get()), clearDatabase()).Times(1);

    m_alertScheduler->clearData();

    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(alert->getStopReason(), Alert::StopReason::SHUTDOWN);
}

/**
 * Test if AlertScheduler clears data
 */
TEST_F(AlertSchedulerTest, clearDataLogout) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup(true);
    EXPECT_CALL(*(m_alertStorage.get()), clearDatabase()).Times(1);

    m_alertScheduler->clearData(Alert::StopReason::LOG_OUT);

    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(alert->getStopReason(), Alert::StopReason::LOG_OUT);
}

/**
 * Test if AlertScheduler shuts down appropriately
 */
TEST_F(AlertSchedulerTest, shutdown) {
    doSimpleTestSetup(true);

    m_alertScheduler->shutdown();

    const unsigned int expectedRemainingAlerts = 0;
    /// check if all scheduled alerts are cleared out
    ASSERT_EQ(m_alertScheduler->getContextInfo().scheduledAlerts.size(), expectedRemainingAlerts);
}

/**
 * Test Alert state change to Active on an inactive alert
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeStartedInactiveAlert) {
    const std::string testReason = "stateStarted";
    auto testState = AlertScheduler::State::STARTED;

    doSimpleTestSetup(false, true);

    /// check that we ignore inactive alerts
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(0);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
}

/**
 * Test Alert state change to Active on an active alert
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeStartedActiveAlert) {
    const std::string testReason = "stateStarted";
    auto testState = AlertScheduler::State::STARTED;

    doSimpleTestSetup(true, true);

    /// active alerts should be handled
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Stopped
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeStopped) {
    const std::string testReason = "stateStopped";
    auto testState = AlertScheduler::State::STOPPED;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Completed
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeCompleted) {
    const std::string testReason = "stateCompleted";
    auto testState = AlertScheduler::State::COMPLETED;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Snoozed
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeSnoozed) {
    const std::string testReason = "stateSnoozed";
    auto testState = AlertScheduler::State::SNOOZED;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Error on an active alert
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeErrorActiveAlert) {
    const std::string testReason = "stateError";
    auto testState = AlertScheduler::State::ERROR;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Error on an inactive alert
 */
TEST_F(AlertSchedulerTest, onAlertStateChangeErrorInactiveAlert) {
    const std::string testReason = "stateError";
    auto testState = AlertScheduler::State::ERROR;

    doSimpleTestSetup(false, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

}  // namespace test
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
