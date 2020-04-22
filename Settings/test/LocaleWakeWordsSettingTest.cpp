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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/MockLocaleAssetsManager.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingObserverInterface.h>
#include <Settings/SettingStringConversion.h>
#include <Settings/Types/LocaleWakeWordsSetting.h>

#include "Settings/MockDeviceSettingStorage.h"
#include "Settings/MockSettingEventSender.h"
#include "Settings/MockSettingObserver.h"

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils;
using namespace storage::test;
using namespace testing;
using namespace types;

/// Constant representing "ALEXA" enabled.
static const std::string ALEXA_VALUE = "ALEXA";

/// Constant representing "ECHO" enabled value.
static const std::string ECHO_VALUE = "ECHO";

/// Constant representing "en-CA" enabled db value.
static const std::string ENGLISH_CANADA_VALUE = "en-CA";

/// Constant representing "fr-CA" enabled db value.
static const std::string FRENCH_CANADA_VALUE = "fr-CA";

/// Constant representing an invalid wake word / locale value.
static const std::string INVALID_VALUE = "INVALID";

/// Constant representing the timeout for test events.
/// @note Use a large enough value that should not fail even in slower systems.
static const std::chrono::seconds MY_WAIT_TIMEOUT{5};

/// Set of combinations of supported wake words.
static const MockLocaleAssetsManager::LocaleAssetsManagerInterface::WakeWordsSets SUPPORTED_WAKE_WORDS_COMBINATION{
    {ALEXA_VALUE},
    {ECHO_VALUE},
    {ALEXA_VALUE, ECHO_VALUE}};

/// Set of supported locales.
static const std::set<std::string> SUPPORTED_LOCALES{ENGLISH_CANADA_VALUE, FRENCH_CANADA_VALUE};

/// A vector of locales that contains only en-CA.
static const std::vector<std::string> ENGLISH_LOCALES{ENGLISH_CANADA_VALUE};

/// The database key to be used to save wake words.
static const std::string WAKE_WORDS_KEY = "SpeechRecognizer.wakeWords";

/// The database key to be used to save wake words.
static const std::string LOCALES_KEY = "System.locales";

/**
 * The test class.
 */
class LocaleWakeWordsSettingTest : public Test {
protected:
    /**
     * Create protocol object and dependencies mocks.
     */
    void SetUp() override;

    /**
     * Function to generate the json representation of wake words and locales.
     *
     * @param values The values to be converted to a json string array.
     * @return A string representing the json string array.
     */
    std::string toJson(std::set<std::string> values) const;

    /**
     * Function used to create and initialize the setting with the given values. Function will also add the observers.
     *
     * This function asserts that the object is created successfully.
     *
     * @param locale The initial value for locale.
     * @param wakeWords The initial value for wakeWords.
     */
    void initializeSetting(const LocalesSetting::ValueType& locale, const WakeWordsSetting::ValueType wakeWords);

    /// Mock event sender for wake words.
    std::shared_ptr<MockSettingEventSender> m_wakeWordsSenderMock;

    /// Mock event sender for locales.
    std::shared_ptr<MockSettingEventSender> m_localesSenderMock;

    /// Mock setting storage
    std::shared_ptr<MockDeviceSettingStorage> m_storageMock;

    /// Mock locale assets manager.
    std::shared_ptr<MockLocaleAssetsManager> m_assetsManagerMock;

    /// Mock locale setting observer.
    std::shared_ptr<MockSettingObserver<LocalesSetting>> m_localeObserver;

    /// Mock wake words setting observer.
    std::shared_ptr<MockSettingObserver<WakeWordsSetting>> m_wakeWordsObserver;

    /// The class under test.
    std::shared_ptr<LocaleWakeWordsSetting> m_setting;
};

