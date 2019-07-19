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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXTERNALMEDIAPLAYER_EXTERNALMEDIAADAPTERCONSTANTS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXTERNALMEDIAPLAYER_EXTERNALMEDIAADAPTERCONSTANTS_H_

#include "AVSCommon/AVS/NamespaceAndName.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace externalMediaPlayer {

// The key values used in the context payload from External Media Player to AVS.
const char PLAYER_ID[] = "playerId";
const char ENDPOINT_ID[] = "endpointId";
const char LOGGED_IN[] = "loggedIn";
const char USERNAME[] = "username";
const char IS_GUEST[] = "isGuest";
const char LAUNCHED[] = "launched";
const char ACTIVE[] = "active";

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
const char UNCERTAINITYINMS[] = "uncertaintyInMilliseconds";

}  // namespace externalMediaPlayer
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXTERNALMEDIAPLAYER_EXTERNALMEDIAADAPTERCONSTANTS_H_
