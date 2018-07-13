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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXTERNALMEDIAPLAYER_ADAPTERUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXTERNALMEDIAPLAYER_ADAPTERUTILS_H_

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include "AVSCommon/AVS/NamespaceAndName.h"
#include "AVSCommon/SDKInterfaces/ExternalMediaAdapterInterface.h"
#include "AVSCommon/Utils/RetryTimer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace externalMediaPlayer {

/// Enumeration class for events sent by adapters to AVS.
enum class AdapterEvent {
    /// ChangeReport event sent after adapter's initialization succeeds/fails.
    CHANGE_REPORT,

    /// Event to request token from third party.
    REQUEST_TOKEN,

    /// Login event when a guest user logs in.
    LOGIN,

    /// Logout event when a user logs out.
    LOGOUT,

    /// PlayerEvent to announce all kinds of player events - like play/pause/next etc.
    PLAYER_EVENT,

    /// PlayerErrorEvent to report all errors from the adapters.
    PLAYER_ERROR_EVENT
};

/// Table with the retry times on subsequent retries for session management (token fetch/changeReport send).
extern const std::vector<int> SESSION_RETRY_TABLE;

/// The retry timer for session management (token fetch/changeReport send).
extern avsCommon::utils::RetryTimer SESSION_RETRY_TIMER;

// The NamespaceAndName for events sent from the adapter to AVS.
extern const avsCommon::avs::NamespaceAndName CHANGE_REPORT;
extern const avsCommon::avs::NamespaceAndName REQUEST_TOKEN;
extern const avsCommon::avs::NamespaceAndName LOGIN;
extern const avsCommon::avs::NamespaceAndName LOGOUT;
extern const avsCommon::avs::NamespaceAndName PLAYER_EVENT;
extern const avsCommon::avs::NamespaceAndName PLAYER_ERROR_EVENT;

/**
 * Method to iterate over a vector of supported operation in playback state and convert to JSON.
 *
 * @param supportedOperations The array of supported operations from the current playback state.
 * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the array.
 */
rapidjson::Value buildSupportedOperations(
    const std::vector<avsCommon::sdkInterfaces::externalMediaPlayer::SupportedPlaybackOperation>& supportedOperations,
    rapidjson::Document::AllocatorType& allocator);

/**
 * Method to convert a playbackState to JSON.
 *
 * @param playbackState The playback state of the adapter.
 * @param The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the playback state JSON.
 */
rapidjson::Value buildPlaybackState(
    const avsCommon::sdkInterfaces::externalMediaPlayer::AdapterPlaybackState& playbackState,
    rapidjson::Document::AllocatorType& allocator);

/**
 * Method to convert session state  to JSON.
 *
 * @param sessionState The session state of the adapter.
 * @param The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the session state in JSON.
 */
rapidjson::Value buildSessionState(
    const avsCommon::sdkInterfaces::externalMediaPlayer::AdapterSessionState& sessionState,
    rapidjson::Document::AllocatorType& allocator);

/**
 * Method to build the default player.
 *
 * @param document The JSON Value to write the default player state into.
 * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return @c true if the build of default player state was successful, @c false otherwise.
 */
bool buildDefaultPlayerState(rapidjson::Value* document, rapidjson::Document::AllocatorType& allocator);

}  // namespace externalMediaPlayer
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // end
// ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXTERNALMEDIAPLAYER_ADAPTERUTILS_H_
