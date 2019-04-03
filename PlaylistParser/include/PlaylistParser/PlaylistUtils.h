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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_PLAYLISTUTILS_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_PLAYLISTUTILS_H_

#include <chrono>
#include <string>
#include <vector>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

/**
 * Parses an PLS playlist and returns the "children" URLs in the order they appeared in the playlist.
 *
 * @param content The content to parse.
 * @return A vector of URLs in the order they appeared in the playlist.
 */
std::vector<std::string> parsePLSContent(const std::string& playlistURL, const std::string& content);

/**
 * Removes a carriage return from the end of a line. This is required to handle Windows style line breaks ('\r\n').
 * std::getline reads by default up to '\n' so at times, the '\r' may be included when readinglines.
 *
 * @param line The line to parse
 */
void removeCarriageReturnFromLine(std::string* line);

/**
 * Retrieves playlist content and stores it into a string.
 *
 * @param contentFetcher Object used to retrieve url content.
 * @param [out] content The playlist content.
 * @param shouldShutDown A pointer to allow for the caller to cancel the content retrieval asynchronously
 * @return @c true if no error occured or @c false otherwise.
 * @note This function should be used to retrieve content specifically from playlist URLs. Attempting to use this
 * on a media URL could be blocking forever as the URL might point to a live stream.
 */
bool readFromContentFetcher(
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher,
    std::string* content,
    std::atomic<bool>* shouldShutDown);

/**
 * Determines whether the provided url is an absolute url as opposed to a relative url. This is done by simply
 * checking to see if the string contains the substring "://".
 *
 * @param url The url to check.
 * @return @c true if the url is an absolute url and @c false otherwise.
 */
bool isURLAbsolute(const std::string& url);

/**
 * Creates an absolute url, given a base url and a relative path from that url. For example, if
 * "www.awesomewebsite.com/music/test.m3u" was the base url and the relative path was "music.mp3",
 * "www.awesomewebsite.com/music/music.mp3" would be returned.
 *
 * @param baseURL The base url to add the relative path to.
 * @param relativePath The relative path to add to the base url.
 * @param [out] absoluteURL The absolute url generated
 * @return @c true If everything was successful and @c false otherwise.
 */
bool getAbsoluteURLFromRelativePathToURL(std::string baseURL, std::string relativePath, std::string* absoluteURL);

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_PLAYLISTUTILS_H_
