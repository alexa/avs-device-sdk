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

#include <vector>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/CapabilityChangeNotifier.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationChangeObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockLocaleAssetsManager.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "System/SystemCapabilityProvider.h"

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::test;
using namespace alexaClientSDK::avsCommon::utils::json;
using namespace testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

/// A list of test supported locales.
static const std::set<std::string> SUPPORTED_LOCALES = {"en-US", "en-GB"};

/// A list of test supported locale combinations.
static const std::set<std::vector<std::string>> SUPPORTED_LOCALE_COMBINATIONS = {{"en-US", "es-US"}};

/// Capability configuration key used to give more details about the device configuration.
static const std::string CAPABILITY_INTERFACE_CONFIGURATIONS_KEY = "configurations";

/// Locale key
static const std::string LOCALES_CONFIGURATION_KEY = "locales";

/// Locale Combinations Key
static const std::string LOCALE_COMBINATION_CONFIGURATION_KEY = "localeCombinations";

/// Mock class that implements the @c CapabilityConfigurationChangeObserverInterface.
class MockCapabilityConfigurationChangeObserver
        : public avsCommon::sdkInterfaces::CapabilityConfigurationChangeObserverInterface {
public:
    MOCK_METHOD1(onConfigurationChanged, void(const avsCommon::avs::CapabilityConfiguration& configuration));
};

/// Test harness for @c SystemCapabilityProvider
class SystemCapabilityProviderTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;
    /// Clean up the test harness after running a test.
    void TearDown() override;

protected:
    /// The mock @c CapabilityconfigurationChangeObserverInterface
    std::shared_ptr<MockCapabilityConfigurationChangeObserver> m_mockCapabilityConfigurationChangeObserver;

    /// The @c CapabilityChangeNotifierInterface
    std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface> m_capabilityChangeNotifier;

    /// A mock instance of @c LocaleAssetsManagerInterface
    std::shared_ptr<MockLocaleAssetsManager> m_mockAssetsManager;

    /// The @c SystemCapabilityProvider to test.
    std::shared_ptr<SystemCapabilityProvider> m_systemCapabilityProvider;
};

void SystemCapabilityProviderTest::SetUp() {
    m_mockCapabilityConfigurationChangeObserver =
        std::make_shared<StrictMock<MockCapabilityConfigurationChangeObserver>>();
    m_capabilityChangeNotifier = std::make_shared<avsCommon::avs::CapabilityChangeNotifier>();
    m_capabilityChangeNotifier->addObserver(m_mockCapabilityConfigurationChangeObserver);

    m_mockAssetsManager = std::make_shared<NiceMock<MockLocaleAssetsManager>>();
    ON_CALL(*m_mockAssetsManager, getSupportedLocales())
        .WillByDefault(
            InvokeWithoutArgs([]() -> std::set<LocaleAssetsManagerInterface::Locale> { return SUPPORTED_LOCALES; }));

    m_systemCapabilityProvider = SystemCapabilityProvider::create(m_mockAssetsManager, m_capabilityChangeNotifier);
    ASSERT_NE(m_systemCapabilityProvider, nullptr);
}

void SystemCapabilityProviderTest::TearDown() {
    m_capabilityChangeNotifier->removeObserver(m_mockCapabilityConfigurationChangeObserver);
}

/// Function to verify that @c SystemCapabilityProvider::create) errors out with invalid inputs.
TEST_F(SystemCapabilityProviderTest, test_createWithInvalidInputs) {
    /// With an invalid @c LocaleAssetsManagerInterface
    m_systemCapabilityProvider = SystemCapabilityProvider::create(nullptr, m_capabilityChangeNotifier);
    EXPECT_EQ(m_systemCapabilityProvider, nullptr);

    /// With an invalid @c CapabilityChangeNotifierInterface
    m_systemCapabilityProvider = SystemCapabilityProvider::create(m_mockAssetsManager, nullptr);
    EXPECT_EQ(m_systemCapabilityProvider, nullptr);
}

/// Function to verify that @c SystemCapabilityProvider notifies observers when locale asset is changed and updates
/// its own capability configurations.
TEST_F(SystemCapabilityProviderTest, test_localeAssetsChanged) {
    std::string localeString;
    for (auto locale : SUPPORTED_LOCALES) {
        if (!localeString.empty()) {
            localeString += ",";
        }
        localeString += "\"" + locale + "\"";
    }

    std::string localeCombinationsString;
    for (auto localeCombo : SUPPORTED_LOCALE_COMBINATIONS) {
        if (!localeCombinationsString.empty()) {
            localeCombinationsString += ",";
        }
        std::string localeComboString;
        for (auto locale : localeCombo) {
            if (!localeComboString.empty()) {
                localeComboString += ",";
            }
            localeComboString += "\"" + locale + "\"";
        }
        localeCombinationsString += localeComboString;
    }

    auto oldLocalesRegex = R"(.*\[)" + localeString + R"(\].*)";
    auto newLocalesRegex = R"(.*\[)" + localeString + R"(\].*)" + R"(.*\[)" + localeCombinationsString + R"(\].*)";

    // Check the old System capability configuration.
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> caps =
        m_systemCapabilityProvider->getCapabilityConfigurations();
    auto cap = *caps.begin();
    auto configuration = cap->additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    ASSERT_NE(configuration, cap->additionalConfigurations.end());
    EXPECT_THAT(configuration->second, MatchesRegex(oldLocalesRegex));

    // Update the System capability configuration.
    EXPECT_CALL(*m_mockAssetsManager, getSupportedLocaleCombinations())
        .WillOnce(InvokeWithoutArgs(
            []() -> LocaleAssetsManagerInterface::LocaleCombinations { return SUPPORTED_LOCALE_COMBINATIONS; }));
    EXPECT_CALL(*m_mockCapabilityConfigurationChangeObserver, onConfigurationChanged(_))
        .WillOnce(Invoke([this,
                          newLocalesRegex](const avsCommon::avs::CapabilityConfiguration& capabilityConfiguration) {
            auto cap = capabilityConfiguration.additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
            EXPECT_THAT(cap->second, MatchesRegex(newLocalesRegex));

            // Retrieve the new System capability configuration.
            auto newCaps = m_systemCapabilityProvider->getCapabilityConfigurations();
            auto newCap = *newCaps.begin();
            auto newConfiguration = newCap->additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
            ASSERT_NE(newConfiguration, newCap->additionalConfigurations.end());
            EXPECT_THAT(newConfiguration->second, MatchesRegex(newLocalesRegex));
        }));
    m_systemCapabilityProvider->onLocaleAssetsChanged();
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK