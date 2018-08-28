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
#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGURLINPUTCONTROLLER_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGURLINPUTCONTROLLER_H_

#include <chrono>
#include <memory>
#include <queue>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/PlaylistParser/IterativePlaylistParserInterface.h>

#include "AndroidSLESMediaPlayer/FFmpegInputControllerInterface.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/**
 * This class provides the FFmpegDecoder input access to the content of a url (playlist or single media file).
 */
class FFmpegUrlInputController : public FFmpegInputControllerInterface {
public:
    /**
     * Creates an input stream object.
     *
     * @param playlistParser Pointer to a valid playlist parser used to fill up the @c Playlist.
     * @param url The playlist / media url that we would like to decode.
     * @param offset The audio input should start from the given offset.
     * @return A pointer to the @c FFmpegUrlInputReader if succeed; otherwise, @c nullptr.
     */
    static std::unique_ptr<FFmpegUrlInputController> create(
        std::shared_ptr<avsCommon::utils::playlistParser::IterativePlaylistParserInterface> playlistParser,
        const std::string& url,
        const std::chrono::milliseconds& offset);

    /// @name FFmpegInputControllerInterface methods
    /// @{
    std::tuple<Result, std::shared_ptr<AVFormatContext>, std::chrono::milliseconds> getCurrentFormatContext() override;
    bool hasNext() const override;
    bool next() override;
    /// @}

    /**
     * Destructor.
     */
    ~FFmpegUrlInputController();

private:
    /**
     * Constructor
     *
     * @param parser Pointer to a valid playlist parser used to parse the target URL.
     * @param offset The audio input should start from the given offset.
     */
    FFmpegUrlInputController(
        std::shared_ptr<avsCommon::utils::playlistParser::IterativePlaylistParserInterface> parser,
        const std::chrono::milliseconds& offset);

    /**
     * Finds the first playlist entry. This is unique to the first audio track because of the initial offset logic.
     *
     * @return @c true if it succeeds to find the first entry point; false, otherwise.
     */
    bool findFirstEntry();

    /// A pointer to a valid playlist parser used to parse the target URL.
    std::shared_ptr<avsCommon::utils::playlistParser::IterativePlaylistParserInterface> m_parser;

    /// The current url that we are parsing.
    std::string m_currentUrl;

    /// Music should start playing from the given offset.
    std::chrono::milliseconds m_offset;

    /// Flag indicating whether playlist has finished. This is set in the last url received.
    bool m_done;
};

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_ANDROIDSLESMEDIAPLAYER_INCLUDE_ANDROIDSLESMEDIAPLAYER_FFMPEGURLINPUTCONTROLLER_H_
