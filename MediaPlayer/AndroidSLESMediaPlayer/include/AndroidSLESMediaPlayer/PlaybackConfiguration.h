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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_PLAYBACKCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_PLAYBACKCONFIGURATION_H_

#include <cstddef>
#include <cstdint>

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * An object of this class holds information about the playback configuration used by the Android media player.
 *
 * By default, the playback configuration is:
 *
 * - 16bit Linear PCM
 * - 48kHz sample rate
 * - 2 channels (Left | Right)
 * - Native-endianness
 *
 * To change the default configuration, user can either:
 * - Provide a ConfigurationNode to create method. All the configurations found in the node will override the default
 *   values.
 * - Create the playback configuration object with explicit configuration values for all fields.
 */
class PlaybackConfiguration {
public:
    /**
     * Enum with the supported channel layouts.
     */
    enum class ChannelLayout {
        LAYOUT_MONO,
        LAYOUT_STEREO,
        LAYOUT_SURROUND,
        LAYOUT_5POINT1,
    };

    /**
     * The sample format related to signess and number of bits.
     *
     * @note: DO NOT use the enum underlying values. This is for internal usage and it may change in the future.
     */
    enum class SampleFormat { UNSIGNED_8 = 8, SIGNED_16 = 16, SIGNED_32 = 32 };

    /**
     * Check whether the configuration is little endian.
     *
     * @return @c true if endianness is little endian and @false if big endian.
     */
    inline bool isLittleEndian() const;

    /**
     * Get the sample rate in Hz.
     *
     * @return The sample rate.
     */
    inline size_t sampleRate() const;

    /**
     * Get the sample format.
     *
     * @return The sample format.
     */
    inline SampleFormat sampleFormat() const;

    /**
     * Get the channel layout.
     *
     * @return The channel layout.
     */
    inline ChannelLayout channelLayout() const;

    /**
     * Get the number of channels used.
     *
     * @return The number of channels.
     */
    inline size_t numberChannels() const;

    /**
     * Get the number of bytes per sample.
     */
    inline size_t sampleSizeBytes() const;

    /**
     * Destructor.
     */
    ~PlaybackConfiguration() = default;

    /**
     * Constructor where the new object will have default configuration values.
     */
    PlaybackConfiguration();

    /**
     * Configurable constructor that can be used to set custom configuration values.
     */
    PlaybackConfiguration(
        const bool isLittleEndian,
        const size_t sampleRate,
        const ChannelLayout layout,
        const SampleFormat sampleFormat);

private:
    /// Convert layout to number of channels.
    static size_t layoutToChannels(ChannelLayout layout);

    /// Boolean to check if the playback format is little endian or not.
    const bool m_isLittleEndian;

    /// The playback sample rate.
    const size_t m_sampleRate;

    /// The playback sample format.
    const SampleFormat m_sampleFormat;

    /// The channel layout.
    const ChannelLayout m_layout;

    /// The number of channels.
    const size_t m_numberChannels;
};

bool PlaybackConfiguration::isLittleEndian() const {
    return m_isLittleEndian;
}

size_t PlaybackConfiguration::sampleRate() const {
    return m_sampleRate;
}

PlaybackConfiguration::SampleFormat PlaybackConfiguration::sampleFormat() const {
    return m_sampleFormat;
}

size_t PlaybackConfiguration::numberChannels() const {
    return m_numberChannels;
}

size_t PlaybackConfiguration::sampleSizeBytes() const {
    constexpr size_t byteSize = 8;
    return static_cast<size_t>(m_sampleFormat) / byteSize;
}

PlaybackConfiguration::ChannelLayout PlaybackConfiguration::channelLayout() const {
    return m_layout;
}

inline std::ostream& operator<<(std::ostream& stream, const PlaybackConfiguration::SampleFormat& format) {
    switch (format) {
        case PlaybackConfiguration::SampleFormat::UNSIGNED_8:
            return stream << "UNSIGNED_8";
        case PlaybackConfiguration::SampleFormat::SIGNED_16:
            return stream << "SIGNED_16";
        case PlaybackConfiguration::SampleFormat::SIGNED_32:
            return stream << "SIGNED_32";
    }
    return stream << "INVALID";
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_PLAYBACKCONFIGURATION_H_
