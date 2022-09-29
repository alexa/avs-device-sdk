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

/// @file ExternalMediaPlayerTest.cpp

#include <chrono>
#include <future>
#include <map>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockPlaybackRouter.h>
#include <AVSCommon/SDKInterfaces/MockRenderPlayerInfoCardsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerManager.h>
#include <AVSCommon/SDKInterfaces/Endpoints/MockEndpointCapabilitiesRegistrar.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <MockCertifiedSender.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/MockApplicationAudioPipelineFactory.h>
#include <acsdkShutdownManagerInterfaces/MockShutdownNotifier.h>
#include <acsdkStartupManagerInterfaces/MockStartupNotifier.h>
#include <acsdkStartupManagerInterfaces/StartupNotifierInterface.h>

#include "acsdkExternalMediaPlayer/ExternalMediaPlayer.h"
#include "acsdkExternalMediaPlayer/ExternalMediaAdapterHandler.h"
#include "acsdkExternalMediaPlayer/MockExternalMediaAdapterHandler.h"
#include <acsdkExternalMediaPlayerInterfaces/MockExternalMediaPlayerAdapter.h>
#include <acsdkExternalMediaPlayerInterfaces/MockExternalMediaPlayerObserver.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {
namespace test {

using namespace acsdkExternalMediaPlayerInterfaces;
using namespace acsdkExternalMediaPlayerInterfaces::test;
using namespace acsdkStartupManagerInterfaces;
using namespace acsdkStartupManagerInterfaces::test;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace avsCommon::utils::metrics::test;

using namespace ::testing;
using namespace rapidjson;

/// Provide State Token for testing.
static const unsigned int PROVIDE_STATE_TOKEN_TEST{1};

/// Plenty of time for a test to complete.
static std::chrono::milliseconds MY_WAIT_TIMEOUT(5000);

// The namespaces used in the context.
static const std::string EXTERNALMEDIAPLAYER_STATE_NAMESPACE = "ExternalMediaPlayer";
static const std::string PLAYBACKSTATEREPORTER_STATE_NAMESPACE = "Alexa.PlaybackStateReporter";

// The names used in the context.
static const std::string EXTERNALMEDIAPLAYER_NAME = "ExternalMediaPlayerState";
static const std::string PLAYBACKSTATEREPORTER_NAME = "playbackState";

static const std::string EXTERNALMEDIAPLAYER_NAMESPACE = "ExternalMediaPlayer";
static const std::string PLAYBACKCONTROLLER_NAMESPACE = "Alexa.PlaybackController";
static const std::string PLAYLISTCONTROLLER_NAMESPACE = "Alexa.PlaylistController";
static const std::string SEEKCONTROLLER_NAMESPACE = "Alexa.SeekController";
static const std::string FAVORITESCONTROLLER_NAMESPACE = "Alexa.FavoritesController";

// field values used in Adapter State response
static const std::string PLAYER_USER_NAME = "userName";
static const std::string PLAYER_TRACK = "testTrack";
static const std::string PLAYER_STATE = "IDLE";

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

// @c ExternalMediaPlayer events.
static const NamespaceAndName REPORT_DISCOVERED_PLAYERS{EXTERNALMEDIAPLAYER_NAMESPACE, "ReportDiscoveredPlayers"};
static const NamespaceAndName AUTHORIZATION_COMPLETE{EXTERNALMEDIAPLAYER_NAMESPACE, "AuthorizationComplete"};

// A playRequestor for testing
static const PlayRequestor testPlayRequestor{.type = "ALERT", .id = "123"};

// clang-format off
static const std::string IDLE_PLAYBACK_STATE =
    R"({
        "state":"IDLE",
        "supportedOperations":[],
        "shuffle":"NOT_SHUFFLED",
        "repeat":"NOT_REPEATED",
        "favorite":"NOT_RATED",
        "positionMilliseconds":0,
        "players":[{
            "playerId":"MSP1_PLAYERID",
            "state":"IDLE",
            "supportedOperations":[],
            "positionMilliseconds":0,
            "shuffle":"NOT_SHUFFLED",
            "repeat":"NOT_REPEATED",
            "favorite":"NOT_RATED",
            "media":{
                "type":"",
                "value":{
                    "playbackSource":"",
                    "playbackSourceId":"",
                    "playbackId":"",
                    "trackName":"",
                    "trackId":"",
                    "trackNumber":"",
                    "artist":"",
                    "artistId":"",
                    "album":"",
                    "albumId":"",
                    "coverUrls":{"tiny":"","small":"","medium":"","large":""},
                    "coverId":"",
                    "mediaProvider":"",
                    "mediaType":"TRACK",
                    "durationInMilliseconds":0
                }
            }
        }]
    })";
// clang-format on

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");
static const std::string MESSAGE_ID_TEST2("MessageId_Test2");

/// Dialog Request Id for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogId_Test");

/// String to identify log entries originating from this file.
static const std::string TAG("ExternalMediaPlayerTest");

/// Music service provider id 1.
static const std::string MSP1_LOCAL_PLAYER_ID("MSP1_LOCAL_PLAYER_ID");
/// Cloud assigned playerId for this MSP.
static const std::string MSP1_PLAYER_ID("MSP1_PLAYERID");
/// Associated skillToken for this MSP.
static const std::string MSP1_SKILLTOKEN("MSP1_SKILLTOKEN");

/// Music service provider id 2.
static const std::string MSP2_LOCAL_PLAYER_ID("MSP2_LOCAL_PLAYER_ID");
/// Cloud assigned playerId for this MSP.
static const std::string MSP2_PLAYER_ID("MSP2_PLAYERID");
/// Associated skillToken for this MSP.
static const std::string MSP2_SKILLTOKEN("MSP2_SKILLTOKEN");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Custom matcher to check an event of a certain name is sent.
 * Returns true if the event with that name is sent.
 * */
MATCHER_P2(
    EventNamed,
    /* std::string */ expectedNameSpace,
    /* std::string */ expectedName,
    "") {
    // Throw obvious compile error if not a MessageRequest.
    std::shared_ptr<MessageRequest> request = arg;

    if (!request) {
        return false;
    }

    rapidjson::Document document;
    ParseResult result = document.Parse(request->getJsonContent());

    if (!result) {
        return false;
    }

    rapidjson::Value::ConstMemberIterator eventIt;
    rapidjson::Value::ConstMemberIterator headerIt;
    if (!json::jsonUtils::findNode(document, "event", &eventIt) ||
        !json::jsonUtils::findNode(eventIt->value, "header", &headerIt)) {
        return false;
    }

    std::string name;
    std::string nameSpace;

    if (!json::jsonUtils::retrieveValue(headerIt->value, "name", &name) ||
        !json::jsonUtils::retrieveValue(headerIt->value, "namespace", &nameSpace) || nameSpace != expectedNameSpace ||
        name != expectedName) {
        return false;
    }

    return true;
}

/**
 * Method to create an adapter state struct response to getState();
 *
 * @return an adpater state partially filled with test values
 */
static AdapterState createAdapterState() {
    AdapterSessionState sessionState;
    sessionState.loggedIn = false;
    sessionState.userName = PLAYER_USER_NAME;
    sessionState.playerId = MSP1_PLAYER_ID;

    AdapterPlaybackState playbackState;
    playbackState.state = PLAYER_STATE;
    playbackState.playRequestor = testPlayRequestor;
    playbackState.playerId = MSP1_PLAYER_ID;

    AdapterState adapterState;
    adapterState.sessionState = sessionState;
    adapterState.playbackState = playbackState;
    return adapterState;
}

