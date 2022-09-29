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
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdk/AlexaPresentationAPL/private/AlexaPresentationAPLVideoConfigParser.h"

namespace alexaClientSDK {
namespace aplCapabilityAgent {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::aplCapabilityCommonInterfaces;
using namespace alexaClientSDK::avsCommon::utils::configuration;
/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

/// Alexa.Presentation.APL.Video key for video related settings
const std::string ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY = "video";
/// Alexa.Presentation.APL.Video key for supported codecs
const std::string ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY = "codecs";
/// String representing the H_264_41 codec
const std::string H_264_41 = "H_264_41";
/// String representing the H_264_42 codec
const std::string H_264_42 = "H_264_42";

const std::string APL_VIDEO_CONFIG = R"({")" + ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY + R"(":{")" +
                                     ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY + R"(":[")" + H_264_41 + R"(",")" +
                                     H_264_42 + R"("]}})";
const std::string INVALID_KEY = "invalidKey";
const std::string APL_VIDEO_CONFIG_INVALID_VIDEO_KEY = R"({")" + INVALID_KEY + R"(":{")" +
                                                       ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY + R"(":[")" +
                                                       H_264_41 + R"(",")" + H_264_42 + R"("]}})";
const std::string APL_VIDEO_CONFIG_INVALID_CODECS_KEY = R"({")" + ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY +
                                                        R"(":{")" + INVALID_KEY + R"(":[")" + H_264_41 + R"(",")" +
                                                        H_264_42 + R"("]}})";
const std::string UNSUPPORTED_CODEC = "unsupportedCodec";
const std::string APL_VIDEO_CONFIG_UNSUPPORTED_CODEC = R"({")" + ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY +
                                                       R"(":{")" + ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY +
                                                       R"(":[")" + H_264_41 + R"(",")" + UNSUPPORTED_CODEC + R"("]}})";

/// Test harness for @c AlexaPresentationAPLVideoConfigParser class.
class AlexaPresentationAPLVideoConfigParserTest : public ::testing::Test {
public:
    static bool areVideoSettingsEqual(const VideoSettings& videoSettings1, const VideoSettings& videoSettings2);
    static ConfigurationNode getConfigurationNode(std::string jsonConfig);
    /// Clean up the test harness after running a test.
    void TearDown() override;
    /// Constructor.
    AlexaPresentationAPLVideoConfigParserTest() {
    }
};

void AlexaPresentationAPLVideoConfigParserTest::TearDown() {
    ConfigurationNode::uninitialize();
}

ConfigurationNode AlexaPresentationAPLVideoConfigParserTest::getConfigurationNode(std::string jsonConfig) {
    auto stream = std::shared_ptr<std::stringstream>(new std::stringstream(jsonConfig));
    JSONStream jsonStream({stream});
    ConfigurationNode::initialize(jsonStream);
    return ConfigurationNode::getRoot();
}

bool AlexaPresentationAPLVideoConfigParserTest::areVideoSettingsEqual(
    const VideoSettings& videoSettings1,
    const VideoSettings& videoSettings2) {
    return videoSettings1.codecs == videoSettings2.codecs;
}

/**
 * Tests video settings serialization.
 */
TEST_F(AlexaPresentationAPLVideoConfigParserTest, testSerializeVideoSettings) {
    std::string serializedVideoConfig;
    VideoSettings video_settings_2;
    video_settings_2.codecs.insert(VideoSettings::Codec::H_264_41);
    video_settings_2.codecs.insert(VideoSettings::Codec::H_264_42);
    ASSERT_TRUE(AlexaPresentationAPLVideoConfigParser::serializeVideoSettings(video_settings_2, serializedVideoConfig));
    ASSERT_EQ(serializedVideoConfig, APL_VIDEO_CONFIG);
}

/**
 * Tests parsing video settings with invalid video key.
 */
TEST_F(AlexaPresentationAPLVideoConfigParserTest, testParseVideoSettingsInvalidVideoKey) {
    auto node = getConfigurationNode(APL_VIDEO_CONFIG_INVALID_VIDEO_KEY);
    VideoSettings videoSettings;
    ASSERT_FALSE(AlexaPresentationAPLVideoConfigParser::parseVideoSettings(node, videoSettings));
}

/**
 * Tests parsing video settings with invalid codecs key.
 */
TEST_F(AlexaPresentationAPLVideoConfigParserTest, testParseVideoSettingsInvalidCodecsKey) {
    auto node = getConfigurationNode(APL_VIDEO_CONFIG_INVALID_CODECS_KEY);
    VideoSettings videoSettings;
    ASSERT_FALSE(AlexaPresentationAPLVideoConfigParser::parseVideoSettings(node, videoSettings));
}

/**
 * Tests parsing video settings with unsupported codec.
 */
TEST_F(AlexaPresentationAPLVideoConfigParserTest, testParseVideoSettingsUnsupportedCodec) {
    auto node = getConfigurationNode(APL_VIDEO_CONFIG_UNSUPPORTED_CODEC);
    VideoSettings videoSettings;
    VideoSettings video_settings_1;
    video_settings_1.codecs.insert(VideoSettings::Codec::H_264_41);
    ASSERT_TRUE(AlexaPresentationAPLVideoConfigParser::parseVideoSettings(node, videoSettings));
    ASSERT_TRUE(areVideoSettingsEqual(videoSettings, video_settings_1));
}
}  // namespace test
}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK