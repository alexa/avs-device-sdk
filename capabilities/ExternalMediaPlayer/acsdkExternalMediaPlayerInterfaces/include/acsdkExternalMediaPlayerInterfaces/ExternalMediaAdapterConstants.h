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

#ifndef ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERCONSTANTS_H_
#define ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERCONSTANTS_H_

#include <AVSCommon/AVS/NamespaceAndName.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {

/// The const char for the players key field in the context.
static const char PLAYERS[] = "players";
/// The const char for the playerInFocus key field in the context.
static const char PLAYER_IN_FOCUS[] = "playerInFocus";
/// The const char for agent key.
static const char AGENT_KEY[] = "agent";
/// The root key for configuration values for the ExternalMediaPlayer.
static const std::string EMP_CONFIG_KEY("externalMediaPlayer");
/// Key for the @c agent id string unter the @c EMP_CONFIG_KEY configuration
/// node
static const std::string EMP_AGENT_KEY("agentString");

// The key values used in the context payload from External Media Player to AVS.
const char PLAYER_ID[] = "playerId";
const char ENDPOINT_ID[] = "endpointId";
const char LOGGED_IN[] = "loggedIn";
const char USERNAME[] = "username";
const char IS_GUEST[] = "isGuest";
const char LAUNCHED[] = "launched";
const char ACTIVE[] = "active";
const char SPI_VERSION[] = "spiVersion";
const char PLAYER_COOKIE[] = "playerCookie";
const char SKILL_TOKEN[] = "skillToken";
const char PLAYBACK_SESSION_ID[] = "playbackSessionId";

// The key values used in the context payload from External Media Player to AVS.
const char STATE[] = "state";
const char OPERATIONS[] = "supportedOperations";
const char MEDIA[] = "media";
const char POSITIONINMS[] = "positionMilliseconds";
const char SHUFFLE[] = "shuffle";
const char REPEAT[] = "repeat";
const char FAVORITE[] = "favorite";
const char PLAYBACK_SOURCE[] = "playbackSource";
const char TYPE[] = "type";
const char PLAYBACK_SOURCE_ID[] = "playbackSourceId";
const char PLAYBACK_ID[] = "playbackId";
const char TRACKNAME[] = "trackName";
const char TRACK_ID[] = "trackId";
const char TRACK_NUMBER[] = "trackNumber";
const char ARTIST[] = "artist";
const char ARTIST_ID[] = "artistId";
const char ALBUM[] = "album";
const char ALBUM_ID[] = "albumId";
const char COVER_URLS[] = "coverUrls";
const char TINY_URL[] = "tiny";
const char SMALL_URL[] = "small";
const char MEDIUM_URL[] = "medium";
const char LARGE_URL[] = "large";
const char COVER_ID[] = "coverId";
const char MEDIA_PROVIDER[] = "mediaProvider";
const char MEDIA_TYPE[] = "mediaType";
const char DURATIONINMS[] = "durationInMilliseconds";
const char VALUE[] = "value";

}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERCONSTANTS_H_
