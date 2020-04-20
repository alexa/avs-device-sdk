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

#include <memory>

#include <AVSCommon/AVS/Attachment/MockAttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockLocaleAssetsManager.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/MockDeviceSettingStorage.h>
#include <Settings/MockSettingEventSender.h>
#include <Settings/Types/LocaleWakeWordsSetting.h>

#include "System/LocaleHandler.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;
using namespace alexaClientSDK::capabilityAgents::system;
using namespace alexaClientSDK::settings;
using namespace alexaClientSDK::settings::test;
using namespace alexaClientSDK::settings::storage::test;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment::test;
using namespace avsCommon::utils::json;
using namespace testing;

/// The namespace for this capability agent.
constexpr char NAMESPACE[] = "System";

/// Crafted message ID
constexpr char MESSAGE_ID[] = "1";

/// The value of the payload key for locales
static const std::string LOCALES_PAYLOAD_KEY = "locales";

/// A list of test locales.
static const std::set<std::string> TEST_LOCALES = {"en-US"};

/// A list of test supported wake words.
static const std::set<std::string> SUPPORTED_WAKE_WORDS = {"ALEXA", "ECHO"};

/// A list of test supported locales.
static const std::set<std::string> SUPPORTED_LOCALES = {"en-CA", "en-US"};

/// Default locale.
static const std::string DEFAULT_LOCALE = "en-CA";

/// The SetLocales directive signature.
static const avsCommon::avs::NamespaceAndName SET_WAKE_WORDS{NAMESPACE, "SetLocales"};

class LocaleHandlerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    /// The capability agent for handling modifying locales.
    std::shared_ptr<LocaleHandler> m_localeHandler;

    /// The DeviceSettingsManager used to generate the setting.
    std::shared_ptr<DeviceSettingsManager> m_deviceSettingsManager;

    /// The mock DeviceSettingStorage used to store the setting state.
    std::shared_ptr<MockDeviceSettingStorage> m_mockDeviceSettingStorage;

    /// The mock @c MessageSenderInterface for locale settings.
    std::shared_ptr<MockSettingEventSender> m_mockLocaleSettingMessageSender;

    /// The mock @c MessageSenderInterface for wake words settings.
    std::shared_ptr<MockSettingEventSender> m_mockWakeWordSettingMessageSender;

    // A mock instance of LocaleAssetsManagerInterface
    std::shared_ptr<MockLocaleAssetsManager> m_mockAssetsManager;

    /// The mock @c ExceptionEncounteredSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;

    /// The locale and wake words settings.
    std::shared_ptr<types::LocaleWakeWordsSetting> m_localeSetting;
};

