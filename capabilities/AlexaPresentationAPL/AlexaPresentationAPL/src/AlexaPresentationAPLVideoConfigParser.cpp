/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "acsdk/AlexaPresentationAPL/private/AlexaPresentationAPLVideoConfigParser.h"

namespace alexaClientSDK {
namespace aplCapabilityAgent {

using namespace alexaClientSDK;
using namespace alexaClientSDK::aplCapabilityCommonInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AlexaPresentationAPLVideoConfigParser"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Alexa.Presentation.APL.Video key for video related settings
static const std::string ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY = "video";

/// Alexa.Presentation.APL.Video key for supported codecs
static const std::string ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY = "codecs";

bool AlexaPresentationAPLVideoConfigParser::serializeVideoSettings(
    const alexaClientSDK::aplCapabilityCommonInterfaces::VideoSettings& videoSettings,
    std::string& serializedJson) {
    rapidjson::Document configurationJson(rapidjson::kObjectType);
    auto& alloc = configurationJson.GetAllocator();

    rapidjson::Value codecsJson(rapidjson::kArrayType);
    for (const auto& videoCodec : videoSettings.codecs) {
        auto codecStr = VideoSettings::codecToString(videoCodec);
        rapidjson::Value value;
        value.SetString(codecStr.c_str(), codecStr.length(), alloc);
        codecsJson.PushBack(value, alloc);
    }

    rapidjson::Value videoJson(rapidjson::kObjectType);
    videoJson.AddMember(rapidjson::StringRef(ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY), codecsJson, alloc);
    configurationJson.AddMember(rapidjson::StringRef(ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY), videoJson, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    if (!configurationJson.Accept(writer)) {
        ACSDK_CRITICAL(LX("serializeVideoSettingsFailed").d("reason", "configWriterRefusedJsonObject"));
        return false;
    }
    serializedJson = sb.GetString();

    return true;
}

bool AlexaPresentationAPLVideoConfigParser::parseVideoSettings(
    const alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode& config,
    alexaClientSDK::aplCapabilityCommonInterfaces::VideoSettings& videoSettings) {
    auto videoSettingsJson = config[ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_VIDEO_KEY];
    if (!videoSettingsJson) {
        ACSDK_DEBUG5(LX(__func__).m("Video settings not found in config file"));
        return false;
    }

    std::set<std::string> videoCodecs;
    if (!videoSettingsJson.getStringValues(ALEXAPRESENTATIONAPLVIDEO_CAPABILITY_CODECS_KEY, &videoCodecs)) {
        ACSDK_DEBUG5(LX(__func__).m("Video codecs not found in config file"));
        return false;
    }

    for (const auto& codecStr : videoCodecs) {
        auto codec = VideoSettings::stringToCodec(codecStr);
        if (codec.hasValue()) {
            videoSettings.codecs.insert(codec.value());
        } else {
            ACSDK_WARN(LX(__func__).d("videoCodec", codecStr).m("Unsupported codec"));
        }
    }

    return true;
}

}  // namespace aplCapabilityAgent
}  // namespace alexaClientSDK
