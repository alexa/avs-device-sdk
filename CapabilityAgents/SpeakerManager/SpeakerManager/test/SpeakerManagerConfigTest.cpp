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

#include <acsdk/SpeakerManager/private/SpeakerManagerConfig.h>

namespace alexaClientSDK {
namespace speakerManager {
namespace test {

using namespace avsCommon::utils::configuration;
using namespace ::testing;

/// @addtogroup Test_acsdkSpeakerManager
/// @{
/// Configuration without speaker manager root.
static const auto JSON_TEST_CONFIG_MISSING = R"(
{
})";
/// Configuration with speaker manager root but without entries.
static const auto JSON_TEST_CONFIG_EMPTY = R"(
{
    "speakerManagerCapabilityAgent": {}
})";
/// Configuration with speaker manager root with persistentStorage.
static const auto JSON_TEST_CONFIG_PERSISTENT_STORAGE = R"(
{
    "speakerManagerCapabilityAgent": {
        "persistentStorage": true
    }
})";
/// Configuration with speaker manager root with minUnmuteVolume.
static const auto JSON_TEST_CONFIG_MIN_UNMUTE_VOLUME = R"(
{
    "speakerManagerCapabilityAgent": {
        "minUnmuteVolume": 3
    }
})";
/// Configuration with speaker manager root with defaultSpeakerVolume.
static const auto JSON_TEST_CONFIG_DEFAULT_SPEAKER_VOLUME = R"(
{
    "speakerManagerCapabilityAgent": {
        "defaultSpeakerVolume": 5
    }
})";
/// Configuration with speaker manager root with defaultAlertsVolume.
static const auto JSON_TEST_CONFIG_DEFAULT_ALERTS_VOLUME = R"(
{
    "speakerManagerCapabilityAgent": {
        "defaultAlertsVolume": 6
    }
})";
/// Configuration with speaker manager root with defaultAlertsVolume.
static const auto JSON_TEST_CONFIG_RESTORE_MUTE_STATE = R"(
{
    "speakerManagerCapabilityAgent": {
        "restoreMuteState": false
    }
})";
/// Full configuration.
static const auto JSON_TEST_CONFIG = R"(
{
    "speakerManagerCapabilityAgent": {
        "persistentStorage": true,
        "minUnmuteVolume": 3,
        "defaultSpeakerVolume": 5,
        "defaultAlertsVolume": 6,
        "restoreMuteState": true
    }
}
)";

/// @brief Test fixture for SpeakerManagerConfig.
class SpeakerManagerConfigTest : public Test {
protected:
    /**
     * Configure ConfigurationNode with a given string.
     *
     * @param jsonConfig Configuration string.
     * @return True on success.
     */
    bool configureJsonConfig(const char* jsonConfig);

    /// SetUp before each test.
    void SetUp() override;

    /// TearDown after each test.
    void TearDown() override;
};

/// @}

void SpeakerManagerConfigTest::SetUp() {
    ConfigurationNode::uninitialize();
}

void SpeakerManagerConfigTest::TearDown() {
    ConfigurationNode::uninitialize();
}

bool SpeakerManagerConfigTest::configureJsonConfig(const char* jsonConfig) {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::istream> istr(new std::istringstream(jsonConfig));
    return ConfigurationNode::initialize({istr});
}

/// Validate nothing breaks when config is missing.
TEST_F(SpeakerManagerConfigTest, test_ValidateMissingConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_MISSING));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume;

    ASSERT_FALSE(config.getPersistentStorage(persistentStorage));
    ASSERT_FALSE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_FALSE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_FALSE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_FALSE(config.getDefaultAlertsVolume(defaultAlertsVolume));
}

/// Validate nothing breaks when config is empty.
TEST_F(SpeakerManagerConfigTest, test_ValidateEmptyConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_EMPTY));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume;

    ASSERT_FALSE(config.getPersistentStorage(persistentStorage));
    ASSERT_FALSE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_FALSE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_FALSE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_FALSE(config.getDefaultAlertsVolume(defaultAlertsVolume));
}

