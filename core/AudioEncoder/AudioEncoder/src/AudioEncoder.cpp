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

#include <iostream>
#include <climits>
#include <fstream>

#include <acsdk/AudioEncoder/private/AudioEncoder.h>
#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace audioEncoder {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::utils::error;
using namespace audioEncoderInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AudioEncoder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

AudioEncoder::AudioEncoder(
    const std::shared_ptr<BlockAudioEncoderInterface>& encoder,
    const AudioEncoderParams& params) :
        m_encoder{encoder},
        m_params{params},
        m_executor{},
        m_state{AudioEncoderState::IDLE},
        m_wordsInInputBuffer{0} {
    m_timeoutTimePoint = Clock::time_point();
}

AudioEncoder::~AudioEncoder() {
    stopEncoding(true);
    m_executor.waitForSubmittedTasks();
    m_executor.shutdown();
}

std::shared_ptr<AudioInputStream> AudioEncoder::startEncoding(
    const std::shared_ptr<AudioInputStream>& inputStream,
    AudioFormat inputFormat,
    AudioInputStream::Index begin,
    AudioInputStream::Reader::Reference reference) {
    if (!inputStream) {
        ACSDK_ERROR(LX("startEncodingFailed").d("reason", "inputStreamNull"));
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (AudioEncoderState::IDLE != m_state) {
        ACSDK_ERROR(LX("startEncodingFailed").d("state", m_state));
        return nullptr;
    }

    if (!m_encoder->init(inputFormat)) {
        ACSDK_ERROR(LX("startEncodingFailed").d("reason", "Encoder init failed"));
        return nullptr;
    }

    bool success = false;
    FinallyGuard errorCleanup{[&] {
        // cppcheck-suppress knownConditionTrueFalse
        if (!success) {
            m_inputStream.reset();
            m_outputStream.reset();
            m_state = AudioEncoderState::IDLE;
        }
    }};

    m_requireFullFrameSize = m_encoder->requiresFullyRead();
    m_maxInputFrameSizeWords = m_encoder->getInputFrameSize();
    m_maxOutputFrameSizeBytes = m_encoder->getOutputFrameSize();
    m_outputWordSizeBytes = m_encoder->getAudioFormat().sampleSizeInBits / CHAR_BIT;
    m_outputBufferSizeBytes = AudioInputStream::calculateBufferSize(
        m_maxOutputFrameSizeBytes * m_params.maxOutputStreamBufferedPackets,
        m_outputWordSizeBytes,
        m_params.maxOutputStreamReaders);

    m_state = AudioEncoderState::ENCODING;
    m_inputStream = inputStream;
    m_inputWordSizeBytes = inputStream->getWordSize();
    m_wordsInInputBuffer = 0;
    m_inputBuffer.resize(m_maxInputFrameSizeWords * m_inputWordSizeBytes);
    m_outputBuffer.clear();
    m_outputBuffer.reserve(m_maxOutputFrameSizeBytes + m_outputWordSizeBytes);

    auto buffer = std::make_shared<AudioInputStream::Buffer>(m_outputBufferSizeBytes);
    m_outputStream = AudioInputStream::create(buffer, m_outputWordSizeBytes, m_params.maxOutputStreamReaders);
    if (!m_outputStream) {
        ACSDK_ERROR(LX("startEncodingFailed").d("reason", "AudioInputStream creation failed"));
        return nullptr;
    }

    auto result = m_outputStream;

    ACSDK_DEBUG0(LX("startEncoding").d("begin", begin));
    m_executor.execute([this, begin, reference]() { encodeLoop(begin, reference); });

    success = true;
    return result;
}

void AudioEncoder::stopEncoding(bool stopImmediately) {
    ACSDK_DEBUG0(LX("stopEncoding").d("stopImmediately", stopImmediately));
#ifdef ACSDK_DEBUG_LOG_ENABLED
    const auto start = Clock::now();
#else
    // This variable is not used when debug logging is disabled, but we need it for logging macro evaluation.
    Clock::time_point start;
#endif
    std::unique_lock<std::mutex> lock(m_mutex);

    ACSDK_DEBUG7(LX("stopEncoding").d("state", m_state));

    if (AudioEncoderState::ENCODING == m_state || AudioEncoderState::STOPPING == m_state) {
        if (stopImmediately) {
            m_state = AudioEncoderState::ABORTING;
        } else {
            m_state = AudioEncoderState::STOPPING;
            m_timeoutTimePoint = Clock::now() + m_params.stopTimeoutMs;
        }
        m_cond.wait(lock, [this] { return AudioEncoderState::IDLE == m_state; });
    }

    ACSDK_DEBUG7(LX("stopEncodingSuccess")
                     .d("time", std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count()));
}

AudioFormat::Encoding AudioEncoder::getEncoding() {
    return m_encoder->getAudioFormat().encoding;
}

std::string AudioEncoder::getAVSFormatName() {
    return m_encoder->getAVSFormatName();
}

void AudioEncoder::encodeLoop(AudioInputStream::Index begin, AudioInputStream::Reader::Reference reference) {
    ACSDK_DEBUG5(LX("encodeLoopStarted"));

    // When we return, the state is IDLE
    FinallyGuard markCompleted{[this] { completeEncodingAndNotifyWaiters(); }};

    // Prepare reader
    m_inputStreamReader = m_inputStream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_inputStreamReader) {
        ACSDK_ERROR(LX("encodeLoopError").d("reason", "createReaderFailed"));
        return;
    }
    // Close and release reader on return
    FinallyGuard readerClose{[this] { m_inputStreamReader->close(); }};
    if (!m_inputStreamReader->seek(begin, reference)) {
        ACSDK_ERROR(LX("encodeLoopError").d("reason", "readerSeekFailed"));
        return;
    }

    // Prepare writer
    m_outputStreamWriter = m_outputStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    if (!m_outputStreamWriter) {
        ACSDK_ERROR(LX("encodeLoopError").d("reason", "createWriterFailed"));
        return;
    }
    // Close writer and release on return
    FinallyGuard writerClose{[this]() { m_outputStreamWriter->close(); }};

    // Prepare encoder
    if (!m_encoder->start(m_outputBuffer)) {
        ACSDK_ERROR(LX("encodeLoopError").d("reason", "encoderStartFailed"));
        return;
    }
    // Stop encoder on return
    FinallyGuard encoderClose{[this] { m_encoder->close(); }};

    if (!writeEncodedWordsFromOutput()) {
        ACSDK_DEBUG0(LX("encodeLoopError").d("reason", "failedToWritePrologue"));
        return;
    }

    while (readWordsFromInput() && encodeWordsIntoOutput() && writeEncodedWordsFromOutput())
        ;

    if (mayFinishEncoding()) {
        if (m_encoder->flush(m_outputBuffer)) {
            writeEncodedWordsFromOutput();
        }
    }
}

