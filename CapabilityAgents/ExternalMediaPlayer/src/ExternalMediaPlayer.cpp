/**
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

/// @file ExternalMediaPlayer.cpp
#include <utility>
#include <vector>

#include "ExternalMediaPlayer/ExternalMediaPlayer.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/ExternalMediaPlayer/AdapterUtils.h>
#include <AVSCommon/AVS/ExternalMediaPlayer/ExternalMediaAdapterConstants.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include <AVSCommon/Utils/String/StringUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace externalMediaPlayer {

using namespace avsCommon::avs;
using namespace avsCommon::avs::externalMediaPlayer;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::externalMediaPlayer;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils;
using namespace avsCommon::utils::metrics;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace avsCommon::utils::metrics;

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

// Capability constants
/// The AlexaInterface constant type.
static const std::string ALEXA_INTERFACE_TYPE = "AlexaInterface";

/// ExternalMediaPlayer capability constants
/// ExternalMediaPlayer interface type
static const std::string EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_TYPE = ALEXA_INTERFACE_TYPE;
/// ExternalMediaPlayer interface name
static const std::string EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_NAME = "ExternalMediaPlayer";
/// ExternalMediaPlayer interface version
static const std::string EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_VERSION = "1.2";

/// Alexa.PlaybackStateReporter name.
static const std::string PLAYBACKSTATEREPORTER_CAPABILITY_INTERFACE_NAME = PLAYBACKSTATEREPORTER_STATE_NAMESPACE;
/// Alexa.PlaybackStateReporter version.
static const std::string PLAYBACKSTATEREPORTER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Alexa.PlaybackController name.
static const std::string PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_NAME = PLAYBACKCONTROLLER_NAMESPACE;
/// Alexa.PlaybackController version.
static const std::string PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Alexa.PlaylistController name.
static const std::string PLAYLISTCONTROLLER_CAPABILITY_INTERFACE_NAME = PLAYLISTCONTROLLER_NAMESPACE;
/// Alexa.PlaylistController version.
static const std::string PLAYLISTCONTROLLER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Alexa.SeekController name.
static const std::string SEEKCONTROLLER_CAPABILITY_INTERFACE_NAME = SEEKCONTROLLER_NAMESPACE;
/// Alexa.SeekController version.
static const std::string SEEKCONTROLLER_CAPABILITY_INTERFACE_VERSION = "1.0";

/// Alexa.FavoritesController name.
static const std::string FAVORITESCONTROLLER_CAPABILITY_INTERFACE_NAME = FAVORITESCONTROLLER_NAMESPACE;
/// Alexa.FavoritesController version.
static const std::string FAVORITESCONTROLLER_CAPABILITY_INTERFACE_VERSION = "1.0";

// The @c External media player play directive signature.
static const NamespaceAndName PLAY_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE, "Play"};
static const NamespaceAndName LOGIN_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE, "Login"};
static const NamespaceAndName LOGOUT_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE, "Logout"};
static const NamespaceAndName AUTHORIZEDISCOVEREDPLAYERS_DIRECTIVE{EXTERNALMEDIAPLAYER_NAMESPACE,
                                                                   "AuthorizeDiscoveredPlayers"};

// The @c Transport control directive signatures.
static const NamespaceAndName RESUME_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Play"};
static const NamespaceAndName PAUSE_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Pause"};
static const NamespaceAndName STOP_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Stop"};
static const NamespaceAndName NEXT_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Next"};
static const NamespaceAndName PREVIOUS_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Previous"};
static const NamespaceAndName STARTOVER_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "StartOver"};
static const NamespaceAndName REWIND_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "Rewind"};
static const NamespaceAndName FASTFORWARD_DIRECTIVE{PLAYBACKCONTROLLER_NAMESPACE, "FastForward"};

// The @c PlayList control directive signature.
static const NamespaceAndName ENABLEREPEATONE_DIRECTIVE{PLAYLISTCONTROLLER_NAMESPACE, "EnableRepeatOne"};
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

/// The root key for configuration values for the ExternalMediaPlayer.
static const std::string EMP_CONFIG_KEY("externalMediaPlayer");

/// The agent key.
static const char AGENT_KEY[] = "agent";

/// Key for the @c agent id string unter the @c EMP_CONFIG_KEY configuration node
static const std::string EMP_AGENT_KEY("agentString");

/// The authorized key.
static const char AUTHORIZED[] = "authorized";

/// The deauthorized key.
static const char DEAUTHORIZED[] = "deauthorized";

/// The localPlayerId key.
static const char LOCAL_PLAYER_ID[] = "localPlayerId";

/// The metadata key.
static const char METADATA[] = "metadata";

/// The spiVersion key.
static const char SPI_VERSION_KEY[] = "spiVersion";

/// The validationMethod key.
static const char VALIDATION_METHOD[] = "validationMethod";

/// The validationData key.
static const char VALIDATION_DATA[] = "validationData";

/// The ReportDiscoveredPlayers Event key.
static const char REPORT_DISCOVERED_PLAYERS[] = "ReportDiscoveredPlayers";

/// The AuthorizationComplete Event key.
static const char AUTHORIZATION_COMPLETE[] = "AuthorizationComplete";

/// Metric name prefix for AudioPlayer metric source
static const std::string AUDIO_PLAYER_METRIC_PREFIX = "AUDIO_PLAYER-";

/// The directive received metric is used in place of 'first bytes' for UPL because pre-buffering may distort the metric
static const std::string PLAY_DIRECTIVE_RECEIVED = "PLAY_DIRECTIVE_RECEIVED";

/// The directive received metric is used in place of 'first bytes' for Stop UPL because there are no bytes involved
static const std::string STOP_DIRECTIVE_RECEIVED = "STOP_DIRECTIVE_RECEIVED";

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param metricName The name of the Metric event.
 * @param dataPoint The @c DataPoint of this Metric event (e.g. duration).
 * @param msgId The message id string from the retrieved from the directive.
 * @param trackId The track id string from the retrieved from the directive.
 * @param playerId The player id string from the retrieved from the directive.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string metricName,
    const DataPoint& dataPoint,
    const std::string& msgId,
    const std::string& trackId,
    const std::string& playerId) {
    auto metricBuilder =
        MetricEventBuilder{}.setActivityName(AUDIO_PLAYER_METRIC_PREFIX + metricName).addDataPoint(dataPoint);
    if (msgId.size() > 0) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("DIRECTIVE_MESSAGE_ID").setValue(msgId).build());
    }
    if (trackId.size() > 0) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("TRACK_ID").setValue(trackId).build());
    }
    if (playerId.size() > 0) {
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("PLAYER_ID").setValue(playerId).build());
    }

    auto metricEvent = metricBuilder.build();
    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric."));
        return;
    }

    recordMetric(metricRecorder, metricEvent);
}
/**
 * Creates the ExternalMediaPlayer capability configuration.
 *
 * @return The ExternalMediaPlayer capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getExternalMediaPlayerCapabilityConfiguration();

/// The @c m_directiveToHandlerMap Map of the directives to their handlers.
std::unordered_map<NamespaceAndName, std::pair<RequestType, ExternalMediaPlayer::DirectiveHandler>>
    ExternalMediaPlayer::m_directiveToHandlerMap = {
        {AUTHORIZEDISCOVEREDPLAYERS_DIRECTIVE,
         std::make_pair(RequestType::NONE, &ExternalMediaPlayer::handleAuthorizeDiscoveredPlayers)},
        {LOGIN_DIRECTIVE, std::make_pair(RequestType::LOGIN, &ExternalMediaPlayer::handleLogin)},
        {LOGOUT_DIRECTIVE, std::make_pair(RequestType::LOGOUT, &ExternalMediaPlayer::handleLogout)},
        {PLAY_DIRECTIVE, std::make_pair(RequestType::PLAY, &ExternalMediaPlayer::handlePlay)},
        {PAUSE_DIRECTIVE, std::make_pair(RequestType::PAUSE, &ExternalMediaPlayer::handlePlayControl)},
        {STOP_DIRECTIVE, std::make_pair(RequestType::STOP, &ExternalMediaPlayer::handlePlayControl)},
        {RESUME_DIRECTIVE, std::make_pair(RequestType::RESUME, &ExternalMediaPlayer::handlePlayControl)},
        {NEXT_DIRECTIVE, std::make_pair(RequestType::NEXT, &ExternalMediaPlayer::handlePlayControl)},
        {PREVIOUS_DIRECTIVE, std::make_pair(RequestType::PREVIOUS, &ExternalMediaPlayer::handlePlayControl)},
        {STARTOVER_DIRECTIVE, std::make_pair(RequestType::START_OVER, &ExternalMediaPlayer::handlePlayControl)},
        {FASTFORWARD_DIRECTIVE, std::make_pair(RequestType::FAST_FORWARD, &ExternalMediaPlayer::handlePlayControl)},
        {REWIND_DIRECTIVE, std::make_pair(RequestType::REWIND, &ExternalMediaPlayer::handlePlayControl)},
        {ENABLEREPEATONE_DIRECTIVE,
         std::make_pair(RequestType::ENABLE_REPEAT_ONE, &ExternalMediaPlayer::handlePlayControl)},
        {ENABLEREPEAT_DIRECTIVE, std::make_pair(RequestType::ENABLE_REPEAT, &ExternalMediaPlayer::handlePlayControl)},
        {DISABLEREPEAT_DIRECTIVE, std::make_pair(RequestType::DISABLE_REPEAT, &ExternalMediaPlayer::handlePlayControl)},
        {ENABLESHUFFLE_DIRECTIVE, std::make_pair(RequestType::ENABLE_SHUFFLE, &ExternalMediaPlayer::handlePlayControl)},
        {DISABLESHUFFLE_DIRECTIVE,
         std::make_pair(RequestType::DISABLE_SHUFFLE, &ExternalMediaPlayer::handlePlayControl)},
        {FAVORITE_DIRECTIVE, std::make_pair(RequestType::FAVORITE, &ExternalMediaPlayer::handlePlayControl)},
        {UNFAVORITE_DIRECTIVE, std::make_pair(RequestType::UNFAVORITE, &ExternalMediaPlayer::handlePlayControl)},
        {SEEK_DIRECTIVE, std::make_pair(RequestType::SEEK, &ExternalMediaPlayer::handleSeek)},
        {ADJUSTSEEK_DIRECTIVE, std::make_pair(RequestType::ADJUST_SEEK, &ExternalMediaPlayer::handleAdjustSeek)}};

// TODO: ARC-227 Verify default values
auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

static DirectiveHandlerConfiguration g_configuration = {{AUTHORIZEDISCOVEREDPLAYERS_DIRECTIVE, audioNonBlockingPolicy},
                                                        {PLAY_DIRECTIVE, audioNonBlockingPolicy},
                                                        {LOGIN_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {LOGOUT_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {RESUME_DIRECTIVE, audioNonBlockingPolicy},
                                                        {PAUSE_DIRECTIVE, audioNonBlockingPolicy},
                                                        {STOP_DIRECTIVE, audioNonBlockingPolicy},
                                                        {NEXT_DIRECTIVE, audioNonBlockingPolicy},
                                                        {PREVIOUS_DIRECTIVE, audioNonBlockingPolicy},
                                                        {STARTOVER_DIRECTIVE, audioNonBlockingPolicy},
                                                        {REWIND_DIRECTIVE, audioNonBlockingPolicy},
                                                        {FASTFORWARD_DIRECTIVE, audioNonBlockingPolicy},
                                                        {ENABLEREPEATONE_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {ENABLEREPEAT_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {DISABLEREPEAT_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {ENABLESHUFFLE_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {DISABLESHUFFLE_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {SEEK_DIRECTIVE, audioNonBlockingPolicy},
                                                        {ADJUSTSEEK_DIRECTIVE, audioNonBlockingPolicy},
                                                        {FAVORITE_DIRECTIVE, neitherNonBlockingPolicy},
                                                        {UNFAVORITE_DIRECTIVE, neitherNonBlockingPolicy}};

static std::unordered_map<PlaybackButton, RequestType> g_buttonToRequestType = {
    {PlaybackButton::PLAY, RequestType::PAUSE_RESUME_TOGGLE},
    {PlaybackButton::PAUSE, RequestType::PAUSE_RESUME_TOGGLE},
    {PlaybackButton::NEXT, RequestType::NEXT},
    {PlaybackButton::PREVIOUS, RequestType::PREVIOUS}};

static std::unordered_map<PlaybackToggle, std::pair<RequestType, RequestType>> g_toggleToRequestType = {
    {PlaybackToggle::SHUFFLE, std::make_pair(RequestType::ENABLE_SHUFFLE, RequestType::DISABLE_SHUFFLE)},
    {PlaybackToggle::LOOP, std::make_pair(RequestType::ENABLE_REPEAT, RequestType::DISABLE_REPEAT)},
    {PlaybackToggle::REPEAT, std::make_pair(RequestType::ENABLE_REPEAT_ONE, RequestType::DISABLE_REPEAT_ONE)},
    {PlaybackToggle::THUMBS_UP, std::make_pair(RequestType::FAVORITE, RequestType::DESELECT_FAVORITE)},
    {PlaybackToggle::THUMBS_DOWN, std::make_pair(RequestType::UNFAVORITE, RequestType::DESELECT_UNFAVORITE)}};

/**
 * Generate a @c CapabilityConfiguration object.
 *
 * @param type The Capability interface type.
 * @param interfaceName The Capability interface name.
 * @param version The Capability interface verison.
 */
