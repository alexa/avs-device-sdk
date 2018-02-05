/*
 * UrlSource.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_INCLUDE_MEDIAPLAYER_URLSOURCE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_INCLUDE_MEDIAPLAYER_URLSOURCE_H_

#include <future>
#include <memory>
#include <mutex>
#include <queue>

#include <gst/gst.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

#include "MediaPlayer/SourceInterface.h"

namespace alexaClientSDK {
namespace mediaPlayer {

class UrlSource
        : public SourceInterface
        , public avsCommon::utils::playlistParser::PlaylistParserObserverInterface
        , public std::enable_shared_from_this<UrlSource> {
public:
    /**
     * Creates an instance of the @c UrlSource and installs the source within the GStreamer pipeline.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param playlistParser The @c PlaylistParserInterface which will parse playlist urls.
     * @param url The url from which to create the pipeline source from.
     *
     * @return An instance of the @c UrlSource if successful else a @c nullptr.
     */
    static std::shared_ptr<UrlSource> create(
        PipelineInterface* pipeline,
        std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserInterface> playlistParser,
        const std::string& url);

    void onPlaylistEntryParsed(
        int requestId,
        std::string url,
        avsCommon::utils::playlistParser::PlaylistParseResult parseResult,
        std::chrono::milliseconds duration =
            avsCommon::utils::playlistParser::PlaylistParserObserverInterface::INVALID_DURATION) override;

    bool hasAdditionalData() override;
    bool handleEndOfStream() override;

    /**
     * @note This function will block until the @c onPlaylistParsed callback.
     * To avoid deadlock, callers must ensure that @c preprocess is not called on the same thread
     * as the event loop handling @c onPlaylistParsed.
     */
    void preprocess() override;

    /// @name Overridden StreamSourceInterface methods.
    /// @{
    bool isPlaybackRemote() const override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param playlistParser The @c PlaylistParserInterface which will parse playlist urls.
     * @param url The @c url from which to create the pipeline source from.
     */
    UrlSource(
        PipelineInterface* pipeline,
        std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserInterface> playlistParser,
        const std::string& url);

    /**
     * Initializes the UrlSource by doing the following:
     *
     * -# Attempt to parse the url as a playlist.
     * -# Initialize internal url queue.
     * -# Create the source element for the audio pipeline.
     * -# Add the source element to the audio pipeline @c m_pipeline.
     */
    bool init();

    /// A lock to serialize access to m_audioUrlQueue and m_url.
    std::mutex m_mutex;

    /// The url to read audioData from.
    std::string m_url;

    /// A queue of parsed audio urls. This should not contain any playlist urls.
    std::queue<std::string> m_audioUrlQueue;

    /// A Playlist Parser.
    std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserInterface> m_playlistParser;

    /// Indicates if the initial callback has been received from the playlist parser.
    bool m_hasReceivedAPlaylistCallback;

    /// Promise to notify when at least one url has been returned from playlist parsing.
    std::promise<void> m_playlistParsedPromise;

    bool m_isValid;

    /// The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
    PipelineInterface* m_pipeline;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_INCLUDE_MEDIAPLAYER_URLSOURCE_H_
