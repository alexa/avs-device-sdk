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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERTYPES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERTYPES_H_

#include <array>
#include <string>
#include <unordered_map>

#include <AVSCommon/Utils/functional/hash.h>
#include <AVSCommon/Utils/Error/SuccessResult.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace audio {

/**
 * Enum representing all the equalizer bands supported by SDK. Customer device may support only a subset of these.
 */
enum class EqualizerBand {
    /// Bass equalizer band.
    BASS,
    /// Mid-range equalizer band.
    MIDRANGE,
    /// Treble equalizer band.
    TREBLE
};

/**
 * An iterable array that contains all the values of @c EqualizerBand in order from lower frequencies to higher.
 */
const std::array<EqualizerBand, 3> EqualizerBandValues = {
    {EqualizerBand::BASS, EqualizerBand::MIDRANGE, EqualizerBand::TREBLE}};

/**
 * Enum representing all the equalizer modes supported by SDK. Customer device may support only a subset of these.
 * 'NONE' represents a special mode to reflect custom band values with no named preset.
 */
enum class EqualizerMode {
    /// Equalizer mode representing default (no mode) behavior
    NONE,
    /// Movie equalizer mode.
    MOVIE,
    /// Music equalizer mode.
    MUSIC,
    /// Night equalizer mode.
    NIGHT,
    /// Sport equalizer mode.
    SPORT,
    /// TV equalizer mode.
    TV
};

/**
 * An iterable array that contains all the values of @c EqualizerMode
 */
const std::array<EqualizerMode, 6> EqualizerModeValues = {{EqualizerMode::NONE,
                                                           EqualizerMode::MOVIE,
                                                           EqualizerMode::MUSIC,
                                                           EqualizerMode::NIGHT,
                                                           EqualizerMode::SPORT,
                                                           EqualizerMode::TV}};

/// A collection of bands with their level values. This is an alias for @c EqualizerBand to band level (int) map.
using EqualizerBandLevelMap = std::unordered_map<EqualizerBand, int, utils::functional::EnumClassHash>;

/**
 * Structure to represent Current state of the equalizer.
 */
struct EqualizerState {
    /// Equalizer mode selected. Use @c EqualizerMode::NONE value to represent no specific mode.
    EqualizerMode mode;
    /// Device supported bands with their levels.
    EqualizerBandLevelMap bandLevels;
};

inline bool operator==(const EqualizerState& state1, const EqualizerState& state2) {
    return state1.mode == state2.mode && state1.bandLevels == state2.bandLevels;
}

/**
 * Provides a string representation for @c EqualizerBand value.
 *
 * @param band @c EqualizerBand to convert.
 * @return A string representation for @c EqualizerBand value.
 */
inline std::string equalizerBandToString(EqualizerBand band) {
    switch (band) {
        case EqualizerBand::BASS:
            return "BASS";
        case EqualizerBand::MIDRANGE:
            return "MIDRANGE";
        case EqualizerBand::TREBLE:
            return "TREBLE";
    }

    return "UNKNOWN";
}

/**
 * Write a @c EqualizerBand value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param band The @c EqualizerBand value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, EqualizerBand band) {
    return stream << equalizerBandToString(band);
}

/**
 * Provides a string representation for @c EqualizerMode value.
 *
 * @param mode @c EqualizerMode to convert.
 * @return A string representation for @c EqualizerMode value.
 */
inline std::string equalizerModeToString(EqualizerMode mode) {
    switch (mode) {
        case EqualizerMode::NONE:
            return "NONE";
        case EqualizerMode::MOVIE:
            return "MOVIE";
        case EqualizerMode::MUSIC:
            return "MUSIC";
        case EqualizerMode::NIGHT:
            return "NIGHT";
        case EqualizerMode::SPORT:
            return "SPORT";
        case EqualizerMode::TV:
            return "TV";
    }

    return "UNKNOWN";
}

/**
 * Write a @c EqualizerMode value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param mode The @c EqualizerMode value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, EqualizerMode mode) {
    return stream << equalizerModeToString(mode);
}

/**
 * Parses a string to @c EqualizerBand value.
 *
 * @param stringValue a string containing the value to parse.
 * @return @c SuccessResult describing the result of the operation
 */
inline utils::error::SuccessResult<EqualizerBand> stringToEqualizerBand(const std::string& stringValue) {
    if (stringValue == "BASS") {
        return utils::error::SuccessResult<EqualizerBand>::success(EqualizerBand::BASS);
    }
    if (stringValue == "MIDRANGE") {
        return utils::error::SuccessResult<EqualizerBand>::success(EqualizerBand::MIDRANGE);
    }
    if (stringValue == "TREBLE") {
        return utils::error::SuccessResult<EqualizerBand>::success(EqualizerBand::TREBLE);
    }

    return utils::error::SuccessResult<EqualizerBand>::failure();
}

/**
 * Parses a string to @c EqualizerMode value.
 *
 * @param stringValue a string containing the value to parse.
 * @param[out] mode Pointer to @c EqualizerMode to receive parsed result. If function fails, the value is not
 * changed.
 * @return true if parsing was successful, false otherwise.
 */
inline utils::error::SuccessResult<EqualizerMode> stringToEqualizerMode(const std::string& stringValue) {
    if (stringValue == "NONE") {
        return utils::error::SuccessResult<EqualizerMode>::success(EqualizerMode::NONE);
    }
    if (stringValue == "MOVIE") {
        return utils::error::SuccessResult<EqualizerMode>::success(EqualizerMode::MOVIE);
    }
    if (stringValue == "MUSIC") {
        return utils::error::SuccessResult<EqualizerMode>::success(EqualizerMode::MUSIC);
    }
    if (stringValue == "NIGHT") {
        return utils::error::SuccessResult<EqualizerMode>::success(EqualizerMode::NIGHT);
    }
    if (stringValue == "SPORT") {
        return utils::error::SuccessResult<EqualizerMode>::success(EqualizerMode::SPORT);
    }
    if (stringValue == "TV") {
        return utils::error::SuccessResult<EqualizerMode>::success(EqualizerMode::TV);
    }

    return utils::error::SuccessResult<EqualizerMode>::failure();
}

}  // namespace audio
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AUDIO_EQUALIZERTYPES_H_