static std::shared_ptr<CapabilityConfiguration> generateCapabilityConfiguration(
    const std::string& type,
    const std::string& interfaceName,
    const std::string& version) {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, type});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, interfaceName});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, version});

    return std::make_shared<CapabilityConfiguration>(configMap);
}

std::shared_ptr<ExternalMediaPlayer> ExternalMediaPlayer::create(
    const AdapterMediaPlayerMap& mediaPlayers,
    const AdapterSpeakerMap& speakers,
    const AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) {
    if (nullptr == speakerManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSpeakerManager"));
        return nullptr;
    }

    if (nullptr == messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }
    if (nullptr == certifiedMessageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCertifiedMessageSender"));
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

    auto externalMediaPlayer = std::shared_ptr<ExternalMediaPlayer>(new ExternalMediaPlayer(
        std::move(speakerManager),
        std::move(messageSender),
        std::move(certifiedMessageSender),
        std::move(contextManager),
        std::move(exceptionSender),
        std::move(playbackRouter),
        std::move(metricRecorder)));

    if (!externalMediaPlayer->init(mediaPlayers, speakers, adapterCreationMap, std::move(focusManager))) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initFailed"));
        return nullptr;
    }

    return externalMediaPlayer;
}

ExternalMediaPlayer::ExternalMediaPlayer(
    std::shared_ptr<SpeakerManagerInterface> speakerManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) :
        CapabilityAgent{EXTERNALMEDIAPLAYER_NAMESPACE, std::move(exceptionSender)},
        RequiresShutdown{"ExternalMediaPlayer"},
        m_metricRecorder{std::move(metricRecorder)},
        m_speakerManager{std::move(speakerManager)},
        m_messageSender{std::move(messageSender)},
        m_certifiedMessageSender{std::move(certifiedMessageSender)},
        m_contextManager{std::move(contextManager)},
        m_playbackRouter{std::move(playbackRouter)} {
    // Register all supported capabilities.
    m_capabilityConfigurations.insert(getExternalMediaPlayerCapabilityConfiguration());

    // Register all supported capabilities.
    m_capabilityConfigurations.insert(generateCapabilityConfiguration(
        ALEXA_INTERFACE_TYPE,
        PLAYBACKSTATEREPORTER_CAPABILITY_INTERFACE_NAME,
        PLAYBACKSTATEREPORTER_CAPABILITY_INTERFACE_VERSION));

    m_capabilityConfigurations.insert(generateCapabilityConfiguration(
        ALEXA_INTERFACE_TYPE,
        PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_NAME,
        PLAYBACKCONTROLLER_CAPABILITY_INTERFACE_VERSION));

    m_capabilityConfigurations.insert(generateCapabilityConfiguration(
        ALEXA_INTERFACE_TYPE,
        PLAYLISTCONTROLLER_CAPABILITY_INTERFACE_NAME,
        PLAYLISTCONTROLLER_CAPABILITY_INTERFACE_VERSION));

    m_capabilityConfigurations.insert(generateCapabilityConfiguration(
        ALEXA_INTERFACE_TYPE, SEEKCONTROLLER_CAPABILITY_INTERFACE_NAME, SEEKCONTROLLER_CAPABILITY_INTERFACE_VERSION));

    m_capabilityConfigurations.insert(generateCapabilityConfiguration(
        ALEXA_INTERFACE_TYPE,
        FAVORITESCONTROLLER_CAPABILITY_INTERFACE_NAME,
        FAVORITESCONTROLLER_CAPABILITY_INTERFACE_VERSION));
}

