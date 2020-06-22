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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>

#include "acsdkAlerts/AlertScheduler.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

using namespace acsdkAlertsInterfaces;
using namespace testing;

/// Tokens for alerts.
static const std::string ALERT1_TOKEN = "token1";
static const std::string ALERT2_TOKEN = "token2";
static const std::string ALERT3_TOKEN = "token3";
static const std::string ALERT4_TOKEN = "token4";

/// Test alert type
static const std::string ALERT_TYPE = "TEST_ALERT_TYPE";

/// A schedule instant in the past for alerts.
static const std::string PAST_INSTANT = "2000-01-01T12:34:56+0000";

/// A schedule instant in the future for alerts.
static const std::string FUTURE_INSTANT_SUFFIX = "-01-01T12:34:56+0000";

/// Amount of time that the alert observer should wait for a task to finish.
static const std::chrono::milliseconds TEST_TIMEOUT{100};

/// Alert past due time limit.
static const std::chrono::seconds ALERT_PAST_DUE_TIME_LIMIT{10};

class MockRenderer : public renderer::RendererInterface {
public:
    MOCK_METHOD7(
        start,
        void(
            std::shared_ptr<acsdkAlerts::renderer::RendererObserverInterface> observer,
            std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> audioFactory,
            bool alarmVolumeRampEnabled,
            const std::vector<std::string>& urls,
            int loopCount,
            std::chrono::milliseconds loopPause,
            bool startWithPause));
    MOCK_METHOD0(stop, void());
};

class TestAlert : public Alert {
public:
    TestAlert() :
            Alert(defaultAudioFactory, shortAudioFactory, nullptr),
            m_alertType{ALERT_TYPE},
            m_renderer{std::make_shared<MockRenderer>()} {
        this->setRenderer(m_renderer);
    }

    TestAlert(const std::string& token, const std::string& schedTime) :
            Alert(defaultAudioFactory, shortAudioFactory, nullptr),
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
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> defaultAudioFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream("default audio")),
            avsCommon::utils::MediaType::MPEG);
    }

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> shortAudioFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream("short audio")),
            avsCommon::utils::MediaType::MPEG);
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
    bool load(
        std::vector<std::shared_ptr<Alert>>* alertContainer,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager) {
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

    MOCK_METHOD1(bulkErase, bool(const std::list<std::shared_ptr<Alert>>&));
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

    void onAlertStateChange(
        const std::string& alertToken,
        const std::string& alertType,
        AlertScheduler::State newState,
        const std::string& reason) {
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
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;
    std::shared_ptr<AlertScheduler> m_alertScheduler;
    std::shared_ptr<TestAlertObserver> m_testAlertObserver;
    std::shared_ptr<settings::DeviceSettingsManager> m_settingsManager;
};

static std::string getFutureInstant(int yearsPlus) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    tm utc_tm = *gmtime(&tt);
    return std::to_string(utc_tm.tm_year + 1900 + yearsPlus) + FUTURE_INSTANT_SUFFIX;
}

static std::string getTimeNow() {
    std::string timeNowStr;
    auto timeNow = std::chrono::system_clock::now();
    avsCommon::utils::timing::TimeUtils().convertTimeToUtcIso8601Rfc3339(timeNow, &timeNowStr);
    return timeNowStr;
}

AlertSchedulerTest::AlertSchedulerTest() :
        m_alertStorage{std::make_shared<MockAlertStorage>()},
        m_alertRenderer{std::make_shared<MockRenderer>()},
        m_alertPastDueTimeLimit{ALERT_PAST_DUE_TIME_LIMIT},
        m_metricRecorder{std::make_shared<NiceMock<avsCommon::utils::metrics::test::MockMetricRecorder>>()},
        m_alertScheduler{std::make_shared<AlertScheduler>(
            m_alertStorage,
            m_alertRenderer,
            m_alertPastDueTimeLimit,
            m_metricRecorder)},
        m_testAlertObserver{std::make_shared<TestAlertObserver>()} {
}

