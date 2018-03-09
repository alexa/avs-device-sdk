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

/// @file ExternalMediaPlayer.cpp
#include "ExternalMediaPlayer/ExternalMediaPlayer.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/ExternalMediaPlayer/AdapterUtils.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace externalMediaPlayer {

using namespace avsCommon::avs;
using namespace avsCommon::avs::externalMediaPlayer;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::externalMediaPlayer;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("ExternalMediaPlayer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// The namespaces used in the context.
static const std::string EXTERNALMEDIAPLAYER_STATE_NAMESPACE = "ExternalMediaPlayer";
static const std::string PLAYBACKSTATEREPORTER_STATE_NAMESPACE = "Alexa.PlaybackStateReporter";

// The names used in the context.
static const std::string EXTERNALMEDIAPLAYER_NAME = "ExternalMediaPlayerState";
static const std::string PLAYBACKSTATEREPORTER_NAME = "playbackState";

// The namespace for this capability agent.
static const std::string EXTERNALMEDIAPLAYER_NAMESPACE = "ExternalMediaPlayer";
static const std::string PLAYBACKCONTROLLER_NAMESPACE = "Alexa.PlaybackController";
static const std::string PLAYLISTCONTROLLER_NAMESPACE = "Alexa.PlaylistController";
static const std::string SEEKCONTROLLER_NAMESPACE = "Alexa.SeekController";
static const std::string FAVORITESCONTROLLER_NAMESPACE = "Alexa.FavoritesController";

// The @c External media player play directive signature.
static const NamespaceAndName PLAY_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE, "Play"};
static const NamespaceAndName LOGIN_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE, "Login"};
static const NamespaceAndName LOGOUT_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE, "Logout"};

// The @c Transport control directive signatures.
static const NamespaceAndName RESUME_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Play"};
static const NamespaceAndName PAUSE_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Pause"};
static const NamespaceAndName NEXT_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Next"};
static const NamespaceAndName PREVIOUS_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Previous"};
static const NamespaceAndName STARTOVER_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "StartOver"};
static const NamespaceAndName REWIND_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Rewind"};
static const NamespaceAndName FASTFORWARD_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "FastForward"};

// The @c PlayList control directive signature.
static const NamespaceAndName ENABLEREPEAT_DIRECTIVE{PLAYLISTCONTROLLER_NAMESPACE, "EnableRepeat"};
static const NamespaceAndName DISABLEREPEAT_DIRECTIVE{PLAYLISTCONTROLLER_NAMESPACE, "DisableRepeat"};
static const NamespaceAndName ENABLESHUFFLE_DIRECTIVE{PLAYLISTCONTROLLER_NAMESPACE, "EnableShuffle"};
static const NamespaceAndName DISABLESHUFFLE_DIRECTIVE{PLAYLISTCONTROLLER_NAMESPACE, "DisableShuffle"};

// The @c Seek control directive signature.
static const NamespaceAndName SEEK_DIRECTIVE{SEEKCONTROLLER_NAMESPACE, "SetSeekPosition"};
static const NamespaceAndName ADJUSTSEEK_DIRECTIVE{SEEKCONTROLLER_NAMESPACE, "AdjustSeekPosition"};

// The @c favorites control directive signature.
static const NamespaceAndName FAVORITE_DIRECTIVE{FAVORITESCONTROLLER_NAMESPACE, "Favorite"};
static const NamespaceAndName UNFAVORITE_DIRECTIVE{FAVORITESCONTROLLER_NAMESPACE, "Unfavorite"};

// The @c ExternalMediaPlayer context state signatures.
static const NamespaceAndName SESSION_STATE{EXTERNALMEDIAPLAYER_STATE_NAMESPACE, EXTERNALMEDIAPLAYER_NAME};
static const NamespaceAndName PLAYBACK_STATE{PLAYBACKSTATEREPORTER_STATE_NAMESPACE, PLAYBACKSTATEREPORTER_NAME};

/// The const char for the players key field in the context.
static const char PLAYERS[] = "players";

/// The const char for the playerInFocus key field in the context.
static const char PLAYER_IN_FOCUS[] = "playerInFocus";

