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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "acsdkExternalMediaPlayer/ExternalMediaAdapterHandler.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

using namespace avsCommon::utils::logger;
using acsdkExternalMediaPlayerInterfaces::Navigation;
using acsdkExternalMediaPlayerInterfaces::PlayerInfo;

/// String to identify log entries originating from this file.
static const std::string TAG("ExternalMediaAdapterHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const uint8_t DEFAULT_SPEAKER_VOLUME = 50;

ExternalMediaAdapterHandler::ExternalMediaAdapterHandler(const std::string& name) :
        ExternalMediaAdapterHandlerInterface::ExternalMediaAdapterHandlerInterface{name},
        m_muted{false},
        m_volume{DEFAULT_SPEAKER_VOLUME} {
}

bool ExternalMediaAdapterHandler::initializeAdapterHandler(
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) {
    speakerManager->addSpeakerManagerObserver(shared_from_this());
    return true;
}

bool ExternalMediaAdapterHandler::validatePlayer(const std::string& localPlayerId, bool checkAuthorized) {
    if (localPlayerId.empty()) return false;
    auto it = m_playerInfoMap.find(localPlayerId);
    return it != m_playerInfoMap.end() && (it->second.playerSupported || checkAuthorized == false);
}

std::vector<PlayerInfo> ExternalMediaAdapterHandler::updatePlayerInfo(const std::vector<PlayerInfo>& playerList) {
    std::vector<PlayerInfo> supportedPlayerList;

    for (const auto& next : playerList) {
        if (validatePlayer(next.localPlayerId, false)) {
            AuthorizedPlayerInfo info;

            info.localPlayerId = next.localPlayerId;
            info.authorized = next.playerSupported;
            info.defaultSkillToken = next.skillToken;
            info.playerId = next.playerId;

            handleAuthorization(info);

            supportedPlayerList.push_back(next);

            // copy the player info into the player info map
            m_playerInfoMap[next.localPlayerId] = next;
        }
    }

    return supportedPlayerList;
}

bool ExternalMediaAdapterHandler::login(
    const std::string& localPlayerId,
    const std::string& accessToken,
    const std::string& userName,
    bool forceLogin,
    std::chrono::milliseconds tokenRefreshInterval) {
    // call the platform media adapter
    return handleLogin(localPlayerId, accessToken, userName, forceLogin, tokenRefreshInterval);
}

bool ExternalMediaAdapterHandler::logout(const std::string& localPlayerId) {
    // call the platform media adapter
    return handleLogout(localPlayerId);
}

bool ExternalMediaAdapterHandler::play(const PlayParams& params) {
    if (!validatePlayer(params.localPlayerId)) {
        ACSDK_WARN(LX("playFailed")
                       .d("reason", "player is not configured or not authorized")
                       .d("localPlayerId", params.localPlayerId));
        return false;
    }

    auto playerInfo = m_playerInfoMap[params.localPlayerId];

    playerInfo.skillToken = params.skillToken;
    playerInfo.playbackSessionId = params.playbackSessionId;

    return handlePlay(params);
}

bool ExternalMediaAdapterHandler::playControl(
    const std::string& localPlayerId,
    acsdkExternalMediaPlayerInterfaces::RequestType requestType,
    const std::string& playbackTarget) {
    if (!validatePlayer(localPlayerId)) {
        ACSDK_WARN(LX("playControlFailed")
                       .d("reason", "player is not configured or not authorized")
                       .d("localPlayerId", localPlayerId));
        return false;
    }

    return handlePlayControl(localPlayerId, requestType, playbackTarget);
}

bool ExternalMediaAdapterHandler::seek(const std::string& localPlayerId, std::chrono::milliseconds offset) {
    if (!validatePlayer(localPlayerId)) {
        ACSDK_ERROR(
            LX("seekFailed").d("reason", "player is not configured or not authorized").d("playerId", localPlayerId));
        return false;
    }

    return handleSeek(localPlayerId, offset);
}

bool ExternalMediaAdapterHandler::adjustSeek(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset) {
    if (!validatePlayer(localPlayerId)) {
        ACSDK_ERROR(LX("adjustSeekFailed")
                        .d("reason", "player is not configured or not authorized")
                        .d("playerId", localPlayerId));
        return false;
    }
    return handleAdjustSeek(localPlayerId, deltaOffset);
}

acsdkExternalMediaPlayerInterfaces::AdapterState ExternalMediaAdapterHandler::getAdapterState(
    const std::string& localPlayerId) {
    const auto it = m_playerInfoMap.find(localPlayerId);
    if (it == m_playerInfoMap.end()) {
        ACSDK_ERROR(LX("getAdapterStateFailed")
                        .d("reason", "player is not configured or not authorized")
                        .d("playerId", localPlayerId));
        return {};
    }

    const auto& playerInfo = it->second;

    acsdkExternalMediaPlayerInterfaces::AdapterState state;

    // default session state
    state.sessionState.playerId = playerInfo.playerId;
    state.sessionState.skillToken = playerInfo.skillToken;
    state.sessionState.playbackSessionId = playerInfo.playbackSessionId;
    state.sessionState.spiVersion = playerInfo.spiVersion;

    // default playback state
    state.playbackState.playerId = playerInfo.playerId;
    if (!handleGetAdapterState(localPlayerId, state)) {
        ACSDK_ERROR(LX("getAdapterStateFailed").d("reason", "handleGetAdapterState failed"));
        return {};
    }

    return state;
}

std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> ExternalMediaAdapterHandler::getAdapterStates() {
    std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> adapterStateList;

    for (const auto& next : m_playerInfoMap) {
        auto playerInfo = next.second;

        if (playerInfo.playerSupported) {
            acsdkExternalMediaPlayerInterfaces::AdapterState state;

            // default session state
            state.sessionState.playerId = playerInfo.playerId;
            state.sessionState.skillToken = playerInfo.skillToken;
            state.sessionState.playbackSessionId = playerInfo.playbackSessionId;
            state.sessionState.spiVersion = playerInfo.spiVersion;

            // default playback state
            state.playbackState.playerId = playerInfo.playerId;

            // get the player state from the adapter implementation
            if (handleGetAdapterState(playerInfo.localPlayerId, state)) {
                adapterStateList.push_back(state);
            }
        }
    }

    return adapterStateList;
}

std::chrono::milliseconds ExternalMediaAdapterHandler::getOffset(const std::string& localPlayerId) {
    if (!validatePlayer(localPlayerId)) {
        ACSDK_ERROR(LX("getOffsetFailed")
                        .d("reason", "player is not configured or not authorized")
                        .d("playerId", localPlayerId));
        return std::chrono::milliseconds::zero();
    }

    return handleGetOffset(localPlayerId);
}

void ExternalMediaAdapterHandler::setExternalMediaPlayer(
    const std::shared_ptr<alexaClientSDK::acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>
        externalMediaPlayer) {
    m_externalMediaPlayer = externalMediaPlayer;
}

std::string ExternalMediaAdapterHandler::createExternalMediaPlayerEvent(
    const std::string& localPlayerId,
    const std::string& event,
    bool includePlaybackSessionId,
    std::function<void(rapidjson::Value::Object&, rapidjson::Value::AllocatorType&)> createPayload) {
    if (!validatePlayer(localPlayerId)) {
        ACSDK_ERROR(LX(__func__).d("reason", "localPlayerId is not valid").d("playerId", localPlayerId));
        return "";
    }
    auto playerInfo = m_playerInfoMap[localPlayerId];

    // create the event payload
    rapidjson::Document document(rapidjson::kObjectType);

    // create payload data
    auto payload = document.GetObject();

    // call the lamda createPayload() function
    createPayload(payload, document.GetAllocator());

    payload.AddMember(
        "playerId",
        rapidjson::Value().SetString(playerInfo.playerId.c_str(), playerInfo.playerId.length()),
        document.GetAllocator());
    payload.AddMember(
        "skillToken",
        rapidjson::Value().SetString(playerInfo.skillToken.c_str(), playerInfo.skillToken.length()),
        document.GetAllocator());

    if (includePlaybackSessionId) {
        payload.AddMember(
            "playbackSessionId",
            rapidjson::Value().SetString(playerInfo.playbackSessionId.c_str(), playerInfo.playbackSessionId.length()),
            document.GetAllocator());
    }

    // create payload string
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);

    return alexaClientSDK::avsCommon::avs::buildJsonEventString("ExternalMediaPlayer", event, "", buffer.GetString())
        .second;
}