bool ExternalMediaPlayer::init(
    const AdapterMediaPlayerMap& mediaPlayers,
    const AdapterSpeakerMap& speakers,
    const AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<FocusManagerInterface> focusManager) {
    ACSDK_DEBUG5(LX(__func__));

    m_authorizedSender = AuthorizedSender::create(m_messageSender);
    if (!m_authorizedSender) {
        ACSDK_ERROR(LX("initFailed").d("reason", "createAuthorizedSenderFailed"));
        return false;
    }

    m_contextManager->setStateProvider(SESSION_STATE, shared_from_this());
    m_contextManager->setStateProvider(PLAYBACK_STATE, shared_from_this());
    auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot();
    auto empGroup = config[EMP_CONFIG_KEY];
    empGroup.getString(EMP_AGENT_KEY, &m_agentString, "");

    createAdapters(mediaPlayers, speakers, adapterCreationMap, m_authorizedSender, focusManager, m_contextManager);

    return true;
}

std::shared_ptr<CapabilityConfiguration> getExternalMediaPlayerCapabilityConfiguration() {
    return generateCapabilityConfiguration(
        EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_TYPE,
        EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_NAME,
        EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_VERSION);
}

void ExternalMediaPlayer::onContextAvailable(const std::string& jsonContext) {
    // Do not block the ContextManager thread.
    m_executor.submit([this, jsonContext] {
        ACSDK_DEBUG5(LX("onContextAvailableLambda"));

        while (!m_eventQueue.empty()) {
            std::pair<std::string, std::string> nameAndPayload = m_eventQueue.front();
            m_eventQueue.pop();
            auto event = buildJsonEventString(nameAndPayload.first, "", nameAndPayload.second, jsonContext);

            ACSDK_DEBUG5(LX("onContextAvailableLambda").d("event", event.second));
            auto request = std::make_shared<MessageRequest>(event.second);
            m_messageSender->sendMessage(request);
        }
    });
}

