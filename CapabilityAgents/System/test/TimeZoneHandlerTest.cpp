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

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/MockSetting.h>

#include "System/TimeZoneHandler.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;
using namespace alexaClientSDK::settings;
using namespace alexaClientSDK::settings::test;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::utils::memory;
using namespace testing;

/// The namespace for this capability agent.
constexpr char NAMESPACE[] = "System";

/// The Directive name to set the timezone
constexpr char SET_TIMEZONE_DIRECTIVE_NAME[] = "SetTimeZone";

/// Crafted message ID
constexpr char MESSAGE_ID[] = "1";

/// The timeout used throughout the tests.
static const auto TEST_TIMEOUT = std::chrono::seconds(5);

/// New York Timezone setting value.
static const std::string TIMEZONE_NEW_YORK = "America/New_York";
/// Default Timezone setting value.
static const std::string TIMEZONE_DEFAULT = "Etc/GMT";
/// New York Timezone as json value
static const std::string TIMEZONE_NEW_YORK_JSON = R"(")" + TIMEZONE_NEW_YORK + R"(")";
/// New York Json Payload
// clang-format off
static const std::string TIMEZONE_PAYLOAD_NEW_YORK =
    "{"
        R"("timeZone":)" + TIMEZONE_NEW_YORK_JSON +
    "}";
// clang-format on

class TimeZoneHandlerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    /// The mock for @c TimeZoneSetting.
    std::shared_ptr<MockSetting<TimeZoneSetting::ValueType>> m_mockTzSetting;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;

    /// The TimeZoneHandler to handle AVS TimeZone Setting Directives.
    std::shared_ptr<system::TimeZoneHandler> m_timeZoneHandler;

    /// The mock directive to send to the TimeZoneHandler.
    std::unique_ptr<StrictMock<MockDirectiveHandlerResult>> m_mockDirectiveHandlerResult;
};

void TimeZoneHandlerTest::SetUp() {
    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();
    m_mockDirectiveHandlerResult = make_unique<StrictMock<MockDirectiveHandlerResult>>();
    m_mockTzSetting = std::make_shared<MockSetting<TimeZoneSetting::ValueType>>(TIMEZONE_DEFAULT);

    m_timeZoneHandler = system::TimeZoneHandler::create(m_mockTzSetting, m_mockExceptionEncounteredSender);

    ASSERT_NE(m_timeZoneHandler, nullptr);
}

void TimeZoneHandlerTest::TearDown() {
}

/**
 * Test that TimeZoneHandler returns a nullptr with invalid args.
 */
TEST_F(TimeZoneHandlerTest, test_createWithoutTimezoneSetting) {
    std::shared_ptr<system::TimeZoneHandler> nullHandler =
        system::TimeZoneHandler::create(nullptr, m_mockExceptionEncounteredSender);

    ASSERT_EQ(nullHandler, nullptr);
}

/**
 * Test that TimeZoneHandler returns a nullptr with invalid args.
 */
TEST_F(TimeZoneHandlerTest, test_createWithoutExceptionSender) {
    std::shared_ptr<system::TimeZoneHandler> nullHandler = system::TimeZoneHandler::create(m_mockTzSetting, nullptr);

    ASSERT_EQ(nullHandler, nullptr);
}

/**
 * Test that TimeZoneHandler correctly handles a setTimeZone Directive
 */
TEST_F(TimeZoneHandlerTest, test_handleSetTimeZoneDirective) {
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE, SET_TIMEZONE_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TIMEZONE_PAYLOAD_NEW_YORK, attachmentManager, "");

    avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockTzSetting, setAvsChange(TIMEZONE_NEW_YORK)).WillOnce(InvokeWithoutArgs([&waitEvent] {
        waitEvent.wakeUp();
        return true;
    }));

    auto timeZoneCA = std::static_pointer_cast<CapabilityAgent>(m_timeZoneHandler);
    timeZoneCA->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    timeZoneCA->handleDirective(MESSAGE_ID);

    // Wait till last expectation is met.
    ASSERT_TRUE(waitEvent.wait(TEST_TIMEOUT));
}

/**
 * Test that TimeZoneHandler correctly handles a failed setting value.
 *
 * TODO(ACSDK-2544): This should send an exception back to AVS.
 */
TEST_F(TimeZoneHandlerTest, DISABLED_settingCallbackFails) {
    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(NAMESPACE, SET_TIMEZONE_DIRECTIVE_NAME, MESSAGE_ID);
    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, TIMEZONE_PAYLOAD_NEW_YORK, attachmentManager, "");

    avsCommon::utils::WaitEvent waitEvent;
    EXPECT_CALL(*m_mockTzSetting, setAvsChange(TIMEZONE_NEW_YORK)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockExceptionEncounteredSender, sendExceptionEncountered(_, _, _))
        .WillOnce(InvokeWithoutArgs([&waitEvent] { waitEvent.wakeUp(); }));

    auto timeZoneCA = std::static_pointer_cast<CapabilityAgent>(m_timeZoneHandler);
    timeZoneCA->preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    timeZoneCA->handleDirective(MESSAGE_ID);

    // Wait till last expectation is met.
    ASSERT_TRUE(waitEvent.wait(TEST_TIMEOUT));
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