void AlertSchedulerTest::SetUp() {
    m_alertStorage->setOpenRetVal(true);
    m_settingsManager =
        std::make_shared<settings::DeviceSettingsManager>(std::make_shared<registrationManager::CustomerDataManager>());
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
    std::shared_ptr<TestAlert> alert = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    alertToAdd.push_back(alert);
    m_alertStorage->setAlerts(alertToAdd);

    if (initWithAlertObserver) {
        m_alertScheduler->initialize(m_testAlertObserver, m_settingsManager);
        ;
    } else {
        std::shared_ptr<AlertScheduler> alertSchedulerObs{std::make_shared<AlertScheduler>(
            m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
        m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);
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
TEST_F(AlertSchedulerTest, test_initialize) {
    /// check if init fails if scheduler is not available
    ASSERT_FALSE(m_alertScheduler->initialize(nullptr, nullptr));
    ASSERT_FALSE(m_alertScheduler->initialize(nullptr, m_settingsManager));

    /// check if init fails if a database for alerts cant be created
    m_alertStorage->setOpenRetVal(false);
    m_alertStorage->setCreateDatabaseRetVal(false);
    ASSERT_FALSE(m_alertScheduler->initialize(m_alertScheduler, m_settingsManager));

    /// check if init succeeds.
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    m_alertStorage->setOpenRetVal(true);

    ASSERT_TRUE(m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager, false));
}

/**
 * Test AlertScheduler getting focus
 */
TEST_F(AlertSchedulerTest, test_updateGetFocus) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup();

    // check if focus changes to foreground
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::FOREGROUND);
    ASSERT_EQ(m_alertScheduler->getFocusState(), avsCommon::avs::FocusState::FOREGROUND);

    // check if focus changes to background
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);
    ASSERT_EQ(m_alertScheduler->getFocusState(), avsCommon::avs::FocusState::BACKGROUND);

    // check alert state change if focus is gone
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::NONE);
    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
}

/**
 * Test scheduling alerts
 */
TEST_F(AlertSchedulerTest, DISABLED_test_scheduleAlert) {
    // check that a future alert is scheduled
    std::shared_ptr<TestAlert> alert1 = doSimpleTestSetup(true);
    ASSERT_TRUE(m_alertScheduler->scheduleAlert(alert1));

    // check that a future alert is not scheduled if you cant store the alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(1));
    m_alertStorage->setStoreRetVal(false);
    ASSERT_FALSE(m_alertScheduler->scheduleAlert(alert2));

    // check that past alerts cant be scheduled
    std::shared_ptr<TestAlert> alert3 = std::make_shared<TestAlert>(ALERT3_TOKEN, PAST_INSTANT);
    m_alertStorage->setStoreRetVal(true);
    ASSERT_TRUE(m_alertScheduler->scheduleAlert(alert2));
    ASSERT_FALSE(m_alertScheduler->scheduleAlert(alert3));
}

/**
 * Test reloading alerts from the database and simultaneously scheduling them.
 */
TEST_F(AlertSchedulerTest, DISABLED_test_reloadAlertsFromDatabaseWithScheduling) {
    /// check if reload with shouldScheduleAlerts set to true succeeds. Pass in 3 alerts of which 1 is expired. Only 2
    /// should actually remain in the end.
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    m_alertStorage->setOpenRetVal(true);
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// initialize the alert scheduler
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager, false);

    /// past alert
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, PAST_INSTANT);
    alertsToAdd.push_back(alert1);

    /// future active alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(1));
    alert2->activate();
    alert2->setStateActive();
    alertsToAdd.push_back(alert2);

    /// future inactive alert
    std::shared_ptr<TestAlert> alert3 = std::make_shared<TestAlert>(ALERT3_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert3);
    m_alertStorage->setAlerts(alertsToAdd);

    /// past alert should get removed
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);

    /// active alert should get modified
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(1);

    const bool shouldScheduleAlerts = true;

    ASSERT_TRUE(m_alertScheduler->reloadAlertsFromDatabase(m_settingsManager, shouldScheduleAlerts));

    const unsigned int expectedRemainingAlerts = 2;

    /// only the 2 future alerts remain
    ASSERT_EQ(m_alertScheduler->getContextInfo().scheduledAlerts.size(), expectedRemainingAlerts);
}

