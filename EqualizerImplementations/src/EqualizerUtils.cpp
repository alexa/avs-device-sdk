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

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "EqualizerImplementations/EqualizerUtils.h"

namespace alexaClientSDK {
namespace equalizer {

using namespace avsCommon::sdkInterfaces::audio;
using namespace avsCommon::utils::error;
using namespace avsCommon::utils::json::jsonUtils;

/// String to identify log entries originating from this file.
static const std::string TAG{"EqualizerUtils"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name for "bands" JSON branch.
constexpr char JSON_KEY_BANDS[] = "bands";
/// Name for "name" JSON value.
constexpr char JSON_KEY_NAME[] = "name";
/// Name for "level" JSON value.
constexpr char JSON_KEY_LEVEL[] = "level";
/// Name for "mode" JSON value.
constexpr char JSON_KEY_MODE[] = "mode";

std::string EqualizerUtils::serializeEqualizerState(const EqualizerState& state) {
    rapidjson::Document payload(rapidjson::kObjectType);
    auto& allocator = payload.GetAllocator();
    rapidjson::Value bandsBranch(rapidjson::kArrayType);

    for (const auto& bandIter : state.bandLevels) {
        EqualizerBand band = bandIter.first;
        int bandLevel = bandIter.second;

        rapidjson::Value bandDesc(rapidjson::kObjectType);
        bandDesc.AddMember(JSON_KEY_NAME, equalizerBandToString(band), allocator);
        bandDesc.AddMember(JSON_KEY_LEVEL, bandLevel, allocator);
        bandsBranch.PushBack(bandDesc, allocator);
    }

    payload.AddMember(JSON_KEY_BANDS, bandsBranch, allocator);

    if (EqualizerMode::NONE != state.mode) {
        payload.AddMember(JSON_KEY_MODE, equalizerModeToString(state.mode), allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("serializeEqualizerStateFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

SuccessResult<EqualizerState> EqualizerUtils::deserializeEqualizerState(const std::string& serializedState) {
    rapidjson::Document document;
    rapidjson::ParseResult result = document.Parse(serializedState);
    if (!result) {
        ACSDK_ERROR(LX("deserializeEqualizerStateFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return SuccessResult<EqualizerState>::failure();
    }

    EqualizerState state;

    rapidjson::Value::ConstMemberIterator it;

    if (!findNode(document, JSON_KEY_BANDS, &it) || !it->value.IsArray()) {
        ACSDK_ERROR(LX("deserializeEqualizerStateFailed").d("reason", "'bands' value is missing"));
        return SuccessResult<EqualizerState>::failure();
    }

    for (const auto& bandDesc : it->value.GetArray()) {
        std::string bandName;
        if (!retrieveValue(bandDesc, JSON_KEY_NAME, &bandName)) {
            ACSDK_ERROR(LX("deserializeEqualizerStateFailed").d("reason", "'name' value is missing"));
            return SuccessResult<EqualizerState>::failure();
        }

        SuccessResult<EqualizerBand> bandResult = stringToEqualizerBand(bandName);
        if (!bandResult.isSucceeded()) {
            ACSDK_ERROR(LX("deserializeEqualizerStateFailed").d("reason", "Invalid band").d("band", bandName));
            return SuccessResult<EqualizerState>::failure();
        }

        EqualizerBand band = bandResult.value();

        int64_t bandLevel = 0;
        if (!retrieveValue(bandDesc, JSON_KEY_LEVEL, &bandLevel)) {
            ACSDK_ERROR(LX("deserializeEqualizerStateFailed").d("reason", "Invalid band level").d("band", bandName));
            return SuccessResult<EqualizerState>::failure();
        }

        state.bandLevels[band] = static_cast<int>(bandLevel);
    }

    std::string modeName;
    if (!retrieveValue(document, JSON_KEY_MODE, &modeName)) {
        // Default mode (NONE)
        state.mode = EqualizerMode::NONE;
    } else {
        SuccessResult<EqualizerMode> modeResult = stringToEqualizerMode(modeName);
        if (!modeResult.isSucceeded()) {
            ACSDK_ERROR(LX("deserializeEqualizerStateFailed").d("reason", "Invalid mode").d("band", modeName));
            return SuccessResult<EqualizerState>::failure();
        }
        state.mode = modeResult.value();
    }

    return SuccessResult<EqualizerState>::success(state);
}

}  // namespace equalizer
}  // namespace alexaClientSDK
