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
#include <array>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/common.h>
#include <libavutil/error.h>
}

#include <AVSCommon/AVS/Attachment/AttachmentReader.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <Audio/Data/med_alerts_notification_01.mp3.h>

#include "AndroidSLESMediaPlayer/FFmpegAttachmentInputController.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils;

/// The size of the buffers used to read input.
static constexpr size_t BUFFER_SIZE = 1024;

/// The size of the input buffer.
static const size_t INPUT_SIZE = applicationUtilities::resources::audio::data::med_alerts_notification_01_mp3_len;

/// An input buffer with an mp3 file.
static const auto INPUT_CSTR = applicationUtilities::resources::audio::data::med_alerts_notification_01_mp3;

class MockAttachmentReader : public AttachmentReader {
public:
    MOCK_METHOD1(seek, bool(uint64_t offset));
    MOCK_CONST_METHOD0(tell, uint64_t());
    MOCK_METHOD0(getNumUnreadBytes, uint64_t());
    MOCK_METHOD1(close, void(ClosePoint closePoint));

    size_t read(
        void* buf,
        std::size_t numBytes,
        AttachmentReader::ReadStatus* readStatus,
        std::chrono::milliseconds timeoutMs);

private:
    std::size_t m_index{0};
};

size_t MockAttachmentReader::read(
    void* buf,
    std::size_t numBytes,
    AttachmentReader::ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
    if (m_index + numBytes <= INPUT_SIZE) {
        memcpy(buf, &INPUT_CSTR[m_index], numBytes);
        (*readStatus) = AttachmentReader::ReadStatus::OK;
        m_index += numBytes;
        return numBytes;
    }
    (*readStatus) = AttachmentReader::ReadStatus::CLOSED;
    return 0;
}

/// A mock AttachmentReader that mocks reading from an empty attachment (immediate EOF).
class MockEmptyAttachmentReader : public AttachmentReader {
public:
    MOCK_METHOD1(seek, bool(uint64_t offset));
    MOCK_CONST_METHOD0(tell, uint64_t());
    MOCK_METHOD0(getNumUnreadBytes, uint64_t());
    MOCK_METHOD1(close, void(ClosePoint closePoint));

    size_t read(
        void* buf,
        std::size_t numBytes,
        AttachmentReader::ReadStatus* readStatus,
        std::chrono::milliseconds timeoutMs);
};

size_t MockEmptyAttachmentReader::read(
    void* buf,
    std::size_t numBytes,
    AttachmentReader::ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
    (*readStatus) = AttachmentReader::ReadStatus::CLOSED;
    return 0;
}

/// Test create controller fail.
TEST(FFmpegAttachmentInputControllerTest, test_createFailed) {
    auto reader = FFmpegAttachmentInputController::create(nullptr);
    EXPECT_EQ(reader, nullptr);
}

/// Test raw input format.
TEST(FFmpegAttachmentInputControllerTest, test_rawArgument) {
    AudioFormat format{.encoding = AudioFormat::Encoding::LPCM,
                       .endianness = AudioFormat::Endianness::LITTLE,
                       .sampleRateHz = 48000,
                       .sampleSizeInBits = 16,
                       .numChannels = 1,
                       .dataSigned = true};
    auto mockReader = std::make_shared<MockAttachmentReader>();
    auto reader = FFmpegAttachmentInputController::create(mockReader, &format);
    ASSERT_NE(reader, nullptr);

    auto result = reader->getCurrentFormatContext();
    EXPECT_EQ(FFmpegAttachmentInputController::Result::OK, std::get<0>(result));
    auto inputFormat = std::get<1>(result);
    ASSERT_TRUE(inputFormat);
    EXPECT_NE(inputFormat->iformat, nullptr);
    EXPECT_EQ(inputFormat->iformat, av_find_input_format("s16le"));
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());

    std::array<uint8_t, BUFFER_SIZE> buffer;
    auto read = avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_EQ(static_cast<size_t>(read), buffer.size());
}

/// Test read from attachment reader.
TEST(FFmpegAttachmentInputControllerTest, test_readOk) {
    auto mockReader = std::make_shared<MockAttachmentReader>();
    auto reader = FFmpegAttachmentInputController::create(mockReader);
    ASSERT_NE(reader, nullptr);

    auto result = reader->getCurrentFormatContext();
    auto inputFormat = std::get<1>(result);
    EXPECT_EQ(FFmpegAttachmentInputController::Result::OK, std::get<0>(result));
    ASSERT_TRUE(inputFormat);
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());

    std::array<uint8_t, BUFFER_SIZE> buffer;
    auto read = avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_EQ(static_cast<size_t>(read), buffer.size());
}

/// Test opening empty input from attachment reader.
TEST(FFmpegAttachmentInputControllerTest, test_getCurrentFormatContextOnEmptyInput) {
    auto mockReader = std::make_shared<MockEmptyAttachmentReader>();
    auto reader = FFmpegAttachmentInputController::create(mockReader);
    ASSERT_NE(reader, nullptr);

    auto currentFormatContext = reader->getCurrentFormatContext();

    auto result = std::get<0>(currentFormatContext);
    auto inputFormat = std::get<1>(currentFormatContext);
    auto playbackPosition = std::get<2>(currentFormatContext);

    EXPECT_EQ(result, FFmpegAttachmentInputController::Result::OK_EMPTY);
    EXPECT_EQ(inputFormat, nullptr);
    EXPECT_EQ(playbackPosition, std::chrono::milliseconds::zero());
}

/// Test read from stream until the end.
TEST(FFmpegAttachmentInputControllerTest, test_readEof) {
    auto mockReader = std::make_shared<MockAttachmentReader>();
    auto reader = FFmpegAttachmentInputController::create(mockReader);
    ASSERT_NE(reader, nullptr);

    auto result = reader->getCurrentFormatContext();
    auto inputFormat = std::get<1>(result);
    EXPECT_EQ(FFmpegAttachmentInputController::Result::OK, std::get<0>(result));
    ASSERT_TRUE(inputFormat);
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());

    std::array<uint8_t, INPUT_SIZE> buffer;
    auto read = avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_GT(static_cast<size_t>(read), 0u);
    EXPECT_EQ(avio_read(inputFormat->pb, buffer.data(), buffer.size()), AVERROR_EOF);
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