void ExternalMediaPlayer::onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    std::pair<std::string, std::string> nameAndPayload = m_eventQueue.front();
    m_eventQueue.pop();
    ACSDK_ERROR(LX(__func__)
                    .d("error", error)
                    .d("eventName", nameAndPayload.first)
                    .sensitive("payload", nameAndPayload.second));
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
        sendExceptionEncounteredAndReportFailed(
            info, "Unhandled directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
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
        return nullptr;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(*document, PLAYER_ID, &playerId)) {
        ACSDK_ERROR(LX("preprocessDirectiveFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
        return nullptr;
    }

    auto adapter = getAdapterByPlayerId(playerId);

    if (!adapter) {
        ACSDK_ERROR(LX("preprocessDirectiveFailed").d("reason", "nullAdapter").d(PLAYER_ID, playerId));
        sendExceptionEncounteredAndReportFailed(info, "nullAdapter.");
        return nullptr;
    }

    return adapter;
}

void ExternalMediaPlayer::handleAuthorizeDiscoveredPlayers(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    ACSDK_DEBUG5(LX(__func__));

    rapidjson::Document payload;

    // A map of playerId to skillToken
    std::unordered_map<std::string, std::string> authorizedForJson;
    // The new map of m_authorizedAdapters.
    std::unordered_map<std::string, std::string> newAuthorizedAdapters;
    // The keys of the newAuthorizedAdapters map.
    std::unordered_set<std::string> newAuthorizedAdaptersKeys;
    // Deauthroized localId
    std::unordered_set<std::string> deauthorizedLocal;

    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    // If a player fails to parse, make note but continue to parse the rest.
    bool parseAllSucceeded = true;

    rapidjson::Value::ConstMemberIterator playersIt;
    if (json::jsonUtils::findNode(payload, PLAYERS, &playersIt)) {
        for (rapidjson::Value::ConstValueIterator playerIt = playersIt->value.Begin();
             playerIt != playersIt->value.End();
             playerIt++) {
            bool authorized = false;
            std::string localPlayerId, playerId, defaultSkillToken;

            if (!(*playerIt).IsObject()) {
                ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed").d("reason", "unexpectedFormat"));
                parseAllSucceeded = false;
                continue;
            }

            if (!json::jsonUtils::retrieveValue(*playerIt, LOCAL_PLAYER_ID, &localPlayerId)) {
                ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed")
                                .d("reason", "missingAttribute")
                                .d("attribute", LOCAL_PLAYER_ID));
                parseAllSucceeded = false;
                continue;
            }

            if (!json::jsonUtils::retrieveValue(*playerIt, AUTHORIZED, &authorized)) {
                ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed")
                                .d("reason", "missingAttribute")
                                .d("attribute", AUTHORIZED));
                parseAllSucceeded = false;
                continue;
            }

            if (authorized) {
                rapidjson::Value::ConstMemberIterator metadataIt;

                if (!json::jsonUtils::findNode(*playerIt, METADATA, &metadataIt)) {
                    ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed")
                                    .d("reason", "missingAttribute")
                                    .d("attribute", METADATA));
                    parseAllSucceeded = false;
                    continue;
                }

                if (!json::jsonUtils::retrieveValue(metadataIt->value, PLAYER_ID, &playerId)) {
                    ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed")
                                    .d("reason", "missingAttribute")
                                    .d("attribute", PLAYER_ID));
                    parseAllSucceeded = false;
                    continue;
                }
                if (!json::jsonUtils::retrieveValue(metadataIt->value, SKILL_TOKEN, &defaultSkillToken)) {
                    ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed")
                                    .d("reason", "missingAttribute")
                                    .d("attribute", SKILL_TOKEN));
                    parseAllSucceeded = false;
                    continue;
                }
            }

            ACSDK_DEBUG(LX(__func__)
                            .d("localPlayerId", localPlayerId)
                            .d("authorized", authorized)
                            .d("playerId", playerId)
                            .d("defaultSkillToken", defaultSkillToken));

            auto it = m_adapters.find(localPlayerId);
            if (m_adapters.end() != it) {
                m_executor.submit([it, localPlayerId, authorized, playerId, defaultSkillToken]() {
                    it->second->handleAuthorized(authorized, playerId, defaultSkillToken);
                });

                if (authorized) {
                    if (newAuthorizedAdapters.count(playerId) > 0) {
                        ACSDK_WARN(LX("duplicatePlayerIdFound")
                                       .d("playerId", playerId)
                                       .d("priorSkillToken", authorizedForJson[playerId])
                                       .d("newSkillToken", defaultSkillToken)
                                       .m("Overwriting prior entry"));
                    }

                    authorizedForJson[playerId] = defaultSkillToken;
                    newAuthorizedAdapters[playerId] = localPlayerId;
                    newAuthorizedAdaptersKeys.insert(playerId);
                }

            } else {
                ACSDK_ERROR(LX("handleAuthorizeDiscoveredPlayersFailed").d("reason", "adapterNotFound"));
                parseAllSucceeded = false;
            }
        }
    }

    // One or more players failed to be parsed.
    if (!parseAllSucceeded) {
        sendExceptionEncounteredAndReportFailed(
            info, "One or more player was not successfuly parsed", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    } else {
        setHandlingCompleted(info);
    }

    {
        std::lock_guard<std::mutex> lock(m_authorizedMutex);

        for (const auto& idToLocalId : m_authorizedAdapters) {
            if (newAuthorizedAdapters.count(idToLocalId.first) == 0) {
                deauthorizedLocal.insert(idToLocalId.second);

                const auto& adapter = getAdapterByLocalPlayerId(idToLocalId.second);
                if (adapter) {
                    adapter->handleAuthorized(false, "", "");
                }
            }
        }

        m_authorizedAdapters = newAuthorizedAdapters;
    }

    // Update the sender.
    m_authorizedSender->updateAuthorizedPlayers(newAuthorizedAdaptersKeys);

    m_executor.submit([this, authorizedForJson, deauthorizedLocal]() {
        sendAuthorizationCompleteEvent(authorizedForJson, deauthorizedLocal);
    });
}

