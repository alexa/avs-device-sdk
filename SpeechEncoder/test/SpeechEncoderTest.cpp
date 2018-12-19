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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>

#include "SpeechEncoder/SpeechEncoder.h"

namespace alexaClientSDK {
namespace speechencoder {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;

/// Word size per PCM frame = 2byte (16bit)
static constexpr size_t FRAME_WORDSIZE = 2;

/// Number of dummy frames that will be sent to encoder.
static constexpr int NUM_AUDIO_WRITE = 100;

/// Input frame size for mock encoder. Will accept 2 frames. (2 * 2byte = 4)
static constexpr size_t MOCK_ENCODER_INPUT_FRAME_SIZE = 4;

/// Output frame size for mock encoder. Will output 2 frames per encode. (2 * 2byte = 4)
static constexpr size_t MOCK_ENCODER_OUTPUT_FRAME_SIZE = 4;

/// Number of words in input stream SDS buffer (must be large enough to fill encoder's output stream)
static constexpr size_t INPUT_WORD_COUNT = 4096;

/// Short timeout (needs to be greater than @c SpeechEncoder's BLOCKING writer timeout).
static auto PROCESSING_TIMEOUT = std::chrono::milliseconds(200);

/// The maximum factor of compression we expect to encounter.
static constexpr unsigned int MAX_COMPRESSION_FACTOR = 10;

/// Output format for mock encoder.
static const AudioFormat MOCK_ENCODER_FORMAT = {
    .encoding = AudioFormat::Encoding::LPCM,
    .endianness = AudioFormat::Endianness::LITTLE,
    .sampleRateHz = 16000,
    .sampleSizeInBits = FRAME_WORDSIZE * CHAR_BIT,
    .numChannels = 1,
    .dataSigned = false,
    .layout = AudioFormat::Layout::INTERLEAVED,
};

using ::testing::Return;
using ::testing::_;
using namespace ::testing;

/**
 * A mock encoder backend implementation that inherits from @c EncoderContext.
 */
class MockEncoderContext : public EncoderContext {
public:
    MOCK_METHOD1(init, bool(alexaClientSDK::avsCommon::utils::AudioFormat inputFormat));
    MOCK_METHOD0(getInputFrameSize, size_t());
    MOCK_METHOD0(getOutputFrameSize, size_t());
    MOCK_METHOD0(requiresFullyRead, bool());
    MOCK_METHOD0(getAudioFormat, AudioFormat());
    MOCK_METHOD0(getAVSFormatName, std::string());

    MOCK_METHOD0(start, bool());
    MOCK_METHOD3(processSamples, ssize_t(void* samples, size_t nWords, uint8_t* buffer));
    MOCK_METHOD0(close, void());
};

class SpeechEncoderTest : public ::testing::Test {
protected:
    /// The SpeechEncoder to test.
    std::shared_ptr<SpeechEncoder> m_encoder;

    /// The mock @c EncoderContext.
    std::shared_ptr<MockEncoderContext> m_encoderCtx;

    /**
     * Set up the test harness for running a test.
     */
    virtual void SetUp() {
        m_encoderCtx = std::make_shared<MockEncoderContext>();
        m_encoder = std::make_shared<SpeechEncoder>(m_encoderCtx);
    }
};

/*
 * Test to verify if SpeechEncoder::startEncoding properly call backend implementation.
 * This test will feed a dummy PCM stream into SpeechEncoder, then test the behavior with
 * the mock @c EncoderContext.
 */
TEST_F(SpeechEncoderTest, testStartEncoding) {
    AudioFormat audioFormat = {
        .encoding = AudioFormat::Encoding::LPCM,
        .endianness = AudioFormat::Endianness::LITTLE,
        .sampleRateHz = 16000,
        .sampleSizeInBits = FRAME_WORDSIZE * CHAR_BIT,
        .numChannels = 1,
        .dataSigned = false,
        .layout = AudioFormat::Layout::INTERLEAVED,
    };
    auto buffer = std::make_shared<AudioInputStream::Buffer>(4000);
    std::shared_ptr<AudioInputStream> inputStream = AudioInputStream::create(buffer, FRAME_WORDSIZE, 1);
    ASSERT_TRUE(inputStream);

    // EncoderContext::init should be called once.
    EXPECT_CALL(*m_encoderCtx, init(_)).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*m_encoderCtx, getInputFrameSize()).WillRepeatedly(Return(MOCK_ENCODER_INPUT_FRAME_SIZE));
    EXPECT_CALL(*m_encoderCtx, getAudioFormat()).WillRepeatedly(Return(MOCK_ENCODER_FORMAT));
    EXPECT_CALL(*m_encoderCtx, getOutputFrameSize()).WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE));

    // Mock encoder requires fully read.
    EXPECT_CALL(*m_encoderCtx, requiresFullyRead()).WillRepeatedly(Return(true));

    // EncoderContext::start should be called once.
    EXPECT_CALL(*m_encoderCtx, start()).Times(1).WillOnce(Return(true));

    // EncoderContext::close should be called once.
    EXPECT_CALL(*m_encoderCtx, close()).Times(1);

    // Start the encoder.
    m_encoder->startEncoding(inputStream, audioFormat, 0, AudioInputStream::Reader::Reference::ABSOLUTE);

    // EncoderContext::processSamples should be called with MOCK_ENCODER_INPUT_FRAME_SIZE size.
    EXPECT_CALL(*m_encoderCtx, processSamples(_, MOCK_ENCODER_INPUT_FRAME_SIZE, _))
        .Times(NUM_AUDIO_WRITE / MOCK_ENCODER_INPUT_FRAME_SIZE)
        .WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE));

    // Begin feeding dummy(empty) PCM frames into SDS.
    std::shared_ptr<AudioInputStream::Writer> writer =
        inputStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    uint8_t dummy[FRAME_WORDSIZE] = {0, 0};
    for (int i = 0; i < NUM_AUDIO_WRITE; i++) {
        writer->write(&dummy, 1);
    }

    // Let encoder thread to process...
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

