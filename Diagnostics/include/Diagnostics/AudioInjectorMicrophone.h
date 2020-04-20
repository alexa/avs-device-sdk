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

#ifndef ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_AUDIOINJECTORMICROPHONE_H_
#define ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_AUDIOINJECTORMICROPHONE_H_

#include <memory>
#include <mutex>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <Audio/MicrophoneInterface.h>

#include "Diagnostics/DiagnosticsUtils.h"

namespace alexaClientSDK {
namespace diagnostics {

/// This represents a microphone which injects audio data into the shared data stream.
class AudioInjectorMicrophone : public applicationUtilities::resources::audio::MicrophoneInterface {
public:
    /**
     * Creates a @c AudioInjectorMicrophone.
     *
     * @param stream The shared data stream to write to.
     * @param compatibleAudioFormat The audio format.
     * @return A unique_ptr to a @c AudioInjectorMicrophone if creation was successful and @c nullptr otherwise.
     */
    static std::unique_ptr<AudioInjectorMicrophone> create(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
        const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat);

    /// @name MicrophoneInterface methods
    /// @{
    bool stopStreamingMicrophoneData() override;
    bool startStreamingMicrophoneData() override;
    bool isStreaming() override;
    /// @}

    /**
     * Injects audio into the audio buffer at the next possible moment.
     * If the microphone is muted, audio will be injected when the microphone is unmuted.
     *
     * @param audioData The reference to the audioBuffer vector.
     */
    void injectAudio(const std::vector<uint16_t>& audioData);

    /**
     * Destructor.
     */
    ~AudioInjectorMicrophone();

private:
    /**
     * Constructor.
     *
     * @param stream The shared data stream to write to.
     * @param compatibleAudioFormat The audio format.
     */
    AudioInjectorMicrophone(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& stream,
        const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat);

    /*
     * Reset m_injectionData to empty and m_injectionDataCounter to 0.
     */
    void resetAudioInjection();

    /**
     * Initializes the microphone.
     */
    bool initialize();

    /**
     * Starts timer for writing data to the shared stream.
     */
    void startTimer();

    /**
     * Writes data to the shared stream.
     */
    void write();

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_audioInputStream;

    /// A lock to serialize access to the writer between different threads.
    std::mutex m_mutex;

    /// The writer that will be used to write audio data into the sds.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Writer> m_writer;

    /// Whether the microphone is streaming data.
    bool m_isStreaming;

    /// Timer that is responsible for writing to the shared data stream.
    avsCommon::utils::timing::Timer m_timer;

    /// Maximum of samples per timeout period.
    unsigned int m_maxSampleCountPerTimeout;

    /// The audio buffer of silence (0's).
    alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer m_silenceBuffer;

    /// The audio injection vector.
    std::vector<uint16_t> m_injectionData;

    /// Counter for how much of current audio injection data has been injected so far.
    unsigned long m_injectionDataCounter;
};

}  // namespace diagnostics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_AUDIOINJECTORMICROPHONE_H_
