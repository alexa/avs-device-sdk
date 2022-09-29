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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <acsdk/AudioEncoder/AudioEncoderFactory.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry("AudioEncoderTest", event)

namespace alexaClientSDK {
namespace audioEncoder {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::utils;

/// Word size per PCM frame = 2byte (16bit)
static constexpr size_t FRAME_WORDSIZE = 2;

/// Number of dummy frames that will be sent to encoder.
static constexpr size_t NUM_AUDIO_WRITE = 100;

/// Input frame size for mock encoder. Will accept 2 frames. (2 * 2byte = 4)
static constexpr size_t MOCK_ENCODER_INPUT_FRAME_SIZE_WORDS = 2;

/// Output frame size for mock encoder. Will output 2 frames per encode. (2 * 2byte = 4)
static constexpr size_t MOCK_ENCODER_OUTPUT_FRAME_SIZE_BYTES = 4;

/// Number of words in input stream SDS buffer (must be large enough to fill encoder's output stream)
static constexpr size_t INPUT_WORD_COUNT = 4096;

/// Short timeout (needs to be greater than @c SpeechEncoder's BLOCKING writer timeout).
static auto PROCESSING_TIMEOUT = std::chrono::milliseconds(200);

/// The maximum factor of compression we expect to encounter.
static constexpr unsigned int MAX_COMPRESSION_FACTOR = 10;

/// Output format for mock encoder.
static const AudioFormat MOCK_ENCODER_FORMAT = {
    AudioFormat::Encoding::LPCM,
    AudioFormat::Endianness::LITTLE,
    16000,
    FRAME_WORDSIZE* CHAR_BIT,
    1,
    false,
    AudioFormat::Layout::INTERLEAVED,
};

using ::testing::Return;
using namespace ::testing;
using namespace audioEncoderInterfaces;
using namespace audioEncoder;
using Bytes = BlockAudioEncoderInterface::Bytes;

/**
 * A mock encoder backend implementation that inherits from @c EncoderContext.
 */
class MockEncoderContext : public BlockAudioEncoderInterface {
public:
    MOCK_METHOD1(init, bool(alexaClientSDK::avsCommon::utils::AudioFormat inputFormat));
    MOCK_METHOD0(getInputFrameSize, size_t());
    MOCK_METHOD0(getOutputFrameSize, size_t());
    MOCK_METHOD0(requiresFullyRead, bool());
    MOCK_METHOD0(getAudioFormat, AudioFormat());
    MOCK_METHOD0(getAVSFormatName, std::string());

    MOCK_METHOD1(start, bool(Bytes&));
    MOCK_METHOD3(processSamples, bool(Bytes::const_iterator, Bytes::const_iterator, Bytes&));
    MOCK_METHOD1(flush, bool(Bytes&));
    MOCK_METHOD0(close, void());
};

class AudioEncoderTest : public ::testing::Test {
protected:
    /// The AudioEncoder to test.
    std::shared_ptr<AudioEncoderInterface> m_encoder;

    /// The mock @c EncoderContext.
    std::shared_ptr<MockEncoderContext> m_encoderCtx;

    /**
     * Set up the test harness for running a test.
     */
    virtual void SetUp() {
        m_encoderCtx = std::make_shared<MockEncoderContext>();
        m_encoder = createAudioEncoder(m_encoderCtx);

        EXPECT_CALL(*m_encoderCtx, getInputFrameSize()).WillRepeatedly(Return(MOCK_ENCODER_INPUT_FRAME_SIZE_WORDS));
        EXPECT_CALL(*m_encoderCtx, getAudioFormat()).WillRepeatedly(Return(MOCK_ENCODER_FORMAT));
        EXPECT_CALL(*m_encoderCtx, getOutputFrameSize()).WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE_BYTES));
    }
};

/*
 * Test to verify if SpeechEncoder::startEncoding properly call backend implementation.
 * This test will feed a dummy PCM stream into SpeechEncoder, then test the behavior with
 * the mock @c EncoderContext.
 */
