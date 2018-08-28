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
#include <array>
#include <sstream>

#include <gtest/gtest.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/common.h>
#include <libavutil/error.h>
}

#include <Audio/Data/med_alerts_notification_01._TTH_.mp3.h>

#include "AndroidSLESMediaPlayer/FFmpegStreamInputController.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {
namespace test {

using namespace ::testing;

/// The size of the buffers used to read input.
static constexpr size_t BUFFER_SIZE = 1024;

/// The size of the input buffer.
static constexpr size_t INPUT_SIZE =
    applicationUtilities::resources::audio::data::med_alerts_notification_01__TTH__mp3_len;

/// An input buffer with an mp3 file.
static const auto INPUT_CSTR = applicationUtilities::resources::audio::data::med_alerts_notification_01__TTH__mp3;

std::shared_ptr<std::stringstream> createStream() {
    auto stream = std::make_shared<std::stringstream>();
    stream->write(reinterpret_cast<const char*>(INPUT_CSTR), INPUT_SIZE);
    return stream;
}

/// Test decoder input create succeed.
TEST(FFmpegStreamInputControllerTest, testCreateSucceed) {
    auto stream = createStream();
    auto reader = FFmpegStreamInputController::create(stream, false);
    EXPECT_NE(reader, nullptr);
}

/// Test decoder input create with null input failed.
TEST(FFmpegStreamInputControllerTest, testCreateFailed) {
    auto reader = FFmpegStreamInputController::create(nullptr, false);
    EXPECT_EQ(reader, nullptr);
}

/// Test read from stream.
TEST(FFmpegStreamInputControllerTest, testReadSucceed) {
    auto stream = createStream();
    auto reader = FFmpegStreamInputController::create(stream, false);
    ASSERT_NE(reader, nullptr);

    auto result = reader->getCurrentFormatContext();
    auto inputFormat = std::get<1>(result);
    EXPECT_EQ(FFmpegStreamInputController::Result::OK, std::get<0>(result));
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());
    ASSERT_TRUE(inputFormat);

    std::array<uint8_t, BUFFER_SIZE> buffer;
    auto read = avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_EQ(static_cast<size_t>(read), buffer.size());
}

/// Test read from stream until the end.
TEST(FFmpegStreamInputControllerTest, testReadEof) {
    auto stream = createStream();
    auto reader = FFmpegStreamInputController::create(stream, false);
    ASSERT_NE(reader, nullptr);

    auto result = reader->getCurrentFormatContext();
    auto inputFormat = std::get<1>(result);
    EXPECT_EQ(FFmpegStreamInputController::Result::OK, std::get<0>(result));
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());
    ASSERT_TRUE(inputFormat);

    std::array<uint8_t, INPUT_SIZE> buffer;
    auto read = avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_GT(static_cast<size_t>(read), 0u);
    EXPECT_EQ(avio_read(inputFormat->pb, buffer.data(), buffer.size()), AVERROR_EOF);
}

/// Test read with repeat on from a stream.
TEST(FFmpegStreamInputControllerTest, testReadRepeat) {
    auto stream = createStream();
    auto reader = FFmpegStreamInputController::create(stream, true);
    ASSERT_NE(reader, nullptr);

    auto result = reader->getCurrentFormatContext();
    auto inputFormat = std::get<1>(result);
    EXPECT_EQ(FFmpegStreamInputController::Result::OK, std::get<0>(result));
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());
    ASSERT_TRUE(inputFormat);

    std::array<uint8_t, INPUT_SIZE> buffer;
    avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_EQ(avio_read(inputFormat->pb, buffer.data(), buffer.size()), AVERROR_EOF);

    EXPECT_TRUE(reader->hasNext());
    EXPECT_TRUE(reader->next());

    result = reader->getCurrentFormatContext();
    inputFormat = std::get<1>(result);
    EXPECT_EQ(FFmpegStreamInputController::Result::OK, std::get<0>(result));
    EXPECT_EQ(std::get<2>(result), std::chrono::milliseconds::zero());
    ASSERT_TRUE(inputFormat);

    auto read = avio_read(inputFormat->pb, buffer.data(), buffer.size());
    EXPECT_GT(static_cast<size_t>(read), 0u);
}

}  // namespace test
}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
