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
#include <thread>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/Attachment/AttachmentWriter.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AndroidUtilities/AndroidLogger.h>

#include "AndroidSLESMediaPlayer/FFmpegAttachmentInputController.h"
#include "AndroidSLESMediaPlayer/FFmpegDecoder.h"

/// String to identify log entries originating from this file.
static const std::string TAG("FFmpegDecoderTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs;
using namespace avsCommon::utils::logger;

using Byte = FFmpegDecoder::Byte;

/// Configure @c MockAttachmentReader to simulate getting the entire file before being read.
static const std::vector<size_t> RECEIVE_SIZES{std::numeric_limits<size_t>::max()};

/// Test input folder.
static std::string inputFolder;

/// MP3 test file path relative to the input folder.
static const std::string MP3_FILE_PATH("/fox_dog.mp3");

/// Some arbitrary size that should fit valid audio samples.
constexpr size_t BUFFER_SIZE = 8192;

/**
 * Test class for FFmpegDecoder
 */
class FFmpegDecoderTest : public Test {
protected:
    void writeInput(size_t max_bytes = std::numeric_limits<size_t>::max());

    void writeCorruptedInput(size_t skipInterval);

    void SetUp() override {
        m_inAttachment = std::make_shared<attachment::InProcessAttachment>("input");
        m_attachment =
            m_inAttachment->createReader(attachment::InProcessAttachmentReader::SDSTypeReader::Policy::NONBLOCKING);
        m_reader = FFmpegAttachmentInputController::create(m_attachment);
        m_inputFileName = inputFolder + MP3_FILE_PATH;
        m_inputSize = 0;
    }

    void TearDown() override {
        m_reader.reset();
        m_inAttachment.reset();
    }

    /// Attachment used for the input.
    std::shared_ptr<attachment::InProcessAttachment> m_inAttachment;

    /// Mock the attachment reader.
    std::shared_ptr<attachment::AttachmentReader> m_attachment;

    /// Create an input reader.
    std::unique_ptr<FFmpegAttachmentInputController> m_reader;

    /// String with the input file name.
    std::string m_inputFileName;

    /// The amount of bytes written to the input attachment.
    size_t m_inputSize;
};

void FFmpegDecoderTest::writeInput(size_t maxBytes) {
    auto inputWriter = m_inAttachment->createWriter();
    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];
    std::ifstream mediaFile{m_inputFileName, std::ios_base::binary};
    m_inputSize = 0;
    while (!mediaFile.eof() && m_inputSize + bufferSize < maxBytes) {
        mediaFile.read(buffer, bufferSize);
        attachment::AttachmentWriter::WriteStatus status;
        m_inputSize += inputWriter->write(buffer, mediaFile.gcount(), &status);
    }
    inputWriter->close();
}

void FFmpegDecoderTest::writeCorruptedInput(size_t skipInterval) {
    auto inputWriter = m_inAttachment->createWriter();
    char buffer[BUFFER_SIZE];
    std::ifstream mediaFile{m_inputFileName, std::ios_base::binary};
    size_t iteration = 0;
    while (!mediaFile.eof()) {
        iteration++;
        if ((iteration % skipInterval) != 0) {
            mediaFile.read(buffer, BUFFER_SIZE);
            attachment::AttachmentWriter::WriteStatus status;
            m_inputSize += inputWriter->write(buffer, mediaFile.gcount(), &status);
        }
    }
    inputWriter->close();
}

/// Test decoder create.
TEST_F(FFmpegDecoderTest, testCreateSucceed) {
    writeInput();
    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    EXPECT_NE(decoder, nullptr);
}

/// Test decoder create with null reader.
TEST_F(FFmpegDecoderTest, testCreateFailedNullReader) {
    writeInput();
    auto decoder = FFmpegDecoder::create(nullptr, PlaybackConfiguration());
    EXPECT_EQ(decoder, nullptr);
}

/// Test decoding an entire file.
TEST_F(FFmpegDecoderTest, testDecodeFullFile) {
    writeInput();
    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    ASSERT_NE(decoder, nullptr);

    FFmpegDecoder::Status status = FFmpegDecoder::Status::OK;
    Byte buffer[BUFFER_SIZE];
    size_t totalWordsRead = 0;
    while (status == FFmpegDecoder::Status::OK) {
        size_t wordsRead;
        std::tie(status, wordsRead) = decoder->read(buffer, BUFFER_SIZE);
        totalWordsRead += wordsRead;
        EXPECT_TRUE(status != FFmpegDecoder::Status::OK || wordsRead > 0);
    }
    EXPECT_EQ(status, FFmpegDecoder::Status::DONE);
    EXPECT_GT(totalWordsRead * sizeof(buffer[0]), m_inputSize);
}