class MockRenderPlayerInfoCardsProviderRegistrar : public RenderPlayerInfoCardsProviderRegistrarInterface {
public:
    MOCK_METHOD1(
        registerProvider,
        bool(const std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>& provider));
    MOCK_METHOD0(
        getProviders,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>());
};

/**
 * Method to create AuthorizeDiscoveredPlayers payload.
 *
 * @param players A set of players JSON objects.
 *
 * @return A string representation of the payload.
 */
static std::string createAuthorizeDiscoveredPlayersPayload(
    std::unordered_set<std::string> players = std::unordered_set<std::string>()) {
    // clang-format off
    std::string payload = R"(
    {
        "players" : [
        )";

    for (auto it = players.begin(); it != players.end(); it++) {
        if (it != players.begin()) {
            payload += ",";
        }
        payload += *it;
    }
    payload += R"(]})";
    // clang-format on

    return payload;
}

/**
 * Create the players json object.
 *
 * @param localPlayerId The localPlayerId.
 * @param authorized Whether the player is authorized.
 * @param playerId The cloud assigned playerId.
 * @param skillToken The skillToken.
 *
 * @return A string representation of the payload.
 */
static std::string createPlayerJson(
    const std::string& localPlayerId,
    bool authorized,
    const std::string& playerId,
    const std::string& skillToken) {
    // clang-format off
    return
        R"({
            "localPlayerId" : ")" + localPlayerId + R"(",
            "authorized" : )" + (authorized ? "true" : "false") + R"(,
            "metadata" : {
                "playerId" : ")" + playerId + R"(",
                "skillToken" : ")" + skillToken + R"("
            }
        })";
    // clang-format on
}

/**
 * Get idle session state json object
 *
 * @return A string representation of idle session state json object
 */
static std::string getIdleSessionStateJson(std::string agent) {
    // clang-format off
    std::string idle_session_state =
        R"({
            "agent":")" + std::string(agent) + R"(",
            "spiVersion":")" + std::string(ExternalMediaPlayer::SPI_VERSION) + R"(",
            "playerInFocus":"",
            "players":[{
                "playerId":"MSP1_PLAYERID",
                "endpointId":"",
                "loggedIn":false,
                "username":"userName",
                "isGuest":false,
                "launched":false,
                "active":false,
                "spiVersion":"",
                "playerCookie":"",
                "skillToken":"",
                "playbackSessionId":""
            }]
        })";
    // clang-format on
    return idle_session_state;
}

/**
 * Method to create payload with parse error.
 *
 * @param playContext Play context {Track/playlist/album/artist/station/podcast} identifier.
 * @param index The index of the media item in the container, if the container is indexable.
 * @param offsetInMilliseconds The offset position within media item, in milliseconds.
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createPlayPayloadWithParseError(
    const std::string& playContext,
    int index,
    int64_t offsetInMilliseconds,
    const std::string& playerId,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ""
        "}";
    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create payload with only playerId.
 *
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createPayloadWithPlayerId(const std::string& playerId) {
    // clang-format off
    const std::string PLAYERID_PAYLOAD_TEST =
        "{"
            "\"playerId\":\"" + playerId + "\""
        "}";
    // clang-format on

    return PLAYERID_PAYLOAD_TEST;
}

/**
 * Method to create a Play payload with playContext, index, offsetInMilliseconds, playerId.
 *
 * @param playContext Play context {Track/playlist/album/artist/station/podcast} identifier.
 * @param index The index of the media item in the container, if the container is indexable.
 * @param offsetInMilliseconds The offset position within media item, in milliseconds.
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createPlayPayload(
    const std::string& playContext,
    int index,
    int64_t offsetInMilliseconds,
    const std::string& playerId,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ""
        "}";
    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create a Play payload with playContext, index, offsetInMilliseconds, playerId, and playRequestor.
 *
 * @param playContext Play context {Track/playlist/album/artist/station/podcast} identifier.
 * @param index The index of the media item in the container, if the container is indexable.
 * @param offsetInMilliseconds The offset position within media item, in milliseconds.
 * @param playerId The business name of the player.
 * @param skillToken The token identifying the domain or skill associated with this player.
 * @param playbackSessionId A UUID associated with the most recent @c Play directive.
 * @param navigation Communicates desired visual display behavior for the app associated with playback.
 * @param preload Indicates if @c Play directive is intended to preload the identified content only but not begin
 * playback.
 * @param playRequestor The playRequestor object of the @c play directive.
 *
 * @return A string representation of the payload.
 */
