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
#include <acsdkAlertsInterfaces/AlertObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>

#include "acsdkAlerts/Alert.h"
#include "AVSCommon/Utils/Timing/TimeUtils.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

using namespace acsdkAlerts::renderer;
using namespace testing;

/// Token for testing.
static const std::string TOKEN_TEST("Token_Test");

/// Assets for testing.
static const size_t NUM_ASSETS{2};
static const std::string ASSET_ID1("assetId1");
static const std::string ASSET_ID2("assetId2");
static const std::string ASSET_PLAY_ORDER("[\"assetId1\",\"assetId2\"]");
static const std::string BACKGROUND_ALERT_ASSET("assetId2");
static const std::string ASSET_URL1("cid:Test1");
static const std::string ASSET_URL2("cid:Test2");

/// Alert type.
static const std::string ALERT_TYPE("MOCK_ALERT_TYPE");

/// Scheduled time.
static const std::string SCHED_TIME{"2030-01-01T12:34:56+0000"};
static const std::string INVALID_FORMAT_SCHED_TIME{"abc"};

/// A test date in the past with which to compare regular Alert timestamps.
static const std::string TEST_DATE_IN_THE_PAST{"2000-02-02T12:56:34+0000"};
/// A test date in the future with which to compare regular Alert timestamps.
static const std::string TEST_DATE_IN_THE_FUTURE{"2030-02-02T12:56:34+0000"};

/// Loop info.
static const long LOOP_COUNT{2};
static const long LOOP_PAUSE_MS{300};

/// Data to be made into a stringstream for testing purposes.
static const std::string DEFAULT_AUDIO{"default audio"};
static const std::string SHORT_AUDIO{"short audio"};

/// Label for testing.
static const std::string LABEL_TEST("Test label");

/// Original time for testing.
static const std::string ORIGINAL_TIME_TEST("17:00:00.000");
static const std::string INVALID_ORIGINAL_TIME_TEST{"-1:00:00.000"};

/// reason for alert state change
static const std::string INTERRUPTED("interrupted");

class MockAlert : public Alert {
public:
    MockAlert() : Alert(defaultAudioFactory, shortAudioFactory, nullptr) {
    }

    std::string getTypeName() const override;

private:
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> defaultAudioFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(DEFAULT_AUDIO)),
            avsCommon::utils::MediaType::MPEG);
    }
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> shortAudioFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(SHORT_AUDIO)), avsCommon::utils::MediaType::MPEG);
    }
};

std::string MockAlert::getTypeName() const {
    return ALERT_TYPE;
}

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

class MockAlertObserverInterface : public acsdkAlertsInterfaces::AlertObserverInterface {
public:
    void onAlertStateChange(const AlertObserverInterface::AlertInfo& alertInfo) override;
};

void MockAlertObserverInterface::onAlertStateChange(const AlertObserverInterface::AlertInfo& alertInfo) {
    return;
}

class AlertTest : public ::testing::Test {
public:
    AlertTest();

protected:
    std::shared_ptr<MockAlert> m_alert;
    std::shared_ptr<MockRenderer> m_renderer;
    MockAlertObserverInterface m_alertObserverInterface;
};

AlertTest::AlertTest() :
        m_alert{std::make_shared<MockAlert>()},
        m_renderer{std::make_shared<MockRenderer>()},
        m_alertObserverInterface{MockAlertObserverInterface()} {
    m_alert->setRenderer(m_renderer);
    m_alert->setObserver(&m_alertObserverInterface);
}

