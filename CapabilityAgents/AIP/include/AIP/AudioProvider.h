/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOPROVIDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOPROVIDER_H_

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include "ASRProfile.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

/**
 * Wrapper for an audio input @c ByteStream which includes information about the audio format and policies for using
 * it.
 */
struct AudioProvider {
    /**
     * Initialization constructor.
     *
     * @param stream The @c ByteStream to use for audio input.
     * @param format The @c AudioFormat of the data in @c byteStream.
     * @param profile The @c AudioProfile describing the acoustic environment for the audio input.
     * @param alwaysReadable Whether new audio data can be read at any time from @c byteStream.  This must be @c true
     *     for a stream to be automatically opened up by an @c ExpectSpeech directive.
     * @param canOverride Whether this @c AudioProvider should be allowed to interrupt/override another
     *     @c AudioProvider.
     * @param canBeOverridden Whether this @c AudioProvider allows another @c AudioProvider to interrupt it.
     */
    AudioProvider(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        const avsCommon::utils::AudioFormat& format,
        ASRProfile profile,
        bool alwaysReadable,
        bool canOverride,
        bool canBeOverridden);

    /**
     * This function provides an invalid AudioProvider which has no stream associated with it.
     *
     * @return An invalid AudioProvider which has no stream associated with it.
     */
    static const AudioProvider& null();

    /**
     * This function checks whether this is a valid @c AudioProvider.  An @c AudioProvider is valid if it has a stream
     * associated with it.
     *
     * @return @c true if `stream != nullptr`, else @c false.
     */
    operator bool() const;

    /// The @c ByteStream to use for audio input.
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream;

    /// The @c AudioFormat of the data in @c byteStream.
    avsCommon::utils::AudioFormat format;

    /// The @c ASRProfile describing the acoustic environment for the audio input.
    ASRProfile profile;

    /// Whether new audio data can be read at any time from @c byteStream.
    bool alwaysReadable;

    /// Whether this @c AudioProvider should be allowed to interrupt/override another @c AudioProvider.
    bool canOverride;

    /// Whether this @c AudioProvider should allow another @c AudioProvider to interrupt it.
    bool canBeOverridden;
};

inline AudioProvider::AudioProvider(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    const avsCommon::utils::AudioFormat& format,
    ASRProfile profile,
    bool alwaysReadable,
    bool canOverride,
    bool canBeOverridden)
        // Note: There is an issue with braced initialization of an aggregate type in GCC 4.8, so parentheses are used
        //     below to initialize format.
        :
        stream{stream},
        format(format),
        profile{profile},
        alwaysReadable{alwaysReadable},
        canOverride{canOverride},
        canBeOverridden{canBeOverridden} {
}

inline const AudioProvider& AudioProvider::null() {
    static AudioProvider nullAudioProvider{nullptr,
                                           {avsCommon::utils::AudioFormat::Encoding::LPCM,
                                            avsCommon::utils::AudioFormat::Endianness::LITTLE,
                                            0,
                                            0,
                                            0,
                                            true,
                                            avsCommon::utils::AudioFormat::Layout::NON_INTERLEAVED},
                                           ASRProfile::CLOSE_TALK,
                                           false,
                                           false,
                                           false};
    return nullAudioProvider;
}

inline AudioProvider::operator bool() const {
    return stream != nullptr;
}

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_AUDIOPROVIDER_H_