/**
 * Test reloading alerts from the database without scheduling.
 */
TEST_F(AlertSchedulerTest, test_reloadAlertsFromDatabaseWithoutScheduling) {
    /// check if reload with shouldScheduleAlerts set to false succeeds. Pass in 3 alerts of which 1 is expired. All
    /// alerts should remain in the end.
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    m_alertStorage->setOpenRetVal(true);
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// initialize the alert scheduler
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager, false);

    /// past alert
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, PAST_INSTANT);
    alertsToAdd.push_back(alert1);

    /// future active alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(1));
    alert2->activate();
    alert2->setStateActive();
    alertsToAdd.push_back(alert2);

    /// future inactive alert
    std::shared_ptr<TestAlert> alert3 = std::make_shared<TestAlert>(ALERT3_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert3);
    m_alertStorage->setAlerts(alertsToAdd);

    /// no alerts should be modified or erased
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(0);
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(0);

    const bool shouldScheduleAlerts = false;

    ASSERT_TRUE(m_alertScheduler->reloadAlertsFromDatabase(m_settingsManager, shouldScheduleAlerts));

    const unsigned int expectedRemainingAlerts = 3;

    /// all alerts should remain
    ASSERT_EQ(m_alertScheduler->getContextInfo().scheduledAlerts.size(), expectedRemainingAlerts);
}

/**
 * Test update alert scheduled time.
 */
TEST_F(AlertSchedulerTest, test_updateAlertTime) {
    // Schedule an alert and create an updated version with the same token.
    auto oldAlert = doSimpleTestSetup();
    auto newAlert = std::make_shared<TestAlert>(oldAlert->getToken(), getFutureInstant(2));

    ASSERT_NE(oldAlert->getScheduledTime_ISO_8601(), newAlert->getScheduledTime_ISO_8601());

    // Expect database to be updated with new schedule.
    EXPECT_CALL(*m_alertStorage, modify(_)).WillOnce(Return(true));

    // Call schedule alert for an alert that already exists.
    EXPECT_TRUE(m_alertScheduler->scheduleAlert(newAlert));
    ASSERT_EQ(oldAlert->getScheduledTime_ISO_8601(), newAlert->getScheduledTime_ISO_8601());
}

/**
 * Test update alert for new assets.
 */
TEST_F(AlertSchedulerTest, test_updateAlertAssets) {
    // Schedule an alert and create an updated verision with the same token.
    auto oldAlert = doSimpleTestSetup();

    auto newAlert = std::make_shared<TestAlert>(oldAlert->getToken(), oldAlert->getScheduledTime_ISO_8601());
    ASSERT_EQ(oldAlert->getScheduledTime_ISO_8601(), newAlert->getScheduledTime_ISO_8601());

    // Create assets for Alert.
    std::unordered_map<std::string, Alert::Asset> assets;
    assets["A"] = Alert::Asset("A", "http://test.com/a");
    assets["B"] = Alert::Asset("B", "http://test.com/a");

    std::vector<std::string> playOrderItems;
    playOrderItems.push_back("A");
    playOrderItems.push_back("B");

    Alert::AssetConfiguration c;
    c.assets = assets;
    c.assetPlayOrderItems = playOrderItems;
    c.backgroundAssetId = "A";
    c.loopPause = std::chrono::milliseconds(100);

    // Assign assets to the new alert.
    ASSERT_TRUE(newAlert->setAssetConfiguration(c));

    // Expect database to be updated with new schedule.
    EXPECT_CALL(*m_alertStorage, modify(_)).WillOnce(Return(true));

    // Call schedule alert for an alert that already exists.
    EXPECT_TRUE(m_alertScheduler->scheduleAlert(newAlert));
    ASSERT_EQ(oldAlert->getScheduledTime_ISO_8601(), newAlert->getScheduledTime_ISO_8601());

    // Check assets were assigned correctly.
    Alert::AssetConfiguration oldAlertAssets = oldAlert->getAssetConfiguration();
    Alert::AssetConfiguration newAlertAssets = newAlert->getAssetConfiguration();
    ASSERT_EQ(oldAlertAssets.assets.size(), newAlertAssets.assets.size());
    ASSERT_EQ(oldAlertAssets.assets["A"].id, newAlertAssets.assets["A"].id);
    ASSERT_EQ(oldAlertAssets.assets["A"].url, newAlertAssets.assets["A"].url);
    ASSERT_EQ(oldAlertAssets.assets["B"].id, newAlertAssets.assets["B"].id);
    ASSERT_EQ(oldAlertAssets.assets["B"].url, newAlertAssets.assets["B"].url);

    ASSERT_EQ(oldAlertAssets.assetPlayOrderItems, newAlertAssets.assetPlayOrderItems);
    ASSERT_EQ(oldAlertAssets.backgroundAssetId, newAlertAssets.backgroundAssetId);
    ASSERT_EQ(oldAlertAssets.loopPause, newAlertAssets.loopPause);
}

