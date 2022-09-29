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

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <acsdkExternalMediaPlayerInterfaces/AdapterUtils.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterConstants.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "acsdkExternalMediaPlayer/ExternalMediaPlayer.h"
#include "acsdkExternalMediaPlayer/StaticExternalMediaPlayerAdapterHandler.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

using namespace acsdkExternalMediaPlayerInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
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

/// The max relative time in the past that we can  seek to in
/// milliseconds(-12hours in ms).
static const int64_t MAX_PAST_OFFSET = -86400000;

/// The max relative time in the past that we can  seek to in milliseconds(+12
/// hours in ms).
static const int64_t MAX_FUTURE_OFFSET = 86400000;

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

/// The directive received metric is used in place of 'first bytes' for UPL
/// because pre-buffering may distort the metric
static const std::string PLAY_DIRECTIVE_RECEIVED = "PLAY_DIRECTIVE_RECEIVED";

/// The directive received metric is used in place of 'first bytes' for Stop UPL
/// because there are no bytes involved
static const std::string STOP_DIRECTIVE_RECEIVED = "STOP_DIRECTIVE_RECEIVED";

/**
 * Handles a Metric event by creating and recording it. Failure to create or
 * record the event results in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric
 * events.
 * @param metricName The name of the Metric event.
 * @param dataPoint The @c DataPoint of this Metric event (e.g. duration).
 * @param msgId The message id string from the retrieved from the directive.
 * @param trackId The track id string from the retrieved from the directive.
 * @param playerId The player id string from the retrieved from the directive.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& metricName,
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
        metricBuilder.addDataPoint(DataPointStringBuilder{}.setName("EXTERNAL_PLAYER_ID").setValue(playerId).build());
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

std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface> ExternalMediaPlayer::
    createExternalMediaPlayerInterface(std::shared_ptr<ExternalMediaPlayer> player) {
    return player;
}

std::shared_ptr<ExternalMediaPlayer> ExternalMediaPlayer::createExternalMediaPlayer(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
        renderPlayerInfoCardsProviderRegistrar,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    auto externalMediaPlayer = ExternalMediaPlayer::create(
        messageSender,
        certifiedMessageSender,
        contextManager,
        exceptionSender,
        playbackRouter,
        endpointCapabilitiesRegistrar,
        shutdownNotifier,
        startupNotifier,
        renderPlayerInfoCardsProviderRegistrar,
        metricRecorder);

    if (!externalMediaPlayer) {
        ACSDK_ERROR(LX("createExternalMediaPlayerFailed").d("reason", "failed to create ExternalMediaPlayer"));
        return nullptr;
    }

    return externalMediaPlayer;
}

std::shared_ptr<ExternalMediaPlayer> ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
        audioPipelineFactory,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
        renderPlayerInfoCardsProviderRegistrar,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) {
    if (!audioPipelineFactory || !audioFocusManager || !speakerManager) {
        ACSDK_ERROR(LX("createExternalMediaPlayerWithAdaptersFailed")
                        .d("isAudioPipelineFactoryNull", !audioPipelineFactory)
                        .d("isAudioFocusManagerNull", !audioFocusManager)
                        .d("isSpeakerManagerNull", !speakerManager));
        return nullptr;
    }

    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager = audioFocusManager;

    auto externalMediaPlayer = ExternalMediaPlayer::create(
        messageSender,
        certifiedMessageSender,
        contextManager,
        exceptionSender,
        playbackRouter,
        endpointCapabilitiesRegistrar,
        shutdownNotifier,
        startupNotifier,
        renderPlayerInfoCardsProviderRegistrar,
        metricRecorder);

    if (!externalMediaPlayer) {
        ACSDK_ERROR(
            LX("createExternalMediaPlayerWithAdaptersFailed").d("reason", "failed to create ExternalMediaPlayer"));
        return nullptr;
    }

    externalMediaPlayer->createAdapters(adapterCreationMap, audioPipelineFactory, focusManager, speakerManager);

    return externalMediaPlayer;
}

std::shared_ptr<ExternalMediaPlayer> ExternalMediaPlayer::create(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface> endpointCapabilitiesRegistrar,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> startupNotifier,
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
        renderPlayerInfoCardsProviderRegistrar,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    if (!messageSender || !certifiedMessageSender || !contextManager || !exceptionSender || !playbackRouter ||
        !shutdownNotifier || !startupNotifier || !endpointCapabilitiesRegistrar ||
        !renderPlayerInfoCardsProviderRegistrar) {
        ACSDK_ERROR(LX("createFailed")
                        .d("isMessageSenderNull", !messageSender)
                        .d("isCertifiedMessageSenderNull", !certifiedMessageSender)
                        .d("isContextManagerNull", !contextManager)
                        .d("isExceptionSenderNull", !exceptionSender)
                        .d("isPlaybackRouterNull", !playbackRouter)
                        .d("isShutdownNotifierNull", !shutdownNotifier)
                        .d("isStartupNotifierNull", !startupNotifier)
                        .d("isEndpointCapabilitiesRegistrarNull", !endpointCapabilitiesRegistrar)
                        .d("isRenderPlayerInfoCardsProviderRegistrarNull", !renderPlayerInfoCardsProviderRegistrar));
        return nullptr;
    }

    auto externalMediaPlayer = std::shared_ptr<ExternalMediaPlayer>(new ExternalMediaPlayer(
        std::move(messageSender),
        std::move(certifiedMessageSender),
        std::move(contextManager),
        std::move(exceptionSender),
        std::move(playbackRouter),
        std::move(metricRecorder)));
    if (!externalMediaPlayer) {
        ACSDK_ERROR(LX("createFailed").m("null ExternalMediaPlayer"));
        return nullptr;
    }

    if (!externalMediaPlayer->init()) {
        ACSDK_ERROR(LX("createExternalMediaPlayerFailed").d("reason", "initFailed"));
        return nullptr;
    }

    shutdownNotifier->addObserver(externalMediaPlayer);
    startupNotifier->addObserver(externalMediaPlayer);
    renderPlayerInfoCardsProviderRegistrar->registerProvider(externalMediaPlayer);
    endpointCapabilitiesRegistrar->withCapability(externalMediaPlayer, externalMediaPlayer);

    return externalMediaPlayer;
}

ExternalMediaPlayer::ExternalMediaPlayer(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) :
        CapabilityAgent{EXTERNALMEDIAPLAYER_NAMESPACE, std::move(exceptionSender)},
        RequiresShutdown{"ExternalMediaPlayer"},
        m_metricRecorder{std::move(metricRecorder)},
        m_messageSender{std::move(messageSender)},
        m_certifiedMessageSender{std::move(certifiedMessageSender)},
        m_contextManager{std::move(contextManager)},
        m_playbackRouter{std::move(playbackRouter)},
        m_onStartupHasBeenCalled{false} {
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

bool ExternalMediaPlayer::init() {
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

    return true;
}

void ExternalMediaPlayer::createAdapters(
    const AdapterCreationMap& adapterCreationMap,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>
        audioPipelineFactory,
    std::shared_ptr<FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (!audioPipelineFactory) {
        ACSDK_ERROR(LX("createAdaptersFailed").m("null audioPipelineFactory"));
        return;
    }

    bool hasAddedAdapters = false;
    std::shared_ptr<StaticExternalMediaPlayerAdapterHandler> handler{new StaticExternalMediaPlayerAdapterHandler()};
    std::vector<DiscoveredPlayerInfo> discoveredPlayers;

    for (auto& entry : adapterCreationMap) {
        auto playerId = entry.first;
        /**
         * We create the audio pipeline with all default arguments except for playerId, because
         * EMP is only responsible for creating adapters here when maintaining backwards compatibility with applications
         * that have not yet moved to the manufactory when instantiating DefaultClient.
         *
         * In the backwards-compatible case, applications are also using the backwards-compatible stub application audio
         * pipeline factory. The stub factory does not actually create audio pipelines but instead forwards pre-made
         * pipelines from the application (in which case all arguments are ignored regardless).
         */
        auto audioPipeline = audioPipelineFactory->createApplicationMediaInterfaces(playerId + "MediaPlayer");

        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer;
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> channelVolume;
        if (!audioPipeline) {
            ACSDK_WARN(LX(__func__).d("failed to create audioPipeline for playerId", playerId));
        } else {
            mediaPlayer = audioPipeline->mediaPlayer;
            channelVolume = audioPipeline->channelVolume;
        }

        auto adapter = entry.second(
            m_metricRecorder,
            mediaPlayer,
            channelVolume,
            speakerManager,
            m_messageSender,
            focusManager,
            m_contextManager,
            shared_from_this());
        if (adapter) {
            auto state = adapter->getState();
            DiscoveredPlayerInfo discoveredPlayerInfo;
            discoveredPlayerInfo.localPlayerId = entry.first;
            discoveredPlayerInfo.spiVersion = state.sessionState.spiVersion;
            // We currently do not support cloud based app validation for statically added players
            discoveredPlayerInfo.validationMethod = ValidationMethod::NONE;
            discoveredPlayers.push_back(std::move(discoveredPlayerInfo));

            m_staticAdapters[entry.first] = adapter;
            handler->addAdapter(entry.first, adapter);
            hasAddedAdapters = true;
        } else {
            ACSDK_ERROR(LX("adapterCreationFailed").d(PLAYER_ID, entry.first));
        }
    }

    if (hasAddedAdapters) {
        m_executor.execute([this, handler, discoveredPlayers]() {
            m_adapterHandlers.insert(handler);
            updateDiscoveredPlayers(discoveredPlayers, {});
        });
    } else {
        handler->shutdown();
    }
}

