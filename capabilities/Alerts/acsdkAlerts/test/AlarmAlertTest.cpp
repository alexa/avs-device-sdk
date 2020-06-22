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

#include "acsdkAlerts/Alarm.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

using namespace avsCommon::utils;

static const std::string ALARM_DEFAULT_DATA = "alarm default data";
static const std::string ALARM_SHORT_DATA = "alarm short data";

class AlarmAlertTest : public ::testing::Test {
public:
    AlarmAlertTest();

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> alarmDefaultFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(ALARM_DEFAULT_DATA)),
            avsCommon::utils::MediaType::MPEG);
    }

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> alarmShortFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(ALARM_SHORT_DATA)),
            avsCommon::utils::MediaType::MPEG);
    }

    std::shared_ptr<Alarm> m_alarm;
};

AlarmAlertTest::AlarmAlertTest() : m_alarm{std::make_shared<Alarm>(alarmDefaultFactory, alarmShortFactory, nullptr)} {
}

TEST_F(AlarmAlertTest, test_defaultAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_alarm->getDefaultAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(ALARM_DEFAULT_DATA, oss.str());
}

TEST_F(AlarmAlertTest, test_shortAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_alarm->getShortAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(ALARM_SHORT_DATA, oss.str());
}

TEST_F(AlarmAlertTest, test_getTypeName) {
    ASSERT_EQ(m_alarm->getTypeName(), Alarm::getTypeNameStatic());
}

}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