/**
 * Test update alert scheduled time to now will start rendering the alert.
 */
TEST_F(AlertSchedulerTest, test_rescheduleAlertNow) {
    // Schedule an alert and create an updated version with the same token.
    auto oldAlert = doSimpleTestSetup(/*activeAlert*/ false, /*initWithAlertObserver*/ true);
    auto newAlert = std::make_shared<TestAlert>(oldAlert->getToken(), getTimeNow());
    ASSERT_NE(oldAlert->getScheduledTime_ISO_8601(), newAlert->getScheduledTime_ISO_8601());

    // Expect database to be updated with new schedule.
    EXPECT_CALL(*m_alertStorage, modify(_)).WillOnce(Return(true));

    // Call schedule alert for an alert that already exists.
    EXPECT_TRUE(m_alertScheduler->scheduleAlert(newAlert));
    ASSERT_EQ(oldAlert->getScheduledTime_ISO_8601(), newAlert->getScheduledTime_ISO_8601());

    // Wait till alarm is ready to be rendered.
    EXPECT_TRUE(m_testAlertObserver->waitFor(AlertObserverInterface::State::READY));
}

/**
 * Test update alert scheduled time fails.
 */
TEST_F(AlertSchedulerTest, test_rescheduleAlertFails) {
    // Schedule an alert and create an updated version with the same token.
    auto oldAlert = doSimpleTestSetup();
    auto newAlert = std::make_shared<TestAlert>(oldAlert->getToken(), getFutureInstant(2));
    auto oldScheduledTime = oldAlert->getScheduledTime_ISO_8601();
    ASSERT_NE(newAlert->getScheduledTime_ISO_8601(), oldScheduledTime);

    // Simulate database failure.
    EXPECT_CALL(*m_alertStorage, modify(_)).WillOnce(Return(false));

    // Call schedule alert for an alert that already exists.
    EXPECT_FALSE(m_alertScheduler->scheduleAlert(newAlert));
    EXPECT_EQ(oldAlert->getScheduledTime_ISO_8601(), oldScheduledTime);
}

/**
 * Test snoozing alerts
 */
TEST_F(AlertSchedulerTest, test_snoozeAlert) {
    doSimpleTestSetup(true);

    // check that a random alert token is ignored
    ASSERT_FALSE(m_alertScheduler->snoozeAlert(ALERT2_TOKEN, getFutureInstant(1)));

    // check that we succeed if the correct token is available
    ASSERT_TRUE(m_alertScheduler->snoozeAlert(ALERT1_TOKEN, getFutureInstant(1)));
}

/**
 * Test deleting single alert
 */
TEST_F(AlertSchedulerTest, test_deleteAlertSingle) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert1);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);

    // if active alert and the token matches, ensure that we dont delete it (we deactivate the alert actually)
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(0);
    ASSERT_TRUE(m_alertScheduler->deleteAlert(ALERT1_TOKEN));

    // check that a random alert token is ignored
    ASSERT_TRUE(m_alertScheduler->deleteAlert(ALERT2_TOKEN));

    // if inactive alert, then check that we succeed if the correct token is available
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);
    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    ASSERT_TRUE(m_alertScheduler->deleteAlert(ALERT2_TOKEN));
}