std::map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaAdapterInterface>>
ExternalMediaPlayer::getAdaptersMap() {
    return m_adapters;
}

std::shared_ptr<ExternalMediaAdapterInterface> ExternalMediaPlayer::getAdapterByPlayerId(const std::string& playerId) {
    ACSDK_DEBUG5(LX(__func__));

    std::string localPlayerId;
    {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        auto playerIdToLocalPlayerId = m_authorizedAdapters.find(playerId);

        if (m_authorizedAdapters.end() == playerIdToLocalPlayerId) {
            ACSDK_ERROR(LX("getAdapterByPlayerIdFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, playerId));

            return nullptr;
        } else {
            localPlayerId = playerIdToLocalPlayerId->second;
        }
    }

    return getAdapterByLocalPlayerId(localPlayerId);
}

std::shared_ptr<ExternalMediaAdapterInterface> ExternalMediaPlayer::getAdapterByLocalPlayerId(
    const std::string& localPlayerId) {
    ACSDK_DEBUG5(LX(__func__));
    auto lock = std::unique_lock<std::mutex>(m_adaptersMutex);
    if (localPlayerId.empty()) {
        return nullptr;
    }
    auto localPlayerIdToAdapter = m_adapters.find(localPlayerId);

    if (m_adapters.end() == localPlayerIdToAdapter) {
        ACSDK_ERROR(
            LX("getAdapterByLocalPlayerIdFailed").d("reason", "noAdapterForLocalId").d("localPlayerId", localPlayerId));
        return nullptr;
    }

    return localPlayerIdToAdapter->second;
}

void ExternalMediaPlayer::sendAuthorizationCompleteEvent(
    const std::unordered_map<std::string, std::string>& authorized,
    const std::unordered_set<std::string>& deauthorized) {
    ACSDK_DEBUG5(LX(__func__));

    rapidjson::Document payload(rapidjson::kObjectType);
    rapidjson::Value authorizedJson(rapidjson::kArrayType);
    rapidjson::Value deauthorizedJson(rapidjson::kArrayType);

    for (const auto& playerIdToSkillToken : authorized) {
        rapidjson::Value player(rapidjson::kObjectType);

        player.AddMember(PLAYER_ID, playerIdToSkillToken.first, payload.GetAllocator());
        player.AddMember(SKILL_TOKEN, playerIdToSkillToken.second, payload.GetAllocator());
        authorizedJson.PushBack(player, payload.GetAllocator());
    }

    for (const auto& localPlayerId : deauthorized) {
        rapidjson::Value player(rapidjson::kObjectType);

        player.AddMember(LOCAL_PLAYER_ID, localPlayerId, payload.GetAllocator());
        deauthorizedJson.PushBack(player, payload.GetAllocator());
    }

    payload.AddMember(AUTHORIZED, authorizedJson, payload.GetAllocator());
    payload.AddMember(DEAUTHORIZED, deauthorizedJson, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendAuthorizationCompleteEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    // Request Context and wait.
    m_eventQueue.push(std::make_pair(AUTHORIZATION_COMPLETE, buffer.GetString()));
    m_contextManager->getContext(shared_from_this());
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
    if (!jsonUtils::retrieveValue(payload, USERNAME, &userName)) {
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

    std::string skillToken;
    if (!jsonUtils::retrieveValue(payload, "skillToken", &skillToken)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullSkillToken"));
        sendExceptionEncounteredAndReportFailed(info, "missing skillToken in Play directive");
        return;
    }

    std::string playbackSessionId;
    if (!jsonUtils::retrieveValue(payload, "playbackSessionId", &playbackSessionId)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullPlaybackSessionId"));
        sendExceptionEncounteredAndReportFailed(info, "missing playbackSessionId in Play directive");
        return;
    }

    std::string navigation;
    if (!jsonUtils::retrieveValue(payload, "navigation", &navigation)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullNavigation"));
        sendExceptionEncounteredAndReportFailed(info, "missing navigation in Play directive");
        return;
    }

    bool preload;
    if (!jsonUtils::retrieveValue(payload, "preload", &preload)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullPreload"));
        sendExceptionEncounteredAndReportFailed(info, "missing preload in Play directive");
        return;
    }

    rapidjson::Value::ConstMemberIterator playRequestorJson;
    PlayRequestor playRequestor;
    if (jsonUtils::findNode(payload, "playRequestor", &playRequestorJson)) {
        if (!jsonUtils::retrieveValue(playRequestorJson->value, "type", &playRequestor.type)) {
            ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                            .d("reason", "missingPlayRequestorType")
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(info, "missing playRequestor type in Play directive");
            return;
        }
        if (!jsonUtils::retrieveValue(playRequestorJson->value, "id", &playRequestor.id)) {
            ACSDK_ERROR(LX("handlePlayDirectiveFailed")
                            .d("reason", "missingPlayRequestorId")
                            .d("messageId", info->directive->getMessageId()));
            sendExceptionEncounteredAndReportFailed(info, "missing playRequestor id in Play directive");
            return;
        }
    }

    auto messageId = info->directive->getMessageId();
    submitMetric(
        m_metricRecorder,
        PLAY_DIRECTIVE_RECEIVED,
        DataPointCounterBuilder{}.setName(PLAY_DIRECTIVE_RECEIVED).increment(1).build(),
        messageId,
        playbackSessionId,
        adapter->name());

    setHandlingCompleted(info);
    adapter->handlePlay(
        playbackContextToken,
        index,
        std::chrono::milliseconds(offset),
        skillToken,
        playbackSessionId,
        navigation,
        preload,
        playRequestor);
}

void ExternalMediaPlayer::handleSeek(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    auto adapter = preprocessDirective(info, &payload);
    if (!adapter) {
        return;
    }

    int64_t position;
    if (!jsonUtils::retrieveValue(payload, POSITIONINMS, &position)) {
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

    if (STOP_DIRECTIVE.name == info->directive->getName() || PAUSE_DIRECTIVE.name == info->directive->getName()) {
        auto messageId = info->directive->getMessageId();
        submitMetric(
            m_metricRecorder,
            STOP_DIRECTIVE_RECEIVED,
            DataPointCounterBuilder{}.setName(STOP_DIRECTIVE_RECEIVED).increment(1).build(),
            messageId,
            "",
            adapter->name());
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

void ExternalMediaPlayer::setObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_renderPlayerObserver = observer;
}

bool ExternalMediaPlayer::localOperation(PlaybackOperation op) {
    ACSDK_DEBUG5(LX(__func__));

    std::string playerInFocus;
    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        playerInFocus = m_playerInFocus;
    }
    std::string localPlayerId;
    if (!playerInFocus.empty()) {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        auto playerIdToLocalPlayerId = m_authorizedAdapters.find(playerInFocus);

        if (m_authorizedAdapters.end() == playerIdToLocalPlayerId) {
            ACSDK_ERROR(LX("stopPlaybackFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, m_playerInFocus));
            return false;
        }
        localPlayerId = playerIdToLocalPlayerId->second;

        lock.unlock();

        auto adapter = getAdapterByLocalPlayerId(localPlayerId);

        if (!adapter) {
            // Should never reach here as playerInFocus is always set based on a contract with AVS.
            ACSDK_ERROR(LX("AdapterNotFound").d("player", localPlayerId));
            return false;
        }

        switch (op) {
            case PlaybackOperation::STOP_PLAYBACK:
                adapter->handlePlayControl(RequestType::STOP);
                break;
            case PlaybackOperation::PAUSE_PLAYBACK:
                adapter->handlePlayControl(RequestType::PAUSE);
                break;
            case PlaybackOperation::RESUME_PLAYBACK:
                adapter->handlePlayControl(RequestType::RESUME);
                break;
        }
        return true;
    }
    return false;
}

bool ExternalMediaPlayer::localSeekTo(std::chrono::milliseconds location, bool fromStart) {
    ACSDK_DEBUG5(LX(__func__));

    std::string playerInFocus;
    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        playerInFocus = m_playerInFocus;
    }
    std::string localPlayerId;
    if (!playerInFocus.empty()) {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        auto playerIdToLocalPlayerId = m_authorizedAdapters.find(playerInFocus);

        if (m_authorizedAdapters.end() == playerIdToLocalPlayerId) {
            ACSDK_ERROR(LX("stopPlaybackFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, m_playerInFocus));
            return false;
        }
        localPlayerId = playerIdToLocalPlayerId->second;

        lock.unlock();

        auto adapter = getAdapterByLocalPlayerId(localPlayerId);

        if (!adapter) {
            // Should never reach here as playerInFocus is always set based on a contract with AVS.
            ACSDK_ERROR(LX("AdapterNotFound").d("player", localPlayerId));
            return false;
        }

        if (fromStart) {
            adapter->handleSeek(location);
        } else {
            adapter->handleAdjustSeek(location);
        }
        return true;
    }
    return false;
}

std::chrono::milliseconds ExternalMediaPlayer::getAudioItemOffset() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
    if (!m_adapterInFocus) {
        ACSDK_ERROR(LX("getAudioItemOffsetFailed").d("reason", "NoActiveAdapter").d("player", m_playerInFocus));
        return std::chrono::milliseconds::zero();
    }
    return m_adapterInFocus->getOffset();
}

std::chrono::milliseconds ExternalMediaPlayer::getAudioItemDuration() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
    if (!m_adapterInFocus) {
        ACSDK_ERROR(LX("getAudioItemDurationFailed").d("reason", "NoActiveAdapter").d("player", m_playerInFocus));
        return std::chrono::milliseconds::zero();
    }
    return m_adapterInFocus->getState().playbackState.duration;
}

void ExternalMediaPlayer::setPlayerInFocus(const std::string& playerInFocus) {
    ACSDK_DEBUG5(LX(__func__));
    {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        if (m_authorizedAdapters.find(playerInFocus) == m_authorizedAdapters.end()) {
            ACSDK_ERROR(LX("setPlayerInFocusFailed").d("reason", "unauthorizedPlayer").d("playerId", playerInFocus));
            return;
        }
    }
    ACSDK_DEBUG(LX(__func__).d("playerInFocus", playerInFocus));
    auto adapterInFocus = getAdapterByPlayerId(playerInFocus);

    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        m_playbackRouter->setHandler(shared_from_this());
        if (m_playerInFocus == playerInFocus) {
            ACSDK_DEBUG5(LX(__func__).m("Aborting - no change"));
            return;
        }
        m_playerInFocus = playerInFocus;
        m_adapterInFocus = adapterInFocus;
    }
}

void ExternalMediaPlayer::onButtonPressed(PlaybackButton button) {
    std::string playerInFocus;
    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        playerInFocus = m_playerInFocus;
    }
    std::string localPlayerId;
    if (!playerInFocus.empty()) {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        auto playerIdToLocalPlayerId = m_authorizedAdapters.find(playerInFocus);

        if (m_authorizedAdapters.end() == playerIdToLocalPlayerId) {
            ACSDK_ERROR(LX("onButtonPressedFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, m_playerInFocus));
            return;
        }
        localPlayerId = playerIdToLocalPlayerId->second;

        lock.unlock();

        auto adapter = getAdapterByLocalPlayerId(localPlayerId);

        if (!adapter) {
            // Should never reach here as playerInFocus is always set based on a contract with AVS.
            ACSDK_ERROR(LX("AdapterNotFound").d("player", localPlayerId));
            return;
        }

        auto buttonIt = g_buttonToRequestType.find(button);

        if (g_buttonToRequestType.end() == buttonIt) {
            ACSDK_ERROR(LX("ButtonToRequestTypeNotFound").d("button", button));
            return;
        }

        adapter->handlePlayControl(buttonIt->second);
    }
}

void ExternalMediaPlayer::onTogglePressed(PlaybackToggle toggle, bool action) {
    std::string playerInFocus;
    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        playerInFocus = m_playerInFocus;
    }
    std::string localPlayerId;
    if (!playerInFocus.empty()) {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        auto playerIdToLocalPlayerId = m_authorizedAdapters.find(m_playerInFocus);

        if (m_authorizedAdapters.end() == playerIdToLocalPlayerId) {
            ACSDK_ERROR(LX("onTogglePressedFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, m_playerInFocus));
            return;
        }

        localPlayerId = playerIdToLocalPlayerId->second;

        lock.unlock();
        auto adapter = getAdapterByLocalPlayerId(localPlayerId);

        if (!adapter) {
            // Should never reach here as playerInFocus is always set based on a contract with AVS.
            ACSDK_ERROR(LX("AdapterNotFound").d("player", localPlayerId));
            return;
        }

        auto toggleIt = g_toggleToRequestType.find(toggle);
        if (g_toggleToRequestType.end() == toggleIt) {
            ACSDK_ERROR(LX("ToggleToRequestTypeNotFound").d("toggle", toggle));
            return;
        }

        // toggleStates map is <SELECTED,DESELECTED>
        auto toggleStates = toggleIt->second;

        if (action) {
            adapter->handlePlayControl(toggleStates.first);
        } else {
            adapter->handlePlayControl(toggleStates.second);
        }
    }
}

void ExternalMediaPlayer::doShutdown() {
    m_executor.shutdown();
    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    m_contextManager->setStateProvider(SESSION_STATE, nullptr);
    m_contextManager->setStateProvider(PLAYBACK_STATE, nullptr);

    {
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

    state.AddMember(rapidjson::StringRef(AGENT_KEY), std::string(m_agentString), stateAlloc);
    state.AddMember(rapidjson::StringRef(SPI_VERSION_KEY), std::string(ExternalMediaPlayer::SPI_VERSION), stateAlloc);
    state.AddMember(rapidjson::StringRef(PLAYER_IN_FOCUS), m_playerInFocus, stateAlloc);

    rapidjson::Value players(rapidjson::kArrayType);

    std::unordered_map<std::string, std::string> authorizedAdaptersCopy;
    {
        std::lock_guard<std::mutex> lock(m_authorizedMutex);
        authorizedAdaptersCopy = m_authorizedAdapters;
    }

    for (const auto& idToLocalId : authorizedAdaptersCopy) {
        const auto& adapter = getAdapterByLocalPlayerId(idToLocalId.second);
        if (!adapter) {
            continue;
        }
        auto state = adapter->getState().sessionState;
        rapidjson::Value playerJson = buildSessionState(state, stateAlloc);
        players.PushBack(playerJson, stateAlloc);
        ObservableSessionProperties update{state.loggedIn, state.userName};
        notifyObservers(state.playerId, &update);
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

    std::unordered_map<std::string, std::string> authorizedAdaptersCopy;
    {
        std::lock_guard<std::mutex> lock(m_authorizedMutex);
        authorizedAdaptersCopy = m_authorizedAdapters;
    }

    for (const auto& idToLocalId : authorizedAdaptersCopy) {
        const auto& adapter = getAdapterByLocalPlayerId(idToLocalId.second);
        if (!adapter) {
            continue;
        }
        auto playbackState = adapter->getState().playbackState;
        auto sessionState = adapter->getState().sessionState;
        rapidjson::Value playerJson = buildPlaybackState(playbackState, stateAlloc);
        players.PushBack(playerJson, stateAlloc);
        ObservablePlaybackStateProperties update{
            playbackState.state, playbackState.trackName, playbackState.playRequestor};
        notifyObservers(sessionState.playerId, &update);
    }

    notifyRenderPlayerInfoCardsObservers();

    state.AddMember(PLAYERS, players, stateAlloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("providePlaybackState").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

void ExternalMediaPlayer::createAdapters(
    const AdapterMediaPlayerMap& mediaPlayers,
    const AdapterSpeakerMap& speakers,
    const AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<ContextManagerInterface> contextManager) {
    ACSDK_DEBUG0(LX("createAdapters"));

    for (auto& entry : adapterCreationMap) {
        auto mediaPlayerIt = mediaPlayers.find(entry.first);
        auto speakerIt = speakers.find(entry.first);

        if (mediaPlayerIt == mediaPlayers.end()) {
            ACSDK_ERROR(LX("adapterCreationFailed").d(PLAYER_ID, entry.first).d("reason", "nullMediaPlayer"));
            continue;
        }

        if (speakerIt == speakers.end()) {
            ACSDK_ERROR(LX("adapterCreationFailed").d(PLAYER_ID, entry.first).d("reason", "nullSpeaker"));
            continue;
        }

        auto adapter = entry.second(
            m_metricRecorder,
            (*mediaPlayerIt).second,
            (*speakerIt).second,
            m_speakerManager,
            messageSender,
            focusManager,
            contextManager,
            shared_from_this());
        if (adapter) {
            std::lock_guard<std::mutex> lock{m_adaptersMutex};
            m_adapters[entry.first] = adapter;
        } else {
            ACSDK_ERROR(LX("adapterCreationFailed").d(PLAYER_ID, entry.first));
        }
    }

    sendReportDiscoveredPlayersEvent();
}

void ExternalMediaPlayer::sendReportDiscoveredPlayersEvent() {
    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(AGENT_KEY, std::string(m_agentString), payload.GetAllocator());

    rapidjson::Value players(rapidjson::kArrayType);

    for (const auto& idToAdapter : m_adapters) {
        rapidjson::Value player(rapidjson::kObjectType);
        std::shared_ptr<ExternalMediaAdapterInterface> adapter = idToAdapter.second;

        player.AddMember(LOCAL_PLAYER_ID, adapter->getState().sessionState.localPlayerId, payload.GetAllocator());

        player.AddMember(SPI_VERSION_KEY, adapter->getState().sessionState.spiVersion, payload.GetAllocator());

        // We do not currently support cloud based app validation.
        player.AddMember(VALIDATION_METHOD, "NONE", payload.GetAllocator());
        rapidjson::Value validationData(rapidjson::kArrayType);
        player.AddMember(VALIDATION_DATA, validationData, payload.GetAllocator());

        players.PushBack(player, payload.GetAllocator());
    }

    payload.AddMember(PLAYERS, players, payload.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendReportDiscoveredPlayersEventFailed").d("reason", "writerRefusedJsonObject"));
        return;
    }

    auto event = buildJsonEventString(REPORT_DISCOVERED_PLAYERS, "", buffer.GetString());
    auto request = std::make_shared<MessageRequest>(event.second);

    /*
     * CertifiedSender has a limit on the number of events it can store. This limit could be reached if
     * ExternalMediaPlayer restarts excessively without a chance for the CertifiedSender to drain its internal queue.
     */
    m_certifiedMessageSender->sendJSONMessage(request->getJsonContent());
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> ExternalMediaPlayer::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void ExternalMediaPlayer::addObserver(std::shared_ptr<ExternalMediaPlayerObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.insert(observer);
}

void ExternalMediaPlayer::removeObserver(std::shared_ptr<ExternalMediaPlayerObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.erase(observer);
}

void ExternalMediaPlayer::notifyObservers(
    const std::string& playerId,
    const ObservableSessionProperties* sessionProperties) {
    notifyObservers(playerId, sessionProperties, nullptr);
}

void ExternalMediaPlayer::notifyObservers(
    const std::string& playerId,
    const ObservablePlaybackStateProperties* playbackProperties) {
    notifyObservers(playerId, nullptr, playbackProperties);
}

void ExternalMediaPlayer::notifyObservers(
    const std::string& playerId,
    const ObservableSessionProperties* sessionProperties,
    const ObservablePlaybackStateProperties* playbackProperties) {
    if (playerId.empty()) {
        ACSDK_ERROR(LX("notifyObserversFailed").d("reason", "emptyPlayerId"));
        return;
    }

    std::unique_lock<std::mutex> lock{m_observersMutex};
    auto observers = m_observers;
    lock.unlock();

    for (const auto& observer : observers) {
        if (sessionProperties) {
            observer->onLoginStateProvided(playerId, *sessionProperties);
        }

        if (playbackProperties) {
            observer->onPlaybackStateProvided(playerId, *playbackProperties);
        }
    }
}

void ExternalMediaPlayer::notifyRenderPlayerInfoCardsObservers() {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> lock{m_inFocusAdapterMutex};
    if (m_adapterInFocus) {
        auto adapterState = m_adapterInFocus->getState();
        lock.unlock();
        std::stringstream ss{adapterState.playbackState.state};
        avsCommon::avs::PlayerActivity playerActivity = avsCommon::avs::PlayerActivity::IDLE;
        ss >> playerActivity;
        if (ss.fail()) {
            ACSDK_ERROR(LX("notifyRenderPlayerInfoCardsFailed")
                            .d("reason", "invalidState")
                            .d("state", adapterState.playbackState.state));
            return;
        }
        RenderPlayerInfoCardsObserverInterface::Context context;
        context.audioItemId = adapterState.playbackState.trackId;
        context.offset = getAudioItemOffset();
        context.mediaProperties = shared_from_this();
        {
            std::lock_guard<std::mutex> lock{m_observersMutex};
            if (m_renderPlayerObserver) {
                m_renderPlayerObserver->onRenderPlayerCardsInfoChanged(playerActivity, context);
            }
        }
    }
}

}  // namespace externalMediaPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