void LocaleWakeWordsSettingTest::SetUp() {
    m_wakeWordsSenderMock = std::make_shared<StrictMock<MockSettingEventSender>>();
    m_localesSenderMock = std::make_shared<StrictMock<MockSettingEventSender>>();
    m_storageMock = std::make_shared<StrictMock<MockDeviceSettingStorage>>();
    m_assetsManagerMock = std::make_shared<StrictMock<MockLocaleAssetsManager>>();
    m_localeObserver = std::make_shared<MockSettingObserver<LocalesSetting>>();
    m_wakeWordsObserver = std::make_shared<MockSettingObserver<WakeWordsSetting>>();

    EXPECT_CALL(*m_assetsManagerMock, getDefaultLocale()).WillRepeatedly(Return(ENGLISH_CANADA_VALUE));
    EXPECT_CALL(*m_assetsManagerMock, getSupportedLocales()).WillRepeatedly(Return(SUPPORTED_LOCALES));
    EXPECT_CALL(*m_assetsManagerMock, getDefaultSupportedWakeWords())
        .WillRepeatedly(Return(SUPPORTED_WAKE_WORDS_COMBINATION));
    EXPECT_CALL(*m_assetsManagerMock, getSupportedWakeWords(_))
        .WillRepeatedly(Return(SUPPORTED_WAKE_WORDS_COMBINATION));

    // Destructor might be called before a request is cleared. This should not be a problem, so expect 0..1 cancel call.
    EXPECT_CALL(*m_assetsManagerMock, cancelOngoingChange()).Times(AtMost(1));
}

std::string LocaleWakeWordsSettingTest::toJson(std::set<std::string> values) const {
    return jsonUtils::convertToJsonString(values);
}

/**
 * Creates a vector of tuples given of setting key, value, and status
 *
 * @param key The setting key.
 * @param value The setting value.
 * @param status The setting status.
 * @return The setting vector tuple.
 */
std::vector<std::tuple<std::string, std::string, SettingStatus>> convertToDBData(
    std::string key,
    std::string value,
    SettingStatus status) {
    std::vector<std::tuple<std::string, std::string, SettingStatus>> datum;
    datum.push_back(std::make_tuple(key, value, status));
    return datum;
}

void LocaleWakeWordsSettingTest::initializeSetting(
    const LocalesSetting::ValueType& locales,
    const WakeWordsSetting::ValueType wakeWords) {
    WaitEvent e;

    EXPECT_CALL(*m_assetsManagerMock, getDefaultLocale()).WillOnce(Return(FRENCH_CANADA_VALUE));
    EXPECT_CALL(*m_storageMock, loadSetting(LOCALES_KEY))
        .WillOnce(Return(
            std::make_pair(SettingStatus::SYNCHRONIZED, settings::toSettingString<DeviceLocales>(locales).second)));
    EXPECT_CALL(*m_storageMock, loadSetting(WAKE_WORDS_KEY))
        .WillOnce(Return(std::make_pair(SettingStatus::SYNCHRONIZED, jsonUtils::convertToJsonString(wakeWords))));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(locales, wakeWords)).WillOnce(InvokeWithoutArgs([&e] {
        e.wakeUp();
        return true;
    }));

    m_setting =
        LocaleWakeWordsSetting::create(m_localesSenderMock, m_wakeWordsSenderMock, m_storageMock, m_assetsManagerMock);
    ASSERT_THAT(m_setting, NotNull());

    LocalesSetting& localeSetting = *m_setting;
    WakeWordsSetting& wakeWordsSetting = *m_setting;
    ASSERT_EQ(localeSetting.get(), locales);
    ASSERT_EQ(wakeWordsSetting.get(), wakeWords);

    localeSetting.addObserver(m_localeObserver);
    wakeWordsSetting.addObserver(m_wakeWordsObserver);

    e.wait(MY_WAIT_TIMEOUT);
}

/// Test create with null event senders.
TEST_F(LocaleWakeWordsSettingTest, test_nullEventSenders) {
    ASSERT_FALSE(LocaleWakeWordsSetting::create(nullptr, m_wakeWordsSenderMock, m_storageMock, m_assetsManagerMock));
    ASSERT_FALSE(LocaleWakeWordsSetting::create(m_localesSenderMock, nullptr, m_storageMock, m_assetsManagerMock));
}

/// Test create with null storage.
TEST_F(LocaleWakeWordsSettingTest, test_nullStorage) {
    ASSERT_FALSE(
        LocaleWakeWordsSetting::create(m_localesSenderMock, m_wakeWordsSenderMock, nullptr, m_assetsManagerMock));
}

/// Test create with null assets manager.
TEST_F(LocaleWakeWordsSettingTest, test_nullAssetsManager) {
    ASSERT_FALSE(LocaleWakeWordsSetting::create(m_localesSenderMock, m_wakeWordsSenderMock, m_storageMock, nullptr));
}