/**
 * Test deleting multiple alerts - one at a time
 */
TEST_F(AlertSchedulerTest, test_bulkDeleteAlertsSingle) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(2));

    ON_CALL(*m_alertStorage.get(), bulkErase(_)).WillByDefault(Return(true));

    alertsToAdd.push_back(alert1);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);

    // Delete one existing
    ASSERT_EQ(m_alertScheduler->getAllAlerts().size(), 2u);
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT1_TOKEN}));
    ASSERT_EQ(m_alertScheduler->getAllAlerts().size(), 1u);

    // Delete one non-existing
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT3_TOKEN}));
    ASSERT_EQ(m_alertScheduler->getAllAlerts().size(), 1u);

    // Delete the last existing
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT2_TOKEN}));
    ASSERT_EQ(m_alertScheduler->getAllAlerts().size(), 0u);
}

/**
 * Test deleting multiple existing alerts
 */
TEST_F(AlertSchedulerTest, test_bulkDeleteAlertsMultipleExisting) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(2));

    ON_CALL(*m_alertStorage.get(), bulkErase(_)).WillByDefault(Return(true));

    alertsToAdd.push_back(alert1);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);

    // Delete multiple existing
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT1_TOKEN, ALERT2_TOKEN}));
    EXPECT_EQ(m_alertScheduler->getAllAlerts().size(), 0u);
}

/**
 * Test deleting multiple alerts, both existing and not
 */
TEST_F(AlertSchedulerTest, test_bulkDeleteAlertsMultipleMixed) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(2));

    ON_CALL(*m_alertStorage.get(), bulkErase(_)).WillByDefault(Return(true));

    alertsToAdd.push_back(alert1);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);

    // Delete multiple mixed
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT1_TOKEN, ALERT3_TOKEN}));
    EXPECT_EQ(m_alertScheduler->getAllAlerts().size(), 1u);
}

/**
 * Test deleting multiple non-existing alerts
 */
TEST_F(AlertSchedulerTest, test_bulkDeleteAlertsMultipleMissing) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(2));

    ON_CALL(*m_alertStorage.get(), bulkErase(_)).WillByDefault(Return(true));

    alertsToAdd.push_back(alert1);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);

    // Delete multiple non-existing
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT3_TOKEN, ALERT4_TOKEN}));
    EXPECT_EQ(m_alertScheduler->getAllAlerts().size(), 2u);
}

/**
 * Test deleting same alerts multiple times
 */
TEST_F(AlertSchedulerTest, test_bulkDeleteAlertsMultipleSame) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(2));

    ON_CALL(*m_alertStorage.get(), bulkErase(_)).WillByDefault(Return(true));

    alertsToAdd.push_back(alert1);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);

    // Delete same multiple times
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({ALERT1_TOKEN, ALERT1_TOKEN}));
    EXPECT_EQ(m_alertScheduler->getAllAlerts().size(), 1u);
}

/**
 * Test bulk deleting with empty list
 */
TEST_F(AlertSchedulerTest, test_bulkDeleteAlertsMultipleEmpty) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(2));

    ON_CALL(*m_alertStorage.get(), bulkErase(_)).WillByDefault(Return(true));

    alertsToAdd.push_back(alert1);
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);

    // Delete empty
    EXPECT_TRUE(m_alertScheduler->deleteAlerts({}));
    EXPECT_EQ(m_alertScheduler->getAllAlerts().size(), 2u);
}

/**
 * Test method that checks if an alert is active
 */