TEST_F(AudioEncoderTest, testTimer_startEncoding) {
    AudioFormat audioFormat = {AudioFormat::Encoding::LPCM,
                               AudioFormat::Endianness::LITTLE,
                               16000,
                               FRAME_WORDSIZE * CHAR_BIT,
                               1,
                               false,
                               AudioFormat::Layout::INTERLEAVED};
    auto buffer = std::make_shared<AudioInputStream::Buffer>(4000);
    std::shared_ptr<AudioInputStream> inputStream = AudioInputStream::create(buffer, FRAME_WORDSIZE, 1);
    ASSERT_TRUE(inputStream);

    // EncoderContext::init should be called once.
    EXPECT_CALL(*m_encoderCtx, init(_)).Times(1).WillOnce(Return(true));

    // Mock encoder requires fully read.
    EXPECT_CALL(*m_encoderCtx, requiresFullyRead()).WillRepeatedly(Return(true));

    // EncoderContext::start should be called once.
    EXPECT_CALL(*m_encoderCtx, start(_)).Times(1).WillOnce(Return(true));

    // EncoderContext::flush should be called once.
    EXPECT_CALL(*m_encoderCtx, flush(_)).Times(1).WillOnce(Return(true));

    // EncoderContext::close should be called once.
    EXPECT_CALL(*m_encoderCtx, close()).Times(1);

    // Start the encoder.
    auto encodedStream =
        m_encoder->startEncoding(inputStream, audioFormat, 0, AudioInputStream::Reader::Reference::ABSOLUTE);
    ASSERT_TRUE(encodedStream);

    // EncoderContext::processSamples should be called with MOCK_ENCODER_INPUT_FRAME_SIZE size.
    EXPECT_CALL(*m_encoderCtx, processSamples(_, _, _))
        .Times(NUM_AUDIO_WRITE / MOCK_ENCODER_INPUT_FRAME_SIZE_WORDS)
        .WillRepeatedly(Invoke(
            [](Bytes::const_iterator begin, Bytes::const_iterator end, BlockAudioEncoderInterface::Bytes& output) {
                if (MOCK_ENCODER_INPUT_FRAME_SIZE_WORDS * FRAME_WORDSIZE != end - begin) {
                    ACSDK_ERROR(LX("testTimer_startEncoding").d("BadInputSize", end - begin));
                    return false;
                }
                output.resize(output.size() + MOCK_ENCODER_OUTPUT_FRAME_SIZE_BYTES);
                return true;
            }));

    // Begin feeding dummy(empty) PCM frames into SDS.
    std::shared_ptr<AudioInputStream::Writer> writer =
        inputStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    BlockAudioEncoderInterface::Byte dummy[FRAME_WORDSIZE] = {0, 0};
    for (size_t i = 0; i < NUM_AUDIO_WRITE; i++) {
        writer->write(&dummy, 1);
    }
    writer->close();

    ASSERT_EQ(FRAME_WORDSIZE, encodedStream->getWordSize());
    auto reader = encodedStream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    ASSERT_TRUE(reader);
    BlockAudioEncoderInterface::Bytes result;
    result.resize(FRAME_WORDSIZE * NUM_AUDIO_WRITE * 2);
    size_t wordsRead = 0;
    bool open = true;
    while (open) {
        auto readResult = reader->read(
            result.data() + wordsRead * reader->getWordSize(),
            result.size() / FRAME_WORDSIZE - wordsRead,
            std::chrono::milliseconds(100));
        if (readResult > 0) {
            wordsRead += static_cast<size_t>(readResult);
        } else {
            switch (readResult) {
                case AudioInputStream::Reader::Error::TIMEDOUT:
                    continue;
                case AudioInputStream::Reader::Error::CLOSED:
                    open = false;
                    continue;
                default:
                    ASSERT_TRUE(false);
            }
        }
    }
    ASSERT_EQ(NUM_AUDIO_WRITE, wordsRead);
}

/**
 * Test if encoding thread will exit if encoder output is not being consumed on exit.
 */
