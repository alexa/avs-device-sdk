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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/rapidjson.h>

#include <AVSCommon/AVS/ComponentConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "SoftwareComponentReporter/SoftwareComponentReporterCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace softwareComponentReporter {
namespace test {

using namespace avsCommon::avs;
using namespace rapidjson;
using namespace ::testing;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::configuration;

class SoftwareComponentReporterCapabilityAgentTest : public ::testing::Test {
public:
    /// SetUp before each test.
    void SetUp() override;

    /// TearDown before each test.
    void TearDown() override;

protected:
    /// A pointer to an instance of the softwareComponentReporter that will be instantiated per test.
    std::shared_ptr<SoftwareComponentReporterCapabilityAgent> m_componentReporter;
};

void SoftwareComponentReporterCapabilityAgentTest::SetUp() {
    m_componentReporter = SoftwareComponentReporterCapabilityAgent::create();
}

void SoftwareComponentReporterCapabilityAgentTest::TearDown() {
    m_componentReporter.reset();
}

/**
 * Test adding a ComponentConfiguration
 */
TEST_F(SoftwareComponentReporterCapabilityAgentTest, test_addComponentConfiguration) {
    std::shared_ptr<ComponentConfiguration> config =
        ComponentConfiguration::createComponentConfiguration("component", "1.0");
    ASSERT_NE(config, nullptr);

    EXPECT_TRUE(m_componentReporter->addConfiguration(config));
}

/**
 * Test adding multiple Configurations
 */
TEST_F(SoftwareComponentReporterCapabilityAgentTest, test_addMultipleComponentConfiguration) {
    std::shared_ptr<ComponentConfiguration> config =
        ComponentConfiguration::createComponentConfiguration("component", "1.0");
    std::shared_ptr<ComponentConfiguration> config2 =
        ComponentConfiguration::createComponentConfiguration("component2", "2.0");
    ASSERT_NE(config, nullptr);
    ASSERT_NE(config2, nullptr);

    EXPECT_TRUE(m_componentReporter->addConfiguration(config));
    EXPECT_TRUE(m_componentReporter->addConfiguration(config2));
}

/**
 * Test adding duplicate Configuration is not added
 */
TEST_F(SoftwareComponentReporterCapabilityAgentTest, test_addDupeComponentConfiguration) {
    std::shared_ptr<ComponentConfiguration> config =
        ComponentConfiguration::createComponentConfiguration("component", "1.0");
    std::shared_ptr<ComponentConfiguration> configDupe =
        ComponentConfiguration::createComponentConfiguration("component", "2.0");
    ASSERT_NE(config, nullptr);
    ASSERT_NE(configDupe, nullptr);

    EXPECT_TRUE(m_componentReporter->addConfiguration(config));
    EXPECT_FALSE(m_componentReporter->addConfiguration(configDupe));
}

TEST_F(SoftwareComponentReporterCapabilityAgentTest, test_configurationJsonBuilds) {
    std::shared_ptr<ComponentConfiguration> config =
        ComponentConfiguration::createComponentConfiguration("component", "1.0");
    ASSERT_NE(config, nullptr);
    EXPECT_TRUE(m_componentReporter->addConfiguration(config));

    auto configurationsSet = m_componentReporter->getCapabilityConfigurations();
    EXPECT_EQ(configurationsSet.size(), 1u);
    auto configuration = *configurationsSet.begin();

    ASSERT_TRUE(
        configuration->additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY) !=
        configuration->additionalConfigurations.end());
    std::string configurationJson = configuration->additionalConfigurations[CAPABILITY_INTERFACE_CONFIGURATIONS_KEY];

    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(configurationJson, &document));
    std::vector<std::map<std::string, std::string>> softwareComponents;
    EXPECT_TRUE(jsonUtils::retrieveArrayOfStringMapFromArray(document, "softwareComponents", softwareComponents));

    EXPECT_EQ(softwareComponents.size(), 1u);
    auto component = softwareComponents.front();
    EXPECT_EQ(component["name"], config->name);
    EXPECT_EQ(component["version"], config->version);
}

TEST_F(SoftwareComponentReporterCapabilityAgentTest, test_configurationJsonEmpty) {
    auto configurationsSet = m_componentReporter->getCapabilityConfigurations();
    EXPECT_EQ(configurationsSet.size(), 1u);
    auto configuration = *configurationsSet.begin();

    ASSERT_TRUE(
        configuration->additionalConfigurations.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY) ==
        configuration->additionalConfigurations.end());
}

}  // namespace test
}  // namespace softwareComponentReporter
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
