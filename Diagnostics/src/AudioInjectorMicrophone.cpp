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

#include "Diagnostics/AudioInjectorMicrophone.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace diagnostics {

using avsCommon::avs::AudioInputStream;
using namespace avsCommon::utils::timing;

/// String to identify log entries originating from this file.
#define TAG "AudioInjectorMicrophone"

/// The timeout to use for writing to the SharedDataStream.
static const std::chrono::milliseconds TIMEOUT_FOR_WRITING{500};

/// Milliseconds per second.
static constexpr unsigned int MILLISECONDS_PER_SECOND = 1000;

/*
 * Calculate the maximum number of samples to be written to the shared stream per the writing timeout.
 *
 * @param sampleRateHz The audio format's sample rate Hz.
 * @return The maximum number of samples to write per write timeout.
 */
static inline unsigned int calculateMaxSampleCountPerTimeout(unsigned int sampleRateHz) {
    return (sampleRateHz / MILLISECONDS_PER_SECOND) * TIMEOUT_FOR_WRITING.count();
}

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<AudioInjectorMicrophone> AudioInjectorMicrophone::create(
    const std::shared_ptr<AudioInputStream>& stream,
    const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat) {
    if (!stream) {
        ACSDK_ERROR(LX("createFileBasedMicrophoneFailed").d("reason", "invalid stream"));
        return nullptr;
    }
    std::unique_ptr<AudioInjectorMicrophone> fileBasedMicrophone(
        new AudioInjectorMicrophone(stream, compatibleAudioFormat));
    if (!fileBasedMicrophone->initialize()) {
        ACSDK_ERROR(LX("createFileBasedMicrophoneFailed").d("reason", "failed to initialize microphone"));
        return nullptr;
    }
    return fileBasedMicrophone;
}

AudioInjectorMicrophone::AudioInjectorMicrophone(
    const std::shared_ptr<AudioInputStream>& stream,
    const alexaClientSDK::avsCommon::utils::AudioFormat& compatibleAudioFormat) :
        m_audioInputStream{std::move(stream)},
        m_isStreaming{false},
        m_maxSampleCountPerTimeout{calculateMaxSampleCountPerTimeout(compatibleAudioFormat.sampleRateHz)},
        m_injectionData{std::vector<uint16_t>()},
        m_injectionDataCounter{0} {
    /// Fill silence buffer with 0's for writing to shared stream.
    m_silenceBuffer = alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer(m_maxSampleCountPerTimeout);
    std::fill(m_silenceBuffer.begin(), m_silenceBuffer.end(), 0);
}

AudioInjectorMicrophone::~AudioInjectorMicrophone() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timer.stop();
}

bool AudioInjectorMicrophone::initialize() {
    m_writer = m_audioInputStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    if (!m_writer) {
        ACSDK_ERROR(LX("initializeFileBasedMicrophoneFailed").d("reason", "failed to create stream writer"));
        return false;
    }

    return true;
}

bool AudioInjectorMicrophone::isStreaming() {
    ACSDK_DEBUG5(LX(__func__).d("isStreaming", m_isStreaming));
    return m_isStreaming;
}

bool AudioInjectorMicrophone::startStreamingMicrophoneData() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isStreaming = true;
    startTimer();
    return true;
}

void AudioInjectorMicrophone::startTimer() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_timer.isActive()) {
        m_timer.start(
            std::chrono::milliseconds(0),
            TIMEOUT_FOR_WRITING,
            Timer::PeriodType::RELATIVE,
            Timer::getForever(),
            std::bind(&AudioInjectorMicrophone::write, this));
    }
}

bool AudioInjectorMicrophone::stopStreamingMicrophoneData() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isStreaming = false;
    resetAudioInjection();
    m_timer.stop();
    return true;
}

void AudioInjectorMicrophone::injectAudio(const std::vector<uint16_t>& audioData) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_injectionData = audioData;
    m_injectionDataCounter = 0;
}

void AudioInjectorMicrophone::write() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isStreaming) {
        // If there is no audio to inject, write silence to the shared data stream at the sample rate.
        if (m_injectionData.empty()) {
            ssize_t writeResult = m_writer->write(
                static_cast<void*>(m_silenceBuffer.data()), m_maxSampleCountPerTimeout, TIMEOUT_FOR_WRITING);
            if (writeResult <= 0) {
                switch (writeResult) {
                    case avsCommon::avs::AudioInputStream::Writer::Error::WOULDBLOCK:
                    case avsCommon::avs::AudioInputStream::Writer::Error::INVALID:
                    case avsCommon::avs::AudioInputStream::Writer::Error::CLOSED:
                        ACSDK_ERROR(LX("writeSilenceFailed")
                                        .d("reason", "failed to write to stream")
                                        .d("errorCode", writeResult));
                        break;
                    case avsCommon::avs::AudioInputStream::Writer::Error::TIMEDOUT:
                        ACSDK_DEBUG9(LX("writeSilenceTimedOut"));
                        break;
                }
            } else {
                ACSDK_DEBUG9(LX("writeSilence").d("wordsWritten", writeResult));
            }
        } else {
            // Otherwise, write audio injection file to the shared stream.

            // Sanity check.
            if (m_injectionDataCounter >= m_injectionData.size()) {
                ACSDK_ERROR(LX("injectAudioFailed")
                                .d("reason", "bufferOverrun")
                                .d("overrun", m_injectionDataCounter - m_injectionData.size()));
                resetAudioInjection();
                return;
            }

            // m_injectionData.size() is guaranteed to be greater than m_injectDataCounter
            size_t injectionDataLeft = m_injectionData.size() - m_injectionDataCounter;
            size_t amountToWrite =
                (m_maxSampleCountPerTimeout > injectionDataLeft) ? injectionDataLeft : m_maxSampleCountPerTimeout;

            ssize_t writeResult = m_writer->write(
                static_cast<void*>(m_injectionData.data() + m_injectionDataCounter),
                amountToWrite,
                TIMEOUT_FOR_WRITING);

            if (writeResult <= 0) {
                switch (writeResult) {
                    case avsCommon::avs::AudioInputStream::Writer::Error::WOULDBLOCK:
                    case avsCommon::avs::AudioInputStream::Writer::Error::INVALID:
                    case avsCommon::avs::AudioInputStream::Writer::Error::CLOSED:
                        ACSDK_ERROR(LX("injectAudioFailed").d("error", writeResult));
                        resetAudioInjection();
                        break;
                    case avsCommon::avs::AudioInputStream::Writer::Error::TIMEDOUT:
                        // No need to reset audio injection here; simply retry until a reader
                        // frees space in the SDS for writing.
                        ACSDK_DEBUG9(LX("injectAudioTimedOut"));
                        break;
                }
            } else {
                ACSDK_DEBUG9(LX("injectAudio").d("wordsWritten", writeResult));
                m_injectionDataCounter += writeResult;

                // All audio has been injected.
                if (m_injectionDataCounter == m_injectionData.size()) {
                    resetAudioInjection();
                }
            }
        }
    }
}

void AudioInjectorMicrophone::resetAudioInjection() {
    m_injectionData.clear();
    m_injectionDataCounter = 0;
}

}  // namespace diagnostics
}  // namespace alexaClientSDK
