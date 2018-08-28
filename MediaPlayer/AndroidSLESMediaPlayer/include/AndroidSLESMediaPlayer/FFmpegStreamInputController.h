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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGSTREAMINPUTCONTROLLER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGSTREAMINPUTCONTROLLER_H_

#include <istream>
#include <memory>

#include "AndroidSLESMediaPlayer/FFmpegInputControllerInterface.h"

struct AVIOContext;

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * This class provides the FFmpegDecoder input access to the content of an input stream.
 *
 * This class support repeat functionality implemented by returning @c true to @c hasNext and rewinding the stream
 * pointer when @c next is called.
 */
class FFmpegStreamInputController : public FFmpegInputControllerInterface {
public:
    /**
     * Creates an input stream object.
     *
     * @param input A pointer to the input stream. It shall point to a valid object.
     * @param repeat Whether to play the input stream in a loop.
     * @return A pointer to the @c FFmpegStreamInputController if succeed; @c nullptr otherwise.
     */
    static std::unique_ptr<FFmpegStreamInputController> create(std::shared_ptr<std::istream> stream, bool repeat);

    /// @name FFmpegInputControllerInterface methods
    /// @{
    bool hasNext() const override;
    bool next() override;
    std::tuple<Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds> getCurrentFormatContext() override;
    /// @}

    /**
     * Destructor.
     */
    ~FFmpegStreamInputController();

private:
    /**
     * Constructor
     *
     * @param A pointer to the input stream.
     * @param repeat Whether to play the input stream in a loop.
     */
    FFmpegStreamInputController(std::shared_ptr<std::istream> stream, bool repeat);

    /**
     * Function used to provide input data to the decoder.
     *
     * @param buffer Buffer to copy the data to.
     * @param bufferSize The buffer size in bytes.
     * @return The size read if the @c read call succeeded or the AVError code.
     */
    int read(uint8_t* buffer, int bufferSize);

    /**
     * Feed AvioBuffer with some data from the input controller.
     *
     * @param userData A pointer to the input controller instance used to read the encoded input.
     * @param buffer Buffer to copy the data to.
     * @param bufferSize The buffer size in bytes.
     * @return The size read if the @c read call succeeded or the AVError code.
     */
    static int feedBuffer(void* userData, uint8_t* buffer, int bufferSize);

    /// Pointer to the data stream.
    const std::shared_ptr<std::istream> m_stream;

    /// Keep a pointer to the avioContext to avoid memory leaks.
    std::shared_ptr<AVIOContext> m_ioContext;

    /// Flag that indicates whether repeat is on or not.
    const bool m_repeat;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGSTREAMINPUTCONTROLLER_H_
