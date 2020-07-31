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

#include "acsdkAlerts/Reminder.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

static const std::string REMINDER_DEFAULT_DATA = "reminder default data";
static const std::string REMINDER_SHORT_DATA = "reminder short data";

class ReminderAlertTest : public ::testing::Test {
public:
    ReminderAlertTest();

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> reminderDefaultFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(REMINDER_DEFAULT_DATA)),
            avsCommon::utils::MediaType::MPEG);
    }

    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> reminderShortFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream(REMINDER_SHORT_DATA)),
            avsCommon::utils::MediaType::MPEG);
    }
    std::shared_ptr<Reminder> m_reminder;
};

ReminderAlertTest::ReminderAlertTest() :
        m_reminder{std::make_shared<Reminder>(reminderDefaultFactory, reminderShortFactory, nullptr)} {
}

TEST_F(ReminderAlertTest, test_defaultAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_reminder->getDefaultAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(REMINDER_DEFAULT_DATA, oss.str());
}

TEST_F(ReminderAlertTest, test_shortAudio) {
    std::ostringstream oss;
    auto audioStream = std::get<0>(m_reminder->getShortAudioFactory()());
    oss << audioStream->rdbuf();

    ASSERT_EQ(REMINDER_SHORT_DATA, oss.str());
}

TEST_F(ReminderAlertTest, test_getTypeName) {
    ASSERT_EQ(m_reminder->getTypeName(), Reminder::getTypeNameStatic());
}

}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