/**
 * Test if encoding thread will exit if encoder output is not being consumed on exit.
 */
TEST_F(SpeechEncoderTest, testShutdownOnBlockingWrite) {
    AudioFormat audioFormat = {
        .encoding = AudioFormat::Encoding::LPCM,
        .endianness = AudioFormat::Endianness::LITTLE,
        .sampleRateHz = 16000,
        .sampleSizeInBits = FRAME_WORDSIZE * CHAR_BIT,
        .numChannels = 1,
        .dataSigned = false,
        .layout = AudioFormat::Layout::INTERLEAVED,
    };

    auto inputBufferSize = AudioInputStream::calculateBufferSize(INPUT_WORD_COUNT, FRAME_WORDSIZE, 1);
    auto buffer = std::make_shared<AudioInputStream::Buffer>(inputBufferSize);
    std::shared_ptr<AudioInputStream> inputStream = AudioInputStream::create(buffer, FRAME_WORDSIZE, 1);
    ASSERT_TRUE(inputStream);

    // EncoderContext::init should be called once.
    EXPECT_CALL(*m_encoderCtx, init(_)).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*m_encoderCtx, getInputFrameSize()).WillRepeatedly(Return(MOCK_ENCODER_INPUT_FRAME_SIZE));
    EXPECT_CALL(*m_encoderCtx, getAudioFormat()).WillRepeatedly(Return(MOCK_ENCODER_FORMAT));
    EXPECT_CALL(*m_encoderCtx, getOutputFrameSize()).WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE));

    // Mock encoder requires fully read.
    EXPECT_CALL(*m_encoderCtx, requiresFullyRead()).WillRepeatedly(Return(true));

    // EncoderContext::start should be called once.
    EXPECT_CALL(*m_encoderCtx, start()).Times(1).WillOnce(Return(true));

    // EncoderContext::close should be called once.
    EXPECT_CALL(*m_encoderCtx, close()).Times(1);

    // EncoderContext::processSamples should be called with MOCK_ENCODER_INPUT_FRAME_SIZE size.
    EXPECT_CALL(*m_encoderCtx, processSamples(_, MOCK_ENCODER_INPUT_FRAME_SIZE, _))
        .WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE));

    // Start the encoder.
    m_encoder->startEncoding(inputStream, audioFormat, 0, AudioInputStream::Reader::Reference::ABSOLUTE);

    // Get the encoded stream to find the size of the buffer that the speech encoder writes to.
    auto encodedStream = m_encoder->getEncodedStream();
    ASSERT_TRUE(encodedStream);
    // If this test fails, INPUT_WORD_COUNT should be increased.
    ASSERT_GE(INPUT_WORD_COUNT, encodedStream->getDataSize() * MAX_COMPRESSION_FACTOR);

    // Fill input buffer with dummy PCM data
    std::shared_ptr<AudioInputStream::Writer> writer =
        inputStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    uint8_t dummy[FRAME_WORDSIZE];

    for (size_t i = 0; i < FRAME_WORDSIZE; i++) {
        dummy[i] = 0;
    }

    for (size_t wordsWritten = 0; wordsWritten < INPUT_WORD_COUNT; wordsWritten++) {
        writer->write(dummy, 1);
    }

    // Let encoder thread process as much input data as it can.
    std::this_thread::sleep_for(PROCESSING_TIMEOUT);

    // At this point, speech encoder loop should be experiencing timeout and will keep on retrying.

    // Simulate a shutdown.
    m_encoder.reset();
}

}  // namespace test
}  // namespace speechencoder
}  // namespace alexaClientSDK