static std::string createPlayPayloadWithPlayRequestor(
    const std::string& playContext,
    int index,
    int64_t offsetInMilliseconds,
    const std::string& playerId,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload,
    const PlayRequestor& playRequestor) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ","
            "\"playRequestor\":{"
                "\"type\":\"" + playRequestor.type + "\","
                "\"id\":\"" + playRequestor.id + "\""
            "}"
        "}";
    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create a Play payload with only index, offsetInMilliseconds, playerId.
 *
 * @param index The index of the media item in the container, if the container is indexable.
 * @param offsetInMilliseconds The offset position within media item, in milliseconds.
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createPlayPayloadNoContext(
    int index,
    int64_t offsetInMilliseconds,
    const std::string& playerId,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ""
        "}";
    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create a Play payload with only playContext, index, offsetInMilliseconds.
 *
 * @param playContext Play context {Track/playlist/album/artist/station/podcast} identifier.
 * @param index The index of the media item in the container, if the container is indexable.
 * @param offsetInMilliseconds The offset position within media item, in milliseconds.
 * @return A string representation of the payload.
 */
static std::string createPlayPayloadNoPlayerId(
    const std::string& playContext,
    int index,
    int64_t offsetInMilliseconds,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"index\":\"" + std::to_string(index) + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ""
        "}";

    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create a Play payload with only playContext, offsetInMilliseconds and playerId.
 *
 * @param playContext Play context {Track/playlist/album/artist/station/podcast} identifier.
 * @param offsetInMilliseconds The offset position within media item, in milliseconds.
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createPlayPayloadNoIndex(
    const std::string& playContext,
    int64_t offsetInMilliseconds,
    const std::string& playerId,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":" + std::to_string(offsetInMilliseconds) + ","
            "\"playerId\":\"" + playerId + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ""
        "}";

    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create a Play payload with only playContext, index, playerId.
 *
 * @param playContext Play context {Track/playlist/album/artist/station/podcast} identifier.
 * @param index The index of the media item in the container, if the container is indexable.
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createPlayPayloadNoOffset(
    const std::string& playContext,
    int index,
    const std::string& playerId,
    const std::string& skillToken,
    const std::string& playbackSessionId,
    const std::string& navigation,
    bool preload) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\","
            "\"skillToken\":\"" + skillToken + "\","
            "\"playbackSessionId\":\"" + playbackSessionId + "\","
            "\"navigation\":\"" + navigation + "\","
            "\"preload\":" + (preload ? "true" : "false") + ""
        "}";

    // clang-format on

    return PLAY_PAYLOAD_TEST;
}

/**
 * Method to create a Login payload with accessToken, userName, refresh interval, forceLogin, playerId.
 *
 * @param accessToken The access context of the user identifier.
 * @param userName The userName of the user logging in.
 * @param refreshInterval The duration in milliseconds for which the accessToken is valid.
 * @param forceLogin bool which signifies if the adapter has to a force a login or merely cache the access token.
 * @param playerId The business name of the player.
 * @return A string representation of the payload.
 */
static std::string createLoginPayload(
    const std::string& accessToken,
    const std::string& userName,
    int64_t refreshInterval,
    bool forceLogin,
    const std::string& playerId) {
    // clang-format off
    const std::string LOGIN_PAYLOAD_TEST =
        "{"
            "\"playerId\":\"" + playerId + "\","
            "\"accessToken\":\"" + accessToken + "\","
            "\"tokenRefreshIntervalInMilliseconds\":" + std::to_string(refreshInterval) + ","
            "\"forceLogin\": true" + ","
            "\"username\":\"" + userName + "\""
        "}";

    // clang-format on

    return LOGIN_PAYLOAD_TEST;
}

/**
 * Method to create a Seek payload.
 *
 * @param timeOffset The offset to seek to.
 * @param playerId The business name of the player.
 * @param adjustSeek @c true offset adjusts seek relative to current position
 *                   @c false offset is interpreted as an absolute offset.
 * @return A string representation of the payload.
 */
static std::string createSeekPayload(int64_t timeOffset, const std::string& playerId, bool adjustSeek) {
    std::string SEEK_PAYLOAD_TEST;
    // clang-format off
    if (adjustSeek) {
        SEEK_PAYLOAD_TEST = "{\"playerId\":\"" + playerId + "\",\"deltaPositionMilliseconds\":" +
                             std::to_string(timeOffset) + "}";
    }
    else {
        SEEK_PAYLOAD_TEST = "{\"playerId\":\"" + playerId + "\",\"positionMilliseconds\":" +
                             std::to_string(timeOffset) + "}";
    }
    // clang-format on

    return SEEK_PAYLOAD_TEST;
}

class ExternalMediaPlayerTest : public ::testing::Test {
public:
    ExternalMediaPlayerTest();

    void SetUp() override;
    void TearDown() override;

    /**
     * Verify that the provided state matches the expected state
     *
     * @param jsonState The state to verify
     * @param expectedState The expected state
     */
    void verifyState(const std::string& providedState, const std::string& expectedState);

    /**
     * Helper method to authorize a set of players.
     *
     * @param payload The payload for an AuthorizeDiscoveredPlayers directive.
     * @param resultHandler A result handler associated with the directive upon which expectations can be set.
     */
    void sendAuthorizeDiscoveredPlayersDirective(
        const std::string& payload,
        std::unique_ptr<DirectiveHandlerResultInterface> resultHandler = nullptr,
        const std::string& messageId = MESSAGE_ID_TEST2);

    /**
     * This is invoked in response to a @c setState call.
     *
     * @return @c SUCCESS.
     */
    SetStateResult wakeOnSetState();

    /**
     * This is is invoked to clear the promise set by a setState call for repeat testing
     *
     * @return @c SUCCESS.
     */
    SetStateResult resetWakeOnSetState();

    /**
     * This is invoked in response to a setComplete call.
     *
     * @return @c SUCCESS.
     */
    void wakeOnSetComplete();

    /// The map of adapter creation functions.
    ExternalMediaPlayer::AdapterCreationMap m_adapterMap;

    /// @c ExternalMediaPlayer to test
    std::shared_ptr<ExternalMediaPlayer> m_externalMediaPlayer;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayer;

    /// @c SpeakerInterface to manage volume changes of individual speakers.
    std::shared_ptr<MockChannelVolumeInterface> m_mockSpeakerInterface;

    /// @c SpeakerManager to manage volume changes across speakers.
    std::shared_ptr<MockSpeakerManager> m_mockSpeakerManager;

    /// @c MetricRecorder to send metrics
    std::shared_ptr<MockMetricRecorder> m_metricRecorder;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// @c FocusManager to request focus to the DIALOG channel.
    std::shared_ptr<MockFocusManager> m_mockFocusManager;

    /// A directive handler result to send the result to.
    std::unique_ptr<MockDirectiveHandlerResult> m_mockDirectiveHandlerResult;

    /// A message sender used to send events to AVS.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    std::shared_ptr<certifiedSender::test::MockCertifiedSender> m_mockCertifiedSender;

    /// An exception sender used to send exception encountered events to AVS.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionSender;

    /// A playback router to notify when @c ExternalMediaPlayer becomes active.
    std::shared_ptr<MockPlaybackRouter> m_mockPlaybackRouter;

    /// An endpoint capabilities registrar with which to register the EMP.
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>
        m_mockEndpointCapabilitiesRegistrar;

    /// Object to notify the EMP when to start up.
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> m_startupNotifier;

    /// Object to notify the EMP when to shut down.
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> m_shutdownNotifier;

    /// Registrar for RenderPlayerInfoCardsProviders.
    std::shared_ptr<MockRenderPlayerInfoCardsProviderRegistrar> m_renderPlayerInfoCardsProviderRegistrar;

    /// The mock @c ApplicationAudioPipelineFactoryInterface.
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::test::MockApplicationAudioPipelineFactory>
        m_mockAudioPipelineFactory;

    /// Attachment manager used to create a reader.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Promise to be fulfilled when @c setState is called.
    std::promise<void> m_wakeSetStatePromise;

    /// Future to notify when @c setState is called.
    std::future<void> m_wakeSetStateFuture;

    /// Promise to be fulfilled when directive handling is complete.
    std::promise<void> m_wakeSetCompletePromise;

    /// Future to notify when directive handling is complete.
    std::future<void> m_wakeSetCompleteFuture;
};

ExternalMediaPlayerTest::ExternalMediaPlayerTest() :
        m_wakeSetStatePromise{},
        m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()},
        m_wakeSetCompletePromise{},
        m_wakeSetCompleteFuture{m_wakeSetCompletePromise.get_future()} {
}

void ExternalMediaPlayerTest::SetUp() {
    m_mockSpeakerInterface = std::make_shared<NiceMock<MockChannelVolumeInterface>>();
    m_mockSpeakerManager = std::make_shared<NiceMock<MockSpeakerManager>>();
    m_metricRecorder = std::make_shared<NiceMock<MockMetricRecorder>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_mockMediaPlayer = MockMediaPlayer::create();
    m_mockPlaybackRouter = std::make_shared<NiceMock<MockPlaybackRouter>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockCertifiedSender = std::make_shared<certifiedSender::test::MockCertifiedSender>();
    m_mockEndpointCapabilitiesRegistrar =
        std::make_shared<NiceMock<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>>();
    m_startupNotifier = std::make_shared<NiceMock<MockStartupNotifier>>();
    m_shutdownNotifier = std::make_shared<NiceMock<acsdkShutdownManagerInterfaces::test::MockShutdownNotifier>>();
    m_renderPlayerInfoCardsProviderRegistrar = std::make_shared<NiceMock<MockRenderPlayerInfoCardsProviderRegistrar>>();
    m_mockAudioPipelineFactory =
        std::make_shared<acsdkApplicationAudioPipelineFactoryInterfaces::test::MockApplicationAudioPipelineFactory>();

    EXPECT_CALL(
        *(m_mockAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(MSP1_LOCAL_PLAYER_ID + "MediaPlayer", _, _, _, _, _))
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_mockMediaPlayer, nullptr, nullptr, nullptr, m_mockSpeakerInterface)));

    EXPECT_CALL(
        *(m_mockEndpointCapabilitiesRegistrar.get()),
        withCapability(A<const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&>(), _))
        .WillOnce(ReturnRef(
            *(std::make_shared<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>()).get()));

    m_adapterMap[MSP1_LOCAL_PLAYER_ID] = &MockExternalMediaPlayerAdapter::getInstance;
    m_adapterMap.insert(std::make_pair(MSP1_LOCAL_PLAYER_ID, &MockExternalMediaPlayerAdapter::getInstance));

    auto annotatedFocusManager = acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
            m_mockFocusManager);
    auto annotatedCapabilitiesRegistrar = acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
        m_mockEndpointCapabilitiesRegistrar);

    EXPECT_CALL(*(m_renderPlayerInfoCardsProviderRegistrar.get()), registerProvider(_)).WillRepeatedly(Return(true));

    m_externalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        m_adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);

    m_externalMediaPlayer->startup();

    m_mockDirectiveHandlerResult = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    ASSERT_TRUE(m_externalMediaPlayer);

    // Authorize Players.
    std::promise<void> authorizationCompletePromise;
    std::future<void> authorizationCompleteFuture = authorizationCompletePromise.get_future();
    ON_CALL(*m_mockCertifiedSender->getMockMessageSender(), sendMessage(_))
        .WillByDefault(Invoke([](std::shared_ptr<avs::MessageRequest> request) {
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
        }));
    ON_CALL(*m_mockContextManager, getContext(_, _, _)).WillByDefault(InvokeWithoutArgs([this]() {
        m_externalMediaPlayer->onContextAvailable("");
        return 0;
    }));
    EXPECT_CALL(
        *m_mockMessageSender, sendMessage(EventNamed(AUTHORIZATION_COMPLETE.nameSpace, AUTHORIZATION_COMPLETE.name)))
        .Times(1)
        .WillOnce(Invoke([&authorizationCompletePromise](std::shared_ptr<MessageRequest> request) {
            authorizationCompletePromise.set_value();
        }));

    const std::string playersJson = createPlayerJson(MSP1_LOCAL_PLAYER_ID, true, MSP1_PLAYER_ID, MSP1_SKILLTOKEN);
    sendAuthorizeDiscoveredPlayersDirective(createAuthorizeDiscoveredPlayersPayload({playersJson}));

    ASSERT_TRUE(std::future_status::ready == authorizationCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

void ExternalMediaPlayerTest::TearDown() {
    if (MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter) {
        EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), doShutdown());
    }

    m_externalMediaPlayer->shutdown();
    m_mockMediaPlayer->shutdown();
    MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter.reset();
}

