/*
 * PortAudioMicrophoneWrapper.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_WEBEXTENSION_INCLUDE_WEBEXTENSION_PORTAUDIOMICROPHONEWRAPPER_H_
#define ALEXA_CLIENT_SDK_WEBEXTENSION_INCLUDE_WEBEXTENSION_PORTAUDIOMICROPHONEWRAPPER_H_

#include <mutex>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>

#include <portaudio.h>

namespace alexaClientSDK {
namespace webExtension {

/// This acts as a wrapper around PortAudio, a cross-platform open-source audio I/O library.
class PortAudioMicrophoneWrapper {
public:
    /**
     * Creates a @c PortAudioMicrophoneWrapper.
     *
     * @param stream The shared data stream to write to.
     * @return A unique_ptr to a @c PortAudioMicrophoneWrapper if creation was successful and @c nullptr otherwise.
     */
    static std::unique_ptr<PortAudioMicrophoneWrapper> create(std::shared_ptr<avsCommon::avs::AudioInputStream> stream);

    /**
     * Stops streaming from the microphone.
     *
     * @return Whether the stop was successful.
     */
    bool stopStreamingMicrophoneData();

    /**
     * Starts streaming from the microphone.
     *
     * @return Whether the start was successful.
     */
    bool startStreamingMicrophoneData();

    /**
     * Destructor.
     */
    ~PortAudioMicrophoneWrapper();

private:
    /**
     * Constructor.
     *
     * @param stream The shared data stream to write to.
     */
    PortAudioMicrophoneWrapper(std::shared_ptr<avsCommon::avs::AudioInputStream> stream);

    /**
     * The callback that PortAudio will issue when audio is avaiable to read.
     *
     * @param inputBuffer The temporary buffer that microphone audio data will be available in.
     * @param outputBuffer Not used here.
     * @param numSamples The number of samples available to consume.
     * @param timeInfo Time stamps indicated when the first sample in the buffer was captured. Not used here.
     * @param statusFlags Flags that tell us when underflow or overflow conditions occur. Not used here.
     * @param userData A user supplied pointer.
     * @return A PortAudio code that will indicate how PortAudio should continue.
     */
    static int PortAudioCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long numSamples,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

    /// Initializes PortAudio
    bool initialize();

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_audioInputStream;

    /// The writer that will be used to writer audio data into the sds.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Writer> m_writer;

    /// The PortAudio stream
    PaStream* m_paStream;

    /**
     * A lock to seralize access to startStreamingMicrophoneData() and stopStreamingMicrophoneData() between different
     * threads.
     */
    std::mutex m_mutex;
};

}  // namespace webExtension
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_WEBEXTENSION_INCLUDE_WEBEXTENSION_PORTAUDIOMICROPHONEWRAPPER_H_