std::shared_ptr<CapabilityConfiguration> getExternalMediaPlayerCapabilityConfiguration() {
    return generateCapabilityConfiguration(
        EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_TYPE,
        EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_NAME,
        EXTERNALMEDIAPLAYER_CAPABILITY_INTERFACE_VERSION);
}

void ExternalMediaPlayer::onContextAvailable(const std::string& jsonContext) {
    // Do not block the ContextManager thread.
    m_executor.execute([this, jsonContext] {
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
    m_executor.execute([this, stateProviderName, stateRequestToken] {
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

bool ExternalMediaPlayer::preprocessDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document* document) {
    ACSDK_DEBUG9(LX("preprocessDirective"));

    if (!parseDirectivePayload(info, document)) {
        return false;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(*document, PLAYER_ID, &playerId)) {
        ACSDK_ERROR(LX("preprocessDirectiveFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
        return false;
    }

    return true;
}

avsCommon::utils::Optional<ExternalMediaPlayer::LocalPlayerIdHandler> ExternalMediaPlayer::getHandlerFromPlayerId(
    const std::string& playerId) {
    ACSDK_DEBUG5(LX(__func__));

    {
        auto lock = std::unique_lock<std::mutex>(m_authorizedMutex);
        auto playerIdToLocalPlayerId = m_authorizedAdapters.find(playerId);

        if (m_authorizedAdapters.end() == playerIdToLocalPlayerId) {
            ACSDK_ERROR(LX("getAdapterByPlayerIdFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, playerId));

            return avsCommon::utils::Optional<LocalPlayerIdHandler>();
        } else {
            return playerIdToLocalPlayerId->second;
        }
    }
}

void ExternalMediaPlayer::handleAuthorizeDiscoveredPlayers(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    ACSDK_INFO(LX(__func__));

    rapidjson::Document payload;

    if (!parseDirectivePayload(info, &payload)) {
        return;
    }

    // If a player fails to parse, make note but continue to parse the rest.
    bool parseAllSucceeded = true;

    std::vector<PlayerInfo> playerInfoList;

    rapidjson::Value::ConstMemberIterator playersIt;
    if (json::jsonUtils::findNode(payload, PLAYERS, &playersIt)) {
        for (rapidjson::Value::ConstValueIterator playerIt = playersIt->value.Begin();
             playerIt != playersIt->value.End();
             playerIt++) {
            bool authorized = false;
            std::string localPlayerId, playerId, defaultSkillToken;
            PlayerInfo playerInfo;

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

                playerInfo.playerId = playerId;
                playerInfo.skillToken = defaultSkillToken;
            }

            ACSDK_DEBUG(LX(__func__)
                            .d("localPlayerId", localPlayerId)
                            .d("authorized", authorized)
                            .d("playerId", playerId)
                            .d("defaultSkillToken", defaultSkillToken));

            playerInfo.localPlayerId = localPlayerId;
            playerInfo.playerSupported = authorized;

            playerInfoList.push_back(playerInfo);
        }
    }

    m_executor.execute([this, info, playerInfoList, parseAllSucceeded]() {
        // A map of playerId to skillToken
        std::unordered_map<std::string, std::string> authorizedForJson;
        // The new map of m_authorizedAdapters.
        std::unordered_map<std::string, LocalPlayerIdHandler> newAuthorizedAdapters;
        // The keys of the newAuthorizedAdapters map.
        std::unordered_set<std::string> newAuthorizedAdaptersKeys;
        // Deauthorized localId
        std::unordered_set<std::string> deauthorizedLocal;

        for (auto adapterHandler : m_adapterHandlers) {
            auto authorizedPlayers = adapterHandler->updatePlayerInfo(playerInfoList);
            for (auto player : authorizedPlayers) {
                if (player.playerSupported) {
                    authorizedForJson[player.playerId] = player.skillToken;
                    newAuthorizedAdapters[player.playerId] = {player.localPlayerId, adapterHandler};
                    newAuthorizedAdaptersKeys.insert(player.playerId);
                } else {
                    deauthorizedLocal.insert(player.localPlayerId);
                }
            };
        }

        // One or more players failed to be parsed.
        if (!parseAllSucceeded) {
            sendExceptionEncounteredAndReportFailed(
                info,
                "One or more player was not successfully parsed",
                ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        } else {
            setHandlingCompleted(info);
        }

        {
            std::lock_guard<std::mutex> lock(m_authorizedMutex);

            if (!deauthorizedLocal.empty()) {
                std::unordered_set<std::string> deauthorizedCloud;

                for (const auto& adapters : m_authorizedAdapters) {
                    if (deauthorizedLocal.count(adapters.second.localPlayerId)) {
                        deauthorizedCloud.insert(adapters.first);
                    }
                }

                for (const auto& playerId : deauthorizedCloud) {
                    m_authorizedAdapters.erase(playerId);
                }
            }

            for (const auto& authorizedAdapter : newAuthorizedAdapters) {
                m_authorizedAdapters[authorizedAdapter.first] = authorizedAdapter.second;
            }
        }

        // Update the sender.
        m_authorizedSender->updateAuthorizedPlayers(newAuthorizedAdaptersKeys);

        sendAuthorizationCompleteEvent(authorizedForJson, deauthorizedLocal);
    });
}

std::map<std::string, std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface>>
ExternalMediaPlayer::getAdaptersMap() {
    return m_staticAdapters;
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

    if (!preprocessDirective(info, &payload)) {
        return;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(payload, "playerId", &playerId)) {
        ACSDK_ERROR(LX("handleLoginFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "missing playerId in Login directive");
        return;
    }

    std::string accessToken;
    if (!jsonUtils::retrieveValue(payload, "accessToken", &accessToken)) {
        ACSDK_ERROR(LX("handleLoginFailed").d("reason", "nullAccessToken"));
        sendExceptionEncounteredAndReportFailed(info, "missing accessToken in Login directive");
        return;
    }

    std::string userName;
    if (!jsonUtils::retrieveValue(payload, USERNAME, &userName)) {
        userName = "";
    }

    int64_t refreshInterval;
    if (!jsonUtils::retrieveValue(payload, "tokenRefreshIntervalInMilliseconds", &refreshInterval)) {
        ACSDK_ERROR(LX("handleLoginFailed").d("reason", "nullRefreshInterval"));
        sendExceptionEncounteredAndReportFailed(info, "missing tokenRefreshIntervalInMilliseconds in Login directive");
        return;
    }

    bool forceLogin;
    if (!jsonUtils::retrieveValue(payload, "forceLogin", &forceLogin)) {
        ACSDK_ERROR(LX("handleLoginFailed").d("reason", "nullForceLogin"));
        sendExceptionEncounteredAndReportFailed(info, "missing forceLogin in Login directive");
        return;
    }

    m_executor.execute([this, info, playerId, accessToken, userName, refreshInterval, forceLogin]() {
        auto maybeHandler = getHandlerFromPlayerId(playerId);
        if (maybeHandler.hasValue()) {
            auto handler = maybeHandler.value();
            handler.adapterHandler->login(
                handler.localPlayerId, accessToken, userName, forceLogin, std::chrono::milliseconds(refreshInterval));
            setHandlingCompleted(info);
        } else {
            ACSDK_ERROR(LX("handleLoginFailedInExecutor").d("reason", "unauthorizedPlayerId"));
            sendExceptionEncounteredAndReportFailed(info, "PlayerId is not configured or authorized.");
        }
    });
}

void ExternalMediaPlayer::handleLogout(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    if (!preprocessDirective(info, &payload)) {
        return;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(payload, "playerId", &playerId)) {
        ACSDK_ERROR(LX("handleLogoutFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "missing playerId in Logout directive");
        return;
    }

    m_executor.execute([this, info, playerId]() {
        auto maybeHandler = getHandlerFromPlayerId(playerId);
        if (maybeHandler.hasValue()) {
            auto handler = maybeHandler.value();
            handler.adapterHandler->logout(handler.localPlayerId);
            setHandlingCompleted(info);
        } else {
            ACSDK_ERROR(LX("handleLogoutFailedInExecutor").d("reason", "unauthorizedPlayerId"));
            sendExceptionEncounteredAndReportFailed(info, "PlayerId is not configured or authorized.");
        }
    });
}

void ExternalMediaPlayer::handlePlay(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    if (!preprocessDirective(info, &payload)) {
        return;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(payload, PLAYER_ID, &playerId)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
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

    std::string navigationStr;
    if (!jsonUtils::retrieveValue(payload, "navigation", &navigationStr)) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullNavigation"));
        sendExceptionEncounteredAndReportFailed(info, "missing navigation in Play directive");
        return;
    }
    auto navigation = stringToNavigation(navigationStr);

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

    std::string alias;
    if (!jsonUtils::retrieveValue(payload, "aliasName", &alias)) {
        ACSDK_INFO(LX("handleDirective").m("No playback target"));
    }

    auto messageId = info->directive->getMessageId();

    m_executor.execute([this,
                        info,
                        playerId,
                        playbackContextToken,
                        index,
                        offset,
                        skillToken,
                        playbackSessionId,
                        navigation,
                        preload,
                        playRequestor,
                        messageId,
                        alias]() {
        auto maybeHandler = getHandlerFromPlayerId(playerId);
        if (maybeHandler.hasValue()) {
            auto handler = maybeHandler.value();
            ExternalMediaAdapterHandlerInterface::PlayParams params(
                handler.localPlayerId,
                playbackContextToken,
                index,
                std::chrono::milliseconds(offset),
                skillToken,
                playbackSessionId,
                navigation,
                preload,
                playRequestor,
                alias);

            if (handler.adapterHandler->play(params)) {
                submitMetric(
                    m_metricRecorder,
                    PLAY_DIRECTIVE_RECEIVED,
                    DataPointCounterBuilder{}.setName(PLAY_DIRECTIVE_RECEIVED).increment(1).build(),
                    messageId,
                    playbackSessionId,
                    playerId);
            }
            setHandlingCompleted(info);
        } else {
            ACSDK_ERROR(LX("handlePlayDirectiveFailedInExecutor").d("reason", "unauthorizedPlayerId"));
            sendExceptionEncounteredAndReportFailed(info, "PlayerId is not configured or authorized.");
        }
    });
}

void ExternalMediaPlayer::handleSeek(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    if (!preprocessDirective(info, &payload)) {
        return;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(payload, PLAYER_ID, &playerId)) {
        ACSDK_ERROR(LX("handleSeekFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
        return;
    }

    int64_t position;
    if (!jsonUtils::retrieveValue(payload, POSITIONINMS, &position)) {
        ACSDK_ERROR(LX("handleSeekFailed").d("reason", "nullPosition"));
        sendExceptionEncounteredAndReportFailed(info, "missing positionMilliseconds in SetSeekPosition directive");
        return;
    }

    m_executor.execute([this, info, playerId, position]() {
        auto maybeHandler = getHandlerFromPlayerId(playerId);
        if (maybeHandler.hasValue()) {
            auto handler = maybeHandler.value();
            handler.adapterHandler->seek(handler.localPlayerId, std::chrono::milliseconds(position));
            setHandlingCompleted(info);
        } else {
            ACSDK_ERROR(LX("handleSeekFailedInExecutor").d("reason", "unauthorizedPlayerId"));
            sendExceptionEncounteredAndReportFailed(info, "PlayerId is not configured or authorized.");
        }
    });
}

void ExternalMediaPlayer::handleAdjustSeek(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    if (!preprocessDirective(info, &payload)) {
        return;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(payload, PLAYER_ID, &playerId)) {
        ACSDK_ERROR(LX("handleAdjustSeekFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
        return;
    }

    int64_t deltaPosition;
    if (!jsonUtils::retrieveValue(payload, "deltaPositionMilliseconds", &deltaPosition)) {
        ACSDK_ERROR(LX("handleAdjustSeekFailed").d("reason", "nullDeltaPositionMilliseconds"));
        sendExceptionEncounteredAndReportFailed(
            info, "missing deltaPositionMilliseconds in AdjustSeekPosition directive");
        return;
    }

    if (deltaPosition < MAX_PAST_OFFSET || deltaPosition > MAX_FUTURE_OFFSET) {
        ACSDK_ERROR(LX("handleAdjustSeekFailed").d("reason", "deltaPositionMillisecondsOutOfRange."));
        sendExceptionEncounteredAndReportFailed(
            info, "missing deltaPositionMilliseconds in AdjustSeekPosition directive");
        return;
    }

    m_executor.execute([this, info, playerId, deltaPosition]() {
        auto maybeHandler = getHandlerFromPlayerId(playerId);
        if (maybeHandler.hasValue()) {
            auto handler = maybeHandler.value();
            handler.adapterHandler->adjustSeek(handler.localPlayerId, std::chrono::milliseconds(deltaPosition));
            setHandlingCompleted(info);
        } else {
            ACSDK_ERROR(LX("handleAdjustSeekFailedInExecutor").d("reason", "unauthorizedPlayerId"));
            sendExceptionEncounteredAndReportFailed(info, "PlayerId is not configured or authorized.");
        }
    });
}

void ExternalMediaPlayer::handlePlayControl(std::shared_ptr<DirectiveInfo> info, RequestType request) {
    rapidjson::Document payload;

    if (!preprocessDirective(info, &payload)) {
        return;
    }

    std::string playerId;
    if (!jsonUtils::retrieveValue(payload, PLAYER_ID, &playerId)) {
        ACSDK_ERROR(LX("handlePlayControlFailed").d("reason", "nullPlayerId"));
        sendExceptionEncounteredAndReportFailed(info, "No PlayerId in directive.");
        return;
    }

    std::string playbackSessionId;
    if (!jsonUtils::retrieveValue(payload, "playbackSessionId", &playbackSessionId)) {
        ACSDK_WARN(LX("handlePlayControlFailed").d("reason", "nullPlaybackSessionId"));
    }

    std::string alias;
    if (!jsonUtils::retrieveValue(payload, "aliasName", &alias)) {
        ACSDK_INFO(LX(__FUNCTION__).m("NoAliasName"));
        // fall through, alias name is not required
    }

    m_executor.execute([this, info, playerId, request, playbackSessionId, alias]() {
        std::string playbackSessId = playbackSessionId;
        auto maybeHandler = getHandlerFromPlayerId(playerId);
        if (maybeHandler.hasValue()) {
            auto handler = maybeHandler.value();
            if (handler.adapterHandler->playControl(handler.localPlayerId, request, alias)) {
                if (request == RequestType::STOP || request == RequestType::PAUSE) {
                    if (playbackSessId.empty()) {
                        auto state = handler.adapterHandler->getAdapterState(handler.localPlayerId);
                        playbackSessId = state.sessionState.playbackSessionId;
                    }
                    auto messageId = info->directive->getMessageId();
                    submitMetric(
                        m_metricRecorder,
                        STOP_DIRECTIVE_RECEIVED,
                        DataPointCounterBuilder{}.setName(STOP_DIRECTIVE_RECEIVED).increment(1).build(),
                        messageId,
                        playbackSessId,
                        playerId);
                }
            }
            setHandlingCompleted(info);
        } else {
            ACSDK_ERROR(LX("handlePlayControlFailedInExecutor").d("reason", "unauthorizedPlayerId"));
            sendExceptionEncounteredAndReportFailed(info, "PlayerId is not configured or authorized.");
        }
    });
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

    if (!playerInFocus.empty()) {
        auto localPlayerIdHandler = getHandlerFromPlayerId(playerInFocus);

        if (!localPlayerIdHandler.hasValue()) {
            ACSDK_ERROR(LX("localOperationFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, playerInFocus));
            return false;
        }

        auto localPlayerId = localPlayerIdHandler.value().localPlayerId;
        auto adapterHandler = localPlayerIdHandler.value().adapterHandler;

        auto requestType = RequestType::STOP;
        switch (op) {
            case PlaybackOperation::STOP_PLAYBACK:
                requestType = RequestType::STOP;
                break;
            case PlaybackOperation::RESUMABLE_STOP:
            case PlaybackOperation::TRANSIENT_PAUSE:
                requestType = RequestType::PAUSE;
                break;
            case PlaybackOperation::RESUME_PLAYBACK:
                requestType = RequestType::RESUME;
                break;
        }
        adapterHandler->playControl(localPlayerId, requestType, "");
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

    if (!playerInFocus.empty()) {
        auto localPlayerIdHandler = getHandlerFromPlayerId(playerInFocus);

        if (!localPlayerIdHandler.hasValue()) {
            ACSDK_ERROR(LX("localSeekToFailed").d("reason", "noMatchingLocalId").d(PLAYER_ID, playerInFocus));
            return false;
        }

        auto localPlayerId = localPlayerIdHandler.value().localPlayerId;
        auto adapterHandler = localPlayerIdHandler.value().adapterHandler;

        if (fromStart) {
            adapterHandler->seek(localPlayerId, location);
        } else {
            adapterHandler->adjustSeek(localPlayerId, location);
        }
        return true;
    }
    return false;
}

std::chrono::milliseconds ExternalMediaPlayer::getAudioItemOffset() {
    ACSDK_DEBUG5(LX(__func__));
    std::string playerInFocus;
    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        playerInFocus = m_playerInFocus;
    }
    if (playerInFocus.empty()) {
        ACSDK_ERROR(LX("getAudioItemOffsetFailed").d("reason", "NoActiveAdapter"));
        return std::chrono::milliseconds::zero();
    }

    auto maybeHandler = getHandlerFromPlayerId(playerInFocus);
    if (maybeHandler.hasValue()) {
        const auto& handler = maybeHandler.value();
        return handler.adapterHandler->getOffset(handler.localPlayerId);
    } else {
        ACSDK_ERROR(
            LX("getAudioItemOffsetFailed").d("reason", "ActiveAdapterNotAuthorized").d("player", playerInFocus));
        return std::chrono::milliseconds::zero();
    }
}

std::chrono::milliseconds ExternalMediaPlayer::getAudioItemDuration() {
    ACSDK_DEBUG5(LX(__func__));
    std::string playerInFocus;
    {
        std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
        playerInFocus = m_playerInFocus;
    }
    if (playerInFocus.empty()) {
        ACSDK_ERROR(LX("getAudioItemDurationFailed").d("reason", "NoActiveAdapter"));
        return std::chrono::milliseconds::zero();
    }

    auto maybeHandler = getHandlerFromPlayerId(playerInFocus);
    if (maybeHandler.hasValue()) {
        const auto& handler = maybeHandler.value();
        return handler.adapterHandler->getAdapterState(handler.localPlayerId).playbackState.duration;
    } else {
        ACSDK_ERROR(
            LX("getAudioItemDurationFailed").d("reason", "ActiveAdapterNotAuthorized").d("player", playerInFocus));
        return std::chrono::milliseconds::zero();
    }
}

void ExternalMediaPlayer::setPlayerInFocus(const std::string& playerInFocus) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> authlock{m_authorizedMutex};
    if (m_authorizedAdapters.find(playerInFocus) == m_authorizedAdapters.end()) {
        ACSDK_ERROR(LX("setPlayerInFocusFailed").d("reason", "unauthorizedPlayer").d("playerId", playerInFocus));
        return;
    }

    ACSDK_DEBUG(LX(__func__).d("playerInFocus", playerInFocus));

    {
        std::lock_guard<std::mutex> focuslock{m_inFocusAdapterMutex};
        if (m_playerInFocus == playerInFocus) {
            ACSDK_DEBUG5(LX(__func__).m("Aborting - no change"));
            return;
        }
        m_playerInFocus = playerInFocus;
        m_playbackRouter->setHandler(shared_from_this());
    }
}

void ExternalMediaPlayer::updateDiscoveredPlayers(
    const std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>& addedPlayers,
    const std::unordered_set<std::string>& removedLocalPlayerIds) {
    {
        std::lock_guard<std::mutex> lock{m_onStartupHasBeenCalledMutex};
        if (!m_onStartupHasBeenCalled) {
            ACSDK_DEBUG9(LX("updateDiscoveredPlayersDeferred").d("reason", "startup not called yet"));
            for (const auto& player : addedPlayers) {
                if (m_unreportedPlayersToReportAtStartup.count(player.localPlayerId) == 0) {
                    m_unreportedPlayersToReportAtStartup.insert(std::make_pair(player.localPlayerId, player));
                }
            }
            return;
        }
    }

    m_executor.execute([this, addedPlayers, removedLocalPlayerIds] {
        for (const auto& playerId : removedLocalPlayerIds) {
            m_reportedDiscoveredPlayers.erase(playerId);
        }

        {
            // Remove all removed players from the authorized adapters list
            std::vector<std::string> cloudPlayerIdsToRemove;
            std::lock_guard<std::mutex> authlock{m_authorizedMutex};
            for (const auto& adapter : m_authorizedAdapters) {
                if (removedLocalPlayerIds.count(adapter.second.localPlayerId)) {
                    cloudPlayerIdsToRemove.push_back(adapter.first);
                }
            }

            for (const auto& cloudPlayerId : cloudPlayerIdsToRemove) {
                m_authorizedAdapters.erase(cloudPlayerId);
            }
        }

        // Report any newly added players
        std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo> newlyDiscoveredPlayers;
        for (const auto& player : addedPlayers) {
            if (m_reportedDiscoveredPlayers.count(player.localPlayerId) == 0) {
                newlyDiscoveredPlayers.push_back(player);
                m_reportedDiscoveredPlayers.insert(player.localPlayerId);
            }
        }

        sendReportDiscoveredPlayersEvent(newlyDiscoveredPlayers);
    });
}

void ExternalMediaPlayer::onButtonPressed(PlaybackButton button) {
    auto buttonIt = g_buttonToRequestType.find(button);
    if (g_buttonToRequestType.end() == buttonIt) {
        ACSDK_ERROR(LX(__func__).m("ButtonToRequestTypeNotFound").d("button", button));
        return;
    }

    m_executor.execute([this, buttonIt]() {
        std::string playerInFocus;
        {
            std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
            playerInFocus = m_playerInFocus;
        }

        if (!playerInFocus.empty()) {
            auto maybeHandler = getHandlerFromPlayerId(playerInFocus);
            if (maybeHandler.hasValue()) {
                const auto& handler = maybeHandler.value();
                handler.adapterHandler->playControl(handler.localPlayerId, buttonIt->second, "");
            }
        }
    });
}

void ExternalMediaPlayer::onTogglePressed(PlaybackToggle toggle, bool action) {
    auto toggleIt = g_toggleToRequestType.find(toggle);
    if (g_toggleToRequestType.end() == toggleIt) {
        ACSDK_ERROR(LX(__func__).m("ToggleToRequestTypeNotFound").d("toggle", toggle));
        return;
    }

    // toggleStates map is <SELECTED,DESELECTED>
    auto toggleStates = toggleIt->second;

    m_executor.execute([this, action, toggleStates]() {
        std::string playerInFocus;
        {
            std::lock_guard<std::mutex> lock{m_inFocusAdapterMutex};
            playerInFocus = m_playerInFocus;
        }
        if (!playerInFocus.empty()) {
            auto maybeHandler = getHandlerFromPlayerId(playerInFocus);
            if (maybeHandler.hasValue()) {
                const auto& handler = maybeHandler.value();
                if (action) {
                    handler.adapterHandler->playControl(handler.localPlayerId, toggleStates.first, "");
                } else {
                    handler.adapterHandler->playControl(handler.localPlayerId, toggleStates.second, "");
                }
            }
        }
    });
}

void ExternalMediaPlayer::doShutdown() {
    m_executor.shutdown();

    // adapter handler specific code
    for (const auto& handler : m_adapterHandlers) {
        handler->shutdown();
    }
    m_adapterHandlers.clear();
    m_staticAdapters.clear();
    m_authorizedAdapters.clear();
    m_directiveToHandlerMap.clear();

    // Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
    // In those cases there is no messageId to remove because no result was expected.
    m_contextManager->setStateProvider(SESSION_STATE, nullptr);
    m_contextManager->setStateProvider(PLAYBACK_STATE, nullptr);

    m_exceptionEncounteredSender.reset();
    m_contextManager.reset();
    m_playbackRouter.reset();
}

void ExternalMediaPlayer::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    // Check result too, to catch cases where DirectiveInfo was created locally,
    // without a nullptr result. In those cases there is no messageId to remove
    // because no result was expected.
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

    std::vector<AdapterState> adapterStates;
    for (auto adapterHandler : m_adapterHandlers) {
        auto handlerAdapterStates = adapterHandler->getAdapterStates();
        adapterStates.insert(adapterStates.end(), handlerAdapterStates.begin(), handlerAdapterStates.end());
    }

    if (stateProviderName == SESSION_STATE) {
        state = provideSessionState(adapterStates);
        ACSDK_DEBUG(LX("executeProvideState").d("provideSessionState", state));
    } else if (stateProviderName == PLAYBACK_STATE) {
        state = providePlaybackState(adapterStates);
        ACSDK_DEBUG(LX("executeProvideState").d("providePlaybackState", state));
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

// adapter handler specific code
std::string ExternalMediaPlayer::provideSessionState(std::vector<AdapterState> adapterStates) {
    rapidjson::Document state(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& stateAlloc = state.GetAllocator();

    state.AddMember(rapidjson::StringRef(AGENT_KEY), std::string(m_agentString), stateAlloc);
    state.AddMember(rapidjson::StringRef(SPI_VERSION_KEY), std::string(ExternalMediaPlayer::SPI_VERSION), stateAlloc);
    state.AddMember(rapidjson::StringRef(PLAYER_IN_FOCUS), m_playerInFocus, stateAlloc);

    rapidjson::Value players(rapidjson::kArrayType);

    std::unordered_map<std::string, LocalPlayerIdHandler> authorizedAdaptersCopy;
    {
        std::lock_guard<std::mutex> lock(m_authorizedMutex);
        authorizedAdaptersCopy = m_authorizedAdapters;
    }

    for (auto adapterState : adapterStates) {
        if (authorizedAdaptersCopy.find(adapterState.sessionState.playerId) != authorizedAdaptersCopy.end()) {
            rapidjson::Value playerJson = buildSessionState(adapterState.sessionState, stateAlloc);
            players.PushBack(playerJson, stateAlloc);
            ObservableSessionProperties update{adapterState.sessionState.loggedIn, adapterState.sessionState.userName};
            notifyObservers(adapterState.sessionState.playerId, &update);
        }
    }

    state.AddMember(rapidjson::StringRef(PLAYERS), players, stateAlloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX(__func__).m("provideSessionStateFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

// adapter handler playback states
std::string ExternalMediaPlayer::providePlaybackState(std::vector<AdapterState> adapterStates) {
    rapidjson::Document state(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& stateAlloc = state.GetAllocator();

    // Fill the default player state.
    if (!buildDefaultPlayerState(&state, stateAlloc)) {
        return "";
    }

    // Fetch actual PlaybackState from every player supported by the
    // ExternalMediaPlayer.
    rapidjson::Value players(rapidjson::kArrayType);

    std::unordered_map<std::string, LocalPlayerIdHandler> authorizedAdaptersCopy;
    {
        std::lock_guard<std::mutex> lock(m_authorizedMutex);
        authorizedAdaptersCopy = m_authorizedAdapters;
    }

    // adapter handlers
    for (auto adapterState : adapterStates) {
        if (authorizedAdaptersCopy.find(adapterState.sessionState.playerId) != authorizedAdaptersCopy.end()) {
            const auto& playbackState = adapterState.playbackState;
            rapidjson::Value playerJson = buildPlaybackState(playbackState, stateAlloc);
            players.PushBack(playerJson, stateAlloc);
            ObservablePlaybackStateProperties update{
                playbackState.state, playbackState.trackName, playbackState.playRequestor};
            notifyObservers(adapterState.sessionState.playerId, &update);
        }
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

void ExternalMediaPlayer::sendReportDiscoveredPlayersEvent(const std::vector<DiscoveredPlayerInfo>& discoveredPlayers) {
    if (discoveredPlayers.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_onStartupHasBeenCalledMutex};
        if (!m_onStartupHasBeenCalled) {
            ACSDK_INFO(LX("sendReportDiscoveredPlayersEventDeferred").d("reason", "startup not called yet"));
            return;
        }
    }

    rapidjson::Document payload(rapidjson::kObjectType);
    payload.AddMember(AGENT_KEY, std::string(m_agentString), payload.GetAllocator());

    rapidjson::Value players(rapidjson::kArrayType);

    for (const auto& discoveredPlayer : discoveredPlayers) {
        rapidjson::Value player(rapidjson::kObjectType);

        player.AddMember(LOCAL_PLAYER_ID, discoveredPlayer.localPlayerId, payload.GetAllocator());

        player.AddMember(SPI_VERSION_KEY, discoveredPlayer.spiVersion, payload.GetAllocator());

        // We do not currently support cloud based app validation.
        player.AddMember(
            VALIDATION_METHOD, validationMethodToString(discoveredPlayer.validationMethod), payload.GetAllocator());
        rapidjson::Value validationData(rapidjson::kArrayType);
        for (const auto& data : discoveredPlayer.validationData) {
            rapidjson::Value validationValue;
            validationValue.SetString(data, payload.GetAllocator());
            validationData.PushBack(validationValue, payload.GetAllocator());
        }
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
     * CertifiedSender has a limit on the number of events it can store. This
     * limit could be reached if ExternalMediaPlayer restarts excessively without
     * a chance for the CertifiedSender to drain its internal queue.
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

    for (auto adapterHandler : m_adapterHandlers) {
        // check against currently known playback state, not already paused
        auto adapterStates = adapterHandler->getAdapterStates();
        for (auto adapterState : adapterStates) {
            if (adapterState.sessionState.playerId.compare(m_playerInFocus) == 0) {  // match playerId
                std::stringstream ss{adapterState.playbackState.state};
                alexaClientSDK::avsCommon::avs::PlayerActivity playerActivity =
                    alexaClientSDK::avsCommon::avs::PlayerActivity::IDLE;
                ss >> playerActivity;
                ACSDK_ERROR(LX(__func__).d("playerActivity", adapterState.playbackState.state));
                if (ss.fail()) {
                    ACSDK_ERROR(LX(__func__)
                                    .m("notifyRenderPlayerInfoCardsFailed")
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
    }
}

void ExternalMediaPlayer::addAdapterHandler(std::shared_ptr<ExternalMediaAdapterHandlerInterface> adapterHandler) {
    ACSDK_DEBUG5(LX(__func__));
    if (!adapterHandler) {
        ACSDK_ERROR(LX("addAdapterHandler").d("reason", "nullAdapterHandler"));
        return;
    }
    m_executor.execute([this, adapterHandler]() {
        if (!m_adapterHandlers.insert(adapterHandler).second) {
            ACSDK_ERROR(LX("addAdapterHandler").d("reason", "duplicateAdapterHandler"));
        } else {
            adapterHandler->setExternalMediaPlayer(shared_from_this());
        }
    });
}

void ExternalMediaPlayer::removeAdapterHandler(std::shared_ptr<ExternalMediaAdapterHandlerInterface> adapterHandler) {
    ACSDK_DEBUG5(LX(__func__));
    if (!adapterHandler) {
        ACSDK_ERROR(LX("removeAdapterHandler").d("reason", "nullAdapterHandler"));
        return;
    }
    m_executor.execute([this, adapterHandler]() {
        if (m_adapterHandlers.erase(adapterHandler) == 0) {
            ACSDK_WARN(LX("removeAdapterHandler").d("reason", "adapterHandlerNotFound"));
        }
    });
}

bool ExternalMediaPlayer::startup() {
    ACSDK_INFO(LX(__func__));
    {
        std::lock_guard<std::mutex> lock{m_onStartupHasBeenCalledMutex};
        if (m_onStartupHasBeenCalled) {
            ACSDK_ERROR(LX("startupFailed").m("startup already called"));
            return false;
        }

        m_onStartupHasBeenCalled = true;
    }

    // Report any newly added players
    std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo> playersToReport;
    for (const auto& player : m_unreportedPlayersToReportAtStartup) {
        playersToReport.push_back(player.second);
    }

    sendReportDiscoveredPlayersEvent(playersToReport);
    m_unreportedPlayersToReportAtStartup.clear();
    return true;
}

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
