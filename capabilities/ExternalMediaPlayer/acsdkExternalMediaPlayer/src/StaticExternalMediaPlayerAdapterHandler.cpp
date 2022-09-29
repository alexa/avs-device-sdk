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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "acsdkExternalMediaPlayer/StaticExternalMediaPlayerAdapterHandler.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

using acsdkExternalMediaPlayerInterfaces::PlayerInfo;
using HandlePlayParams = acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface::HandlePlayParams;

/// String to identify log entries originating from this file.
static const std::string TAG("StaticExternalMediaPlayerAdapterHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

StaticExternalMediaPlayerAdapterHandler::StaticExternalMediaPlayerAdapterHandler() :
        ExternalMediaAdapterHandlerInterface::ExternalMediaAdapterHandlerInterface(TAG) {
}

void StaticExternalMediaPlayerAdapterHandler::addAdapter(
    const std::string& playerId,
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface> adapter) {
    std::lock_guard<std::mutex> lock{m_adaptersMutex};
    m_adapters[playerId] = std::move(adapter);
}

void StaticExternalMediaPlayerAdapterHandler::doShutdown() {
    std::unique_lock<std::mutex> lock{m_adaptersMutex};
    auto adaptersCopy = m_adapters;
    m_adapters.clear();
    lock.unlock();

    for (const auto& adapter : adaptersCopy) {
        if (!adapter.second) {
            continue;
        }
        adapter.second->shutdown();
    }
}

std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface>
StaticExternalMediaPlayerAdapterHandler::getAdapterByLocalPlayerId(const std::string& localPlayerId) {
    ACSDK_DEBUG5(LX(__func__));
    auto lock = std::unique_lock<std::mutex>(m_adaptersMutex);
    if (localPlayerId.empty()) {
        return nullptr;
    }
    auto localPlayerIdToAdapter = m_adapters.find(localPlayerId);

    if (m_adapters.end() == localPlayerIdToAdapter) {
        return nullptr;
    }

    return localPlayerIdToAdapter->second;
}

std::vector<PlayerInfo> StaticExternalMediaPlayerAdapterHandler::updatePlayerInfo(
    const std::vector<PlayerInfo>& playerList) {
    ACSDK_DEBUG5(LX(__func__));

    std::vector<PlayerInfo> authorizedPlayers;
    for (auto player : playerList) {
        auto adapter = getAdapterByLocalPlayerId(player.localPlayerId);
        if (adapter != nullptr) {
            adapter->handleAuthorized(player.playerSupported, player.playerId, player.skillToken);
            authorizedPlayers.push_back(player);
        }
    }
    return authorizedPlayers;
}

bool StaticExternalMediaPlayerAdapterHandler::login(
    const std::string& localPlayerId,
    const std::string& accessToken,
    const std::string& userName,
    bool forceLogin,
    std::chrono::milliseconds tokenRefreshInterval) {
    auto player = getAdapterByLocalPlayerId(localPlayerId);
    if (!player) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("localPlayerId", localPlayerId));
    player->handleLogin(accessToken, userName, forceLogin, tokenRefreshInterval);
    return true;
}

bool StaticExternalMediaPlayerAdapterHandler::logout(const std::string& localPlayerId) {
    auto player = getAdapterByLocalPlayerId(localPlayerId);
    if (!player) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("playerId", localPlayerId));
    player->handleLogout();
    return true;
}

bool StaticExternalMediaPlayerAdapterHandler::play(const PlayParams& params) {
    auto player = getAdapterByLocalPlayerId(params.localPlayerId);
    if (!player) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("localPlayerId", params.localPlayerId));
    HandlePlayParams handlePlayParams(
        params.playContextToken,
        params.index,
        params.offset,
        params.skillToken,
        params.playbackSessionId,
        params.navigation,
        params.preload,
        params.playRequestor,
        params.playbackTarget);
    player->handlePlay(handlePlayParams);
    return true;
}

bool StaticExternalMediaPlayerAdapterHandler::playControl(
    const std::string& localPlayerId,
    acsdkExternalMediaPlayerInterfaces::RequestType requestType,
    const std::string& playbackTarget) {
    auto player = getAdapterByLocalPlayerId(localPlayerId);
    if (!player) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("localPlayerId", localPlayerId).sensitive("playbackTarget", playbackTarget));
    player->handlePlayControl(requestType, playbackTarget);
    return true;
}

bool StaticExternalMediaPlayerAdapterHandler::seek(const std::string& localPlayerId, std::chrono::milliseconds offset) {
    auto player = getAdapterByLocalPlayerId(localPlayerId);
    if (!player) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("localPlayerId", localPlayerId));
    player->handleSeek(offset);
    return true;
}

bool StaticExternalMediaPlayerAdapterHandler::adjustSeek(
    const std::string& localPlayerId,
    std::chrono::milliseconds deltaOffset) {
    auto player = getAdapterByLocalPlayerId(localPlayerId);
    if (!player) {
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("localPlayerId", localPlayerId));
    player->handleAdjustSeek(deltaOffset);
    return true;
}

acsdkExternalMediaPlayerInterfaces::AdapterState StaticExternalMediaPlayerAdapterHandler::getAdapterState(
    const std::string& localPlayerId) {
    std::unique_lock<std::mutex> lock{m_adaptersMutex};
    const auto it = m_adapters.find(localPlayerId);
    if (it == m_adapters.end()) {
        ACSDK_ERROR(LX("getAdapterStateFailed").d("reason", "localPlayerId does not exist"));
        return {};
    }

    return it->second->getState();
}

std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> StaticExternalMediaPlayerAdapterHandler::
    getAdapterStates() {
    std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> adapterState;
    std::unique_lock<std::mutex> lock{m_adaptersMutex};
    for (const auto& adapter : m_adapters) {
        adapterState.push_back(adapter.second->getState());
    }
    return adapterState;
}

std::chrono::milliseconds StaticExternalMediaPlayerAdapterHandler::getOffset(const std::string& localPlayerId) {
    auto player = getAdapterByLocalPlayerId(localPlayerId);
    if (!player) {
        return std::chrono::milliseconds::zero();
    }

    ACSDK_DEBUG5(LX(__func__).d("playerId", localPlayerId));
    return player->getOffset();
}

void StaticExternalMediaPlayerAdapterHandler::setExternalMediaPlayer(
    const std::shared_ptr<alexaClientSDK::acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>) {
}

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
