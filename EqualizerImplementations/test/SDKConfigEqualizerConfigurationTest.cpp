/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <algorithm>
#include <list>
#include <memory>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "EqualizerImplementations/SDKConfigEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace equalizer {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::utils::configuration;

/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

/// JSON configuration with all bands defined, but not all supported.
const std::string JSON_LIMITED_BANDS = R"({"bands":{"BASS":false, "MIDRANGE":false, "TREBLE":true}})";
/// JSON configuration with a limited set of bands defined.
const std::string JSON_LIMITED_BANDS_ONE_MISSING = R"({"bands":{"BASS":false, "MIDRANGE":false}})";
/// JSON configuration with a an empty supported bands branch.
const std::string JSON_NO_BANDS_PROVIDED = R"({"bands":{}})";
/// JSON configuration with a an invalid band listed as supported.
const std::string JSON_INVALID_BAND = R"({"bands":{"DEEPBASS":true}})";
/// JSON configuration with a one mode defined and supported.
const std::string JSON_ONE_MODE_MENTIONED_ENABLED = R"({"modes":{"NIGHT": true}})";
/// JSON configuration with a one mode defined but unsupported.
const std::string JSON_ONE_MODE_MENTIONED_DISABLED = R"({"modes":{"NIGHT": false}})";
/// JSON configuration with an empty default state branch.
const std::string JSON_EMPTY_DEFAULT_STATE_BRANCH = R"({"defaultState":{}})";
/// JSON configuration with a default supported mode provided.
const std::string JSON_DEFAULT_MODE_SUPPORTED = R"({"modes": {"NIGHT":true}, "defaultState":{"mode":"NIGHT"}})";
/// JSON configuration with a default unsupported mode provided.
const std::string JSON_DEFAULT_MODE_UNSUPPORTED = R"({"defaultState":{"mode":"NIGHT"}})";
/// JSON configuration with a missing band levels in default state.
const std::string JSON_DEFAULT_STATE_MISSING_BANDS = R"({"defaultState":{"bands":{"BASS": 1}}})";
/// JSON configuration with all band defined but unsupported and empty bands branch in default state.s
const std::string JSON_DEFAULT_STATE_BANDS_EMPTY_NO_BANDS_SUPPORTED =
    R"({"bands":{"BASS":false, "MIDRANGE":false, "TREBLE":false}, "defaultState":{"bands":{}}})";

/**
 * Test fixture for @c SDKConfigEqualizerConfiguration tests.
 */
class SDKConfigEqualizerConfigurationTest : public ::testing::Test {
public:
protected:
    /**
     * Test teardown.
     */
    void TearDown() override;

public:
    /**
     * Constructor.
     */
    SDKConfigEqualizerConfigurationTest();

protected:
};

SDKConfigEqualizerConfigurationTest::SDKConfigEqualizerConfigurationTest() {
}

void SDKConfigEqualizerConfigurationTest::TearDown() {
    // Cleanup the configuration so that next intialize() will work.
    ConfigurationNode::uninitialize();
}

/*
 * Utility function to generate @c ConfigurationNode from JSON string.
 */
static ConfigurationNode generateConfigFromJSON(std::string json) {
    auto stream = std::shared_ptr<std::stringstream>(new std::stringstream(json));
    JSONStream jsonStream({stream});
    ConfigurationNode::initialize(jsonStream);
    return ConfigurationNode::getRoot();
}

// Test creation with an empty configuration
TEST_F(SDKConfigEqualizerConfigurationTest, providedEmptyConfig_shouldSucceed) {
    auto defaultConfig = InMemoryEqualizerConfiguration::createDefault();
    ConfigurationNode rootNode;

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());
}

// Empty configuration should lead to default values.
TEST_F(SDKConfigEqualizerConfigurationTest, providedEmptyConfig_shouldUseDefaultConfig) {
    auto defaultConfig = InMemoryEqualizerConfiguration::createDefault();
    ConfigurationNode rootNode;

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);

    EXPECT_EQ(defaultConfig->getMinBandLevel(), config->getMinBandLevel());
    EXPECT_EQ(defaultConfig->getMaxBandLevel(), config->getMaxBandLevel());

    EXPECT_EQ(defaultConfig->getSupportedBands(), config->getSupportedBands());
    EXPECT_EQ(defaultConfig->getSupportedModes(), config->getSupportedModes());

    EXPECT_EQ(defaultConfig->getDefaultState().mode, config->getDefaultState().mode);
    EXPECT_EQ(defaultConfig->getDefaultState().bandLevels, config->getDefaultState().bandLevels);
}

// Test the case when only some of the bands supported
TEST_F(SDKConfigEqualizerConfigurationTest, providedLimitedBandsDefined_shouldSucceed) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_LIMITED_BANDS);

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);

    EXPECT_FALSE(config->isBandSupported(EqualizerBand::BASS));
    EXPECT_FALSE(config->isBandSupported(EqualizerBand::MIDRANGE));
    // This depends on hardcoded defaults we have
    EXPECT_TRUE(config->isBandSupported(EqualizerBand::TREBLE));
}

// Test the case when only some of the bands supported, one of them is not explicitly mentioned.
TEST_F(SDKConfigEqualizerConfigurationTest, providedLimitedBandsOneMissing_shouldSucceed) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_LIMITED_BANDS_ONE_MISSING);

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);

    EXPECT_FALSE(config->isBandSupported(EqualizerBand::BASS));
    EXPECT_FALSE(config->isBandSupported(EqualizerBand::MIDRANGE));
    EXPECT_EQ(
        config->isBandSupported(EqualizerBand::TREBLE),
        SDKConfigEqualizerConfiguration::BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
}

// Test empty bands branch behavior
TEST_F(SDKConfigEqualizerConfigurationTest, havingEmptyBandList_shouldUseHardDefaults) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_NO_BANDS_PROVIDED);

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());

    EXPECT_EQ(
        config->isBandSupported(EqualizerBand::BASS),
        SDKConfigEqualizerConfiguration::BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isBandSupported(EqualizerBand::MIDRANGE),
        SDKConfigEqualizerConfiguration::BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isBandSupported(EqualizerBand::TREBLE),
        SDKConfigEqualizerConfiguration::BAND_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
}

// Test invalid band listed in supported bands branch
TEST_F(SDKConfigEqualizerConfigurationTest, havingOnlyInvalidBand_shouldSucceedAndSupportNone) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_INVALID_BAND);

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());

    EXPECT_FALSE(config->isBandSupported(EqualizerBand::BASS));
    EXPECT_FALSE(config->isBandSupported(EqualizerBand::MIDRANGE));
    EXPECT_FALSE(config->isBandSupported(EqualizerBand::TREBLE));
}

// Test modes branch containing one mode enabled
TEST_F(SDKConfigEqualizerConfigurationTest, oneModeDefinedAndEnabled_shouldPutOthersToDefaults) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_ONE_MODE_MENTIONED_ENABLED);

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());

    EXPECT_TRUE(config->isModeSupported(EqualizerMode::NIGHT));
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::TV),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::SPORT),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::MUSIC),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::MOVIE),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
}

// Test modes branch containing one mode disabled
TEST_F(SDKConfigEqualizerConfigurationTest, oneModeDefinedAndDisabled_shouldPutOthersToDefaults) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_ONE_MODE_MENTIONED_DISABLED);

    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());

    EXPECT_FALSE(config->isModeSupported(EqualizerMode::NIGHT));
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::TV),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::SPORT),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::MUSIC),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
    EXPECT_EQ(
        config->isModeSupported(EqualizerMode::MOVIE),
        SDKConfigEqualizerConfiguration::MODE_IS_SUPPORTED_IF_MISSING_IN_CONFIG);
}

// Test the empty default state branch
TEST_F(SDKConfigEqualizerConfigurationTest, givenEmptyDefaultStateBranchEmpty_shouldUseHardDefaults) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_EMPTY_DEFAULT_STATE_BRANCH);
    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());
    auto defaultConfig = InMemoryEqualizerConfiguration::createDefault();

    EXPECT_EQ(config->getDefaultState().mode, defaultConfig->getDefaultState().mode);
    EXPECT_EQ(defaultConfig->getDefaultState().bandLevels, config->getDefaultState().bandLevels);
}

// Test default state with supported mode
TEST_F(SDKConfigEqualizerConfigurationTest, givenSupportedDefaultMode_shouldSucceed) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_DEFAULT_MODE_SUPPORTED);
    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());

    EXPECT_EQ(config->getDefaultState().mode, EqualizerMode::NIGHT);
}

// Test default state with unsupported mode
TEST_F(SDKConfigEqualizerConfigurationTest, givenUnsupportedDefaultMode_shouldFail) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_DEFAULT_MODE_UNSUPPORTED);
    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, IsNull());
}

// Test not all supported bands being provided in default state
TEST_F(SDKConfigEqualizerConfigurationTest, havingNotAllBandsInDefaultState_shouldFail) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_DEFAULT_STATE_MISSING_BANDS);
    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, IsNull());
}

// Test empty default state bands while not bands supported
TEST_F(SDKConfigEqualizerConfigurationTest, havingNoBandsDefinedInDefaultStateWithNoBandsSupported_shouldSucceed) {
    ConfigurationNode rootNode = generateConfigFromJSON(JSON_DEFAULT_STATE_BANDS_EMPTY_NO_BANDS_SUPPORTED);
    auto config = SDKConfigEqualizerConfiguration::create(rootNode);
    ASSERT_THAT(config, NotNull());
}

}  // namespace test
}  // namespace equalizer
}  // namespace alexaClientSDK
