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

#include "Alerts/Reminder.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace test {

static const std::string REMINDER_DEFAULT_DATA = "reminder default data";
static const std::string REMINDER_SHORT_DATA = "reminder short data";

class ReminderAlertTest : public ::testing::Test {
public:
    ReminderAlertTest();

    static std::unique_ptr<std::istream> reminderDefaultFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(REMINDER_DEFAULT_DATA));
    }

    static std::unique_ptr<std::istream> reminderShortFactory() {
        return std::unique_ptr<std::stringstream>(new std::stringstream(REMINDER_SHORT_DATA));
    }
    std::shared_ptr<Reminder> m_reminder;
};

ReminderAlertTest::ReminderAlertTest() :
        m_reminder{std::make_shared<Reminder>(reminderDefaultFactory, reminderShortFactory)} {
}

TEST_F(ReminderAlertTest, defaultAudio) {
    std::ostringstream oss;
    oss << m_reminder->getDefaultAudioFactory()()->rdbuf();

    ASSERT_EQ(REMINDER_DEFAULT_DATA, oss.str());
}

TEST_F(ReminderAlertTest, shortAudio) {
    std::ostringstream oss;
    oss << m_reminder->getShortAudioFactory()()->rdbuf();

    ASSERT_EQ(REMINDER_SHORT_DATA, oss.str());
}

TEST_F(ReminderAlertTest, testGetTypeName) {
    ASSERT_EQ(m_reminder->getTypeName(), Reminder::TYPE_NAME);
}

}  // namespace test
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