void AudioEncoder::completeEncodingAndNotifyWaiters() {
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_state = AudioEncoderState::IDLE;
        m_inputStream.reset();
        m_inputStreamReader.reset();
        m_outputStreamWriter.reset();
        m_timeoutTimePoint = Clock::time_point{};
        m_wordsInInputBuffer = 0;
        m_inputBuffer.clear();
        m_outputBuffer.clear();
        m_cond.notify_all();
    }

    ACSDK_DEBUG5(LX("encodeLoopFinished"));
}

void AudioEncoder::setErrorState() {
    std::unique_lock<std::mutex> lock{m_mutex};
    m_state = AudioEncoderState::ENCODING_ERROR;
}

bool AudioEncoder::needReadMoreWordsFromInput() {
    if (m_requireFullFrameSize) {
        return m_wordsInInputBuffer < m_maxInputFrameSizeWords;
    } else {
        return !m_wordsInInputBuffer;
    }
}

bool AudioEncoder::mayProcessNextFrame() {
    std::lock_guard<std::mutex> lock{m_mutex};
    switch (m_state.load()) {
        case AudioEncoderState::ENCODING:
            return true;
        case AudioEncoderState::STOPPING:
        case AudioEncoderState::ABORTING:
            ACSDK_DEBUG7(LX("mayProcessNextFrame").m("stopOrAbort"));
            return false;
        case AudioEncoderState::ENCODING_ERROR:
            return false;
        default:
            ACSDK_ERROR(LX("mayProcessNextFrameError").d("unexpectedState", m_state));
            return false;
    }
}

bool AudioEncoder::mayContinueEncoding() {
    std::lock_guard<std::mutex> lock{m_mutex};
    switch (m_state.load()) {
        case AudioEncoderState::ENCODING:
            return true;
        case AudioEncoderState::STOPPING:
            if (Clock::now() < m_timeoutTimePoint) {
                return true;
            } else {
                ACSDK_DEBUG7(LX("mayContinueEncoding").m("stopTimeoutExpired"));
                return false;
            }
        case AudioEncoderState::ABORTING:
            ACSDK_DEBUG7(LX("mayContinueEncoding").m("aborting"));
            return false;
        case AudioEncoderState::ENCODING_ERROR:
            return false;
        default:
            ACSDK_ERROR(LX("mayContinueEncodingError").d("unexpectedState", m_state));
            return false;
    }
}