/// The max relative time in the past that we can  seek to in milliseconds(-12hours in ms).
static const int64_t MAX_PAST_OFFSET = -86400000;

/// The max relative time in the past that we can  seek to in milliseconds(+12 hours in ms).
static const int64_t MAX_FUTURE_OFFSET = 86400000;

/// The @c m_directiveToHandlerMap Map of the directives to their handlers.
std::unordered_map<NamespaceAndName, std::pair<RequestType, ExternalMediaPlayer::DirectiveHandler>>
    ExternalMediaPlayer::m_directiveToHandlerMap = {
        {LOGIN_DIRECTIVE, std::make_pair(RequestType::LOGIN, &ExternalMediaPlayer::handleLogin)},
        {LOGOUT_DIRECTIVE, std::make_pair(RequestType::LOGOUT, &ExternalMediaPlayer::handleLogout)},
        {PLAY_DIRECTIVE, std::make_pair(RequestType::PLAY, &ExternalMediaPlayer::handlePlay)},
        {PAUSE_DIRECTIVE, std::make_pair(RequestType::PAUSE, &ExternalMediaPlayer::handlePlayControl)},
        {RESUME_DIRECTIVE, std::make_pair(RequestType::RESUME, &ExternalMediaPlayer::handlePlayControl)},
        {NEXT_DIRECTIVE, std::make_pair(RequestType::NEXT, &ExternalMediaPlayer::handlePlayControl)},
        {PREVIOUS_DIRECTIVE, std::make_pair(RequestType::PREVIOUS, &ExternalMediaPlayer::handlePlayControl)},
        {STARTOVER_DIRECTIVE, std::make_pair(RequestType::START_OVER, &ExternalMediaPlayer::handlePlayControl)},
        {FASTFORWARD_DIRECTIVE, std::make_pair(RequestType::FAST_FORWARD, &ExternalMediaPlayer::handlePlayControl)},
        {REWIND_DIRECTIVE, std::make_pair(RequestType::REWIND, &ExternalMediaPlayer::handlePlayControl)},
        {ENABLEREPEAT_DIRECTIVE, std::make_pair(RequestType::ENABLE_REPEAT, &ExternalMediaPlayer::handlePlayControl)},
        {DISABLEREPEAT_DIRECTIVE, std::make_pair(RequestType::DISABLE_REPEAT, &ExternalMediaPlayer::handlePlayControl)},
        {ENABLESHUFFLE_DIRECTIVE, std::make_pair(RequestType::ENABLE_SHUFFLE, &ExternalMediaPlayer::handlePlayControl)},
        {DISABLESHUFFLE_DIRECTIVE,
         std::make_pair(RequestType::DISABLE_SHUFFLE, &ExternalMediaPlayer::handlePlayControl)},
        {FAVORITE_DIRECTIVE, std::make_pair(RequestType::FAVORITE, &ExternalMediaPlayer::handlePlayControl)},
        {UNFAVORITE_DIRECTIVE, std::make_pair(RequestType::UNFAVORITE, &ExternalMediaPlayer::handlePlayControl)},
        {SEEK_DIRECTIVE, std::make_pair(RequestType::SEEK, &ExternalMediaPlayer::handleSeek)},
        {ADJUSTSEEK_DIRECTIVE, std::make_pair(RequestType::ADJUST_SEEK, &ExternalMediaPlayer::handleAdjustSeek)}};