const std::string getPayloadJson(
    bool inclToken,
    bool inclSchedTime,
    const std::string& schedTime,
    const std::string& label = "",
    const std::string& originalTime = "") {
    std::string tokenJson;
    if (inclToken) {
        tokenJson = "\"token\": \"" + TOKEN_TEST + "\",";
    }

    std::string schedTimeJson;
    if (inclSchedTime) {
        schedTimeJson = "\"scheduledTime\": \"" + schedTime + "\",";
    }

    std::string labelJson;
    if (!label.empty()) {
        labelJson = "\"label\": \"" + label + "\",";
    }

    std::string originalTimeJson;
    if (!originalTime.empty()) {
        originalTimeJson = "\"originalTime\": \"" + originalTime + "\",";
    }

    // clang-format off
    const std::string payloadJson =
        "{" +
            tokenJson +
            "\"type\": \"" + ALERT_TYPE + "\"," +
            schedTimeJson +
            labelJson +
            originalTimeJson +
            "\"assets\": ["
                "{"
                    "\"assetId\": \"" + ASSET_ID1 + "\","
                    "\"url\": \"" + ASSET_URL1 + "\""
                "},"
                "{"
                    "\"assetId\": \"" + ASSET_ID2 + "\","
                    "\"url\": \"" + ASSET_URL2 + "\""
                "}"
            "],"
            "\"assetPlayOrder\": "  + ASSET_PLAY_ORDER + ","
            "\"backgroundAlertAsset\": \"" + BACKGROUND_ALERT_ASSET + "\","
            "\"loopCount\": " + std::to_string(LOOP_COUNT) + ","
            "\"loopPauseInMilliSeconds\": " + std::to_string(LOOP_PAUSE_MS) +
        "}";
    // clang-format on

    return payloadJson;
}

TEST_F(AlertTest, test_defaultAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_alert->getDefaultAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(DEFAULT_AUDIO, oss.str());
}

TEST_F(AlertTest, test_defaultShortAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_alert->getShortAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(SHORT_AUDIO, oss.str());
}

TEST_F(AlertTest, test_parseFromJsonHappyCase) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, true, SCHED_TIME, LABEL_TEST, ORIGINAL_TIME_TEST);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);
    Alert::AssetConfiguration assetConfiguration = m_alert->getAssetConfiguration();

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::OK);
    ASSERT_EQ(m_alert->getToken(), TOKEN_TEST);
    ASSERT_EQ(m_alert->getScheduledTime_ISO_8601(), SCHED_TIME);
    ASSERT_EQ(m_alert->getBackgroundAssetId(), BACKGROUND_ALERT_ASSET);
    ASSERT_EQ(m_alert->getLoopCount(), LOOP_COUNT);
    ASSERT_EQ(m_alert->getLoopPause(), std::chrono::milliseconds{LOOP_PAUSE_MS});
    ASSERT_EQ(m_alert->getOriginalTime(), m_alert->validateOriginalTimeString(ORIGINAL_TIME_TEST));
    ASSERT_EQ(m_alert->getLabel(), m_alert->validateLabelString(LABEL_TEST));

    auto alertInfo =
        m_alert->createAlertInfo(acsdkAlertsInterfaces::AlertObserverInterface::State::STARTED, INTERRUPTED);
    ASSERT_EQ(alertInfo.token, TOKEN_TEST);

    // ALERT_TYPE("MOCK_ALERT_TYPE") will be mapped to default value ALARM
    ASSERT_EQ(alertInfo.type, acsdkAlertsInterfaces::AlertObserverInterface::Type::ALARM);

    ASSERT_EQ(alertInfo.state, acsdkAlertsInterfaces::AlertObserverInterface::State::STARTED);
    ASSERT_EQ(alertInfo.scheduledTime, m_alert->getScheduledTime_Utc_TimePoint());
    ASSERT_EQ(alertInfo.originalTime, m_alert->validateOriginalTimeString(ORIGINAL_TIME_TEST));
    ASSERT_EQ(alertInfo.label, m_alert->validateLabelString(LABEL_TEST));
    ASSERT_EQ(alertInfo.reason, INTERRUPTED);

    std::vector<std::string> assetPlayOrderItems;
    assetPlayOrderItems.push_back(ASSET_ID1);
    assetPlayOrderItems.push_back(ASSET_ID2);
    ASSERT_EQ(assetConfiguration.assetPlayOrderItems, assetPlayOrderItems);

    std::unordered_map<std::string, Alert::Asset> assetsMap = assetConfiguration.assets;
    ASSERT_EQ(assetsMap.size(), NUM_ASSETS);
    ASSERT_EQ(assetsMap[ASSET_ID1].id, ASSET_ID1);
    ASSERT_EQ(assetsMap[ASSET_ID1].url, ASSET_URL1);
    ASSERT_EQ(assetsMap[ASSET_ID2].id, ASSET_ID2);
    ASSERT_EQ(assetsMap[ASSET_ID2].url, ASSET_URL2);
}

TEST_F(AlertTest, test_parseFromJsonMissingToken) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(false, true, SCHED_TIME);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY);
}