void ExternalMediaAdapterHandler::reportDiscoveredPlayers(
    const std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>& discoveredPlayers) {
    // add the player info to the registered player map
    for (const auto& next : discoveredPlayers) {
        m_playerInfoMap[next.localPlayerId] = PlayerInfo(next.localPlayerId, next.spiVersion);
    }

    auto externalMediaPlayer = m_externalMediaPlayer.lock();
    if (externalMediaPlayer == nullptr) {
        ACSDK_ERROR(LX(__func__).d("reason", "unable to retrieve external media player"));
        return;
    }

    // used the discovered player sender to report the players
    externalMediaPlayer->updateDiscoveredPlayers(discoveredPlayers, {});
}

bool ExternalMediaAdapterHandler::removeDiscoveredPlayer(const std::string& localPlayerId) {
    auto it = m_playerInfoMap.find(localPlayerId);
    if (it == m_playerInfoMap.end()) {
        ACSDK_ERROR(LX(__func__).d("reason", "localPlayerId not found").d("localPlayerId", localPlayerId));
        return false;
    }

    // remove the player info map entry
    m_playerInfoMap.erase(it);

    auto externalMediaPlayer = m_externalMediaPlayer.lock();
    if (externalMediaPlayer == nullptr) {
        ACSDK_ERROR(LX(__func__).d("reason", "unable to retrieve external media player"));
        return false;
    }

    // notify the discovered player sender that the player has been removed
    externalMediaPlayer->updateDiscoveredPlayers({}, {localPlayerId});

    return true;
}

void ExternalMediaAdapterHandler::doShutdown() {
    m_executor.shutdown();

    if (!m_externalMediaPlayer.expired()) {
        m_externalMediaPlayer.reset();
    }
}

std::chrono::milliseconds ExternalMediaAdapterHandler::handleGetOffset(const std::string& localPlayerId) {
    acsdkExternalMediaPlayerInterfaces::AdapterState state;
    if (handleGetAdapterState(localPlayerId, state)) {
        return state.playbackState.trackOffset;
    }
    return std::chrono::milliseconds::zero();
}

void ExternalMediaAdapterHandler::onSpeakerSettingsChanged(
    const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
    const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type,
    const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) {
    if (avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME == type) {
        if (m_muted != settings.mute) {
            handleSetMute(settings.mute);
            m_muted = settings.mute;
        }

        if (m_volume != settings.volume) {
            handleSetVolume(settings.volume);
            m_volume = settings.volume;
        }
    }
}

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
