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

#ifndef ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERHANDLERINTERFACE_H_

#include <chrono>
#include <string>
#include <vector>

#include <AVSCommon/AVS/PlayRequestor.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterInterface.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerCommon.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {

/// DEE-267369: Avoid cyclic dependency by having ExternalMediaPlayerInterface and
/// ExternalMediaAdapeExternalMediaAdapterHandlerInterface depend on a separate interface,
/// and limit the scope of adapters to change EMP.
class ExternalMediaPlayerInterface;

/**
 * The ExternalMediaAdapterHandlerInterface specifies the interface of adapter handler objects which interact with third
 * party music service providers. The adapter handler may handle multiple players distinguished by a different player ID
 * and provides users with an interface to manage playback control and session management.
 *
 * @note Multiple handlers are supported by the AVS SDK, and each handler has the ability to support multiple
 * players, both options are equally valid and it is up to the implementor to decide which implementation better suits
 * their use case. For example, registering two handlers, one of which supports a single player, and a second handler
 * which supports two players is a valid use case.
 */
class ExternalMediaAdapterHandlerInterface : public alexaClientSDK::avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor
     * @param name The name to identify this ExternalMediaAdapterHandler
     */
    explicit ExternalMediaAdapterHandlerInterface(const std::string& name);

    /**
     * Destructor
     */
    virtual ~ExternalMediaAdapterHandlerInterface() = default;

    /// PlayParams  is a struct that contains the parameters for the play method
    struct PlayParams {
        /// Local player id to play with
        std::string localPlayerId;
        /// Play context token for specifying what to play
        std::string playContextToken;
        /// Index for track
        int64_t index;
        /// Offset to play from
        std::chrono::milliseconds offset;
        /// Associated skillToken
        std::string skillToken;
        /// Playback session id for identifying the session
        std::string playbackSessionId;
        /// Navigation for indicating foreground or not
        Navigation navigation;
        /// Whether or not to preload first
        bool preload;
        /// PlayRequestor for indicating who requested playback
        alexaClientSDK::avsCommon::avs::PlayRequestor playRequestor;
        /// Playback target to play on
        std::string playbackTarget;

        /**
         * Construct PlayParams
         *
         * @param localPlayerId The localPlayerId that this play control is targeted at
         * @param playContextToken Play context {Track/playlist/album/artist/station/podcast} identifier.
         * @param index The index of the media item in the container, if the container is indexable.
         * @param offset The offset position within media item, in milliseconds.
         * @param skillToken An opaque token for the domain or skill that is presently associated with this player.
         * @param playbackSessionId A universally unique identifier (UUID) generated to the RFC 4122 specification.
         * @param navigation Communicates desired visual display behavior for the app associated with playback.
         * @param preload If true, this Play directive is intended to preload the identified content only but not begin
         * playback.
         * @param playRequestor The @c PlayRequestor object that is used to distinguish if it's a music alarm or not.
         * @param playbackTarget Playback target to play
         */
        PlayParams(
            const std::string& localPlayerId,
            const std::string& playContextToken,
            int64_t index,
            std::chrono::milliseconds offset,
            const std::string& skillToken,
            const std::string& playbackSessionId,
            Navigation navigation,
            bool preload,
            const alexaClientSDK::avsCommon::avs::PlayRequestor& playRequestor,
            const std::string& playbackTarget) :
                localPlayerId{localPlayerId},
                playContextToken{playContextToken},
                index{index},
                offset{offset},
                skillToken{skillToken},
                playbackSessionId{playbackSessionId},
                navigation{navigation},
                preload{preload},
                playRequestor(playRequestor),
                playbackTarget{playbackTarget} {
        }
    };

    /**
     * Method to notify the handler that the cloud status of given players has been updated.
     * This method also provides the playerId and skillToken as identified by the cloud. The cloud support for a player
     * may be revoked at any time. The state of any players not included on the playerList should be assumed
     * to be unchanged.
     *
     * @param playerList The list of players whose state has changed
     * @return The list of players which were processed by this handler
     */
    virtual std::vector<PlayerInfo> updatePlayerInfo(const std::vector<PlayerInfo>& playerList) = 0;

    /**
     * Method to allow a user to login to a third party music provider.
     *
     * @param localPlayerId The local player ID being logged in
     * @param accessToken The access context of the user identifier.
     * @param userName The userName of the user logging in.
     * @param forceLogin bool which signifies if the adapter has to a force a login or merely cache the access token.
     * @param tokenRefreshInterval The duration in milliseconds for which the accessToken is valid.
     * @return True if the call was handled
     */
    virtual bool login(
        const std::string& localPlayerId,
        const std::string& accessToken,
        const std::string& userName,
        bool forceLogin,
        std::chrono::milliseconds tokenRefreshInterval) = 0;

    /**
     * Method that handles logging out a user from a third party library/cloud.
     * @param localPlayerId The local player ID being logged out
     * @return True if the call was handled
     */
    virtual bool logout(const std::string& localPlayerId) = 0;

    /**
     * Method to allow a user to initiate play from a third party music service provider based on a play context.
     *
     * @param params Play parameters required for playback
     * @return True if the call was handled
     */
    virtual bool play(const PlayParams& params) = 0;

    /**
     * Method to initiate the different types of play control like PLAY/PAUSE/RESUME/NEXT/...
     *
     * @param localPlayerId The localPlayerId that this play control is targeted at
     * @param requestType The type of REQUEST. Will always be PLAY/PAUSE/RESUME/NEXT...
     * @param playbackTarget Playback target to handle play control with
     * @return True if the call was handled
     */
    virtual bool playControl(
        const std::string& localPlayerId,
        acsdkExternalMediaPlayerInterfaces::RequestType requestType,
        const std::string& playbackTarget) = 0;

    /**
     * Method to seek to the given offset.
     *
     * @param localPlayerId The localPlayerId that this seek control is targeted at
     * @param offset The offset to seek to.
     * @return True if the call was handled
     */
    virtual bool seek(const std::string& localPlayerId, std::chrono::milliseconds offset) = 0;

    /**
     * Method to seek to an offset from the current position.
     *
     * @param localPlayerId The localPlayerId that this seek control is targeted at
     * @param deltaOffset The offset to seek to relative to the current offset.
     * @return True if the call was handled
     */
    virtual bool adjustSeek(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset) = 0;

    /**
     * Method to fetch the state(session state and playback state) of an adapter.
     *
     * @param localPlayerId The player ID for which the state is being requested
     * @return The adapter state
     */
    virtual acsdkExternalMediaPlayerInterfaces::AdapterState getAdapterState(const std::string& localPlayerId) = 0;

    /**
     * Method to fetch the state for all adapters handled by this adapter handler
     *
     * @return Vector of all adapter states
     */
    virtual std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> getAdapterStates() = 0;

    /**
     * This function retrieves the offset of the current track the adapter is handling.
     *
     * @param localPlayerId The player ID for which the offset is being requested
     * @return The offset in milliseconds
     */
    virtual std::chrono::milliseconds getOffset(const std::string& localPlayerId) = 0;

    /**
     * Method to set the external media player interface used by this handler
     *
     * @param externalMediaPlayer Pointer to the external media player
     */
    virtual void setExternalMediaPlayer(const std::shared_ptr<ExternalMediaPlayerInterface> externalMediaPlayer) = 0;
};

inline ExternalMediaAdapterHandlerInterface::ExternalMediaAdapterHandlerInterface(const std::string& name) :
        alexaClientSDK::avsCommon::utils::RequiresShutdown{name} {
}

}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYERINTERFACES_INCLUDE_ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAADAPTERHANDLERINTERFACE_H_
