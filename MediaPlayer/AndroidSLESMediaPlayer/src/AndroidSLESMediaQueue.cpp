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

#include <fstream>

#include <SLES/OpenSLES_Android.h>

#include "AndroidSLESMediaPlayer/AndroidSLESMediaQueue.h"

/// String to identify log entries originating from this file.
static const std::string TAG{"AndroidSLESMediaQueue"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/// Represents the most significant byte of silence for unsigned samples. Since we only support UNSIGNED_8, this
/// byte can be used as silence representation. For samples with 2+ bytes, shift this value by the number of extra bits.
constexpr AndroidSLESMediaQueue::Byte PCM_UNSIGNED_SILENCE = 0x80;

/// Represents the most significant byte of silence for signed samples.
constexpr AndroidSLESMediaQueue::Byte PCM_SIGNED_SILENCE = 0x0;

using namespace applicationUtilities::androidUtilities;

static void queueCallback(SLAndroidSimpleBufferQueueItf slQueue, void* mediaQueue) {
    static_cast<AndroidSLESMediaQueue*>(mediaQueue)->onBufferFree();
};

std::unique_ptr<AndroidSLESMediaQueue> AndroidSLESMediaQueue::create(
    std::shared_ptr<AndroidSLESObject> queueObject,
    std::unique_ptr<DecoderInterface> decoder,
    EventCallback callbackFunction,
    const PlaybackConfiguration& playbackConfig) {
    if (!queueObject) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAndroidSLESObject"));
        return nullptr;
    }

    if (!decoder) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDecoder"));
        return nullptr;
    }

    if (!callbackFunction) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptyCallback"));
        return nullptr;
    }

    SLAndroidSimpleBufferQueueItf queueInterface;
    if (!queueObject->getInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &queueInterface)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "BufferQueueInterfaceUnavailable"));
        return nullptr;
    }

    // Create target object.
    auto mediaQueue = std::unique_ptr<AndroidSLESMediaQueue>(
        new AndroidSLESMediaQueue(queueObject, queueInterface, std::move(decoder), callbackFunction));

    // Register callback on the buffer queue
    if ((*queueInterface)->RegisterCallback(queueInterface, queueCallback, mediaQueue.get()) != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("createFailed").d("reason", "registerCallbackFailed"));
        return nullptr;
    }

    // Kickoff decoding and buffer enqueue.
    mediaQueue->fillAllBuffers(playbackConfig);

    return mediaQueue;
}

void AndroidSLESMediaQueue::onBufferFree() {
    m_executor.submit([this]() { fillBuffer(); });
}

AndroidSLESMediaQueue::~AndroidSLESMediaQueue() {
    // Remove callback before cleanup.
    auto result = (*m_queueInterface)->RegisterCallback(m_queueInterface, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_WARN(LX("removeCallbackFailed").d("result", result));
    }

    m_decoder->abort();

    m_executor.waitForSubmittedTasks();
    m_executor.shutdown();

    result = (*m_queueInterface)->Clear(m_queueInterface);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_WARN(LX("clearBuffersFailed").d("result", result));
    }
}

AndroidSLESMediaQueue::AndroidSLESMediaQueue(
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESObject> queueObject,
    SLAndroidSimpleBufferQueueItf bufferQueue,
    std::unique_ptr<DecoderInterface> decoder,
    EventCallback callbackFunction) :
        m_slObject{queueObject},
        m_queueInterface{bufferQueue},
        m_bufferSizes(NUMBER_OF_BUFFERS, 0),
        m_decoder{std::move(decoder)},
        m_index{0u},
        m_inputEof{false},
        m_failure{false},
        m_eventCallback{callbackFunction},
        m_bufferedWords{0},
        m_playedWords{0} {
}