/// Test restore when all values are available and synchronized.
TEST_F(LocaleWakeWordsSettingTest, test_restoreSynchronized) {
    initializeSetting({ENGLISH_CANADA_VALUE}, WakeWordsSetting::ValueType({ALEXA_VALUE}));
}

/// Test success flow for avs wake word request.
TEST_F(LocaleWakeWordsSettingTest, test_AVSChangeWakeWordsRequest) {
    initializeSetting({ENGLISH_CANADA_VALUE}, WakeWordsSetting::ValueType({ALEXA_VALUE}));
    WakeWordsSetting::ValueType newWakeWords{ALEXA_VALUE, ECHO_VALUE};
    auto newWakeWordsJson = toJson({ALEXA_VALUE, ECHO_VALUE});

    WaitEvent event;

    EXPECT_CALL(
        *m_storageMock,
        storeSettings(convertToDBData(WAKE_WORDS_KEY, newWakeWordsJson, SettingStatus::AVS_CHANGE_IN_PROGRESS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(ENGLISH_LOCALES, newWakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(newWakeWords, SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_wakeWordsSenderMock, sendReportEvent(newWakeWordsJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    {
        InSequence dummy;
        EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
            .WillOnce(Return(true));
        EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED))
            .WillOnce(InvokeWithoutArgs([&event] {
                event.wakeUp();
                return true;
            }));
    }

    m_setting->setAvsChange(newWakeWords);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<WakeWordsSetting>>(m_setting)->get(), newWakeWords);
}

/// Test that if the set value fails for avs request, the value doesn't change and report event is sent with original
/// value.
TEST_F(LocaleWakeWordsSettingTest, test_AVSChangeWakeWordsRequestSetFailed) {
    WakeWordsSetting::ValueType initialWakeWords{ALEXA_VALUE};
    initializeSetting({ENGLISH_CANADA_VALUE}, initialWakeWords);
    WakeWordsSetting::ValueType newWakeWords{ALEXA_VALUE, ECHO_VALUE};

    WaitEvent event;
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(ENGLISH_LOCALES, newWakeWords)).WillOnce(Return(false));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(initialWakeWords, SettingNotifications::AVS_CHANGE_FAILED));
    EXPECT_CALL(*m_wakeWordsSenderMock, sendReportEvent(toJson({ALEXA_VALUE}))).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    {
        InSequence dummy;
        EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
            .WillOnce(Return(true));
        EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED))
            .WillOnce(InvokeWithoutArgs([&event] {
                event.wakeUp();
                return true;
            }));
    }

    m_setting->setAvsChange(newWakeWords);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<WakeWordsSetting>>(m_setting)->get(), initialWakeWords);
}

