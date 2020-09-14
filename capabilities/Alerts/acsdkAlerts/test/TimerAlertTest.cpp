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

#include "acsdkAlerts/Timer.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

static const std::string TIMER_DEFAULT_DATA = "timer default data";
static const std::string TIMER_SHORT_DATA = "timer short data";

class TimerAlertTest : public ::testing::Test {
public:
    TimerAlertTest();

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> timerDefaultFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(TIMER_DEFAULT_DATA)),
            avsCommon::utils::MediaType::MPEG);
    }

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> timerShortFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(TIMER_SHORT_DATA)),
            avsCommon::utils::MediaType::MPEG);
    }

    std::shared_ptr<Timer> m_timer;
};

TimerAlertTest::TimerAlertTest() : m_timer{std::make_shared<Timer>(timerDefaultFactory, timerShortFactory, nullptr)} {
}

TEST_F(TimerAlertTest, test_defaultAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_timer->getDefaultAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(TIMER_DEFAULT_DATA, oss.str());
}

TEST_F(TimerAlertTest, test_shortAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_timer->getShortAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(TIMER_SHORT_DATA, oss.str());
}

TEST_F(TimerAlertTest, test_getTypeName) {
    ASSERT_EQ(m_timer->getTypeName(), Timer::getTypeNameStatic());
}
}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