TEST_F(AlertTest, test_parseFromJsonMissingSchedTime) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, false, SCHED_TIME);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY);
}

TEST_F(AlertTest, test_parseFromJsonBadSchedTimeFormat) {
    const std::string schedTime{INVALID_FORMAT_SCHED_TIME};
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, true, schedTime);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::INVALID_VALUE);
}

TEST_F(AlertTest, test_parseFromJsonInvalidOriginalTime) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, true, SCHED_TIME, LABEL_TEST, INVALID_ORIGINAL_TIME_TEST);

    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::OK);
    ASSERT_FALSE(m_alert->getOriginalTime().hasValue());
    ASSERT_TRUE(m_alert->getLabel().hasValue());
    ASSERT_EQ(m_alert->getLabel().value(), LABEL_TEST);
}

TEST_F(AlertTest, test_parseFromJsonEmptyOriginalTimeAndLabel) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, true, SCHED_TIME);

    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::OK);
    ASSERT_FALSE(m_alert->getOriginalTime().hasValue());
    ASSERT_FALSE(m_alert->getLabel().hasValue());
}

TEST_F(AlertTest, test_setStateActiveValid) {
    m_alert->reset();

    std::string schedTime{"2030-02-02T12:56:34+0000"};
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    ASSERT_TRUE(dynamicData.timePoint.setTime_ISO_8601(schedTime));
    m_alert->setAlertData(nullptr, &dynamicData);

    // renderer should be started
    EXPECT_CALL(*(m_renderer.get()), start(_, _, _, _, _, _, _)).WillRepeatedly(Return());
    ASSERT_EQ(m_alert->getState(), Alert::State::SET);
    m_alert->setStateActive();
    ASSERT_NE(m_alert->getState(), Alert::State::ACTIVE);

    m_alert->activate();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVATING);
    m_alert->setStateActive();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVE);
}

TEST_F(AlertTest, test_setStateActiveInvalid) {
    m_alert->reset();

    // set a time in the past
    std::string schedTime{"1990-02-02T12:56:34+0000"};
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    ASSERT_TRUE(dynamicData.timePoint.setTime_ISO_8601(schedTime));
    m_alert->setAlertData(nullptr, &dynamicData);

    // renderer shouldn't be started
    EXPECT_CALL(*(m_renderer.get()), start(_, _, _, _, _, _, _)).Times(0);
    ASSERT_EQ(m_alert->getState(), Alert::State::SET);
    m_alert->setStateActive();
    ASSERT_NE(m_alert->getState(), Alert::State::ACTIVE);

    m_alert->activate();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVATING);
    m_alert->setStateActive();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVE);
}

TEST_F(AlertTest, test_deactivate) {
    Alert::StopReason stopReason = Alert::StopReason::AVS_STOP;
    m_alert->deactivate(stopReason);
    ASSERT_EQ(m_alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(m_alert->getStopReason(), stopReason);
}

TEST_F(AlertTest, test_setTimeISO8601) {
    avsCommon::utils::timing::TimeUtils timeUtils;
    std::string schedTime{"2030-02-02T12:56:34+0000"};
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    ASSERT_TRUE(dynamicData.timePoint.setTime_ISO_8601(schedTime));
    m_alert->setAlertData(nullptr, &dynamicData);
    int64_t unixTime = 0;
    timeUtils.convert8601TimeStringToUnix(schedTime, &unixTime);
    auto sec =
        std::chrono::duration_cast<std::chrono::seconds>(m_alert->getScheduledTime_Utc_TimePoint().time_since_epoch());

    ASSERT_EQ(m_alert->getScheduledTime_ISO_8601(), schedTime);
    ASSERT_EQ(m_alert->getScheduledTime_Unix(), unixTime);
    ASSERT_EQ(static_cast<int64_t>(sec.count()), unixTime);
}

TEST_F(AlertTest, test_updateScheduleActiveFailed) {
    m_alert->activate();
    m_alert->setStateActive();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVE);

    auto oldScheduledTime = m_alert->getScheduledTime_ISO_8601();
    ASSERT_FALSE(m_alert->updateScheduledTime("2030-02-02T12:56:34+0000"));
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVE);
    ASSERT_EQ(m_alert->getScheduledTime_ISO_8601(), oldScheduledTime);
}

TEST_F(AlertTest, test_updateScheduleBadTime) {
    auto oldScheduledTime = m_alert->getScheduledTime_ISO_8601();
    ASSERT_FALSE(m_alert->updateScheduledTime(INVALID_FORMAT_SCHED_TIME));
    ASSERT_EQ(m_alert->getScheduledTime_ISO_8601(), oldScheduledTime);
}

TEST_F(AlertTest, test_updateScheduleHappyCase) {
    m_alert->reset();
    ASSERT_TRUE(m_alert->updateScheduledTime("2030-02-02T12:56:34+0000"));
    ASSERT_EQ(m_alert->getState(), Alert::State::SET);
}

TEST_F(AlertTest, test_snoozeBadTime) {
    m_alert->reset();
    ASSERT_FALSE(m_alert->snooze(INVALID_FORMAT_SCHED_TIME));
    ASSERT_NE(m_alert->getState(), Alert::State::SNOOZING);
}

TEST_F(AlertTest, test_snoozeHappyCase) {
    m_alert->reset();
    ASSERT_TRUE(m_alert->snooze("2030-02-02T12:56:34+0000"));
    ASSERT_EQ(m_alert->getState(), Alert::State::SNOOZING);
}

TEST_F(AlertTest, test_setLoopCountNegative) {
    int loopCount = -1;
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    dynamicData.loopCount = loopCount;
    m_alert->setAlertData(nullptr, &dynamicData);
    ASSERT_NE(m_alert->getLoopCount(), loopCount);
}

TEST_F(AlertTest, test_setLoopCountHappyCase) {
    int loopCount = 3;
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    dynamicData.loopCount = loopCount;
    m_alert->setAlertData(nullptr, &dynamicData);
    ASSERT_EQ(m_alert->getLoopCount(), loopCount);
}

TEST_F(AlertTest, test_setLoopPause) {
    std::chrono::milliseconds loopPause{900};
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    dynamicData.assetConfiguration.loopPause = loopPause;
    m_alert->setAlertData(nullptr, &dynamicData);
    ASSERT_EQ(m_alert->getLoopPause(), loopPause);
}

TEST_F(AlertTest, test_setBackgroundAssetId) {
    std::unordered_map<std::string, Alert::Asset> assets;
    assets["testAssetId"] = Alert::Asset("testAssetId", "http://test.com/a");

    std::string backgroundAssetId{"testAssetId"};
    Alert::DynamicData dynamicData;
    m_alert->getAlertData(nullptr, &dynamicData);
    dynamicData.assetConfiguration.backgroundAssetId = backgroundAssetId;
    dynamicData.assetConfiguration.assets = assets;
    m_alert->setAlertData(nullptr, &dynamicData);
    ASSERT_EQ(m_alert->getBackgroundAssetId(), backgroundAssetId);
}

TEST_F(AlertTest, DISABLED_test_isPastDue) {
    Alert::DynamicData dynamicData;
    avsCommon::utils::timing::TimeUtils timeUtils;
    int64_t currentUnixTime = 0;
    timeUtils.getCurrentUnixTime(&currentUnixTime);

    m_alert->getAlertData(nullptr, &dynamicData);
    ASSERT_TRUE(dynamicData.timePoint.setTime_ISO_8601(TEST_DATE_IN_THE_FUTURE));
    m_alert->setAlertData(nullptr, &dynamicData);
    ASSERT_FALSE(m_alert->isPastDue(currentUnixTime, std::chrono::seconds{1}));

    m_alert->getAlertData(nullptr, &dynamicData);
    ASSERT_TRUE(dynamicData.timePoint.setTime_ISO_8601(TEST_DATE_IN_THE_PAST));
    m_alert->setAlertData(nullptr, &dynamicData);
    ASSERT_TRUE(m_alert->isPastDue(currentUnixTime, std::chrono::seconds{1}));
}

