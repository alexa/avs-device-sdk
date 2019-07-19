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

#include <iostream>
#include <climits>
#include <fstream>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "SpeechEncoder/SpeechEncoder.h"

namespace alexaClientSDK {
namespace speechencoder {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("SpeechEncoder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The maximum number of readers of the output stream.
static constexpr unsigned int MAX_READERS = 10;

/// Reading timeout from input audio stream.
static constexpr unsigned int READ_TIMEOUT_MS = 10;

/// Timeout between write retrying.
static const auto WRITE_TIMEOUT_MS = std::chrono::milliseconds(100);

/// The maximum number of packets to be buffered to the output stream.
static constexpr unsigned int MAX_OUTPUT_PACKETS = 20;

SpeechEncoder::SpeechEncoder(const std::shared_ptr<EncoderContext>& encoder) :
        m_encoder{encoder},
        m_isEncoding{false},
        m_stopRequested{false} {
}

SpeechEncoder::~SpeechEncoder() {
    stopEncoding(true);
    m_executor.shutdown();
}

bool SpeechEncoder::startEncoding(
    const std::shared_ptr<AudioInputStream>& inputStream,
    AudioFormat inputFormat,
    AudioInputStream::Index begin,
    AudioInputStream::Reader::Reference reference) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isEncoding) {
        ACSDK_ERROR(LX("startEncodingFailed").d("reason", "Encoding in progress"));
        return false;
    }

    if (!m_encoder->init(inputFormat)) {
        ACSDK_ERROR(LX("startEncodingFailed").d("reason", "Encoder init failed"));
        return false;
    }

    m_inputStream = inputStream;
    m_inputAudioFormat = inputFormat;
    m_maxFrameSize = m_encoder->getInputFrameSize();
    m_outputAudioFormat = m_encoder->getAudioFormat();

    unsigned int wordSize = m_outputAudioFormat.sampleSizeInBits / CHAR_BIT;

    // Setup Writer for the destination stream
    size_t size = AudioInputStream::calculateBufferSize(
        m_encoder->getOutputFrameSize() * MAX_OUTPUT_PACKETS, wordSize, MAX_READERS);
    auto buffer = std::make_shared<AudioInputStream::Buffer>(size);
    m_encodedStream = AudioInputStream::create(buffer, wordSize, MAX_READERS);
    if (!m_encodedStream) {
        ACSDK_ERROR(LX("startEncodingFailed").d("reason", "AudioInputStream creation failed"));
        return false;
    }

    ACSDK_DEBUG0(LX("startEncoding").d("begin", begin));
    m_isEncoding = true;
    m_stopRequested = false;
    m_executor.submit([this, begin, reference]() { encodeLoop(begin, reference); });

    return true;
}

void SpeechEncoder::stopEncoding(bool stopImmediately) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG0(LX("stopEncoding").d("stopImmediately", stopImmediately));
    if (stopImmediately) {
        m_isEncoding = false;
    } else {
        // Stop after all frames are encoded
        m_stopRequested = true;
    }
}

std::shared_ptr<AudioInputStream> SpeechEncoder::getEncodedStream() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_encodedStream;
}

std::shared_ptr<EncoderContext> SpeechEncoder::getContext() {
    return m_encoder;
}

void SpeechEncoder::encodeLoop(AudioInputStream::Index begin, AudioInputStream::Reader::Reference reference) {
    bool done = false;
    bool readsFull = m_encoder->requiresFullyRead();

    std::shared_ptr<AudioInputStream::Reader> reader =
        m_inputStream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    size_t wordSize = reader->getWordSize();

    reader->seek(begin, reference);

    size_t currentRead = 0;
    std::vector<uint8_t> readBuf(m_maxFrameSize * wordSize);
    std::vector<uint8_t> writeBuf(m_encoder->getOutputFrameSize());

    m_encoder->start();
    std::shared_ptr<AudioInputStream::Writer> writer =
        m_encodedStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    do {
        // May block here
        auto readResult = reader->read(
            readBuf.data() + (currentRead * wordSize),
            m_maxFrameSize - currentRead,
            std::chrono::milliseconds(READ_TIMEOUT_MS));
        if (readResult > 0) {
            currentRead += readResult;
            if (readsFull && (currentRead < m_maxFrameSize)) {
                continue;
            }
            auto processResult = m_encoder->processSamples(readBuf.data(), currentRead, writeBuf.data());
            if (processResult < 0) {
                ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "processSamplesFailed").d("error", processResult));
                done = true;
            } else {
                ssize_t writeResult = AudioInputStream::Writer::Error::INVALID;
                ssize_t totalWordsToSend = processResult / wordSize;
                ssize_t wordsSent = 0;
                unsigned int writeBufIndex = 0;

                // This loop will push the encoded samples to the output stream.
                while (!m_stopRequested && m_isEncoding) {
                    writeResult =
                        writer->write(writeBuf.data() + writeBufIndex, totalWordsToSend - wordsSent, WRITE_TIMEOUT_MS);

                    if (writeResult > 0) {
                        // Some words were sent, update the counters.
                        wordsSent += writeResult;
                        writeBufIndex += writeResult * wordSize;

                        if (wordsSent == totalWordsToSend) {
                            // We are done sending everything.
                            break;
                        }

                        // Sanity check.
                        if (writeBufIndex > writeBuf.size()) {
                            ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "bufferOverRun"));
                            done = true;
                            break;
                        }

                        // There's still something to send

                    } else if (writeResult <= 0) {
                        switch (writeResult) {
                            case AudioInputStream::Writer::Error::WOULDBLOCK:
                                // Should never happen.
                                ACSDK_ERROR(
                                    LX("encodeLoopFailed").d("reason", "WOULDBLOCK error while writing to stream"));
                                break;
                            case AudioInputStream::Writer::Error::INVALID:
                                ACSDK_ERROR(
                                    LX("encodeLoopFailed").d("reason", "INVALID error while writing to stream"));
                                break;
                            case AudioInputStream::Writer::Error::TIMEDOUT:
                                ACSDK_DEBUG9(LX("Timeout occurred while writing to stream"));
                                continue;
                            case AudioInputStream::Writer::Error::CLOSED:
                                ACSDK_DEBUG7(LX("streamClosed"));
                                break;
                            default:
                                ACSDK_DEBUG9(LX("unknownError").d("unknownError", writeResult));
                                break;
                        }
                        done = true;
                        break;
                    }
                }
            }
            currentRead = 0;
        } else {
            switch (readResult) {
                case AudioInputStream::Reader::Error::OVERRUN:
                case AudioInputStream::Reader::Error::INVALID:
                    ACSDK_ERROR(LX("encodeLoopFailed").d("reason", "readerError").d("error", readResult));
                case AudioInputStream::Reader::Error::CLOSED:
                    done = true;
                    break;
                case AudioInputStream::Reader::Error::WOULDBLOCK:
                case AudioInputStream::Reader::Error::TIMEDOUT:
                    // Ignore
                    break;
            }
        }
        if (m_stopRequested) {
            // Reader will close after reads all data within buffer
            reader->close(0, AudioInputStream::Reader::Reference::BEFORE_WRITER);
            m_stopRequested = false;
        }
    } while (!done && m_isEncoding);
    writer->close();
    m_encoder->close();
    reader->close();

    m_isEncoding = false;
}

}  // namespace speechencoder
}  // namespace alexaClientSDK
