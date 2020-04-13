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

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/MockSetting.h>
#include <Settings/SettingsManager.h>

#include "System/StateReportGenerator.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::utils;
using namespace settings;
using namespace settings::test;

/// Alias for setting manager configuration.
using MockSettingManager =
    SettingsManager<SettingInterface<bool>, SettingInterface<int>, SettingInterface<std::string>>;

/// Index used for setting access.
enum Index { BOOL, INT, STRING };

/// @name Mock values and string representantion.
/// @{
constexpr bool BOOL_SETTING_VALUE = true;
constexpr int INT_SETTING_VALUE = 10;
static const std::string BOOL_SETTING_STRING_VALUE = "true";
static const std::string INT_SETTING_STRING_VALUE = "10";
static const std::string STRING_SETTING_VALUE = "string";
static const std::string STRING_SETTING_STRING_VALUE = R"("string")";
/// @}

/**
 * Test class for @c StateReportGenerator.
 */
class StateReportGeneratorTest : public ::testing::Test {
public:
    /**
     * Set up the test environment.
     */
    void SetUp() override;

    /**
     * Tear down test environment.
     */
    void TearDown() override;

protected:
    /// The mock for a boolean setting.
    std::shared_ptr<MockSetting<bool>> m_mockBoolSetting;

    /// The mock for an integer setting.
    std::shared_ptr<MockSetting<int>> m_mockIntSetting;

    /// The mock for a string setting.
    std::shared_ptr<MockSetting<std::string>> m_mockStringSetting;

    /// The setting manager.
    std::shared_ptr<MockSettingManager> m_mockSettingManager;

    /// The setting metadata.
    StateReportGenerator::SettingConfigurations<MockSettingManager> m_configurations;
};

void StateReportGeneratorTest::SetUp() {
    m_mockBoolSetting = std::make_shared<MockSetting<bool>>(BOOL_SETTING_VALUE);
    m_mockIntSetting = std::make_shared<MockSetting<int>>(INT_SETTING_VALUE);
    m_mockStringSetting = std::make_shared<MockSetting<std::string>>(STRING_SETTING_VALUE);
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
    m_mockSettingManager = std::make_shared<MockSettingManager>(customerDataManager);

    SettingEventMetadata boolSettingMetadata{"test", "", "BoolSettingReport", "boolSetting"};
    SettingEventMetadata intSettingMetadata{"test", "", "IntSettingReport", "intSetting"};
    SettingEventMetadata stringSettingMetadata{"test", "", "StringSettingReport", "stringSetting"};
    std::get<Index::BOOL>(m_configurations).metadata.set(boolSettingMetadata);
    std::get<Index::INT>(m_configurations).metadata.set(intSettingMetadata);
    std::get<Index::STRING>(m_configurations).metadata.set(stringSettingMetadata);
}

void StateReportGeneratorTest::TearDown() {
    m_mockBoolSetting.reset();
    m_mockIntSetting.reset();
    m_mockStringSetting.reset();
    m_mockSettingManager.reset();
}

TEST_F(StateReportGeneratorTest, test_createWithoutSettingManagerShouldFail) {
    EXPECT_FALSE(StateReportGenerator::create<MockSettingManager>(nullptr, m_configurations).hasValue());
}

TEST_F(StateReportGeneratorTest, test_createWithEmptySettingManagerShouldSucceed) {
    EXPECT_TRUE(StateReportGenerator::create(m_mockSettingManager, m_configurations).hasValue());
}

TEST_F(StateReportGeneratorTest, test_createWithFullSettingManagerShouldSucceed) {
    m_mockSettingManager->addSetting<Index::BOOL>(m_mockBoolSetting);
    m_mockSettingManager->addSetting<Index::INT>(m_mockIntSetting);
    m_mockSettingManager->addSetting<Index::STRING>(m_mockStringSetting);
    EXPECT_TRUE(StateReportGenerator::create(m_mockSettingManager, m_configurations).hasValue());
}

TEST_F(StateReportGeneratorTest, test_generateReportWithFullSettingManager) {
    m_mockSettingManager->addSetting<Index::BOOL>(m_mockBoolSetting);
    m_mockSettingManager->addSetting<Index::INT>(m_mockIntSetting);
    m_mockSettingManager->addSetting<Index::STRING>(m_mockStringSetting);
    auto generator = StateReportGenerator::create(m_mockSettingManager, m_configurations);
    ASSERT_TRUE(generator.hasValue());

    auto report = generator.value().generateReport();
    ASSERT_EQ(report.size(), 3u);

    EXPECT_TRUE(report[0].find(STRING_SETTING_STRING_VALUE));
    EXPECT_TRUE(report[1].find(INT_SETTING_STRING_VALUE));
    EXPECT_TRUE(report[2].find(BOOL_SETTING_STRING_VALUE));
}

TEST_F(StateReportGeneratorTest, test_generateReportWithPartialSettingManager) {
    m_mockSettingManager->addSetting<Index::INT>(m_mockIntSetting);
    m_mockSettingManager->addSetting<Index::STRING>(m_mockStringSetting);
    auto generator = StateReportGenerator::create(m_mockSettingManager, m_configurations);
    ASSERT_TRUE(generator.hasValue());

    auto report = generator.value().generateReport();
    ASSERT_EQ(report.size(), 2u);

    EXPECT_TRUE(report[0].find(STRING_SETTING_STRING_VALUE));
    EXPECT_TRUE(report[1].find(INT_SETTING_STRING_VALUE));
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