TEST_F(AlertTest, test_stateToString) {
    ASSERT_EQ(m_alert->stateToString(Alert::State::UNSET), "UNSET");
    ASSERT_EQ(m_alert->stateToString(Alert::State::SET), "SET");
    ASSERT_EQ(m_alert->stateToString(Alert::State::READY), "READY");
    ASSERT_EQ(m_alert->stateToString(Alert::State::ACTIVATING), "ACTIVATING");
    ASSERT_EQ(m_alert->stateToString(Alert::State::ACTIVE), "ACTIVE");
    ASSERT_EQ(m_alert->stateToString(Alert::State::SNOOZING), "SNOOZING");
    ASSERT_EQ(m_alert->stateToString(Alert::State::SNOOZED), "SNOOZED");
    ASSERT_EQ(m_alert->stateToString(Alert::State::STOPPING), "STOPPING");
    ASSERT_EQ(m_alert->stateToString(Alert::State::STOPPED), "STOPPED");
    ASSERT_EQ(m_alert->stateToString(Alert::State::COMPLETED), "COMPLETED");
}

TEST_F(AlertTest, test_stopReasonToString) {
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::UNSET), "UNSET");
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::AVS_STOP), "AVS_STOP");
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::LOCAL_STOP), "LOCAL_STOP");
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::SHUTDOWN), "SHUTDOWN");
}

TEST_F(AlertTest, test_parseFromJsonStatusToString) {
    ASSERT_EQ(m_alert->parseFromJsonStatusToString(Alert::ParseFromJsonStatus::OK), "OK");
    ASSERT_EQ(
        m_alert->parseFromJsonStatusToString(Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY),
        "MISSING_REQUIRED_PROPERTY");
    ASSERT_EQ(m_alert->parseFromJsonStatusToString(Alert::ParseFromJsonStatus::INVALID_VALUE), "INVALID_VALUE");
}

TEST_F(AlertTest, test_hasAssetHappy) {
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

    Alert::StaticData d;
    d.token = "aaa";
    d.dbId = 1;

    Alert::DynamicData e;
    e.assetConfiguration = c;

    ASSERT_TRUE(m_alert->setAlertData(&d, &e));
}

TEST_F(AlertTest, test_hasAssetBgAssetIdNotFoundOnAssets) {
    std::unordered_map<std::string, Alert::Asset> assets;
    assets["A"] = Alert::Asset("A", "http://test.com/a");
    assets["B"] = Alert::Asset("B", "http://test.com/a");

    std::vector<std::string> playOrderItems;
    playOrderItems.push_back("A");
    playOrderItems.push_back("B");

    Alert::AssetConfiguration c;
    c.assets = assets;
    c.assetPlayOrderItems = playOrderItems;
    c.backgroundAssetId = "C";
    c.loopPause = std::chrono::milliseconds(100);

    Alert::StaticData d;
    d.token = "aaa";
    d.dbId = 1;

    Alert::DynamicData e;
    e.assetConfiguration = c;

    ASSERT_FALSE(m_alert->setAlertData(&d, &e));
}

TEST_F(AlertTest, test_hasAssetOrderItemNotFoundOnAssets) {
    std::unordered_map<std::string, Alert::Asset> assets;
    assets["A"] = Alert::Asset("A", "http://test.com/a");
    assets["B"] = Alert::Asset("B", "http://test.com/a");

    std::vector<std::string> playOrderItems;
    playOrderItems.push_back("A");
    playOrderItems.push_back("B");
    playOrderItems.push_back("C");

    Alert::AssetConfiguration c;
    c.assets = assets;
    c.assetPlayOrderItems = playOrderItems;
    c.backgroundAssetId = "A";
    c.loopPause = std::chrono::milliseconds(100);

    Alert::StaticData d;
    d.token = "aaa";
    d.dbId = 1;

    Alert::DynamicData e;
    e.assetConfiguration = c;

    ASSERT_FALSE(m_alert->setAlertData(&d, &e));
}

TEST_F(AlertTest, test_focusChangeDuringActivation) {
    m_alert->reset();
    ASSERT_EQ(m_alert->getState(), Alert::State::SET);

    // Activate alert
    EXPECT_CALL(*(m_renderer.get()), start(_, _, _, _, _, _, _)).WillRepeatedly(Return());
    EXPECT_CALL(*(m_renderer.get()), stop()).WillOnce(Return());
    m_alert->setFocusState(avsCommon::avs::FocusState::BACKGROUND, avsCommon::avs::MixingBehavior::MAY_DUCK);
    m_alert->activate();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVATING);
    m_alert->setFocusState(avsCommon::avs::FocusState::FOREGROUND, avsCommon::avs::MixingBehavior::PRIMARY);

    m_alert->onRendererStateChange(RendererObserverInterface::State::STARTED, "started");
}

}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