TEST_F(AlertSchedulerTest, test_isAlertActive) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// active alert
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert1);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);
    m_alertScheduler->updateFocus(avsCommon::avs::FocusState::BACKGROUND);

    /// inactive alert
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(1));
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
TEST_F(AlertSchedulerTest, test_getContextInfo) {
    std::shared_ptr<AlertScheduler> alertSchedulerObs{
        std::make_shared<AlertScheduler>(m_alertStorage, m_alertRenderer, m_alertPastDueTimeLimit, m_metricRecorder)};
    std::vector<std::shared_ptr<TestAlert>> alertsToAdd;

    /// Schedule 2 alerts one of which is active.
    std::shared_ptr<TestAlert> alert1 = std::make_shared<TestAlert>(ALERT1_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert1);
    std::shared_ptr<TestAlert> alert2 = std::make_shared<TestAlert>(ALERT2_TOKEN, getFutureInstant(1));
    alertsToAdd.push_back(alert2);
    m_alertStorage->setAlerts(alertsToAdd);
    m_alertScheduler->initialize(alertSchedulerObs, m_settingsManager);
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
TEST_F(AlertSchedulerTest, test_onLocalStop) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup(true);

    m_alertScheduler->onLocalStop();

    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(alert->getStopReason(), Alert::StopReason::LOCAL_STOP);
}

/**
 * Test if AlertScheduler clears data
 */
TEST_F(AlertSchedulerTest, test_clearData) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup(true);
    EXPECT_CALL(*(m_alertStorage.get()), clearDatabase()).Times(1);

    m_alertScheduler->clearData();

    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(alert->getStopReason(), Alert::StopReason::SHUTDOWN);
}

/**
 * Test if AlertScheduler clears data
 */
TEST_F(AlertSchedulerTest, test_clearDataLogout) {
    std::shared_ptr<TestAlert> alert = doSimpleTestSetup(true);
    EXPECT_CALL(*(m_alertStorage.get()), clearDatabase()).Times(1);

    m_alertScheduler->clearData(Alert::StopReason::LOG_OUT);

    ASSERT_EQ(alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(alert->getStopReason(), Alert::StopReason::LOG_OUT);
}

/**
 * Test if AlertScheduler shuts down appropriately
 */
TEST_F(AlertSchedulerTest, test_shutdown) {
    doSimpleTestSetup(true);

    m_alertScheduler->shutdown();

    const unsigned int expectedRemainingAlerts = 0;
    /// check if all scheduled alerts are cleared out
    ASSERT_EQ(m_alertScheduler->getContextInfo().scheduledAlerts.size(), expectedRemainingAlerts);
}

/**
 * Test Alert state change to Active on an inactive alert
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeStartedInactiveAlert) {
    const std::string testReason = "stateStarted";
    auto testState = AlertScheduler::State::STARTED;

    doSimpleTestSetup(false, true);

    /// check that we ignore inactive alerts
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(0);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
}

/**
 * Test Alert state change to Active on an active alert
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeStartedActiveAlert) {
    const std::string testReason = "stateStarted";
    auto testState = AlertScheduler::State::STARTED;

    doSimpleTestSetup(true, true);

    /// active alerts should be handled
    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Stopped
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeStopped) {
    const std::string testReason = "stateStopped";
    auto testState = AlertScheduler::State::STOPPED;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Completed
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeCompleted) {
    const std::string testReason = "stateCompleted";
    auto testState = AlertScheduler::State::COMPLETED;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Snoozed
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeSnoozed) {
    const std::string testReason = "stateSnoozed";
    auto testState = AlertScheduler::State::SNOOZED;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), modify(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Error on an active alert
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeErrorActiveAlert) {
    const std::string testReason = "stateError";
    auto testState = AlertScheduler::State::ERROR;

    doSimpleTestSetup(true, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

/**
 * Test Alert state change to Error on an inactive alert
 */
TEST_F(AlertSchedulerTest, test_onAlertStateChangeErrorInactiveAlert) {
    const std::string testReason = "stateError";
    auto testState = AlertScheduler::State::ERROR;

    doSimpleTestSetup(false, true);

    EXPECT_CALL(*(m_alertStorage.get()), erase(testing::_)).Times(1);
    m_alertScheduler->onAlertStateChange(ALERT1_TOKEN, ALERT_TYPE, testState, testReason);
    ASSERT_TRUE(m_testAlertObserver->waitFor(testState));
}

}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