/// Test that it's possible to decode a file that was been truncated past the header.
TEST_F(FFmpegDecoderTest, testTruncatedInput) {
    std::ifstream mediaFile{m_inputFileName, std::ios_base::binary};
    mediaFile.seekg(0, std::ios_base::seek_dir::end);
    writeInput(mediaFile.tellg() / 2);  // Write only half of the file.

    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    ASSERT_NE(decoder, nullptr);

    FFmpegDecoder::Status status = FFmpegDecoder::Status::OK;
    Byte buffer[BUFFER_SIZE];
    size_t totalWordsRead = 0;
    while (status == FFmpegDecoder::Status::OK) {
        size_t wordsRead;
        std::tie(status, wordsRead) = decoder->read(buffer, BUFFER_SIZE);
        totalWordsRead += wordsRead;
        EXPECT_TRUE(status != FFmpegDecoder::Status::OK || wordsRead > 0);
    }
    EXPECT_EQ(status, FFmpegDecoder::Status::DONE);
    EXPECT_GT(totalWordsRead * sizeof(buffer[0]), m_inputSize);
}

/// Test that the decoder recovers if the file is missing parts of it.
TEST_F(FFmpegDecoderTest, testCorruptedInput) {
    constexpr size_t interval = 10;  // Skip a write at this interval.
    std::ifstream mediaFile{m_inputFileName, std::ios_base::binary};
    mediaFile.seekg(0, std::ios_base::seek_dir::end);
    writeCorruptedInput(interval);  // Write file with missing bits.

    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    ASSERT_NE(decoder, nullptr);

    FFmpegDecoder::Status status = FFmpegDecoder::Status::OK;
    Byte buffer[BUFFER_SIZE];
    size_t totalWordsRead = 0;
    while (status == FFmpegDecoder::Status::OK) {
        size_t wordsRead;
        std::tie(status, wordsRead) = decoder->read(buffer, BUFFER_SIZE);
        totalWordsRead += wordsRead;
        EXPECT_TRUE(status != FFmpegDecoder::Status::OK || wordsRead > 0);
    }
    EXPECT_EQ(status, FFmpegDecoder::Status::DONE);
    EXPECT_GT(totalWordsRead * sizeof(buffer[0]), m_inputSize);
}

/// Test that the decoder will error if input has invalid media.
TEST_F(FFmpegDecoderTest, testInvalidInput) {
    // Create input with 0101's
    std::vector<Byte> input(BUFFER_SIZE, 0x55);
    attachment::AttachmentWriter::WriteStatus writeStatus;
    auto writer = m_inAttachment->createWriter();
    m_inputSize = writer->write(input.data(), BUFFER_SIZE, &writeStatus);
    EXPECT_EQ(m_inputSize, BUFFER_SIZE);

    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    ASSERT_NE(decoder, nullptr);

    Byte buffer[BUFFER_SIZE];
    FFmpegDecoder::Status status;
    size_t wordsRead;
    std::tie(status, wordsRead) = decoder->read(buffer, BUFFER_SIZE);

    EXPECT_EQ(wordsRead, 0u);
    EXPECT_EQ(status, FFmpegDecoder::Status::ERROR);

    writer->close();
}

/// Check that read with a buffer that is too small fails.
TEST_F(FFmpegDecoderTest, testReadSmallBuffer) {
    writeInput();
    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    ASSERT_NE(decoder, nullptr);

    constexpr size_t smallBufferSize = 1;  // Some arbitrary size that doesn't fit any valid frame.
    Byte buffer[smallBufferSize];
    FFmpegDecoder::Status status;
    size_t wordsRead;
    std::tie(status, wordsRead) = decoder->read(buffer, smallBufferSize);

    EXPECT_EQ(wordsRead, 0u);
    EXPECT_EQ(status, FFmpegDecoder::Status::ERROR);
}

/// Check that we can abort the decoding during initialization.
TEST_F(FFmpegDecoderTest, testAbortInitialization) {
    auto decoder = FFmpegDecoder::create(std::move(m_reader), PlaybackConfiguration());
    ASSERT_NE(decoder, nullptr);

    std::atomic<FFmpegDecoder::Status> status{FFmpegDecoder::Status::OK};
    std::thread decoderThread{[&decoder, &status]() {
        Byte buffer[BUFFER_SIZE];
        size_t wordsRead;
        std::tie(status, wordsRead) = decoder->read(buffer, BUFFER_SIZE);
    }};

    // Wait an arbitrary time before calling abort. The read should not return till abort is called.
    std::chrono::milliseconds timeout{50};
    std::this_thread::sleep_for(timeout);
    EXPECT_EQ(status, FFmpegDecoder::Status::OK);

    decoder->abort();
    decoderThread.join();
    EXPECT_EQ(status, FFmpegDecoder::Status::ERROR);
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0]) << " <absolute path to test inputs folder>" << std::endl;
    } else {
        alexaClientSDK::mediaPlayer::android::test::inputFolder = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
