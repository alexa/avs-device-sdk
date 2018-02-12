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

#include "Alerts/Timer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace test {

static const std::string TIMER_DEFAULT_DATA = "timer default data";
static const std::string TIMER_SHORT_DATA = "timer short data";

class TimerAlertTest : public ::testing::Test {
public:
    TimerAlertTest();

    static std::unique_ptr<std::istream> timerDefaultFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(TIMER_DEFAULT_DATA));
    }

    static std::unique_ptr<std::istream> timerShortFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(TIMER_SHORT_DATA));
    }

    std::shared_ptr<Timer> m_timer;
};

TimerAlertTest::TimerAlertTest() : m_timer{std::make_shared<Timer>(timerDefaultFactory, timerShortFactory)} {
}

TEST_F(TimerAlertTest, defaultAudio) {
    std::ostringstream oss;
    oss << m_timer->getDefaultAudioFactory()()->rdbuf();

    ASSERT_EQ(TIMER_DEFAULT_DATA, oss.str());
}

TEST_F(TimerAlertTest, shortAudio) {
    std::ostringstream oss;
    oss << m_timer->getShortAudioFactory()()->rdbuf();

    ASSERT_EQ(TIMER_SHORT_DATA, oss.str());
}

TEST_F(TimerAlertTest, testGetTypeName) {
    ASSERT_EQ(m_timer->getTypeName(), Timer::TYPE_NAME);
}
}  // namespace test
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