static DirectiveHandlerConfiguration g_configuration = {{PLAY_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {LOGIN_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {LOGOUT_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {RESUME_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {PAUSE_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {NEXT_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {PREVIOUS_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {STARTOVER_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {REWIND_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {FASTFORWARD_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {ENABLEREPEAT_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {DISABLEREPEAT_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {ENABLESHUFFLE_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {DISABLESHUFFLE_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {SEEK_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {ADJUSTSEEK_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {FAVORITE_DIRECTIVE, BlockingPolicy::NON_BLOCKING},
                                                        {UNFAVORITE_DIRECTIVE, BlockingPolicy::NON_BLOCKING}};

static std::unordered_map<avsCommon::avs::PlaybackButton, RequestType> g_buttonToRequestType = {
    {avsCommon::avs::PlaybackButton::PLAY, RequestType::PAUSE_RESUME_TOGGLE},
    {avsCommon::avs::PlaybackButton::PAUSE, RequestType::PAUSE_RESUME_TOGGLE},
    {avsCommon::avs::PlaybackButton::NEXT, RequestType::NEXT},
    {avsCommon::avs::PlaybackButton::PREVIOUS, RequestType::PREVIOUS}};

std::shared_ptr<ExternalMediaPlayer> ExternalMediaPlayer::create(
    const AdapterMediaPlayerMap& mediaPlayers,
    const AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter) {
    if (nullptr == messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }
    if (nullptr == focusManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullFocusManager"));
        return nullptr;
    }
    if (nullptr == contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    if (nullptr == exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }
    if (nullptr == playbackRouter) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullPlaybackRouter"));
        return nullptr;
    }

    auto externalMediaPlayer = std::shared_ptr<ExternalMediaPlayer>(
        new ExternalMediaPlayer(speakerManager, contextManager, exceptionSender, playbackRouter));

    contextManager->setStateProvider(SESSION_STATE, externalMediaPlayer);
    contextManager->setStateProvider(PLAYBACK_STATE, externalMediaPlayer);

    externalMediaPlayer->createAdapters(mediaPlayers, adapterCreationMap, messageSender, focusManager, contextManager);

    return externalMediaPlayer;
}

ExternalMediaPlayer::ExternalMediaPlayer(
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter) :
        CapabilityAgent{EXTERNALMEDIAPLAYER_NAMESPACE, exceptionSender},
        RequiresShutdown{"ExternalMediaPlayer"},
        m_speakerManager{speakerManager},
        m_contextManager{contextManager},
        m_playbackRouter{playbackRouter} {
    m_speakerSettings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
    m_speakerSettings.mute = false;
}

void ExternalMediaPlayer::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    unsigned int stateRequestToken) {
    m_executor.submit([this, stateProviderName, stateRequestToken] {
        executeProvideState(stateProviderName, true, stateRequestToken);
    });
}

void ExternalMediaPlayer::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void ExternalMediaPlayer::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
}

bool ExternalMediaPlayer::parseDirectivePayload(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document) {
    rapidjson::ParseResult result = document->Parse(info->directive->getPayload());

    if (result) {
        return true;
    }

    ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                    .d("reason", rapidjson::GetParseError_En(result.Code()))
                    .d("offset", result.Offset())
                    .d("messageId", info->directive->getMessageId()));

    sendExceptionEncounteredAndReportFailed(
        info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);

    return false;
}

void ExternalMediaPlayer::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    NamespaceAndName directiveNamespaceAndName(info->directive->getNamespace(), info->directive->getName());
    auto handlerIt = m_directiveToHandlerMap.find(directiveNamespaceAndName);
    if (handlerIt == m_directiveToHandlerMap.end()) {
        ACSDK_ERROR(LX("handleDirectivesFailed")
                        .d("reason", "noDirectiveHandlerForDirective")
                        .d("nameSpace", info->directive->getNamespace())
                        .d("name", info->directive->getName()));
        return;
    }

    ACSDK_DEBUG9(LX("handleDirectivesPayload").sensitive("Payload", info->directive->getPayload()));

    auto handler = (handlerIt->second.second);
    (this->*handler)(info, handlerIt->second.first);
}

std::shared_ptr<ExternalMediaAdapterInterface> ExternalMediaPlayer::preprocessDirective(
    std::shared_ptr<DirectiveInfo> info,
    rapidjson::Document* document) {
    ACSDK_DEBUG9(LX("preprocessDirective"));

    if (!parseDirectivePayload(info, document)) {
        sendExceptionEncounteredAndReportFailed(info, "Failed to parse directive.");
        return nullptr;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(*document, "playerId", &playerId)) {
        ACSDK_ERROR(LX("preprocessDirectiveFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
        return nullptr;
    }

    auto adapterIt = m_adapters.find(playerId);
    if (adapterIt == m_adapters.end()) {
        ACSDK_ERROR(LX("preprocessDirectiveFailed").d("reason", "noAdapterForPlayerId").d("playerId", playerId));
        sendExceptionEncounteredAndReportFailed(info, "Unrecogonized PlayerId.");
        return nullptr;
    }

    auto adapter = adapterIt->second;
    if (!adapter) {
        ACSDK_ERROR(LX("preprocessDirectiveFailed").d("reason", "nullAdapter").d("playerId", playerId));
        sendExceptionEncounteredAndReportFailed(info, "nullAdapter.");
        return nullptr;
    }

    return adapter;
}

void ExternalMediaPlayer::handleLogin(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    std::string accessToken;
    if (!jsonUtils::retrieveValue(payload, "accessToken", &accessToken)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullAccessToken"));
        sendExceptionEncounteredAndReportFailed(info, "missing accessToken in Login directive");
        return;
    }

    std::string userName;
    if (!jsonUtils::retrieveValue(payload, "username", &userName)) {
        userName = "";
    }

    int64_t refreshInterval;
    if (!jsonUtils::retrieveValue(payload, "tokenRefreshIntervalInMilliseconds", &refreshInterval)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullRefreshInterval"));
        sendExceptionEncounteredAndReportFailed(info, "missing tokenRefreshIntervalInMilliseconds in Login directive");
        return;
    }

    bool forceLogin;
    if (!jsonUtils::retrieveValue(payload, "forceLogin", &forceLogin)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullForceLogin"));
        sendExceptionEncounteredAndReportFailed(info, "missing forceLogin in Login directive");
        return;
    }

    setHandlingCompleted(info);
    adapter->handleLogin(accessToken, userName, forceLogin, std::chrono::milliseconds(refreshInterval));
}

void ExternalMediaPlayer::handleLogout(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    setHandlingCompleted(info);
    adapter->handleLogout();
}

void ExternalMediaPlayer::handlePlay(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    std::string playbackContextToken;
    if (!jsonUtils::retrieveValue(payload, "playbackContextToken", &playbackContextToken)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullPlaybackContextToken"));
        sendExceptionEncounteredAndReportFailed(info, "missing playbackContextToken in Play directive");
        return;
    }

    int64_t offset;
    if (!jsonUtils::retrieveValue(payload, "offsetInMilliseconds", &offset)) {
        offset = 0;
    }

    int64_t index;
    if (!jsonUtils::retrieveValue(payload, "index", &index)) {
        index = 0;
    }

    setHandlingCompleted(info);
    adapter->handlePlay(playbackContextToken, index, std::chrono::milliseconds(offset));
}

void ExternalMediaPlayer::handleSeek(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    int64_t position;
    if (!jsonUtils::retrieveValue(payload, "positionMilliseconds", &position)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullPosition"));
        sendExceptionEncounteredAndReportFailed(info, "missing positionMilliseconds in SetSeekPosition directive");
        return;
    }

    setHandlingCompleted(info);
    adapter->handleSeek(std::chrono::milliseconds(position));
}

void ExternalMediaPlayer::handleAdjustSeek(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    int64_t deltaPosition;
    if (!jsonUtils::retrieveValue(payload, "deltaPositionMilliseconds", &deltaPosition)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDeltaPositionMilliseconds"));
        sendExceptionEncounteredAndReportFailed(
            info, "missing deltaPositionMilliseconds in AdjustSeekPosition directive");
        return;
    }

    if (deltaPosition < MAX_PAST_OFFSET || deltaPosition > MAX_FUTURE_OFFSET) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "deltaPositionMillisecondsOutOfRange."));
        sendExceptionEncounteredAndReportFailed(
            info, "missing deltaPositionMilliseconds in AdjustSeekPosition directive");
        return;
    }

    setHandlingCompleted(info);
    adapter->handleAdjustSeek(std::chrono::milliseconds(deltaPosition));
}

void ExternalMediaPlayer::handlePlayControl(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    setHandlingCompleted(info);
    adapter->handlePlayControl(request);
}

void ExternalMediaPlayer::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

void ExternalMediaPlayer::onDeregistered() {
}

DirectiveHandlerConfiguration ExternalMediaPlayer::getConfiguration() const {
    return g_configuration;
}

void ExternalMediaPlayer::setPlayerInFocus(const std::string& playerInFocus) {
    ACSDK_DEBUG9(LX("setPlayerInFocus").d("playerInFocus", playerInFocus));
    m_playerInFocus = playerInFocus;
    m_playbackRouter->setHandler(shared_from_this());
}

void ExternalMediaPlayer::onButtonPressed(PlaybackButton button) {
    if (!m_playerInFocus.empty()) {
        auto adapterIt = m_adapters.find(m_playerInFocus);

        if (adapterIt == m_adapters.end()) {
            // Should never reach here as playerInFocus is always set based on a contract with AVS.
            ACSDK_ERROR(LX("AdapterNotFound").d("player", m_playerInFocus));
            return;
        }

        auto buttonIt = g_buttonToRequestType.find(button);

        if (buttonIt == g_buttonToRequestType.end()) {
            ACSDK_ERROR(LX("ButtonToRequestTypeNotFound").d("button", PlaybackButtonToString(button)));
            return;
        }

        ((*adapterIt).second)->handlePlayControl((*buttonIt).second);
    }
}

void ExternalMediaPlayer::doShutdown() {
    m_executor.shutdown();
    // Reset the EMP from being a state provider. If not there would be calls from the adapter to provide context
    // which will try to add tasks to the executor thread.
    m_contextManager->setStateProvider(SESSION_STATE, nullptr);
    m_contextManager->setStateProvider(PLAYBACK_STATE, nullptr);

    for (auto& adapter : m_adapters) {
        if (!adapter.second) {
            continue;
        }
        adapter.second->shutdown();
    }

    m_adapters.clear();
    m_exceptionEncounteredSender.reset();
    m_contextManager.reset();
    m_playbackRouter.reset();
    m_speakerManager.reset();
}

void ExternalMediaPlayer::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void ExternalMediaPlayer::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    if (info && info->result) {
        info->result->setCompleted();
    }

    removeDirective(info);
}

void ExternalMediaPlayer::sendExceptionEncounteredAndReportFailed(
    std::shared_ptr<DirectiveInfo> info,
    const std::string& message,
    avsCommon::avs::ExceptionErrorType type) {
    if (info && info->directive) {
        m_exceptionEncounteredSender->sendExceptionEncountered(info->directive->getUnparsedDirective(), type, message);
    }

    if (info && info->result) {
        info->result->setFailed(message);
    }

    removeDirective(info);
}

void ExternalMediaPlayer::executeProvideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    bool sendToken,
    unsigned int stateRequestToken) {
    ACSDK_DEBUG(LX("executeProvideState").d("sendToken", sendToken).d("stateRequestToken", stateRequestToken));
    std::string state;

    if (stateProviderName == SESSION_STATE) {
        state = provideSessionState();
    } else if (stateProviderName == PLAYBACK_STATE) {
        state = providePlaybackState();
    } else {
        ACSDK_ERROR(LX("executeProvideState").d("reason", "unknownStateProviderName"));
        return;
    }

    SetStateResult result;
    if (sendToken) {
        result = m_contextManager->setState(stateProviderName, state, StateRefreshPolicy::ALWAYS, stateRequestToken);
    } else {
        result = m_contextManager->setState(stateProviderName, state, StateRefreshPolicy::ALWAYS);
    }

    if (result != SetStateResult::SUCCESS) {
        ACSDK_ERROR(LX("executeProvideState").d("reason", "contextManagerSetStateFailedForEMPState"));
    }
}

std::string ExternalMediaPlayer::provideSessionState() {
    rapidjson::Document state(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& stateAlloc = state.GetAllocator();

    state.AddMember(rapidjson::StringRef(PLAYER_IN_FOCUS), m_playerInFocus, stateAlloc);

    rapidjson::Value players(rapidjson::kArrayType);
    for (const auto& adapter : m_adapters) {
        if (!adapter.second) {
            continue;
        }

        rapidjson::Value playerJson = buildSessionState(adapter.second->getState().sessionState, stateAlloc);
        players.PushBack(playerJson, stateAlloc);
    }

    state.AddMember(rapidjson::StringRef(PLAYERS), players, stateAlloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("provideSessionStateFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

std::string ExternalMediaPlayer::providePlaybackState() {
    rapidjson::Document state(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& stateAlloc = state.GetAllocator();

    // Fill the default player state.
    if (!buildDefaultPlayerState(&state, stateAlloc)) {
        return "";
    }

    // Fetch actual PlaybackState from every player supported by the ExternalMediaPlayer.
    rapidjson::Value players(rapidjson::kArrayType);
    for (const auto& adapter : m_adapters) {
        if (!adapter.second) {
            continue;
        }
        rapidjson::Value playerJson = buildPlaybackState(adapter.second->getState().playbackState, stateAlloc);
        players.PushBack(playerJson, stateAlloc);
    }

    state.AddMember(PLAYERS, players, stateAlloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("providePlaybackState").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

/*
 * SpeakerInterface.
 */
bool ExternalMediaPlayer::setVolume(int8_t volume) {
    if (volume > avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX ||
        volume < avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN) {
        ACSDK_ERROR(LX("setVolumeFailed").d("reason", "invalid volume value").d("value", volume));
        return false;
    }

    m_speakerSettings.volume = volume;

    for (auto& adapter : m_adapters) {
        if (!adapter.second) {
            continue;
        }
        adapter.second->handleSetVolume(volume);
    }
    return true;
}

bool ExternalMediaPlayer::adjustVolume(int8_t volume) {
    if (volume > avsCommon::avs::speakerConstants::AVS_ADJUST_VOLUME_MAX ||
        volume < avsCommon::avs::speakerConstants::AVS_ADJUST_VOLUME_MIN) {
        ACSDK_ERROR(LX("adjustVolumeFailed").d("reason", "invalid volume value").d("value", volume));
        return false;
    }

    // TODO: Replace int variables here to the actual volume time when int8_t would be changed to it
    int newVolume = m_speakerSettings.volume + volume;
    newVolume = std::min(newVolume, (int)speakerConstants::AVS_SET_VOLUME_MAX);
    newVolume = std::max(newVolume, (int)speakerConstants::AVS_SET_VOLUME_MIN);

    m_speakerSettings.volume = newVolume;

    for (auto& adapter : m_adapters) {
        if (!adapter.second) {
            continue;
        }
        adapter.second->handleSetVolume((int8_t)newVolume);
    }
    return true;
}

bool ExternalMediaPlayer::setMute(bool mute) {
    m_speakerSettings.mute = mute;

    for (auto& adapter : m_adapters) {
        if (!adapter.second) {
            continue;
        }
        adapter.second->handleSetMute(mute);
    }
    return true;
}

bool ExternalMediaPlayer::getSpeakerSettings(avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) {
    if (!settings) {
        ACSDK_ERROR(LX("getSpeakerSettingsFailed").d("reason", "nullSettings"));
        return false;
    }
    settings->mute = m_speakerSettings.mute;
    settings->volume = m_speakerSettings.volume;
    return true;
}

avsCommon::sdkInterfaces::SpeakerInterface::Type ExternalMediaPlayer::getSpeakerType() {
    return SpeakerInterface::Type::AVS_SYNCED;
}

void ExternalMediaPlayer::createAdapters(
    const AdapterMediaPlayerMap& mediaPlayers,
    const AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager) {
    ACSDK_DEBUG0(LX("createAdapters"));
    for (auto& entry : adapterCreationMap) {
        auto mediaPlayerIt = mediaPlayers.find(entry.first);

        if (mediaPlayerIt == mediaPlayers.end()) {
            ACSDK_ERROR(LX("adapterCreationFailed").d("playerId", entry.first).d("reason", "nullMediaPlayer"));
            continue;
        }

        auto adapter = entry.second(
            (*mediaPlayerIt).second, m_speakerManager, messageSender, focusManager, contextManager, shared_from_this());
        if (adapter) {
            m_adapters[entry.first] = adapter;
        } else {
            ACSDK_ERROR(LX("adapterCreationFailed").d("playerId", entry.first));
        }
    }
}

}  // namespace externalMediaPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
