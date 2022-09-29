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

#ifndef ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYER_INCLUDE_ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAADAPTERHANDLER_H_
#define ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYER_INCLUDE_ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAADAPTERHANDLER_H_

#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <rapidjson/document.h>

#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

/**
 * Describes an external media player's authorization status
 */
struct AuthorizedPlayerInfo {
public:
    /// The opaque token that uniquely identifies the local external player app
    std::string localPlayerId;
    /// Authorization status
    bool authorized;
    /// An opaque token for the domain or skill that is associated with this player
    std::string defaultSkillToken;
    /// The playerId that identifies this player
    std::string playerId;
};

/**
 * The abstract ExternalMediaAdapterHandler class implements the ExternalMediaAdapterHandlerInterface and provides a
 * base for users who wish to add custom External Media Player adapter handlers
 */
class ExternalMediaAdapterHandler
        : public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface
        , public std::enable_shared_from_this<ExternalMediaAdapterHandler> {
protected:
    /**
     * Constructor
     * @param name The name for this adapter handler
     * @param externalMediaPlayer The external media player interface
     */
    ExternalMediaAdapterHandler(const std::string& name);

    /**
     * Initialize this ExternalMediaAdapterHandler
     * @param speakerManager The speaker manager object
     * @return true if successful
     */
    bool initializeAdapterHandler(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager);

    /**
     * Validates if a player exists, and whether it is authorized
     * @param localPlayerId The local player ID
     * @param checkAuthorized Check if the player should be authorized
     * @return true if the validation check passes
     */
    bool validatePlayer(const std::string& localPlayerId, bool checkAuthorized = true);

    /**
     * Helper function to create an external media player event
     * @param localPlayerId The local player ID
     * @param event The event to send
     * @param includePlaybackSessionId Whether the playback session id should be included
     * @param createPayload Optional function which can be used to customize the payload
     * @return
     */
    std::string createExternalMediaPlayerEvent(
        const std::string& localPlayerId,
        const std::string& event,
        bool includePlaybackSessionId = false,
        std::function<void(rapidjson::Value::Object&, rapidjson::Value::AllocatorType&)> createPayload =
            [](rapidjson::Value::Object& v, rapidjson::Value::AllocatorType& a) {});

    /**
     * Add a newly discovered player
     * @param discoveredPlayers The details for the player
     */
    void reportDiscoveredPlayers(
        const std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>& discoveredPlayers);

    /**
     * Removes a player
     * @param localPlayerId
     * @return if the player was removed
     */
    bool removeDiscoveredPlayer(const std::string& localPlayerId);

    /// The following functions are to be overriden by implementors
    /// @{

    /**
     * Called when the list of authorized discovered players is received from AVS
     * @param authorizedPlayer The player and the authorization status
     * @return true if authorization successful
     */
    virtual bool handleAuthorization(const AuthorizedPlayerInfo& authorizedPlayer) = 0;

    /**
     * The handler should login the given player using the provided details
     * @param localPlayerId The player ID to be logged in
     * @param accessToken The access context of the user identifier.
     * @param userName The userName of the user logging in.
     * @param forceLogin bool which signifies if the adapter has to a force a login or merely cache the access token.
     * @param tokenRefreshInterval The duration in milliseconds for which the accessToken is valid.
     * @return true if login was successful
     */
    virtual bool handleLogin(
        const std::string& localPlayerId,
        const std::string& accessToken,
        const std::string& userName,
        bool forceLogin,
        std::chrono::milliseconds tokenRefreshInterval) = 0;

    /**
     * The handler should log out the given player
     * @param localPlayerId The player ID to be logged out
     * @return true if logout was successful
     */
    virtual bool handleLogout(const std::string& localPlayerId) = 0;

    /**
     * The handler should play the given player
     * @param params Play parameters to play with
     * @return true if successful
     */
    virtual bool handlePlay(const ExternalMediaAdapterHandlerInterface::PlayParams& params) = 0;

    /**
     * Method to initiate the different types of play control like PLAY/PAUSE/RESUME/NEXT/...
     *
     * @param localPlayerId The player ID to control
     * @param requestType The type of REQUEST. Will always be PLAY/PAUSE/RESUME/NEXT...
     * @param playbackTarget Target for handling play control
     * @return
     */
    virtual bool handlePlayControl(
        const std::string& localPlayerId,
        acsdkExternalMediaPlayerInterfaces::RequestType requestType,
        const std::string& playbackTarget) = 0;

    /**
     * Method to seek to the given offset.
     *
     * @param localPlayerId The player ID to control
     * @param offset The offset to seek to.
     * @return true if successful
     */
    virtual bool handleSeek(const std::string& localPlayerId, std::chrono::milliseconds offset) = 0;

    /**
     * Method to seek to an offset from the current position.
     * @param localPlayerId The player ID to control.
     * @param deltaOffset The offset to seek to relative to the current offset.
     * @return true if successful
     */
    virtual bool handleAdjustSeek(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset) = 0;

    /**
     * Method to fetch the state of an adapter.
     * @param localPlayerId The local player ID for which the state is requested
     * @param state reference to the @c AdapterState object which should be updated with the current state for the
     * adapter
     * @return true if successful
     */
    virtual bool handleGetAdapterState(
        const std::string& localPlayerId,
        acsdkExternalMediaPlayerInterfaces::AdapterState& state) = 0;

    /**
     * This function retrieves the offset of the current track the adapter is handling.
     * @param localPlayerId The local player Id for which offset is being requested
     * @return This returns the offset in milliseconds.
     */
    virtual std::chrono::milliseconds handleGetOffset(const std::string& localPlayerId);

    /**
     * Called when the SpeakerManager reports a volume change
     * @param volume The new volume
     */
    virtual void handleSetVolume(int8_t volume) = 0;

    /**
     * Called when the SpeakerManager reports a change to the mute state
     * @param volume The new mute state
     */
    virtual void handleSetMute(bool mute) = 0;
    /// @}

    /// alexaClientSDK::avsCommon::utils::RequiresShutdown
    virtual void doShutdown() override;

    /// The pointer to the external media player object
    std::weak_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface> m_externalMediaPlayer;

public:
    /// @name ExternalMediaAdapterHandler functions
    /// @{
    std::vector<acsdkExternalMediaPlayerInterfaces::PlayerInfo> updatePlayerInfo(
        const std::vector<acsdkExternalMediaPlayerInterfaces::PlayerInfo>& playerList) override;
    bool login(
        const std::string& localPlayerId,
        const std::string& accessToken,
        const std::string& userName,
        bool forceLogin,
        std::chrono::milliseconds tokenRefreshInterval) override;
    bool logout(const std::string& localPlayerId) override;
    bool play(const PlayParams& params) override;
    bool playControl(
        const std::string& localPlayerId,
        acsdkExternalMediaPlayerInterfaces::RequestType requestType,
        const std::string& playbackTarget) override;
    bool seek(const std::string& localPlayerId, std::chrono::milliseconds offset) override;
    bool adjustSeek(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset) override;
    acsdkExternalMediaPlayerInterfaces::AdapterState getAdapterState(const std::string& localPlayerId) override;
    std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> getAdapterStates() override;
    std::chrono::milliseconds getOffset(const std::string& localPlayerId) override;
    void setExternalMediaPlayer(const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>
                                    externalMediaPlayer) override;
    /// @}

    /// @name SpeakerManagerObserverInterface Functions
    /// @{
    void onSpeakerSettingsChanged(
        const Source& source,
        const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) override;
    /// @}

private:
    /// Map from localPlayerId to PlayerInfo
    std::unordered_map<std::string, acsdkExternalMediaPlayerInterfaces::PlayerInfo> m_playerInfoMap;

    /// The current speaker mute state
    bool m_muted;

    /// The current speaker volume
    int8_t m_volume;

    /// Serializes generic access. Used for delaying focus state change.
    std::mutex m_mutex;

    /// Generic executor. Used for delaying focus state change.
    alexaClientSDK::avsCommon::utils::threading::Executor m_executor;

    /// Serializes generic condition. Used for delaying focus state change.
    std::condition_variable m_attemptedSetFocusPlayerInFocusCondition;
};

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYER_INCLUDE_ACSDKEXTERNALMEDIAPLAYER_EXTERNALMEDIAADAPTERHANDLER_H_
