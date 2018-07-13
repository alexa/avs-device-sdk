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

#include "Alerts/Alert.h"
#include "AVSCommon/Utils/Timing/TimeUtils.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace test {

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

class MockAlert : public Alert {
public:
    MockAlert() : Alert(defaultAudioFactory, shortAudioFactory) {
    }

    std::string getTypeName() const override;

private:
    static std::unique_ptr<std::istream> defaultAudioFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(DEFAULT_AUDIO));
    }
    static std::unique_ptr<std::istream> shortAudioFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(SHORT_AUDIO));
    }
};

std::string MockAlert::getTypeName() const {
    return ALERT_TYPE;
}

class MockRenderer : public renderer::RendererInterface {
public:
    void setObserver(std::shared_ptr<capabilityAgents::alerts::renderer::RendererObserverInterface> observer){};
    void start(
        std::function<std::unique_ptr<std::istream>()> audioFactory,
        const std::vector<std::string>& urls = std::vector<std::string>(),
        int loopCount = 0,
        std::chrono::milliseconds loopPause = std::chrono::milliseconds{0}){};
    void stop(){};
};

class AlertTest : public ::testing::Test {
public:
    AlertTest();

protected:
    std::shared_ptr<MockAlert> m_alert;
    std::shared_ptr<MockRenderer> m_renderer;
};

AlertTest::AlertTest() : m_alert{std::make_shared<MockAlert>()}, m_renderer{std::make_shared<MockRenderer>()} {
    m_alert->setRenderer(m_renderer);
}

const std::string getPayloadJson(bool inclToken, bool inclSchedTime, const std::string& schedTime) {
    std::string tokenJson;
    if (inclToken) {
        tokenJson = "\"token\": \"" + TOKEN_TEST + "\",";
    }

    std::string schedTimeJson;
    if (inclSchedTime) {
        schedTimeJson = "\"scheduledTime\": \"" + schedTime + "\",";
    }

    // clang-format off
    const std::string payloadJson =
        "{" +
            tokenJson +
            "\"type\": \"" + ALERT_TYPE + "\"," +                                  
            schedTimeJson +
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

TEST_F(AlertTest, defaultAudio) {
    std::ostringstream oss;
    oss << m_alert->getDefaultAudioFactory()()->rdbuf();

    ASSERT_EQ(DEFAULT_AUDIO, oss.str());
}

TEST_F(AlertTest, defaultShortAudio) {
    std::ostringstream oss;
    oss << m_alert->getShortAudioFactory()()->rdbuf();

    ASSERT_EQ(SHORT_AUDIO, oss.str());
}

TEST_F(AlertTest, testParseFromJsonHappyCase) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, true, SCHED_TIME);
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

TEST_F(AlertTest, testParseFromJsonMissingToken) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(false, true, SCHED_TIME);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY);
}

TEST_F(AlertTest, testParseFromJsonMissingSchedTime) {
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, false, SCHED_TIME);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY);
}

TEST_F(AlertTest, testParseFromJsonBadSchedTimeFormat) {
    const std::string schedTime{INVALID_FORMAT_SCHED_TIME};
    std::string errorMessage;
    const std::string payloadJson = getPayloadJson(true, true, schedTime);
    rapidjson::Document payload;
    payload.Parse(payloadJson);

    Alert::ParseFromJsonStatus resultStatus = m_alert->parseFromJson(payload, &errorMessage);

    ASSERT_EQ(resultStatus, Alert::ParseFromJsonStatus::INVALID_VALUE);
}

TEST_F(AlertTest, testSetStateActive) {
    m_alert->reset();
    ASSERT_EQ(m_alert->getState(), Alert::State::SET);
    m_alert->setStateActive();
    ASSERT_NE(m_alert->getState(), Alert::State::ACTIVE);

    m_alert->activate();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVATING);
    m_alert->setStateActive();
    ASSERT_EQ(m_alert->getState(), Alert::State::ACTIVE);
}

