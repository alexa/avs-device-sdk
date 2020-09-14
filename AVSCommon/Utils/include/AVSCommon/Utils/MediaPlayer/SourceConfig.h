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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_SOURCECONFIG_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_SOURCECONFIG_H_

#include <algorithm>
#include <chrono>
#include <ostream>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace mediaPlayer {

/// Maximum gain supported.
static constexpr short MAX_GAIN = 100;

/// Minimum gain supported.
static constexpr short MIN_GAIN = 0;

/*
 * An object that contains configuration for a source media.
 */
struct SourceConfig {
    /*
     * Object that contains Fade-In configuration.
     */
    struct FadeInConfig {
        /// The starting percentage volume when the media starts playing (range 0-100).
        short startGain;

        /// The ending percentage volume when the media played to the fade-in duration (range 0-100).
        short endGain;

        /// The fade-in duration time.
        std::chrono::milliseconds duration;

        /// A boolean to specify whether Fade-In is enabled.
        bool enabled;
    };

    /*
     * Object that contains audio normalization configuration
     */
    struct AudioNormalizationConfig {
        /*
         * Enable audio normalization.
         * This is an optional feature and could be safely ignored
         * if not supported by MediaPlayer implementation.
         */
        bool enabled;
    };

    /// Fade-In configuration.
    FadeInConfig fadeInConfig;

    /// Audio normalization configuration
    AudioNormalizationConfig audioNormalizationConfig;

    /// End offset for where playback should be stopped
    std::chrono::milliseconds endOffset;

    /**
     * Builds a Source Config object with fade in enabled.
     *
     * @param startGain The starting percentage volume when the media starts playing (range 0-100).
     * @param endGain The ending percentage volume when the media played to the fade-in duration (range 0-100).
     * @param duration The fade-in duration time.
     */
    static inline SourceConfig createWithFadeIn(
        short startGain,
        short endGain,
        const std::chrono::milliseconds& duration);
};

/**
 * Builds a Source Config object with fade in disabled.
 * @return a Source Config object without fade in
 */
inline SourceConfig emptySourceConfig() {
    return SourceConfig{
        {100, 100, std::chrono::milliseconds::zero(), false}, {false}, std::chrono::milliseconds::zero()};
}

/**
 * Builds a Source Config object with fade in enabled and with the provided valid values.
 *
 * @param startGain The starting percentage volume when the media starts playing (range 0-100) Will become valid.
 * @param endGain The ending percentage volume when the media played to the fade-in duration (range 0-100). Will become
 * valid.
 * @param duration The fade-in duration time.
 * @return a Source Config object with fade in set for the given valid values
 */
inline SourceConfig SourceConfig::createWithFadeIn(
    short startGain,
    short endGain,
    const std::chrono::milliseconds& duration) {
    short validStartGain = std::max(MIN_GAIN, std::min(MAX_GAIN, startGain));
    short validEndGain = std::max(MIN_GAIN, std::min(MAX_GAIN, endGain));
    return SourceConfig{{validStartGain, validEndGain, duration, true}, {false}, std::chrono::milliseconds::zero()};
}

/**
 * Write a @c SourceConfig value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param config The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const SourceConfig& config) {
    return stream << "fadeIn{"
                  << " enabled:" << config.fadeInConfig.enabled << ", start:" << config.fadeInConfig.startGain
                  << ", end:" << config.fadeInConfig.endGain
                  << ", duration(ms):" << config.fadeInConfig.duration.count() << "}"
                  << ", normalization{"
                  << " enabled: " << config.audioNormalizationConfig.enabled << "}"
                  << ", endOffset(ms): " << config.endOffset.count();
}

}  // namespace mediaPlayer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MEDIAPLAYER_SOURCECONFIG_H_