SetStateResult ExternalMediaPlayerTest::wakeOnSetState() {
    m_wakeSetStatePromise.set_value();
    return SetStateResult::SUCCESS;
}

SetStateResult ExternalMediaPlayerTest::resetWakeOnSetState() {
    m_wakeSetStatePromise = std::promise<void>();
    m_wakeSetStateFuture = m_wakeSetStatePromise.get_future();
    return SetStateResult::SUCCESS;
}

void ExternalMediaPlayerTest::wakeOnSetComplete() {
    m_wakeSetCompletePromise.set_value();
}

void ExternalMediaPlayerTest::verifyState(const std::string& providedState, const std::string& expectedState) {
    rapidjson::Document providedStateParsed;
    providedStateParsed.Parse(providedState);

    rapidjson::Document expectedStateParsed;
    expectedStateParsed.Parse(expectedState);

    EXPECT_EQ(providedStateParsed, expectedStateParsed);
}

/**
 * Test create() with nullptrs
 */
TEST_F(ExternalMediaPlayerTest, test_createWithNullPointers) {
    ExternalMediaPlayer::AdapterCreationMap adapterMap;
    auto annotatedFocusManager = acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
            m_mockFocusManager);
    auto annotatedCapabilitiesRegistrar = acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
        m_mockEndpointCapabilitiesRegistrar);

    auto testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        nullptr,
        m_mockCertifiedSender->get(),
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        nullptr,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        nullptr,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        m_mockContextManager,
        nullptr,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        m_mockContextManager,
        m_mockExceptionSender,
        nullptr,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        nullptr,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        nullptr,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayer(
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        nullptr,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        nullptr,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        nullptr,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        nullptr,
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        nullptr,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        nullptr,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        nullptr,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        nullptr,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        nullptr,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        nullptr,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        nullptr,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        nullptr);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);
}

/**
 * Test getConfiguration on an ExternalMediaPlayer. The operation succeeds.
 */
