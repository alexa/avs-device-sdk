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
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "acsdkExternalMediaPlayerInterfaces/AdapterUtils.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterConstants.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {

using namespace avsCommon::avs;
using namespace avsCommon::avs::constants;

/// String to identify log entries originating from this file.
static const std::string TAG("AdapterUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/*
 * As per the integration spec, request for RequestToken retries shall not be
 * performed in an interval of less than 800 milliseconds.
 */
const std::vector<int> SESSION_RETRY_TABLE = {
    1000 * 60,   // Retry 1:  1.0mins
    5000 * 60,   // Retry 2:  5.0mins
    15000 * 60,  // Retry 3:  15.00mins
    20000 * 60,  // Retry 4:  20.00mins
    30000 * 60,  // Retry 5:  30.00mins
    60000 * 60,  // Retry 6:  60.00mins
};

avsCommon::utils::RetryTimer SESSION_RETRY_TIMER(SESSION_RETRY_TABLE);

const NamespaceAndName CHANGE_REPORT("Alexa", "ChangeReport");
const NamespaceAndName REQUEST_TOKEN("ExternalMediaPlayer", "RequestToken");
const NamespaceAndName LOGIN("ExternalMediaPlayer", "Login");
const NamespaceAndName LOGOUT("ExternalMediaPlayer", "Logout");
const NamespaceAndName PLAYER_EVENT("ExternalMediaPlayer", "PlayerEvent");
const NamespaceAndName PLAYER_ERROR_EVENT("ExternalMediaPlayer", "PlayerError");

std::map<AdapterEvent, std::pair<std::string, std::string>> eventNameSpaceNameMap = {
    {AdapterEvent::LOGIN, std::make_pair(LOGIN.nameSpace, LOGIN.name)},
    {AdapterEvent::LOGOUT, std::make_pair(LOGOUT.nameSpace, LOGOUT.name)},
    {AdapterEvent::CHANGE_REPORT, std::make_pair(CHANGE_REPORT.nameSpace, CHANGE_REPORT.name)},
    {AdapterEvent::REQUEST_TOKEN, std::make_pair(REQUEST_TOKEN.nameSpace, REQUEST_TOKEN.name)},
    {AdapterEvent::PLAYER_EVENT, std::make_pair(PLAYER_EVENT.nameSpace, PLAYER_EVENT.name)},
    {AdapterEvent::PLAYER_ERROR_EVENT, std::make_pair(PLAYER_ERROR_EVENT.nameSpace, PLAYER_ERROR_EVENT.name)}};

// The namespaces used in the context.
static const std::string EXTERNALMEDIAPLAYER_STATE_NAMESPACE = "ExternalMediaPlayer";
static const std::string PLAYBACKSTATEREPORTER_STATE_NAMESPACE = "Alexa.PlaybackStateReporter";

// The names used in the context.
static const std::string EXTERNALMEDIAPLAYER_NAME = "ExternalMediaPlayerState";
static const std::string PLAYBACKSTATEREPORTER_NAME = "playbackState";

/// The default state of a player.
const char DEFAULT_STATE[] = "IDLE";

rapidjson::Value buildSupportedOperations(
    const std::set<SupportedPlaybackOperation>& supportedOperations,
    rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value opArray(rapidjson::kArrayType);

    for (const auto& op : supportedOperations) {
        rapidjson::Value opJson;
        opJson.SetString(SupportedPlaybackOperationToString(op), allocator);
        opArray.PushBack(opJson, allocator);
    }

    return opArray;
}

bool buildMediaState(
    rapidjson::Value* document,
    const AdapterPlaybackState& playbackState,
    rapidjson::Document::AllocatorType& allocator) {
    if (!document) {
        return false;
    }
    rapidjson::Document media(rapidjson::kObjectType);
    media.AddMember(TYPE, playbackState.type, allocator);

    rapidjson::Document value(rapidjson::kObjectType);
    value.AddMember(PLAYBACK_SOURCE, playbackState.playbackSource, allocator);
    value.AddMember(PLAYBACK_SOURCE_ID, playbackState.playbackSourceId, allocator);
    value.AddMember(PLAYBACK_ID, playbackState.playbackId, allocator);
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
    document->AddMember(MEDIA, media, allocator);

    return true;
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

    buildMediaState(&playerJson, playbackState, allocator);
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
    playerJson.AddMember(SPI_VERSION, sessionState.spiVersion, allocator);
    playerJson.AddMember(PLAYER_COOKIE, sessionState.playerCookie, allocator);
    playerJson.AddMember(SKILL_TOKEN, sessionState.skillToken, allocator);
    playerJson.AddMember(PLAYBACK_SESSION_ID, sessionState.playbackSessionId, allocator);

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

    return true;
}

std::string getEmpContextString(AdapterState adapterState) {
    rapidjson::Document document;
    document.SetArray();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    // Session state json object
    rapidjson::Document sessionState(rapidjson::kObjectType);

    rapidjson::Document sessionHeader(rapidjson::kObjectType);
    sessionHeader.AddMember(rapidjson::StringRef(NAME_KEY_STRING), EXTERNALMEDIAPLAYER_NAME, allocator);
    sessionHeader.AddMember(rapidjson::StringRef(NAMESPACE_KEY_STRING), EXTERNALMEDIAPLAYER_STATE_NAMESPACE, allocator);
    sessionState.AddMember(rapidjson::StringRef(HEADER_KEY_STRING), sessionHeader, allocator);

    rapidjson::Value players(rapidjson::kArrayType);
    rapidjson::Value playerJson = buildSessionState(adapterState.sessionState, allocator);
    players.PushBack(playerJson, allocator);

    rapidjson::Document sessionPayload(rapidjson::kObjectType);

    std::string agentSting;
    auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot();
    auto empGroup = config[EMP_CONFIG_KEY];
    empGroup.getString(EMP_AGENT_KEY, &agentSting, "");
    if (agentSting.empty()) {
        ACSDK_ERROR(LX("getEmpContextString").d("reason", "agentStringIsEmpty"));
    }

    sessionPayload.AddMember(rapidjson::StringRef(AGENT_KEY), std::string(agentSting), allocator);
    sessionPayload.AddMember(rapidjson::StringRef(SPI_VERSION), adapterState.sessionState.spiVersion, allocator);
    sessionPayload.AddMember(rapidjson::StringRef(PLAYER_IN_FOCUS), adapterState.sessionState.playerId, allocator);
    sessionPayload.AddMember(rapidjson::StringRef(PLAYERS), players, allocator);
    sessionState.AddMember(rapidjson::StringRef(PAYLOAD_KEY_STRING), sessionPayload, allocator);

    // Playback state json object
    rapidjson::Document playbackState(rapidjson::kObjectType);

    rapidjson::Document playbackHeader(rapidjson::kObjectType);
    playbackHeader.AddMember(rapidjson::StringRef(NAME_KEY_STRING), PLAYBACKSTATEREPORTER_NAME, allocator);
    playbackHeader.AddMember(
        rapidjson::StringRef(NAMESPACE_KEY_STRING), PLAYBACKSTATEREPORTER_STATE_NAMESPACE, allocator);
    playbackState.AddMember(rapidjson::StringRef(HEADER_KEY_STRING), playbackHeader, allocator);

    rapidjson::Value playbackPayload(rapidjson::kObjectType);
    if (!buildDefaultPlayerState(&playbackPayload, allocator)) {
        ACSDK_ERROR(LX("getEmpContextString").d("reason", "buildDefaultPlayerStateFailed"));
        return "";
    }
    buildMediaState(&playbackPayload, adapterState.playbackState, allocator);

    rapidjson::Value playersArray(rapidjson::kArrayType);
    playersArray.PushBack(
        rapidjson::Value(buildPlaybackState(adapterState.playbackState, allocator), allocator), allocator);

    playbackPayload.AddMember(rapidjson::StringRef(PLAYERS), playersArray, allocator);
    playbackState.AddMember(rapidjson::StringRef(PAYLOAD_KEY_STRING), playbackPayload, allocator);

    document.PushBack(sessionState, allocator);
    document.PushBack(playbackState, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    if (!document.Accept(writer)) {
        ACSDK_ERROR(LX("getEmpContextString").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK
