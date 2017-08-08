/*
 * PlaylistParser.h
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

#ifndef ALEXA_CLIENT_SDK_PLAYLIST_PARSER_INCLUDE_PLAYLIST_PARSER_PLAYLIST_PARSER_H_
#define ALEXA_CLIENT_SDK_PLAYLIST_PARSER_INCLUDE_PLAYLIST_PARSER_PLAYLIST_PARSER_H_

#include <condition_variable>
#include <deque>
#include <thread>
#include <utility>

#include <totem-pl-parser.h>

#include <AVSCommon/Utils/PlaylistParser/PlaylistParserInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

/**
 * This implementation of the PlaylistParser uses the Totem Playlist Parser library.
 * @see https://github.com/GNOME/totem-pl-parser
 * @see https://developer.gnome.org/totem-pl-parser/stable/TotemPlParser.html.
 *
 * @note For the playlist parser to function properly, a main event loop needs to be running. If there is no main event
 * loop the signals related to parsing playlist will not be called.
 */
class PlaylistParser : public avsCommon::utils::playlistParser::PlaylistParserInterface {
public:
    /**
     * Creates an instance of the @c PlaylistParser.
     *
     * @return An instance of the @c PlaylistParser if successful else a @c nullptr.
     */
    static std::unique_ptr<PlaylistParser> create();

    /**
     * Destructor.
     */
    ~PlaylistParser();

    bool parsePlaylist(const std::string& url,
            std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer) override;

private:
    /**
     * Class which has all the information to parse a playlist and the observer to whom the notification needs to be
     * sent once the parsing has been completed.
     */
    class PlaylistInfo {
    public:
        /// Constructor. It initializes the parser.
        PlaylistInfo(TotemPlParser* parser);

        /// Destructor.
        ~PlaylistInfo();

        /// An instance of the @c TotemPlParser.
        TotemPlParser* parser;

        /// The playlist url.
        std::string playlistUrl;

        /// An instance of the observer to notify once parsing is complete.
        std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer;

        /// A queue of urls extracted from the playlist.
        std::queue<std::string> urlQueue;

        /// ID of the handler installed to receive playlist started data signals.
        guint playlistStartedHandlerId;

        /// ID of the handler installed to receive entry parsed data signals.
        guint entryParsedHandlerId;

        /// Mutex to serialize access to @c urlQueue.
        std::mutex mutex;
    };

    /**
     * Initializes the @c m_parserThread.
     */
    PlaylistParser();

    /**
     * Creates an instance of the @c PlaylistInfo and initializes its parser with an instance of the @c TotemPlParser.
     * Connects signals for the parser to the callbacks.
     *
     * @param url The url of the playlist to be parsed.
     * @param observer The observer to be notified of when playlist parsing is complete.
     * @return A @c PlaylistInfo if successful else @c nullptr.
     *
     * @note This should be called only after validating the url and observer.
     */
    std::shared_ptr<PlaylistParser::PlaylistInfo> createPlaylistInfo(
            const std::string& url,
            std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer);

    /**
     * Callback for when playlist parsing begins.
     *
     * @param parser The instance of the @c TotemPlParser.
     * @param url The playlist url being parsed.
     * @param metadata The metadata associated with the playlist.
     * @param playlistInfo The instance of the @c PlaylistInfo associated with the parser.
     */
    static void onPlaylistStarted(
            TotemPlParser* parser,
            gchar* url,
            TotemPlParserMetadata* metadata,
            gpointer playlistInfo);

    /**
     * Callback for when an entry has been extracted from the playlist.
     *
     * @param parser The instance of the @c TotemPlParser.
     * @param url The url entry extracted from the playlist.
     * @param metadata The metadata associated with the entry.
     * @param playlistInfo The instance of the @c PlaylistInfo associated with the parser.
     */
    static void onEntryParsed(
            TotemPlParser* parser,
            gchar* url,
            TotemPlParserMetadata* metadata,
            gpointer playlistInfo);

    /**
     * Callback for when the parsing is complete.
     *
     * @param parser The instance of the @c TotemPlParser.
     * @param result The result of the playlist parsing.
     * @param playlistInfo The instance of the @c PlaylistInfo associated with the parser.
     */
    static void onParseComplete(GObject* parser, GAsyncResult* result, gpointer playlistInfo);

    /**
     * Calls the function to start playlist parsing. This function is called after acquiring a lock to @c m_mutex.
     * The function will release the lock before calling the Totem parsing function.
     *
     * @param lock The lock acquired on @c m_mutex.
     */
    void handleParsingLocked(std::unique_lock<std::mutex>& lock);

    /**
     * Adds the url entry to the queue of entries to be sent to the observer.
     *
     * @param url The url entry extracted from the playlist.
     * @param playlistInfo The instance of the @c PlaylistInfo associated with the parser.
     */
    void handleOnEntryParsed(gchar *url, std::shared_ptr<PlaylistInfo> playlistInfo);

    /**
     * Translates the @c result to @c PlaylistParseResult and calls @c onPlaylistParsed.
     *
     * @param result The result of parsing the playlist.
     * @param playlistInfo The instance of the @c PlaylistInfo associated with the parser.
     */
    void handleOnParseComplete(GAsyncResult* result, std::shared_ptr<PlaylistInfo> playlistInfo);

    /**
     * Translates the @c TotemPlParserResult to a @c PlaylistParseResult.
     *
     * @param result The @c TotemPlParserResult of playlist parsing.
     * @return the @c PlaylistParseResult
     */
    avsCommon::utils::playlistParser::PlaylistParseResult mapResult(TotemPlParserResult result);

    /**
     * Method that processes the playlist parsing requests in the @c m_playlistInfoQueue
     */
    void parsingLoop();

    /// The @c PlaylistInfo currently being processed.
    std::shared_ptr<PlaylistInfo> m_playlistInfo;

    /// The thread on which the playlist parse requests are executed.
    std::thread m_parserThread;

    /**
     * Flag to indicate if a playlist is currently being parsed. @c m_mutex must be acquired before accessing this flag.
     */
    bool m_isParsingActive;

    /**
     * Flag to indicate whether the playlist parser is shutting down. @c m_mutex must be acquired before accessing this
     * flag.
     */
    bool m_isShuttingDown;

    /// Condition variable used to wake @c parsingLoop.
    std::condition_variable m_wakeParsingLoop;

    /// Queue to which the playlist parsing requests are added. @c m_mutex must be acquired before accessing this queue.
    std::deque<std::shared_ptr<PlaylistInfo>> m_playlistInfoQueue;

    /// Mutex which serializes access to @c m_isParsingActive, @c m_isShuttingDown, @c m_playlistInfoQueue.
    std::mutex m_mutex;
};

} // namespace playlistParser
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_PLAYLIST_PARSER_INCLUDE_PLAYLIST_PARSER_PLAYLIST_PARSER_H_

