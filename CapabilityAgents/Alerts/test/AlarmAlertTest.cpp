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

#include "Alerts/Alarm.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace test {

static const std::string ALARM_DEFAULT_DATA = "alarm default data";
static const std::string ALARM_SHORT_DATA = "alarm short data";

class AlarmAlertTest : public ::testing::Test {
public:
    AlarmAlertTest();

    static std::unique_ptr<std::istream> alarmDefaultFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(ALARM_DEFAULT_DATA));
    }

    static std::unique_ptr<std::istream> alarmShortFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(ALARM_SHORT_DATA));
    }

    std::shared_ptr<Alarm> m_alarm;
};

AlarmAlertTest::AlarmAlertTest() : m_alarm{std::make_shared<Alarm>(alarmDefaultFactory, alarmShortFactory)} {
}

TEST_F(AlarmAlertTest, defaultAudio) {
    std::ostringstream oss;
    oss << m_alarm->getDefaultAudioFactory()()->rdbuf();

    ASSERT_EQ(ALARM_DEFAULT_DATA, oss.str());
}

TEST_F(AlarmAlertTest, shortAudio) {
    std::ostringstream oss;
    oss << m_alarm->getShortAudioFactory()()->rdbuf();

    ASSERT_EQ(ALARM_SHORT_DATA, oss.str());
}

TEST_F(AlarmAlertTest, testGetTypeName) {
    ASSERT_EQ(m_alarm->getTypeName(), Alarm::TYPE_NAME);
}

}  // namespace test
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
