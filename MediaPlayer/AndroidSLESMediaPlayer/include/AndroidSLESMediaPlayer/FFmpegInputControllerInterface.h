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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGINPUTCONTROLLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGINPUTCONTROLLERINTERFACE_H_

#include <chrono>
#include <ostream>
#include <tuple>

struct AVFormatContext;

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * Interface for an input controller. The controller should provide a @c AVFormatContext that will be used to configure
 * how the decoder will read the current input media.
 *
 * For custom buffer operations, the context may set which read function the FFmpeg decoder will call.
 *
 * The interface also offers hasNext() and next() methods for playing multiple medias in a row. The derived classes that
 * do not support multiple media playing should just return @c false for both methods.
 */
class FFmpegInputControllerInterface {
public:
    /// Enumeration class that represents the possible return values for @c getContext.
    enum class Result {
        /// Get context succeeded. The returned @c AVFormatContext shall be valid.
        OK,
        /// There is not enough input data available to generate the context. Decoder should try again later.
        TRY_AGAIN,
        /// A unrecoverable error was found while trying to create the @c AVFormatContext.
        ERROR
    };

    /**
     * Checks if there is a next track to be played.
     *
     * @return @c true if there is a next track; false, otherwise.
     */
    virtual bool hasNext() const = 0;

    /**
     * Change input to the next track to be played.
     *
     * @return @c true if it succeeds to change; false, otherwise.
     */
    virtual bool next() = 0;

    /**
     * This method will initialize the ffmpeg format context that represents the current input stream.
     *
     * @return A tuple with the return result, a pointer to @c AVFormatContext and the initial playback position.
     */
    virtual std::tuple<Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds>
    getCurrentFormatContext() = 0;

    /**
     * Destructor
     */
    virtual ~FFmpegInputControllerInterface() = default;
};

/**
 * Write a @c FFmpegInputControllerInterface::Result value to the given stream.
 *
 * @param stream The stream to write the value to.
 * @param result The value to write to the stream as a string.
 * @return The stream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const FFmpegInputControllerInterface::Result& result) {
    switch (result) {
        case FFmpegInputControllerInterface::Result::OK:
            stream << "OK";
            break;
        case FFmpegInputControllerInterface::Result::TRY_AGAIN:
            stream << "TRY_AGAIN";
            break;
        case FFmpegInputControllerInterface::Result::ERROR:
            stream << "ERROR";
            break;
    }
    return stream;
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGINPUTCONTROLLERINTERFACE_H_
