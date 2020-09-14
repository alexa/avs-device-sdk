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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_M3UPARSER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_M3UPARSER_H_

#include <chrono>
#include <string>
#include <vector>

#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

/// An invalid media sequence number.
static constexpr long INVALID_MEDIA_SEQUENCE = -1;

/**
 * A struct to contain next play item.
 * A PlayItem could either be a Playlist URL (to parse) or Media Info (to play).
 */
struct PlayItem {
    /**
     * Constructor for play item which holds an URL to playlist.
     *
     * @param playlistURL The URL of a playlist to parse next.
     */
    PlayItem(std::string playlistURL);

    /**
     * Constructor for play item which holds info about @c PlaylistEntry.
     *
     * @param playlistEntry The PlaylistEntry to play next.
     */
    PlayItem(avsCommon::utils::playlistParser::PlaylistEntry playlistEntry);

    /**
     * Enum to hold the type of PlayItem.
     *
     * If PlayItem is another playlist, then type is set to PLAYLIST_URL.
     * If PlayItem is information about media to play, then type is set to MEDIA_INFO.
     */
    enum class Type { PLAYLIST_URL, MEDIA_INFO } type;

    /// If type == PLAYLIST_URL, url to parse next playlist.
    const std::string playlistURL;

    /// If type == MEDIA_INFO, holds information about next @c PlaylistEntry.
    const avsCommon::utils::playlistParser::PlaylistEntry playlistEntry =
        avsCommon::utils::playlistParser::PlaylistEntry::createErrorEntry("");
};

/// A struct used to encapsulate information retrieved from an M3U playlist.
struct M3UContent {
    /**
     * Constructor for parsed content of master playlist.
     *
     * @param variantURLs The variant URLs pointing to media playlists.
     */
    M3UContent(const std::vector<std::string>& variantURLs);

    /**
     * Constructor for parsed content of media playlist.
     *
     * @param entries List of PlaylistEntry from parsed content.
     * @param isLive @c true for live HLS playlists.
     * @param mediaSequence The value of the EXT-X-MEDIA-SEQUENCE tag
     */
    M3UContent(
        const std::vector<avsCommon::utils::playlistParser::PlaylistEntry>& entries,
        bool isLive,
        long mediaSequence = INVALID_MEDIA_SEQUENCE);

    /**
     * Helper method to check if content is a master playlist.
     *
     * @return @c true if this represents a master playlist
     */
    bool isMasterPlaylist() const;

    /**
     * Helper method to tell if the media sequence field is present in the M3U8 document.
     *
     * @return @c true if this represents a playlist that has the media sequence field.
     */
    bool hasMediaSequence() const;

    /**
     * Helper method to check if no URLs are parsed.
     */
    bool empty() const;

    /// If this is a master playlist, variantURLs has list of media playlists.
    const std::vector<std::string> variantURLs;

    /// If this is a media playlist, entries has list of parsed entries.
    const std::vector<avsCommon::utils::playlistParser::PlaylistEntry> entries;

    /// If EXT-X-ENDLIST tag exists, isLive is set to false.
    const bool isLive;

    /**
     * The value of the EXT-X-MEDIA-SEQUENCE tag. The client code is responsible for calling the @c hasMediaSequence()
     * method to check if the content of this field should be read.
     */
    const long mediaSequence;
};

/**
 * Determines the playlist type of an M3U playlist.
 *
 * @param playlistContent The M3U playlist in @c std::string format.
 * @return @c true if the playlist is extended M3U or @c false otherwise
 */
bool isPlaylistExtendedM3U(const std::string& playlistContent);

/**
 * Parses an M3U playlist and returns the "children" URLs in the order they appeared in the playlist.
 *
 * @param playlistURL The URL of the M3U playlist that needs to be parsed.
 * @param content Text content of the downloaded M3U playlist to parse.
 * @return @c M3UContent which contains parsed list of variant URLs (for master playlist) OR list of @ Playlist (for
 * media playlist).
 */
M3UContent parseM3UContent(const std::string& playlistURL, const std::string& content);

/**
 * Parses #EXT-X-KEY line of HLS playlist and returns @c EncryptionInfo.
 *
 * @param line The line to parse.
 * @param baseURL The URL of the playlist.
 * @return Returns @c EncryptionInfo.
 */
avsCommon::utils::playlistParser::EncryptionInfo parseHLSEncryptionLine(
    const std::string& line,
    const std::string& baseURL);

/**
 * Helper method that parses the media sequence field. This method assumes that the line passed begins with the media
 * sequence tag.
 *
 * @param line The line to be parsed.
 * @return The media sequence value or @c INVALID_MEDIA_SEQUENCE if any issue happened during parsing.
 */
long parsePlaylistMediaSequence(const std::string& line);

/**
 * Parses #EXT-X-BYTERANGE line of HLS playlist and returns @c ByteRange.
 *
 * @param line The line to parse.
 * @return Returns @c ByteRange - {start, end}.
 */
avsCommon::utils::playlistParser::ByteRange parseHLSByteRangeLine(const std::string& line);

/**
 * Parses #EXT-X-MAP line of HLS playlist and returns @c PlaylistEntry.
 *
 * @param line The line to parse.
 * @param baseURL The url of the playlist.
 * @return Returns @c PlaylistEntry.
 */
avsCommon::utils::playlistParser::PlaylistEntry parseHLSMapLine(const std::string& line, const std::string& baseURL);

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_M3UPARSER_H_