void LocaleHandlerTest::SetUp() {
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
    m_deviceSettingsManager = std::make_shared<settings::DeviceSettingsManager>(customerDataManager);
    m_mockDeviceSettingStorage = std::make_shared<MockDeviceSettingStorage>();
    m_mockExceptionEncounteredSender =
        std::make_shared<avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender>();
    m_mockLocaleSettingMessageSender = std::make_shared<MockSettingEventSender>();
    m_mockWakeWordSettingMessageSender = std::make_shared<NiceMock<MockSettingEventSender>>();
    m_mockAssetsManager = std::make_shared<NiceMock<MockLocaleAssetsManager>>();

    // Assets manager by default will allow all operations.
    ON_CALL(*m_mockAssetsManager, getSupportedWakeWords(_))
        .WillByDefault(InvokeWithoutArgs([]() -> avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::WakeWordsSets {
            return {SUPPORTED_WAKE_WORDS};
        }));
    ON_CALL(*m_mockAssetsManager, getDefaultSupportedWakeWords())
        .WillByDefault(InvokeWithoutArgs([]() -> avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::WakeWordsSets {
            return {SUPPORTED_WAKE_WORDS};
        }));
    ON_CALL(*m_mockAssetsManager, getSupportedLocales()).WillByDefault(InvokeWithoutArgs([] {
        return SUPPORTED_LOCALES;
    }));
    ON_CALL(*m_mockAssetsManager, getDefaultLocale()).WillByDefault(InvokeWithoutArgs([] { return DEFAULT_LOCALE; }));
    ON_CALL(*m_mockAssetsManager, changeAssets(_, _)).WillByDefault(InvokeWithoutArgs([] { return true; }));

    EXPECT_CALL(*m_mockDeviceSettingStorage, loadSetting("System.locales"))
        .WillOnce(Return(std::make_pair(SettingStatus::SYNCHRONIZED, R"(["en-CA"])")));
    EXPECT_CALL(*m_mockDeviceSettingStorage, loadSetting("SpeechRecognizer.wakeWords"))
        .WillOnce(Return(std::make_pair(SettingStatus::SYNCHRONIZED, "")));

    // Allow AVS for events for wake words to be always sent successfully;
    auto settingSendEventSuccess = [](const std::string& value) {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    };

    // By default, all events can be sent succesfully.
    ON_CALL(*m_mockWakeWordSettingMessageSender, sendChangedEvent(_)).WillByDefault(Invoke(settingSendEventSuccess));
    ON_CALL(*m_mockWakeWordSettingMessageSender, sendReportEvent(_)).WillByDefault(Invoke(settingSendEventSuccess));
    ON_CALL(*m_mockLocaleSettingMessageSender, sendChangedEvent(_)).WillByDefault(Invoke(settingSendEventSuccess));
    ON_CALL(*m_mockLocaleSettingMessageSender, sendReportEvent(_)).WillByDefault(Invoke(settingSendEventSuccess));

    m_localeSetting = settings::types::LocaleWakeWordsSetting::create(
        m_mockLocaleSettingMessageSender,
        m_mockWakeWordSettingMessageSender,
        m_mockDeviceSettingStorage,
        m_mockAssetsManager);

    m_localeHandler = LocaleHandler::create(m_mockExceptionEncounteredSender, m_localeSetting);
    ASSERT_NE(m_localeHandler, nullptr);
}

void LocaleHandlerTest::TearDown() {
}

/**
 * Test that LocaleHandler returns a nullptr with invalid args.
 */
TEST_F(LocaleHandlerTest, test_createWithInvalidArgs) {
    ASSERT_EQ(LocaleHandler::create(nullptr, nullptr), nullptr);
    ASSERT_EQ(LocaleHandler::create(m_mockExceptionEncounteredSender, nullptr), nullptr);
    ASSERT_EQ(LocaleHandler::create(nullptr, m_localeSetting), nullptr);
}

/**
 * Test that LocaleHandler correctly handles a setLocales Directive
 */
TEST_F(LocaleHandlerTest, test_setLocalesDirectiveSuccess) {
    JsonGenerator payloadGenerator;
    payloadGenerator.addStringArray(LOCALES_PAYLOAD_KEY, TEST_LOCALES);

    std::promise<bool> messagePromise;

    // Expect that AVS events will be sent due to the handling of the directive.
    EXPECT_CALL(
        *m_mockLocaleSettingMessageSender,
        sendReportEvent(jsonUtils::convertToJsonString(std::set<std::string>({*TEST_LOCALES.begin()}))))
        .WillOnce(InvokeWithoutArgs([&messagePromise] {
            messagePromise.set_value(true);
            std::promise<bool> retPromise;
            retPromise.set_value(true);
            return retPromise.get_future();
        }));

    // Expect that settings will be set into database.
    EXPECT_CALL(*m_mockDeviceSettingStorage, storeSettings(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockDeviceSettingStorage, updateSettingStatus(_, _)).WillRepeatedly(Return(true));

    // Handle the directive

    auto attachmentManager = std::make_shared<StrictMock<MockAttachmentManager>>();
    auto avsMessageHeader =
        std::make_shared<AVSMessageHeader>(SET_WAKE_WORDS.nameSpace, SET_WAKE_WORDS.name, MESSAGE_ID);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, payloadGenerator.toString(), attachmentManager, "");

    auto capabilityAgent = std::static_pointer_cast<CapabilityAgent>(m_localeHandler);
    capabilityAgent->handleDirectiveImmediately(directive);

    // Check that an AVS event is sent with a ["en-CA"] payload.
    auto messageFuture = messagePromise.get_future();
    ASSERT_EQ(messageFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
