/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVS/ExternalMediaPlayer/AdapterUtils.h"
#include "AVSCommon/AVS/ExternalMediaPlayer/ExternalMediaAdapterConstants.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace externalMediaPlayer {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::externalMediaPlayer;

/*
 * As per Spotify integration spec request for RequestToken retries shall not be performed in
 * an interval of less than 800 milliseconds.
 */
const std::vector<int> SESSION_RETRY_TABLE = {
    1600,   // Retry 1:  1.6s -> .8s to 2.4s
    2000,   // Retry 2:  2.0s -> 1.0s to 3s
    3000,   // Retry 3:  3.00s
    5000,   // Retry 4:  5.00s
    10000,  // Retry 5: 10.00s
    20000,  // Retry 6: 20.00s
};

avsCommon::utils::RetryTimer SESSION_RETRY_TIMER(SESSION_RETRY_TABLE);

const NamespaceAndName CHANGE_REPORT("Alexa", "ChangeReport");
const NamespaceAndName REQUEST_TOKEN("ExternalMediaPlayer", "RequestToken");
const NamespaceAndName LOGIN("ExternalMediaPlayer", "Login");
const NamespaceAndName LOGOUT("ExternalMediaPlayer", "Logout");
const NamespaceAndName PLAYER_EVENT("ExternalMediaPlayer", "PlayerEvent");
const NamespaceAndName PLAYER_ERROR_EVENT("ExternalMediaPlayer", "PlayerError");

/// The default state of a player.
const char DEFAULT_STATE[] = "IDLE";

rapidjson::Value buildSupportedOperations(
    const std::vector<SupportedPlaybackOperation>& supportedOperations,
    rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value opArray(rapidjson::kArrayType);

    for (const auto& op : supportedOperations) {
        rapidjson::Value opJson;
        opJson.SetString(SupportedPlaybackOperationToString(op), allocator);
        opArray.PushBack(opJson, allocator);
    }

    return opArray;
}

rapidjson::Value buildPlaybackState(
    const AdapterPlaybackState& playbackState,
    rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value playerJson;
    playerJson.SetObject();

    playerJson.AddMember(PLAYER_ID, playbackState.playerId, allocator);
    playerJson.AddMember(STATE, playbackState.state, allocator);
    auto operations = buildSupportedOperations(playbackState.supportedOperations, allocator);
    playerJson.AddMember(OPERATIONS, operations, allocator);
    playerJson.AddMember(POSITIONINMS, static_cast<uint64_t>(playbackState.trackOffset.count()), allocator);
    playerJson.AddMember(SHUFFLE, SHUFFLE_STATUS_STRING(playbackState.shuffleEnabled), allocator);
    playerJson.AddMember(REPEAT, REPEAT_STATUS_STRING(playbackState.repeatEnabled), allocator);
    playerJson.AddMember(FAVORITE, RatingToString(playbackState.favorites), allocator);

    rapidjson::Document media(rapidjson::kObjectType);
    media.AddMember(TYPE, playbackState.type, allocator);

    rapidjson::Document value(rapidjson::kObjectType);
    value.AddMember(PLAYBACK_SOURCE, playbackState.playbackSource, allocator);
    value.AddMember(PLAYBACK_SOURCE_ID, playbackState.playbackSourceId, allocator);
    value.AddMember(TRACKNAME, playbackState.trackName, allocator);
    value.AddMember(TRACK_ID, playbackState.trackId, allocator);
    value.AddMember(TRACK_NUMBER, playbackState.trackNumber, allocator);
    value.AddMember(ARTIST, playbackState.artistName, allocator);
    value.AddMember(ARTIST_ID, playbackState.artistId, allocator);
    value.AddMember(ALBUM, playbackState.albumName, allocator);
    value.AddMember(ALBUM_ID, playbackState.albumId, allocator);

    rapidjson::Document coverUrl(rapidjson::kObjectType);
    coverUrl.AddMember(TINY_URL, playbackState.tinyURL, allocator);
    coverUrl.AddMember(SMALL_URL, playbackState.smallURL, allocator);
    coverUrl.AddMember(MEDIUM_URL, playbackState.mediumURL, allocator);
    coverUrl.AddMember(LARGE_URL, playbackState.largeURL, allocator);
    value.AddMember(COVER_URLS, coverUrl, allocator);

    value.AddMember(COVER_ID, playbackState.coverId, allocator);
    value.AddMember(MEDIA_PROVIDER, playbackState.mediaProvider, allocator);
    value.AddMember(MEDIA_TYPE, MediaTypeToString(playbackState.mediaType), allocator);
    value.AddMember(DURATIONINMS, static_cast<uint64_t>(playbackState.duration.count()), allocator);

    media.AddMember(VALUE, value, allocator);
    playerJson.AddMember(MEDIA, media, allocator);

    return playerJson;
}

rapidjson::Value buildSessionState(
    const AdapterSessionState& sessionState,
    rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value playerJson;
    playerJson.SetObject();
    playerJson.AddMember(PLAYER_ID, sessionState.playerId, allocator);
    playerJson.AddMember(ENDPOINT_ID, sessionState.endpointId, allocator);
    playerJson.AddMember(LOGGED_IN, sessionState.loggedIn, allocator);
    playerJson.AddMember(USERNAME, sessionState.userName, allocator);
    playerJson.AddMember(IS_GUEST, sessionState.isGuest, allocator);
    playerJson.AddMember(LAUNCHED, sessionState.launched, allocator);
    playerJson.AddMember(ACTIVE, sessionState.active, allocator);

    return playerJson;
}

bool buildDefaultPlayerState(rapidjson::Value* document, rapidjson::Document::AllocatorType& allocator) {
    if (!document) {
        return false;
    }
    document->AddMember(STATE, DEFAULT_STATE, allocator);
    rapidjson::Value opArray(rapidjson::kArrayType);
    document->AddMember(OPERATIONS, opArray, allocator);
    document->AddMember(SHUFFLE, SHUFFLE_STATUS_STRING(false), allocator);
    document->AddMember(REPEAT, REPEAT_STATUS_STRING(false), allocator);
    document->AddMember(FAVORITE, RatingToString(Favorites::NOT_RATED), allocator);
    document->AddMember(POSITIONINMS, 0, allocator);
    document->AddMember(UNCERTAINITYINMS, 0, allocator);

    return true;
}

}  // namespace externalMediaPlayer
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