/// Validate persistentVolume entry is read correctly.
TEST_F(SpeakerManagerConfigTest, test_ValidatePersistentStorageConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_PERSISTENT_STORAGE));

    SpeakerManagerConfig config;
    bool persistentStorage = false;
    bool restoreMuteState;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume;

    ASSERT_TRUE(config.getPersistentStorage(persistentStorage));
    ASSERT_TRUE(persistentStorage);
    ASSERT_FALSE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_FALSE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_FALSE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_FALSE(config.getDefaultAlertsVolume(defaultAlertsVolume));
}

/// Validate restoreMuteState entry is read correctly.
TEST_F(SpeakerManagerConfigTest, test_ValidateRestoreMuteStateConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_RESTORE_MUTE_STATE));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState = true;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume;

    ASSERT_FALSE(config.getPersistentStorage(persistentStorage));
    ASSERT_TRUE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_FALSE(restoreMuteState);
    ASSERT_FALSE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_FALSE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_FALSE(config.getDefaultAlertsVolume(defaultAlertsVolume));
}

/// Validate minUnmuteVolume entry is read correctly.
TEST_F(SpeakerManagerConfigTest, test_ValidateMinUnmuteVolumeConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_MIN_UNMUTE_VOLUME));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState;
    uint8_t minUnmuteVolume = 0;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume;

    ASSERT_FALSE(config.getPersistentStorage(persistentStorage));
    ASSERT_FALSE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_TRUE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_EQ(3u, minUnmuteVolume);
    ASSERT_FALSE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_FALSE(config.getDefaultAlertsVolume(defaultAlertsVolume));
}

/// Validate defaultSpeakerVolume entry is read correctly.
TEST_F(SpeakerManagerConfigTest, test_ValidateDefaultSpeakerVolumeConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_DEFAULT_SPEAKER_VOLUME));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume = 0;
    uint8_t defaultAlertsVolume;

    ASSERT_FALSE(config.getPersistentStorage(persistentStorage));
    ASSERT_FALSE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_FALSE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_TRUE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_EQ(5u, defaultSpeakerVolume);
    ASSERT_FALSE(config.getDefaultAlertsVolume(defaultAlertsVolume));
}

/// Validate defaultAlertsVolume entry is read correctly.
TEST_F(SpeakerManagerConfigTest, test_ValidateDefaultAlertsVolumeConfig) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG_DEFAULT_ALERTS_VOLUME));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume = 0;

    ASSERT_FALSE(config.getPersistentStorage(persistentStorage));
    ASSERT_FALSE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_FALSE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_FALSE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_TRUE(config.getDefaultAlertsVolume(defaultAlertsVolume));
    ASSERT_EQ(6u, defaultAlertsVolume);
}

TEST_F(SpeakerManagerConfigTest, test_ValidateAllValues) {
    ASSERT_TRUE(configureJsonConfig(JSON_TEST_CONFIG));

    SpeakerManagerConfig config;
    bool persistentStorage;
    bool restoreMuteState;
    uint8_t minUnmuteVolume;
    uint8_t defaultSpeakerVolume;
    uint8_t defaultAlertsVolume;

    ASSERT_TRUE(config.getPersistentStorage(persistentStorage));
    ASSERT_TRUE(persistentStorage);
    ASSERT_TRUE(config.getRestoreMuteState(restoreMuteState));
    ASSERT_TRUE(restoreMuteState);
    ASSERT_TRUE(config.getMinUnmuteVolume(minUnmuteVolume));
    ASSERT_EQ(3u, minUnmuteVolume);
    ASSERT_TRUE(config.getDefaultSpeakerVolume(defaultSpeakerVolume));
    ASSERT_EQ(5u, defaultSpeakerVolume);
    ASSERT_TRUE(config.getDefaultAlertsVolume(defaultAlertsVolume));
    ASSERT_EQ(6u, defaultAlertsVolume);
}

}  // namespace test
}  // namespace speakerManager
}  // namespace alexaClientSDK