TEST_F(ExternalMediaPlayerTest, test_getConfiguration) {
    auto configuration = m_externalMediaPlayer->getConfiguration();
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    // TODO: ARC-227 Verify default values
    ASSERT_EQ(configuration[PLAY_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[AUTHORIZEDISCOVEREDPLAYERS_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[LOGIN_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[LOGOUT_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[RESUME_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[PAUSE_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[STOP_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[NEXT_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[PREVIOUS_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[STARTOVER_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[REWIND_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[FASTFORWARD_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[ENABLEREPEATONE_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[ENABLEREPEAT_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[DISABLEREPEAT_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[ENABLESHUFFLE_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[DISABLESHUFFLE_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[SEEK_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[ADJUSTSEEK_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[FAVORITE_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[UNFAVORITE_DIRECTIVE], neitherNonBlockingPolicy);
}

void ExternalMediaPlayerTest::sendAuthorizeDiscoveredPlayersDirective(
    const std::string& payload,
    std::unique_ptr<DirectiveHandlerResultInterface> resultHandler,
    const std::string& messageId) {
    if (!resultHandler) {
        resultHandler = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    }
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        AUTHORIZEDISCOVEREDPLAYERS_DIRECTIVE.nameSpace, AUTHORIZEDISCOVEREDPLAYERS_DIRECTIVE.name, messageId);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, payload, m_attachmentManager, "");

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(resultHandler));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(messageId);
}

/**
 * Test session state information on an ExternalMediaPlayer .
 */
TEST_F(ExternalMediaPlayerTest, test_callingProvideSessionState) {
    EXPECT_CALL(
        *(m_mockContextManager.get()), setState(SESSION_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(DoAll(
            // need to include all four arguments, but only care about jsonState
            Invoke([this](
                       const avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) {
                std::string agent = "";
                verifyState(jsonState, getIdleSessionStateJson(agent));
            }),
            InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState)));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));

    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}
/**
 * Test playback state information on an ExternalMediaPlayer.
 */
TEST_F(ExternalMediaPlayerTest, test_callingProvidePlaybackState) {
    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(PLAYBACK_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(DoAll(
            // need to include all four arguments, but only care about jsonState
            Invoke([this](
                       const avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) { verifyState(jsonState, IDLE_PLAYBACK_STATE); }),
            InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState)));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(createAdapterState()));

    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test payload with parse error in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, test_playParserError) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayloadWithParseError("XXX", 0, 0, "Adapter", "YYY", "ZZZ", "DEFAULT", false),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test PLAY payload without an adapter in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, test_playNoAdapter) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayload("XXX", 0, 0, "Adapter", "YYY", "ZZZ", "DEFAULT", false),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test PLAY payload without play context in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, test_playNoPlayContext) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayloadNoContext(0, 0, MSP1_PLAYER_ID, "YYY", "ZZZ", "DEFAULT", false),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test PLAY payload without playerId in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, test_playNoPlayerId) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayloadNoPlayerId("XXX", 0, 0, "YYY", "ZZZ", "DEFAULT", false),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test PLAY payload without offsetin ExternalMediaPlayer. This should succeed.
 */
TEST_F(ExternalMediaPlayerTest, test_playNoOffset) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayloadNoOffset("XXX", 0, MSP1_PLAYER_ID, "YYY", "ZZZ", "DEFAULT", false),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlay(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test PLAY payload with playRequestor in ExternalMediaPlayer. This should succeed.
 */
TEST_F(ExternalMediaPlayerTest, testPlaywithPlayRequestor) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayloadWithPlayRequestor(
            "XXX", 0, 0, MSP1_PLAYER_ID, "YYY", "ZZZ", "DEFAULT", false, testPlayRequestor),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlay(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test PLAY payload without index in ExternalMediaPlayer. This should succeed.
 */
TEST_F(ExternalMediaPlayerTest, test_playNoIndex) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createPlayPayloadNoIndex("XXX", 0, MSP1_PLAYER_ID, "YYY", "ZZZ", "DEFAULT", false),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlay(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful logout.
 */
TEST_F(ExternalMediaPlayerTest, test_logout) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        LOGOUT_DIRECTIVE.nameSpace, LOGOUT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleLogout());
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful login.
 */
TEST_F(ExternalMediaPlayerTest, test_login) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        LOGIN_DIRECTIVE.nameSpace, LOGIN_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createLoginPayload("XXX", "msploginuser", 1000, false, MSP1_PLAYER_ID),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleLogin(_, _, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test observers of session state are correctly notified
 */
TEST_F(ExternalMediaPlayerTest, test_loginStateChangeObserverIsNotified) {
    // add a mock observer
    auto observer = MockExternalMediaPlayerObserver::getInstance();
    m_externalMediaPlayer->addObserver(observer);

    EXPECT_CALL(
        *(m_mockContextManager.get()), setState(SESSION_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));

    ObservableSessionProperties observableSessionProperties{false, PLAYER_USER_NAME};
    EXPECT_CALL(*(observer), onLoginStateProvided(MSP1_PLAYER_ID, observableSessionProperties)).Times(1);

    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test observers of playback state are correctly notified
 */
TEST_F(ExternalMediaPlayerTest, test_playbackStateChangeObserverIsNotified) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    // add a mock observer
    auto observer = MockExternalMediaPlayerObserver::getInstance();
    m_externalMediaPlayer->addObserver(observer);

    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(PLAYBACK_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1);

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));

    ObservablePlaybackStateProperties observablePlaybackStateProperties{PLAYER_STATE, "", testPlayRequestor};
    EXPECT_CALL(*(observer), onPlaybackStateProvided(MSP1_PLAYER_ID, observablePlaybackStateProperties))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&promise] { promise.set_value(); }));

    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == future.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test that after removal login observers are not called anymore
 */
TEST_F(ExternalMediaPlayerTest, test_loginStateChangeObserverRemoval) {
    // add a mock observer
    auto observer = MockExternalMediaPlayerObserver::getInstance();
    m_externalMediaPlayer->addObserver(observer);

    EXPECT_CALL(
        *(m_mockContextManager.get()), setState(SESSION_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(2)
        .WillRepeatedly(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));
    EXPECT_CALL(*(observer), onLoginStateProvided(_, _)).Times(1);
    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    this->resetWakeOnSetState();

    m_externalMediaPlayer->removeObserver(observer);

    EXPECT_CALL(*(observer), onLoginStateProvided(_, _)).Times(0);
    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test that after removal playback state observers are not called anymore
 */
TEST_F(ExternalMediaPlayerTest, test_playbackStateChangeObserverRemoval) {
    // add a mock observer
    auto observer = MockExternalMediaPlayerObserver::getInstance();
    m_externalMediaPlayer->addObserver(observer);

    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(PLAYBACK_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(2)
        .WillRepeatedly(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));
    EXPECT_CALL(*(observer), onPlaybackStateProvided(_, _)).Times(1);
    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
    this->resetWakeOnSetState();

    m_externalMediaPlayer->removeObserver(observer);

    EXPECT_CALL(*(observer), onPlaybackStateProvided(_, _)).Times(0);
    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful resume.
 */
TEST_F(ExternalMediaPlayerTest, test_play) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        RESUME_DIRECTIVE.nameSpace, RESUME_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));
    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful pause.
 */
TEST_F(ExternalMediaPlayerTest, test_pause) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PAUSE_DIRECTIVE.nameSpace, PAUSE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful stop.
 */
TEST_F(ExternalMediaPlayerTest, testStop) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        STOP_DIRECTIVE.nameSpace, STOP_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful stop.
 */
TEST_F(ExternalMediaPlayerTest, test_stop) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        STOP_DIRECTIVE.nameSpace, STOP_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful next.
 */
TEST_F(ExternalMediaPlayerTest, test_next) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NEXT_DIRECTIVE.nameSpace, NEXT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful previous.
 */
TEST_F(ExternalMediaPlayerTest, test_previous) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PREVIOUS_DIRECTIVE.nameSpace, PREVIOUS_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful StarOver.
 */
TEST_F(ExternalMediaPlayerTest, test_startOver) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        STARTOVER_DIRECTIVE.nameSpace, STARTOVER_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful rewind.
 */
TEST_F(ExternalMediaPlayerTest, test_rewind) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        REWIND_DIRECTIVE.nameSpace, REWIND_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful fast-forward.
 */
TEST_F(ExternalMediaPlayerTest, test_fastForward) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        FASTFORWARD_DIRECTIVE.nameSpace, FASTFORWARD_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful EnableRepeatOne.
 */
TEST_F(ExternalMediaPlayerTest, test_enableRepeatOne) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ENABLEREPEATONE_DIRECTIVE.nameSpace, ENABLEREPEATONE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful EnableRepeat.
 */
TEST_F(ExternalMediaPlayerTest, test_enableRepeat) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ENABLEREPEAT_DIRECTIVE.nameSpace, ENABLEREPEAT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful DisableRepeat.
 */
TEST_F(ExternalMediaPlayerTest, test_disableRepeat) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        DISABLEREPEAT_DIRECTIVE.nameSpace, DISABLEREPEAT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful EnableShuffle.
 */
TEST_F(ExternalMediaPlayerTest, test_enableShuffle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ENABLESHUFFLE_DIRECTIVE.nameSpace, ENABLESHUFFLE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful DisableRepeat.
 */
TEST_F(ExternalMediaPlayerTest, test_disableShuffle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        DISABLESHUFFLE_DIRECTIVE.nameSpace, DISABLESHUFFLE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful Favorite.
 */
TEST_F(ExternalMediaPlayerTest, test_favorite) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        FAVORITE_DIRECTIVE.nameSpace, FAVORITE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful UnFavorite.
 */
TEST_F(ExternalMediaPlayerTest, test_unfavorite) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        UNFAVORITE_DIRECTIVE.nameSpace, UNFAVORITE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test incorrect directive.
 */
TEST_F(ExternalMediaPlayerTest, test_incorrectDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        FAVORITE_DIRECTIVE.nameSpace, PREVIOUS_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP1_PLAYER_ID), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test Seek failure passing incorrect field in payload.
 */
TEST_F(ExternalMediaPlayerTest, test_seekFailure) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        SEEK_DIRECTIVE.nameSpace, SEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(100, MSP1_PLAYER_ID, true), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful Seek.
 */
TEST_F(ExternalMediaPlayerTest, test_seekSuccess) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        SEEK_DIRECTIVE.nameSpace, SEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(100, MSP1_PLAYER_ID, false), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleSeek(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test  AdjustSeek failure incorrect field in payload.
 */
TEST_F(ExternalMediaPlayerTest, test_adjustSeekFailure) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ADJUSTSEEK_DIRECTIVE.nameSpace, ADJUSTSEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(100, MSP1_PLAYER_ID, false), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test  AdjustSeek failure passing in an incorrect offset.
 */
TEST_F(ExternalMediaPlayerTest, test_adjustSeekFailure2) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ADJUSTSEEK_DIRECTIVE.nameSpace, ADJUSTSEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(86400014, MSP1_PLAYER_ID, true), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test  AdjustSeek successful passing in correct payload and offset.
 */
TEST_F(ExternalMediaPlayerTest, test_adjustSeekSuccess) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ADJUSTSEEK_DIRECTIVE.nameSpace, ADJUSTSEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(86400000, MSP1_PLAYER_ID, true), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleAdjustSeek(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted())
        .Times(1)
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetComplete));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);

    ASSERT_TRUE(std::future_status::ready == m_wakeSetCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Checks that the AuthorizationComplete event contains the expected authorized and deauthorized fields.
 *
 * @param request The AuthorizationComplete event sent.
 * @param expectedAuthorized The expected list of @c playerId to @c skillToken pairs to verify against.
 * @param expectedDeauthorized The expected list of @c deauthorized localPlayerId the verify against.
 */
static void verifyAuthorizationCompletePayload(
    std::shared_ptr<MessageRequest> request,
    std::unordered_map<std::string, std::string> expectedAuthorized,
    std::unordered_set<std::string> expectedDeauthorized = std::unordered_set<std::string>()) {
    rapidjson::Document document;
    ParseResult result = document.Parse(request->getJsonContent());

    if (!result) {
        FAIL();
    }

    rapidjson::Value::ConstMemberIterator eventIt;
    rapidjson::Value::ConstMemberIterator payloadIt;
    rapidjson::Value::ConstMemberIterator authorizedIt;
    rapidjson::Value::ConstMemberIterator deauthorizedIt;

    if (!findNode(document, "event", &eventIt) || !findNode(eventIt->value, "payload", &payloadIt) ||
        !findNode(payloadIt->value, "authorized", &authorizedIt) ||
        !findNode(payloadIt->value, "deauthorized", &deauthorizedIt)) {
        FAIL();
    }

    std::unordered_map<std::string, std::string> authorized;
    std::unordered_set<std::string> deauthorized;

    for (rapidjson::Value::ConstValueIterator it = authorizedIt->value.Begin(); it != authorizedIt->value.End(); it++) {
        std::string playerId;
        std::string skillToken;

        if (!retrieveValue(*it, "playerId", &playerId) || !retrieveValue(*it, "skillToken", &skillToken)) {
            FAIL();
        }

        authorized[playerId] = skillToken;
    }

    for (rapidjson::Value::ConstValueIterator it = deauthorizedIt->value.Begin(); it != deauthorizedIt->value.End();
         it++) {
        std::string localPlayerId;

        if (!retrieveValue(*it, "localPlayerId", &localPlayerId)) {
            FAIL();
        }

        deauthorized.insert(localPlayerId);
    }

    ASSERT_THAT(authorized, ContainerEq(expectedAuthorized));
    ASSERT_THAT(deauthorized, ContainerEq(expectedDeauthorized));
}

/**
 * Test that ReportDiscoveredPlayers is sent.
 */
TEST_F(ExternalMediaPlayerTest, testReportDiscoveredPlayers) {
    std::promise<void> eventPromise;
    std::future<void> eventFuture = eventPromise.get_future();

    auto annotatedFocusManager = acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
            m_mockFocusManager);
    auto annotatedCapabilitiesRegistrar = acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
        m_mockEndpointCapabilitiesRegistrar);

    // Initialize the CertifiedSender
    auto mockCertifiedSender = std::make_shared<certifiedSender::test::MockCertifiedSender>();
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> connectionObserver =
        std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(
            mockCertifiedSender->get());
    connectionObserver->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    // Set expectation on m_mockCertifiedSender's @c MessageSenderInterface
    // because @c MockCertifiedSender isn't a gmock mock.
    EXPECT_CALL(
        *(mockCertifiedSender->getMockMessageSender()),
        sendMessage(EventNamed(REPORT_DISCOVERED_PLAYERS.nameSpace, REPORT_DISCOVERED_PLAYERS.name)))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&eventPromise]() { eventPromise.set_value(); }));

    EXPECT_CALL(*(m_renderPlayerInfoCardsProviderRegistrar.get()), registerProvider(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(
        *(m_mockAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(MSP1_LOCAL_PLAYER_ID + "MediaPlayer", _, _, _, _, _))
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_mockMediaPlayer, nullptr, nullptr, nullptr, m_mockSpeakerInterface)));

    EXPECT_CALL(
        *(m_mockEndpointCapabilitiesRegistrar.get()),
        withCapability(A<const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&>(), _))
        .WillOnce(ReturnRef(
            *(std::make_shared<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>()).get()));

    m_externalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        m_adapterMap,
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);

    m_externalMediaPlayer->startup();

    ASSERT_TRUE(std::future_status::ready == eventFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful AuthorizeDiscoveredPlayers directive processing.
 */
TEST_F(ExternalMediaPlayerTest, testTimer_AuthorizeDiscoveredPlayersSuccess) {
    auto annotatedFocusManager = acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
            m_mockFocusManager);
    auto annotatedCapabilitiesRegistrar = acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
        m_mockEndpointCapabilitiesRegistrar);

    std::promise<void> authorizationCompletePromise;
    std::future<void> authorizationCompleteFuture = authorizationCompletePromise.get_future();

    ON_CALL(*m_mockContextManager, getContext(_, _, _)).WillByDefault(InvokeWithoutArgs([this]() {
        m_externalMediaPlayer->onContextAvailable("");
        return 0;
    }));

    // Use another instance to avoid SetUp() interferring with the test.
    auto messageSender = std::make_shared<NiceMock<MockMessageSender>>();

    EXPECT_CALL(*(m_renderPlayerInfoCardsProviderRegistrar.get()), registerProvider(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(
        *(m_mockAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(MSP1_LOCAL_PLAYER_ID + "MediaPlayer", _, _, _, _, _))
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_mockMediaPlayer, nullptr, nullptr, nullptr, m_mockSpeakerInterface)));

    EXPECT_CALL(
        *(m_mockEndpointCapabilitiesRegistrar.get()),
        withCapability(A<const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&>(), _))
        .WillOnce(ReturnRef(
            *(std::make_shared<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>()).get()));

    m_externalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        m_adapterMap,
        m_mockAudioPipelineFactory,
        messageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);

    m_externalMediaPlayer->startup();

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());
    EXPECT_CALL(*messageSender, sendMessage(EventNamed(AUTHORIZATION_COMPLETE.nameSpace, AUTHORIZATION_COMPLETE.name)))
        .Times(1)
        .WillOnce(Invoke([&authorizationCompletePromise](std::shared_ptr<MessageRequest> request) {
            verifyAuthorizationCompletePayload(request, {{MSP1_PLAYER_ID, MSP1_SKILLTOKEN}});
            authorizationCompletePromise.set_value();
        }));
    EXPECT_CALL(
        *(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter),
        handleAuthorized(true, MSP1_PLAYER_ID, MSP1_SKILLTOKEN));

    const std::string playersJson = createPlayerJson(MSP1_LOCAL_PLAYER_ID, true, MSP1_PLAYER_ID, MSP1_SKILLTOKEN);
    sendAuthorizeDiscoveredPlayersDirective(
        createAuthorizeDiscoveredPlayersPayload({playersJson}), std::move(m_mockDirectiveHandlerResult));

    ASSERT_TRUE(std::future_status::ready == authorizationCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test successful AuthorizeDiscoveredPlayers directive processing of multiple directives.
 */
TEST_F(ExternalMediaPlayerTest, testMultipleAuthorizeDiscoveredPlayersSuccess) {
    auto annotatedFocusManager = acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
            m_mockFocusManager);
    auto annotatedCapabilitiesRegistrar = acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
        m_mockEndpointCapabilitiesRegistrar);

    ON_CALL(*m_mockContextManager, getContext(_, _, _)).WillByDefault(InvokeWithoutArgs([this]() {
        m_externalMediaPlayer->onContextAvailable("");
        return 0;
    }));

    // Use another instance to avoid SetUp() interferring with the test.
    auto messageSender = std::make_shared<NiceMock<MockMessageSender>>();

    EXPECT_CALL(*(m_renderPlayerInfoCardsProviderRegistrar.get()), registerProvider(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(
        *(m_mockAudioPipelineFactory.get()),
        createApplicationMediaInterfaces(MSP1_LOCAL_PLAYER_ID + "MediaPlayer", _, _, _, _, _))
        .WillOnce(Return(std::make_shared<avsCommon::sdkInterfaces::ApplicationMediaInterfaces>(
            m_mockMediaPlayer, nullptr, nullptr, nullptr, m_mockSpeakerInterface)));

    EXPECT_CALL(
        *(m_mockEndpointCapabilitiesRegistrar.get()),
        withCapability(A<const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&>(), _))
        .WillOnce(ReturnRef(
            *(std::make_shared<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>()).get()));

    m_externalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        m_adapterMap,
        m_mockAudioPipelineFactory,
        messageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);

    m_externalMediaPlayer->startup();

    std::promise<void> authorizationCompletePromise;
    std::future<void> authorizationCompleteFuture = authorizationCompletePromise.get_future();
    auto mockDirectiveHandlerResult = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    EXPECT_CALL(*mockDirectiveHandlerResult, setCompleted());
    EXPECT_CALL(
        *(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter),
        handleAuthorized(true, MSP1_PLAYER_ID, MSP1_SKILLTOKEN));

    auto mockDirectiveHandlerResult2 = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    EXPECT_CALL(*mockDirectiveHandlerResult2, setCompleted());

    auto mockDirectiveHandlerResult3 = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    EXPECT_CALL(*mockDirectiveHandlerResult3, setCompleted());
    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleAuthorized(false, "", ""));

    {
        InSequence s;
        EXPECT_CALL(
            *messageSender, sendMessage(EventNamed(AUTHORIZATION_COMPLETE.nameSpace, AUTHORIZATION_COMPLETE.name)))
            .Times(1)
            .WillOnce(Invoke([](std::shared_ptr<MessageRequest> request) {
                verifyAuthorizationCompletePayload(request, {{MSP1_PLAYER_ID, MSP1_SKILLTOKEN}});
            }));

        EXPECT_CALL(
            *messageSender, sendMessage(EventNamed(AUTHORIZATION_COMPLETE.nameSpace, AUTHORIZATION_COMPLETE.name)))
            .Times(1)
            .WillOnce(Invoke(
                [](std::shared_ptr<MessageRequest> request) { verifyAuthorizationCompletePayload(request, {}, {}); }));

        EXPECT_CALL(
            *messageSender, sendMessage(EventNamed(AUTHORIZATION_COMPLETE.nameSpace, AUTHORIZATION_COMPLETE.name)))
            .Times(1)
            .WillOnce(Invoke([&authorizationCompletePromise](std::shared_ptr<MessageRequest> request) {
                verifyAuthorizationCompletePayload(
                    request, std::unordered_map<std::string, std::string>(), {MSP1_LOCAL_PLAYER_ID});
                authorizationCompletePromise.set_value();
            }));
    }

    // Authorize first player
    const std::string playersJson = createPlayerJson(MSP1_LOCAL_PLAYER_ID, true, MSP1_PLAYER_ID, MSP1_SKILLTOKEN);
    sendAuthorizeDiscoveredPlayersDirective(
        createAuthorizeDiscoveredPlayersPayload({playersJson}), std::move(mockDirectiveHandlerResult), "authMessage1");

    // No change - should not result in a authorization complete message
    sendAuthorizeDiscoveredPlayersDirective(
        createAuthorizeDiscoveredPlayersPayload({}), std::move(mockDirectiveHandlerResult2), "authMessage2");

    // Deauthorize player
    const std::string playersJson2 = createPlayerJson(MSP1_LOCAL_PLAYER_ID, false, MSP1_PLAYER_ID, MSP1_SKILLTOKEN);
    sendAuthorizeDiscoveredPlayersDirective(
        createAuthorizeDiscoveredPlayersPayload({playersJson2}),
        std::move(mockDirectiveHandlerResult3),
        "authMessage3");

    ASSERT_TRUE(std::future_status::ready == authorizationCompleteFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test setPlayerInFocus succeeds for authorized players.
 */
TEST_F(ExternalMediaPlayerTest, testSetPlayerInFocusSucceedsForAuthorized) {
    EXPECT_CALL(*m_mockContextManager, setState(SESSION_STATE, _, _, _))
        .WillOnce(Invoke([this](
                             const avs::NamespaceAndName& namespaceAndName,
                             const std::string& jsonState,
                             const avs::StateRefreshPolicy& refreshPolicy,
                             const unsigned int stateRequestToken) {
            rapidjson::Document document;
            ParseResult result = document.Parse(jsonState);
            if (!result) {
                return SetStateResult::SUCCESS;
            }

            std::string playerInFocus;
            if (!retrieveValue(document, "playerInFocus", &playerInFocus)) {
                return SetStateResult::SUCCESS;
            }

            if (MSP1_PLAYER_ID == playerInFocus) {
                wakeOnSetState();
            }

            return SetStateResult::SUCCESS;
        }));

    // Authorized from SetUp().
    m_externalMediaPlayer->setPlayerInFocus(MSP1_PLAYER_ID);
    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test setPlayerInFocus fails for unauthorized players.
 */
TEST_F(ExternalMediaPlayerTest, testSetPlayerInFocusFailsForAuthorized) {
    const std::string INVALID_ID = "invalidPlayerId";

    EXPECT_CALL(*m_mockPlaybackRouter, setHandler(_)).Times(0);
    EXPECT_CALL(*m_mockContextManager, setState(_, Not(HasSubstr(INVALID_ID)), _, _))
        .WillOnce(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState));

    m_externalMediaPlayer->setPlayerInFocus(INVALID_ID);
    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test setPlayerInFocus notifies any RenderPlayerInfoCardsObservers.
 */
TEST_F(ExternalMediaPlayerTest, testSetPlayerInFocusNotfiesTemplateRuntimeObserver) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    auto renderCardObserver = std::make_shared<MockRenderPlayerInfoCardsObserver>();
    m_externalMediaPlayer->setObserver(renderCardObserver);

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));
    EXPECT_CALL(*renderCardObserver, onRenderPlayerCardsInfoChanged(_, _))
        .WillOnce(Invoke([&promise](
                             avsCommon::avs::PlayerActivity state,
                             const RenderPlayerInfoCardsObserverInterface::Context& context) { promise.set_value(); }));

    // Authorized from SetUp().
    m_externalMediaPlayer->setPlayerInFocus(MSP1_PLAYER_ID);
    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);

    ASSERT_TRUE(std::future_status::ready == future.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test playerId is the same in sessionState and playbackState on handleAuthorized success.
 */
TEST_F(ExternalMediaPlayerTest, testSamePlayerIdWhenHandleAuthorizedSuccess) {
    std::promise<std::string> playbackStatePromise;
    std::future<std::string> playbackStateFuture = playbackStatePromise.get_future();

    std::promise<std::string> sessionStatePromise;
    std::future<std::string> sessionStateFuture = sessionStatePromise.get_future();

    EXPECT_CALL(*(m_mockContextManager.get()), setState(PLAYBACK_STATE, _, _, _))
        .Times(1)
        .WillOnce(
            // need to include all four arguments, but only care about jsonState
            Invoke([&playbackStatePromise](
                       const avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) {
                playbackStatePromise.set_value(jsonState);
                return SetStateResult::SUCCESS;
            }));

    EXPECT_CALL(*(m_mockContextManager.get()), setState(SESSION_STATE, _, _, _))
        .Times(1)
        .WillOnce(
            // need to include all four arguments, but only care about jsonState
            Invoke([&sessionStatePromise](
                       const avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) {
                sessionStatePromise.set_value(jsonState);
                return SetStateResult::SUCCESS;
            }));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(createAdapterState()));

    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == playbackStateFuture.wait_for(MY_WAIT_TIMEOUT));
    ASSERT_TRUE(std::future_status::ready == sessionStateFuture.wait_for(MY_WAIT_TIMEOUT));

    rapidjson::Document sessionStateParsed;
    sessionStateParsed.Parse(sessionStateFuture.get());

    rapidjson::Document playbackStateParsed;
    playbackStateParsed.Parse(playbackStateFuture.get());

    std::string sessionStatePlayer =
        sessionStateParsed.GetObject()["players"].GetArray()[0].GetObject()["playerId"].GetString();
    std::string playbackStatePlayer =
        playbackStateParsed.GetObject()["players"].GetArray()[0].GetObject()["playerId"].GetString();

    EXPECT_EQ(sessionStatePlayer, playbackStatePlayer);
}

/**
 * Test adding an adapter handler and discovering it
 */
TEST_F(ExternalMediaPlayerTest, testAddAdapterHandlerDiscovery) {
    std::promise<void> eventPromise;
    std::future<void> eventFuture = eventPromise.get_future();
    auto mockAdapterHandler = std::make_shared<MockExternalMediaAdapterHandler>();
    mockAdapterHandler->setExternalMediaPlayer(m_externalMediaPlayer);
    m_externalMediaPlayer->addAdapterHandler(mockAdapterHandler);

    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> connectionObserver =
        std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(
            m_mockCertifiedSender->get());
    connectionObserver->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    // Set expectation on m_mockCertifiedSender's @c MessageSenderInterface
    // because @c MockCertifiedSender isn't a gmock mock.
    EXPECT_CALL(
        *(m_mockCertifiedSender->getMockMessageSender()),
        sendMessage(EventNamed(REPORT_DISCOVERED_PLAYERS.nameSpace, REPORT_DISCOVERED_PLAYERS.name)))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&eventPromise]() { eventPromise.set_value(); }));

    mockAdapterHandler->reportMockPlayers(MSP2_LOCAL_PLAYER_ID);

    ASSERT_TRUE(std::future_status::ready == eventFuture.wait_for(MY_WAIT_TIMEOUT));
}

/**
 * Test sending ReportDiscoveredPlayers aggregates adapters until startup.
 */
TEST_F(ExternalMediaPlayerTest, testAddAdapterHandlerDiscoveryAggregatedUntilStartup) {
    auto annotatedFocusManager = acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
            m_mockFocusManager);
    auto annotatedCapabilitiesRegistrar = acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>(
        m_mockEndpointCapabilitiesRegistrar);

    std::promise<void> eventPromise;
    std::future<void> eventFuture = eventPromise.get_future();

    EXPECT_CALL(
        *(m_mockEndpointCapabilitiesRegistrar.get()),
        withCapability(A<const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>&>(), _))
        .WillOnce(ReturnRef(
            *(std::make_shared<avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar>()).get()));

    auto externalMediaPlayer = ExternalMediaPlayer::createExternalMediaPlayerWithAdapters(
        {},
        m_mockAudioPipelineFactory,
        m_mockMessageSender,
        m_mockCertifiedSender->get(),
        annotatedFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter,
        annotatedCapabilitiesRegistrar,
        m_shutdownNotifier,
        m_startupNotifier,
        m_renderPlayerInfoCardsProviderRegistrar,
        m_metricRecorder,
        m_mockSpeakerManager);

    auto mockAdapterHandler = std::make_shared<MockExternalMediaAdapterHandler>();
    mockAdapterHandler->setExternalMediaPlayer(externalMediaPlayer);
    m_externalMediaPlayer->addAdapterHandler(mockAdapterHandler);

    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> connectionObserver =
        std::static_pointer_cast<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>(
            m_mockCertifiedSender->get());
    connectionObserver->onConnectionStatusChanged(
        ConnectionStatusObserverInterface::Status::CONNECTED,
        ConnectionStatusObserverInterface::ChangedReason::SUCCESS);

    // No message sent here because startup hasn't been called yet ...
    mockAdapterHandler->reportMockPlayers(MSP2_LOCAL_PLAYER_ID);

    // Add a second adapter.
    auto secondMockAdapterHandler = std::make_shared<MockExternalMediaAdapterHandler>();
    secondMockAdapterHandler->setExternalMediaPlayer(m_externalMediaPlayer);
    m_externalMediaPlayer->addAdapterHandler(secondMockAdapterHandler);

    secondMockAdapterHandler->reportMockPlayers("SECOND_PLAYER_ID");

    // Only one message should be sent for both adapters on startup.
    EXPECT_CALL(
        *(m_mockCertifiedSender->getMockMessageSender()),
        sendMessage(EventNamed(REPORT_DISCOVERED_PLAYERS.nameSpace, REPORT_DISCOVERED_PLAYERS.name)))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&eventPromise]() { eventPromise.set_value(); }));
    externalMediaPlayer->startup();

    ASSERT_TRUE(std::future_status::ready == eventFuture.wait_for(MY_WAIT_TIMEOUT));
}

}  // namespace test
}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
