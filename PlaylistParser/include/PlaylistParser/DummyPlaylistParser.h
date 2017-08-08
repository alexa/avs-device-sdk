/*
 * DummyPlaylistParser.h
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

#ifndef ALEXA_CLIENT_SDK_PLAYLIST_PARSER_INCLUDE_PLAYLIST_PARSER_DUMMY_PLAYLIST_PARSER_H_
#define ALEXA_CLIENT_SDK_PLAYLIST_PARSER_INCLUDE_PLAYLIST_PARSER_DUMMY_PLAYLIST_PARSER_H_

#include <AVSCommon/Utils/PlaylistParser/PlaylistParserInterface.h>

namespace alexaClientSDK {
namespace playlistParser {

/**
 * A simple dummy implementation of the PlaylistParser
 *
 */
class DummyPlaylistParser : public avsCommon::utils::playlistParser::PlaylistParserInterface {
public:
    /**
     * Creates an instance of the @c DummyPlaylistParser.
     *
     * @return An instance of the @c DummyPlaylistParser if successful else a @c nullptr.
     */
    static std::shared_ptr<DummyPlaylistParser> create();

    bool parsePlaylist(const std::string& url,
            std::shared_ptr<avsCommon::utils::playlistParser::PlaylistParserObserverInterface> observer) override;
};

} // namespace playlistParser
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_PLAYLIST_PARSER_INCLUDE_PLAYLIST_PARSER_DUMMY_PLAYLIST_PARSER_H_

