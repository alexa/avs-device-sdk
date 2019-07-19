/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTENTRY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTENTRY_H_

#include <chrono>
#include <ostream>
#include <queue>
#include <string>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/HTTPContent.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace playlistParser {

/// Alias for holding start and end offsets of byte range of an URL to download.
typedef std::tuple<long, long> ByteRange;

/**
 * An enum class used to specify the result of a parsing operation.
 */
enum class PlaylistParseResult {

    /// The playlist has been fully parsed successfully. This indicates that parsing of the playlist has completed.
    FINISHED,

    /**
     * The playlist parsing has encountered an error and will abort parsing. In this case, the url in the callback will
     * not be valid.
     */
    ERROR,

    /// The playlist parsing is still ongoing.
    STILL_ONGOING
};

/**
 * Struct to hold encryption info: method, key URL and initialization vector.
 */
struct EncryptionInfo {
    /**
     * Encryption method.
     */
    enum class Method {
        /// No encryption.
        NONE,

        /// AES-128 encryption method.
        AES_128,

        /// SAMPLE-AES encryption method.
        SAMPLE_AES
    };

    /**
     * Default constructor.
     */
    EncryptionInfo();

    /**
     * Constructor.
     *
     * @param method The encryption method of media.
     * @param url The URL for the encryption key to download.
     * @param initVector The intilization vector used for encryption.
     */
    EncryptionInfo(Method method, std::string url, std::string initVector = std::string());

    /**
     * Helper method to check if EncryptionInfo is valid.
     *
     * @return @c true if encryption info is valid or @c false otherwise.
     */
    bool isValid() const;

    /// Encryption method.
    Method method;

    /// The URL of the encryption key.
    std::string keyURL;

    /// The initilization vector used for encryption.
    std::string initVector;
};

/**
 * Struct to hold information about an entry parsed from playlist.
 */
struct PlaylistEntry {
    /**
     * Type of playlist entry.
     */
    enum class Type {
        /// Playlist Entry is about media.
        MEDIA_INFO,

        /// Playlist Entry is about media initialization.
        MEDIA_INIT_INFO,

        /// Playlist Entry is audio content, not a playlist.
        AUDIO_CONTENT
    };

    /**
     * Helper method to create ERROR PlaylistEntry
     *
     * @param url playlist URL
     * @return PlaylistEntry with url, INVALID_DURATION and ERROR parseResult.
     */
    static PlaylistEntry createErrorEntry(const std::string& url);

    /**
     * Helper method to create MEDIA_INIT_INFO PlaylistEntry.
     *
     * @param url playlist URL
     * @param byteRange The byte range of the MEDIA_INIT_INFO to download.
     * @return PlaylistEntry with url, INVALID_DURATION and STILL_ONGOING parseResult.
     */
    static PlaylistEntry createMediaInitInfo(std::string url, ByteRange byteRange = std::make_tuple(0, 0));

    /**
     * Constructor.
     *
     * @param _url The URL of the playlist entry media.
     * @param _duration The duration of media in milliseconds.
     * @param _parseResult The @c PlaylistParseResult.
     * @param _type The Type of the entry.
     * @param _byteRange The byte range of the url to download. Default is (0, 0).
     * @param _encryptionInfo The encryption info of the media. Default value is NONE.
     * @param _contentFetcher Content fetcher related to the entry.
     */
    PlaylistEntry(
        std::string _url,
        std::chrono::milliseconds _duration,
        avsCommon::utils::playlistParser::PlaylistParseResult _parseResult,
        Type _type = Type::MEDIA_INFO,
        ByteRange _byteRange = std::make_tuple(0, 0),
        EncryptionInfo _encryptionInfo = EncryptionInfo(),
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> _contentFetcher = nullptr);

    /**
     * Returns true if byterange is valid.
     */
    bool hasValidByteRange() const;

    /// Type of playlist entry.
    Type type;

    /// The URL for the entry.
    std::string url;

    /// The duration of the content if its known; INVALID_DURATION otherwise.
    std::chrono::milliseconds duration;

    /// The latest parsing result.
    PlaylistParseResult parseResult;

    /// ByteRange to download.
    ByteRange byteRange;

    /// EncryptionInfo of the media.
    EncryptionInfo encryptionInfo;

    /// The content fetcher associated with this playlist item. If a content fetcher is set, then it should be
    /// considered to be safe to use it. Otherwise, a new content fetcher should be created.
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher;
};

inline PlaylistEntry PlaylistEntry::createErrorEntry(const std::string& url) {
    return PlaylistEntry(
        url, std::chrono::milliseconds(-1), PlaylistParseResult::ERROR, Type::MEDIA_INFO, std::make_tuple(0, 0));
}

inline PlaylistEntry PlaylistEntry::createMediaInitInfo(std::string url, ByteRange byteRange) {
    auto duration = std::chrono::milliseconds(-1);
    return PlaylistEntry(url, duration, PlaylistParseResult::STILL_ONGOING, Type::MEDIA_INIT_INFO, byteRange);
}

inline EncryptionInfo::EncryptionInfo() : method(Method::NONE) {
}

inline EncryptionInfo::EncryptionInfo(Method method, std::string url, std::string initVector) :
        method(method),
        keyURL(url),
        initVector(initVector) {
}

inline bool EncryptionInfo::isValid() const {
    return (Method::NONE == method) || (Method::NONE != method && !keyURL.empty() && !initVector.empty());
}

inline bool PlaylistEntry::hasValidByteRange() const {
    long start, end;
    std::tie(start, end) = byteRange;
    return start >= 0 && end > 0;
}

inline PlaylistEntry::PlaylistEntry(
    std::string _url,
    std::chrono::milliseconds _duration,
    avsCommon::utils::playlistParser::PlaylistParseResult _parseResult,
    Type _type,
    ByteRange _byteRange,
    EncryptionInfo _encryptionInfo,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> _contentFetcher) :
        type(_type),
        url(_url),
        duration(_duration),
        parseResult(_parseResult),
        byteRange(_byteRange),
        encryptionInfo(_encryptionInfo),
        contentFetcher(_contentFetcher) {
}

}  // namespace playlistParser
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLAYLISTPARSER_PLAYLISTENTRY_H_