/// Test send event failed for avs request.
TEST_F(LocaleWakeWordsSettingTest, test_AVSChangeWakeWordsRequestSendEventFailed) {
    initializeSetting({ENGLISH_CANADA_VALUE}, WakeWordsSetting::ValueType({ALEXA_VALUE}));
    WakeWordsSetting::ValueType newWakeWords{ALEXA_VALUE, ECHO_VALUE};
    auto newWakeWordsJson = toJson({ALEXA_VALUE, ECHO_VALUE});

    WaitEvent event;
    EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED)).Times(0);
    EXPECT_CALL(
        *m_storageMock,
        storeSettings(convertToDBData(WAKE_WORDS_KEY, newWakeWordsJson, SettingStatus::AVS_CHANGE_IN_PROGRESS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(ENGLISH_LOCALES, newWakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(newWakeWords, SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_wakeWordsSenderMock, sendReportEvent(newWakeWordsJson)).WillOnce(InvokeWithoutArgs([&] {
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        event.wakeUp();
        return retPromise.get_future();
    }));

    m_setting->setAvsChange(newWakeWords);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<WakeWordsSetting>>(m_setting)->get(), newWakeWords);
}

/// Test success flow for avs locale request.
TEST_F(LocaleWakeWordsSettingTest, test_AVSChangeLocaleRequest) {
    WakeWordsSetting::ValueType wakeWords{ALEXA_VALUE};
    initializeSetting({ENGLISH_CANADA_VALUE}, wakeWords);

    LocalesSetting::ValueType newLocale{FRENCH_CANADA_VALUE};
    auto newLocaleJson = toSettingString<DeviceLocales>(newLocale).second;

    WaitEvent event;
    EXPECT_CALL(
        *m_storageMock,
        storeSettings(convertToDBData(LOCALES_KEY, newLocaleJson, SettingStatus::AVS_CHANGE_IN_PROGRESS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_localeObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(newLocale, wakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_localeObserver, onSettingNotification(newLocale, SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_localesSenderMock, sendReportEvent(newLocaleJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    InSequence dummy;
    {
        EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
            .WillOnce(Return(true));

        EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::SYNCHRONIZED))
            .WillOnce(InvokeWithoutArgs([&event] {
                event.wakeUp();
                return true;
            }));
    }

    m_setting->setAvsChange(newLocale);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<LocalesSetting>>(m_setting)->get(), newLocale);
}

/// Test set value failed for avs request.
TEST_F(LocaleWakeWordsSettingTest, test_AVSChangeLocaleRequestSetFailed) {
    WakeWordsSetting::ValueType wakeWords{ALEXA_VALUE};
    initializeSetting({ENGLISH_CANADA_VALUE}, wakeWords);
    LocalesSetting::ValueType newLocale{FRENCH_CANADA_VALUE};
    auto initialLocaleJson = toSettingString<DeviceLocales>({ENGLISH_CANADA_VALUE}).second;

    WaitEvent event;
    EXPECT_CALL(*m_localeObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(newLocale, wakeWords)).WillOnce(Return(false));
    EXPECT_CALL(
        *m_localeObserver,
        onSettingNotification(DeviceLocales({ENGLISH_CANADA_VALUE}), SettingNotifications::AVS_CHANGE_FAILED));
    EXPECT_CALL(*m_localesSenderMock, sendReportEvent(initialLocaleJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    InSequence dummy;
    {
        EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
            .WillOnce(Return(true));
        EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::SYNCHRONIZED))
            .WillOnce(InvokeWithoutArgs([&event] {
                event.wakeUp();
                return true;
            }));
    }

    m_setting->setAvsChange(newLocale);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(
        static_cast<std::shared_ptr<LocalesSetting>>(m_setting)->get(),
        LocalesSetting::ValueType{ENGLISH_CANADA_VALUE});
}

/// Test send event failed for avs request.
TEST_F(LocaleWakeWordsSettingTest, test_AVSChangeLocaleRequestSendEventFailed) {
    WakeWordsSetting::ValueType wakeWords{ALEXA_VALUE};
    initializeSetting({ENGLISH_CANADA_VALUE}, wakeWords);

    LocalesSetting::ValueType newLocale{FRENCH_CANADA_VALUE};
    auto newLocaleJson = toSettingString<DeviceLocales>(newLocale).second;

    WaitEvent event;
    EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED)).Times(0);
    EXPECT_CALL(
        *m_storageMock,
        storeSettings(convertToDBData(LOCALES_KEY, newLocaleJson, SettingStatus::AVS_CHANGE_IN_PROGRESS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_localeObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(newLocale, wakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_localeObserver, onSettingNotification(newLocale, SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_localesSenderMock, sendReportEvent(newLocaleJson)).WillOnce(InvokeWithoutArgs([&] {
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        event.wakeUp();
        return retPromise.get_future();
    }));

    m_setting->setAvsChange(newLocale);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<LocalesSetting>>(m_setting)->get(), newLocale);
}

/// Test restore when no value is available in the database. This should run local change to both wake words and locale.
TEST_F(LocaleWakeWordsSettingTest, test_restoreValueNotAvailable) {
    EXPECT_CALL(*m_assetsManagerMock, getDefaultLocale()).WillOnce(Return(FRENCH_CANADA_VALUE));
    EXPECT_CALL(*m_storageMock, loadSetting(LOCALES_KEY))
        .WillOnce(Return(std::make_pair(SettingStatus::NOT_AVAILABLE, "")));
    EXPECT_CALL(*m_storageMock, loadSetting(WAKE_WORDS_KEY))
        .WillOnce(Return(std::make_pair(SettingStatus::NOT_AVAILABLE, "")));

    WakeWordsSetting::ValueType initialWakeWords{{ALEXA_VALUE}};
    LocalesSetting::ValueType initialLocale{FRENCH_CANADA_VALUE};
    auto wakeWordsJson = toJson({ALEXA_VALUE});
    auto localeJson = toSettingString<DeviceLocales>(DeviceLocales{FRENCH_CANADA_VALUE}).second;

    std::vector<std::tuple<std::string, std::string, SettingStatus>> datum;
    datum.push_back(std::make_tuple(LOCALES_KEY, localeJson, SettingStatus::LOCAL_CHANGE_IN_PROGRESS));
    datum.push_back(std::make_tuple(WAKE_WORDS_KEY, wakeWordsJson, SettingStatus::LOCAL_CHANGE_IN_PROGRESS));

    EXPECT_CALL(*m_storageMock, storeSettings(datum)).WillOnce(Return(true));

    EXPECT_CALL(*m_assetsManagerMock, changeAssets(initialLocale, initialWakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsSenderMock, sendChangedEvent(wakeWordsJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_localesSenderMock, sendChangedEvent(localeJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));

    std::condition_variable updatedCV;
    bool wakeWordStateUpdated = false;
    bool localeStateUpdated = false;
    std::mutex statusMutex;

    EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&wakeWordStateUpdated, &statusMutex, &updatedCV] {
            std::lock_guard<std::mutex> lock{statusMutex};
            wakeWordStateUpdated = true;
            updatedCV.notify_all();
            return true;
        }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&localeStateUpdated, &statusMutex, &updatedCV] {
            std::lock_guard<std::mutex> lock{statusMutex};
            localeStateUpdated = true;
            updatedCV.notify_all();
            return true;
        }));

    m_setting =
        LocaleWakeWordsSetting::create(m_localesSenderMock, m_wakeWordsSenderMock, m_storageMock, m_assetsManagerMock);
    ASSERT_THAT(m_setting, NotNull());

    std::unique_lock<std::mutex> statusLock{statusMutex};
    EXPECT_TRUE(updatedCV.wait_for(statusLock, MY_WAIT_TIMEOUT, [&wakeWordStateUpdated, &localeStateUpdated] {
        return wakeWordStateUpdated && localeStateUpdated;
    }));

    EXPECT_EQ(static_cast<std::shared_ptr<LocalesSetting>>(m_setting)->get(), initialLocale);
    EXPECT_EQ(static_cast<std::shared_ptr<WakeWordsSetting>>(m_setting)->get(), initialWakeWords);
}

/// Test success flow for local request for wake word change.
TEST_F(LocaleWakeWordsSettingTest, test_localChangeWakeWordsRequest) {
    initializeSetting({ENGLISH_CANADA_VALUE}, WakeWordsSetting::ValueType({ALEXA_VALUE}));
    WakeWordsSetting::ValueType newWakeWords{ALEXA_VALUE, ECHO_VALUE};
    auto newWakeWordsJson = toJson({ALEXA_VALUE, ECHO_VALUE});

    WaitEvent event;
    EXPECT_CALL(
        *m_storageMock,
        storeSettings(convertToDBData(WAKE_WORDS_KEY, newWakeWordsJson, SettingStatus::LOCAL_CHANGE_IN_PROGRESS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(_, SettingNotifications::LOCAL_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(ENGLISH_LOCALES, newWakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(newWakeWords, SettingNotifications::LOCAL_CHANGE));
    EXPECT_CALL(*m_wakeWordsSenderMock, sendChangedEvent(newWakeWordsJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_setting->setLocalChange(newWakeWords);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<WakeWordsSetting>>(m_setting)->get(), newWakeWords);
}

/// Test set value failed during local request for wake word change.
TEST_F(LocaleWakeWordsSettingTest, test_localChangeWakeWordsRequestSetFailed) {
    WakeWordsSetting::ValueType initialWakeWords{{ALEXA_VALUE}};
    initializeSetting({ENGLISH_CANADA_VALUE}, initialWakeWords);
    WakeWordsSetting::ValueType newWakeWords{ALEXA_VALUE, ECHO_VALUE};
    auto initialWakeWordsJson = toJson({ALEXA_VALUE});

    WaitEvent event;
    EXPECT_CALL(*m_wakeWordsObserver, onSettingNotification(_, SettingNotifications::LOCAL_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(ENGLISH_LOCALES, newWakeWords)).WillOnce(Return(false));
    EXPECT_CALL(
        *m_wakeWordsObserver, onSettingNotification(initialWakeWords, SettingNotifications::LOCAL_CHANGE_FAILED))
        .WillOnce(InvokeWithoutArgs([&event] { event.wakeUp(); }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(WAKE_WORDS_KEY, SettingStatus::SYNCHRONIZED)).Times(0);

    m_setting->setLocalChange(newWakeWords);
    EXPECT_TRUE(event.wait(MY_WAIT_TIMEOUT));
    EXPECT_EQ(static_cast<std::shared_ptr<WakeWordsSetting>>(m_setting)->get(), initialWakeWords);
}

/// TODO: Test send changed event failed while processing wake words local request.
TEST_F(LocaleWakeWordsSettingTest, test_localChangeWakeWordsRequestSendEventFailed) {
}

/// TODO: Test success flow for local request for wake word change.
TEST_F(LocaleWakeWordsSettingTest, test_localChangeLocaleRequest) {
}

/// TODO: Test set value failed during local request for wake word change.
TEST_F(LocaleWakeWordsSettingTest, test_localChangeLocaleRequestSetFailed) {
}

/// TODO: Test send changed event failed while processing wake words local request.
TEST_F(LocaleWakeWordsSettingTest, test_localChangeLocaleRequestSendEventFailed) {
}

/// TODO: Test locale change when current wake word is not available.
TEST_F(LocaleWakeWordsSettingTest, test_localeChangeTriggerWakeWordsChange) {
}

/// TODO: Test locale change when pending request has a wake word that's not available.
TEST_F(LocaleWakeWordsSettingTest, test_localeChangeCancelPendingWakeWordsChange) {
}

/// TODO: Test locale change when pending request has a valid wake word selected in the current locale.
TEST_F(LocaleWakeWordsSettingTest, test_localeChangeMergePendingWakeWordsChange) {
}

/// TODO: Test wake word change when current locale doesn't support requested wake word.
TEST_F(LocaleWakeWordsSettingTest, test_wakeWordChangeRequestFailedOnCurrentLocale) {
}

/// Test synchronization once the SDK is re-connected and there was an offline change.
TEST_F(LocaleWakeWordsSettingTest, test_reconnectAfterOfflineChange) {
    WakeWordsSetting::ValueType wakeWords{ALEXA_VALUE};
    initializeSetting({ENGLISH_CANADA_VALUE}, wakeWords);

    LocalesSetting::ValueType newLocale{FRENCH_CANADA_VALUE};
    auto newLocaleJson = toSettingString<DeviceLocales>({newLocale}).second;

    // Set offline changes expectations.
    WaitEvent event;
    EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::AVS_CHANGE_IN_PROGRESS))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *m_storageMock,
        storeSettings(convertToDBData(LOCALES_KEY, newLocaleJson, SettingStatus::AVS_CHANGE_IN_PROGRESS)))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_localeObserver, onSettingNotification(_, SettingNotifications::AVS_CHANGE_IN_PROGRESS));
    EXPECT_CALL(*m_assetsManagerMock, changeAssets(newLocale, wakeWords)).WillOnce(Return(true));
    EXPECT_CALL(*m_localeObserver, onSettingNotification(newLocale, SettingNotifications::AVS_CHANGE));
    EXPECT_CALL(*m_localesSenderMock, sendReportEvent(newLocaleJson)).WillOnce(InvokeWithoutArgs([&] {
        std::promise<bool> retPromise;
        retPromise.set_value(false);
        event.wakeUp();
        return retPromise.get_future();
    }));

    // Trigger offline change.
    m_setting->setAvsChange(newLocale);
    ASSERT_TRUE(event.wait(MY_WAIT_TIMEOUT));

    // Set reconnect expectations.
    event.reset();
    EXPECT_CALL(*m_localesSenderMock, sendReportEvent(newLocaleJson)).WillOnce(InvokeWithoutArgs([] {
        std::promise<bool> retPromise;
        retPromise.set_value(true);
        return retPromise.get_future();
    }));
    EXPECT_CALL(*m_storageMock, updateSettingStatus(LOCALES_KEY, SettingStatus::SYNCHRONIZED))
        .WillOnce(InvokeWithoutArgs([&event] {
            event.wakeUp();
            return true;
        }));

    m_setting->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);
    ASSERT_TRUE(event.wait(MY_WAIT_TIMEOUT));
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