TEST_F(AlertTest, testDeactivate) {
    Alert::StopReason stopReason = Alert::StopReason::AVS_STOP;
    m_alert->deactivate(stopReason);
    ASSERT_EQ(m_alert->getState(), Alert::State::STOPPING);
    ASSERT_EQ(m_alert->getStopReason(), stopReason);
}

TEST_F(AlertTest, testSetTimeISO8601) {
    avsCommon::utils::timing::TimeUtils timeUtils;
    std::string schedTime{"2030-02-02T12:56:34+0000"};
    m_alert->setTime_ISO_8601(schedTime);
    int64_t unixTime = 0;
    timeUtils.convert8601TimeStringToUnix(schedTime, &unixTime);

    ASSERT_EQ(m_alert->getScheduledTime_ISO_8601(), schedTime);
    ASSERT_EQ(m_alert->getScheduledTime_Unix(), unixTime);
}

TEST_F(AlertTest, testSnoozeBadTime) {
    m_alert->reset();
    m_alert->snooze(INVALID_FORMAT_SCHED_TIME);
    ASSERT_NE(m_alert->getState(), Alert::State::SNOOZING);
}

TEST_F(AlertTest, testSnoozeHappyCase) {
    m_alert->reset();
    m_alert->snooze("2030-02-02T12:56:34+0000");
    ASSERT_EQ(m_alert->getState(), Alert::State::SNOOZING);
}

TEST_F(AlertTest, testSetLoopCountNegative) {
    int loopCount = -1;
    m_alert->setLoopCount(loopCount);
    ASSERT_NE(m_alert->getLoopCount(), loopCount);
}

TEST_F(AlertTest, testSetLoopCountHappyCase) {
    int loopCount = 3;
    m_alert->setLoopCount(loopCount);
    ASSERT_EQ(m_alert->getLoopCount(), loopCount);
}

TEST_F(AlertTest, testSetLoopPause) {
    std::chrono::milliseconds loopPause{900};
    m_alert->setLoopPause(loopPause);
    ASSERT_EQ(m_alert->getLoopPause(), loopPause);
}

TEST_F(AlertTest, testSetBackgroundAssetId) {
    std::string backgroundAssetId{"testAssetId"};
    m_alert->setBackgroundAssetId(backgroundAssetId);
    ASSERT_EQ(m_alert->getBackgroundAssetId(), backgroundAssetId);
}

TEST_F(AlertTest, testIsPastDue) {
    avsCommon::utils::timing::TimeUtils timeUtils;
    int64_t currentUnixTime = 0;
    timeUtils.getCurrentUnixTime(&currentUnixTime);
    m_alert->setTime_ISO_8601(TEST_DATE_IN_THE_FUTURE);
    ASSERT_FALSE(m_alert->isPastDue(currentUnixTime, std::chrono::seconds{1}));
    m_alert->setTime_ISO_8601(TEST_DATE_IN_THE_PAST);
    ASSERT_TRUE(m_alert->isPastDue(currentUnixTime, std::chrono::seconds{1}));
}

TEST_F(AlertTest, testStateToString) {
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

TEST_F(AlertTest, testStopReasonToString) {
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::UNSET), "UNSET");
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::AVS_STOP), "AVS_STOP");
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::LOCAL_STOP), "LOCAL_STOP");
    ASSERT_EQ(m_alert->stopReasonToString(Alert::StopReason::SHUTDOWN), "SHUTDOWN");
}

TEST_F(AlertTest, testParseFromJsonStatusToString) {
    ASSERT_EQ(m_alert->parseFromJsonStatusToString(Alert::ParseFromJsonStatus::OK), "OK");
    ASSERT_EQ(
        m_alert->parseFromJsonStatusToString(Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY),
        "MISSING_REQUIRED_PROPERTY");
    ASSERT_EQ(m_alert->parseFromJsonStatusToString(Alert::ParseFromJsonStatus::INVALID_VALUE), "INVALID_VALUE");
}

}  // namespace test
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