void AndroidSLESMediaQueue::fillBuffer() {
    if (!m_failure) {
        // Cache index of the buffer used in this function before incrementing m_index. Note that we need to increment
        // index even if EOF has happened because of the playback position logic.
        auto index = m_index;
        m_index = (m_index + 1) % NUMBER_OF_BUFFERS;

        // Update playback position with the duration of the buffer that has finished playing.
        m_playedWords += m_bufferSizes[index];
        m_bufferedWords -= m_bufferSizes[index];
        m_bufferSizes[index] = 0;

        if (!m_inputEof) {
            size_t wordsRead;
            DecoderInterface::Status status;
            std::tie(status, wordsRead) = m_decoder->read(m_buffers[index].data(), m_buffers[index].size());
            m_bufferSizes[index] = wordsRead;
            m_bufferedWords += wordsRead;

            if (!m_inputEof && DecoderInterface::Status::DONE == status) {
                m_eventCallback(QueueEvent::FINISHED_READING, "");
                m_inputEof = true;
            }

            if (wordsRead) {
                auto bytesRead = wordsRead * sizeof(m_buffers[index][0]);
                auto result = (*m_queueInterface)->Enqueue(m_queueInterface, m_buffers[index].data(), bytesRead);
                if (result != SL_RESULT_SUCCESS) {
                    ACSDK_ERROR(
                        LX("fillBufferFailed").d("reason", "enqueueFailed").d("result", result).d("bytes", bytesRead));
                    m_eventCallback(QueueEvent::ERROR, "reason=enqueueBufferFailed");
                    m_failure = true;
                    return;
                }
            }

            if (DecoderInterface::Status::ERROR == status) {
                ACSDK_ERROR(LX("fillBufferFailed").d("reason", "decodingFailed"));
                m_eventCallback(QueueEvent::ERROR, "reason=decodingFailed");
                m_failure = true;
                return;
            }
        } else {
            SLAndroidSimpleBufferQueueState queueState;
            auto result = (*m_queueInterface)->GetState(m_queueInterface, &queueState);
            if (result != SL_RESULT_SUCCESS) {
                ACSDK_ERROR(LX("enqueueBufferFailed").d("reason", "getStateFailed").d("result", result));
                m_eventCallback(QueueEvent::ERROR, "reason=getQueueStatusFailed");
                m_failure = true;
                return;
            }

            if (0 == queueState.count) {
                ACSDK_DEBUG5(LX("emptyQueue"));
                m_eventCallback(QueueEvent::FINISHED_PLAYING, "");
            }
        }
    } else {
        ACSDK_ERROR(LX("fillBufferFailed").d("reason", "previousIterationFailed"));
    }
}

// Add silent sample to workaround android play after stop issue (see ACSDK-1895 for more details).
void AndroidSLESMediaQueue::enqueueSilence(const PlaybackConfiguration& configuration) {
    Byte silenceByte = configuration.sampleFormat() == PlaybackConfiguration::SampleFormat::UNSIGNED_8
                           ? PCM_UNSIGNED_SILENCE
                           : PCM_SIGNED_SILENCE;
    std::vector<Byte> silenceSample(configuration.numberChannels() * configuration.sampleSizeBytes(), silenceByte);
    memcpy(m_buffers[m_index].data(), silenceSample.data(), silenceSample.size());
    auto result = (*m_queueInterface)->Enqueue(m_queueInterface, m_buffers[m_index].data(), silenceSample.size());
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("enqueueSilenceFailed")
                        .d("reason", "enqueueFailed")
                        .d("result", result)
                        .d("bytes", silenceSample.size()));
        m_eventCallback(QueueEvent::ERROR, "reason=enqueueBufferFailed");
        m_failure = true;
        return;
    }
    m_index++;
}

void AndroidSLESMediaQueue::fillAllBuffers(const PlaybackConfiguration& configuration) {
    enqueueSilence(configuration);
    for (; m_index < m_buffers.size(); ++m_index) {
        m_executor.submit([this]() { fillBuffer(); });
    }
    m_index = 0;
}

size_t AndroidSLESMediaQueue::getNumBytesPlayed() const {
    return sizeof(m_buffers[0][0]) * m_playedWords;
}

size_t AndroidSLESMediaQueue::getNumBytesBuffered() const {
    return sizeof(m_buffers[0][0]) * m_bufferedWords;
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