bool AudioEncoder::mayFinishEncoding() {
    std::lock_guard<std::mutex> lock{m_mutex};
    switch (m_state.load()) {
        case AudioEncoderState::ENCODING:
            return true;
        case AudioEncoderState::STOPPING:
            return Clock::now() < m_timeoutTimePoint;
        case AudioEncoderState::ENCODING_ERROR:
        case AudioEncoderState::ABORTING:
        default:
            return false;
    }
}

bool AudioEncoder::readWordsFromInput() {
    while (needReadMoreWordsFromInput()) {
        bool mayContinue;
        if (m_wordsInInputBuffer) {
            mayContinue = mayContinueEncoding();
        } else {
            mayContinue = mayProcessNextFrame();
        }
        if (!mayContinue) {
            return false;
        }

        auto readResult = m_inputStreamReader->read(
            m_inputBuffer.data() + m_wordsInInputBuffer * m_inputWordSizeBytes,
            m_maxInputFrameSizeWords - m_wordsInInputBuffer,
            m_params.readTimeoutMs);
        if (readResult <= 0) {
            switch (readResult) {
                case AudioInputStream::Reader::Error::WOULDBLOCK:
                case AudioInputStream::Reader::Error::TIMEDOUT:
                    // Ignore and retry
                    continue;
                case AudioInputStream::Reader::Error::CLOSED:
                    // End of input stream
                    ACSDK_DEBUG7(LX("readWordsFromInput").m("endOfStream"));
                    return false;
                case AudioInputStream::Reader::Error::OVERRUN:
                case AudioInputStream::Reader::Error::INVALID:
                default:
                    ACSDK_ERROR(LX("encodeLoopFailed").d("error", readResult));
                    return false;
            }
        } else {
            m_wordsInInputBuffer += readResult;
        }
    }

    return true;
}

bool AudioEncoder::encodeWordsIntoOutput() {
    if (!mayContinueEncoding()) {
        return false;
    }

    if (!m_wordsInInputBuffer) {
        return true;
    }

    auto offset = m_wordsInInputBuffer * m_inputWordSizeBytes;
    if (!m_encoder->processSamples(m_inputBuffer.begin(), m_inputBuffer.begin() + offset, m_outputBuffer)) {
        ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "processSamplesFailed"));
        setErrorState();
        return false;
    } else {
        m_wordsInInputBuffer = 0;
        return true;
    }
}

bool AudioEncoder::writeEncodedWordsFromOutput() {
    std::size_t totalWordsToSend = m_outputBuffer.size() / m_outputWordSizeBytes;
    std::size_t wordsSent = 0;
    std::size_t writeBufIndex = 0;

    // This loop will push the encoded samples to the output stream.
    while (wordsSent < totalWordsToSend) {
        if (!mayContinueEncoding()) {
            return false;
        }

        auto writeResult = m_outputStreamWriter->write(
            m_outputBuffer.data() + writeBufIndex, totalWordsToSend - wordsSent, m_params.writeTimeoutMs);
        if (writeResult > 0) {
            // Some words were sent, update the counters.
            wordsSent += writeResult;
            writeBufIndex += writeResult * m_outputWordSizeBytes;
            continue;
        }

        switch (writeResult) {
            case AudioInputStream::Writer::Error::TIMEDOUT:
                ACSDK_DEBUG9(LX("Timeout occurred while writing to stream"));
                continue;
            case AudioInputStream::Writer::Error::WOULDBLOCK:
                // Should never happen.
                ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "WOULDBLOCK error while writing to stream"));
                break;
            case AudioInputStream::Writer::Error::INVALID:
                ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "INVALID error while writing to stream"));
                setErrorState();
                break;
            case AudioInputStream::Writer::Error::CLOSED:
                ACSDK_DEBUG7(LX("streamClosed"));
                setErrorState();
                break;
            default:
                ACSDK_ERROR(LX("unknownError").d("unknownError", writeResult));
                setErrorState();
                break;
        }

        return false;
    }

    if (wordsSent == totalWordsToSend) {
        if (wordsSent) {
            // We are done sending everything we could.
            // If there are some bytes left in the buffer, move them to the beginning
            std::size_t leftoverBytes = m_outputBuffer.size() % m_outputWordSizeBytes;
            std::memmove(
                m_outputBuffer.data(), m_outputBuffer.data() + m_outputBuffer.size() - leftoverBytes, leftoverBytes);
            m_outputBuffer.resize(leftoverBytes);
        }
        return true;
    } else {
        ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "bufferOverRun"));
        return false;
    }
}

}  // namespace audioEncoder
}  // namespace alexaClientSDK