TEST_F(AudioEncoderTest, test_shutdownOnBlockingWrite) {
    AudioFormat audioFormat = {
        AudioFormat::Encoding::LPCM,
        AudioFormat::Endianness::LITTLE,
        16000,
        FRAME_WORDSIZE * CHAR_BIT,
        1,
        false,
        AudioFormat::Layout::INTERLEAVED,
    };

    auto inputBufferSize = AudioInputStream::calculateBufferSize(INPUT_WORD_COUNT, FRAME_WORDSIZE, 1);
    auto buffer = std::make_shared<AudioInputStream::Buffer>(inputBufferSize);
    std::shared_ptr<AudioInputStream> inputStream = AudioInputStream::create(buffer, FRAME_WORDSIZE, 1);
    ASSERT_TRUE(inputStream);

    // EncoderContext::init should be called once.
    EXPECT_CALL(*m_encoderCtx, init(_)).Times(1).WillOnce(Return(true));

    // Mock encoder requires fully read.
    EXPECT_CALL(*m_encoderCtx, requiresFullyRead()).WillRepeatedly(Return(true));

    // EncoderContext::start should be called once.
    EXPECT_CALL(*m_encoderCtx, start(_)).Times(1).WillOnce(Return(true));

    // EncoderContext::flush should never be called.
    EXPECT_CALL(*m_encoderCtx, flush(_)).Times(0);

    // EncoderContext::close should be called once.
    EXPECT_CALL(*m_encoderCtx, close()).Times(1);

    // EncoderContext::processSamples should be called with MOCK_ENCODER_INPUT_FRAME_SIZE size.
    EXPECT_CALL(*m_encoderCtx, processSamples(_, _, _)).WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE_BYTES));

    // Start the encoder.
    auto encodedStream =
        m_encoder->startEncoding(inputStream, audioFormat, 0, AudioInputStream::Reader::Reference::ABSOLUTE);
    ASSERT_TRUE(encodedStream);
    // If this test fails, INPUT_WORD_COUNT should be increased.
    ASSERT_GE(INPUT_WORD_COUNT, encodedStream->getDataSize() * MAX_COMPRESSION_FACTOR);

    // Fill input buffer with dummy PCM data
    std::shared_ptr<AudioInputStream::Writer> writer =
        inputStream->createWriter(AudioInputStream::Writer::Policy::BLOCKING);
    BlockAudioEncoderInterface::Byte dummy[FRAME_WORDSIZE];
    std::memset(dummy, 0, sizeof(dummy));

    for (size_t wordsWritten = 0; wordsWritten < INPUT_WORD_COUNT; wordsWritten++) {
        writer->write(dummy, 1);
    }

    // Let encoder thread process as much input data as it can.
    std::this_thread::sleep_for(PROCESSING_TIMEOUT);

    // At this point, speech encoder loop should be experiencing timeout and will keep on retrying.

    // Simulate a shutdown.
    m_encoder.reset();
}

/**
 * Test if encoding thread will exit and create again when stopEncoding() and startEncoding() is called in quick
 * succession.
 */
TEST_F(AudioEncoderTest, test_stopAndStartEncoder) {
    const AudioFormat audioFormat = {
        AudioFormat::Encoding::LPCM,
        AudioFormat::Endianness::LITTLE,
        16000,
        FRAME_WORDSIZE * CHAR_BIT,
        1,
        false,
        AudioFormat::Layout::INTERLEAVED,
    };

    // number of start and stop encoding to run
    const int numRunOfTest = 10;

    auto inputBufferSize = AudioInputStream::calculateBufferSize(INPUT_WORD_COUNT, FRAME_WORDSIZE, 1);
    auto buffer = std::make_shared<AudioInputStream::Buffer>(inputBufferSize);
    std::shared_ptr<AudioInputStream> inputStream = AudioInputStream::create(buffer, FRAME_WORDSIZE, 1);
    ASSERT_TRUE(inputStream);

    // EncoderContext::init should be called numRunOfTest times
    EXPECT_CALL(*m_encoderCtx, init(_)).Times(numRunOfTest).WillRepeatedly(Return(true));

    // Mock encoder requires fully read.
    EXPECT_CALL(*m_encoderCtx, requiresFullyRead()).WillRepeatedly(Return(true));

    // EncoderContext::start should be called numRunOfTest times.
    EXPECT_CALL(*m_encoderCtx, start(_)).Times(numRunOfTest).WillRepeatedly(Return(true));

    // EncoderContext::flush should never be called.
    EXPECT_CALL(*m_encoderCtx, flush(_)).Times(0);

    // EncoderContext::close should be called numRunOfTest times.
    EXPECT_CALL(*m_encoderCtx, close()).Times(numRunOfTest);

    // EncoderContext::processSamples should be called with MOCK_ENCODER_INPUT_FRAME_SIZE size.
    EXPECT_CALL(*m_encoderCtx, processSamples(_, _, _)).WillRepeatedly(Return(MOCK_ENCODER_OUTPUT_FRAME_SIZE_BYTES));

    for (auto i = 0; i < numRunOfTest; ++i) {
        // Start the encoder.
        auto encodedStream =
            m_encoder->startEncoding(inputStream, audioFormat, 0, AudioInputStream::Reader::Reference::ABSOLUTE);
        ASSERT_TRUE(encodedStream);

        // Stop the encoder
        m_encoder->stopEncoding(true);
    }
}

}  // namespace test
}  // namespace audioEncoder
}  // namespace alexaClientSDK
