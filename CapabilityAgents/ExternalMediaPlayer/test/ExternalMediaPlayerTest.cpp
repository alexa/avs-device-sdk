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

/// @file ExternalMediaPlayerTest.cpp

#include <chrono>
#include <future>
#include <map>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveSequencer.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/SDKInterfaces/MockFocusManager.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockPlaybackRouter.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/MockSpeakerManager.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/ConsoleLogger.h>
#include <AVSCommon/Utils/MediaPlayer/MockMediaPlayer.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "ExternalMediaPlayer/ExternalMediaPlayer.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace externalMediaPlayer {
namespace test {

using namespace avsCommon::utils;
using namespace avsCommon::utils::json;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::externalMediaPlayer;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils::mediaPlayer;
using namespace avsCommon::utils::memory;
using namespace avsCommon::utils::mediaPlayer::test;
using namespace capabilityAgents::externalMediaPlayer;

using namespace ::testing;
using namespace rapidjson;

/// Provide State Token for testing.
static const unsigned int PROVIDE_STATE_TOKEN_TEST{1};

/// Plenty of time for a test to complete.
static std::chrono::milliseconds WAIT_TIMEOUT(1000);

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
static const std::string PLAYER_ID = "testPlayerId";
static const std::string PLAYER_TRACK = "testTrack";
static const std::string PLAYER_STATE = "IDLE";

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

// clang-format off
static const std::string IDLE_SESSION_STATE =
    "{"
        "\"playerInFocus\":\"\","
        "\"players\":[{"
            "\"playerId\":\"\","
            "\"endpointId\":\"\","
            "\"loggedIn\":false,"
            "\"username\":\"\","
            "\"isGuest\":false,"
            "\"launched\":false,"
           "\"active\":false}"
    "]}";

static const std::string IDLE_PLAYBACK_STATE =
    "{"
        "\"state\":\"IDLE\","
        "\"supportedOperations\":[],"
        "\"shuffle\":\"NOT_SHUFFLED\","
        "\"repeat\":\"NOT_REPEATED\","
        "\"favorite\":\"NOT_RATED\","
        "\"positionMilliseconds\":0,"
        "\"uncertaintyInMilliseconds\":0,"
        "\"players\":[{"
            "\"playerId\":\"\","
            "\"state\":\"IDLE\","
            "\"supportedOperations\":[],"
            "\"positionMilliseconds\":0,"
            "\"shuffle\":\"NOT_SHUFFLED\","
            "\"repeat\":\"NOT_REPEATED\","
            "\"favorite\":\"NOT_RATED\","
            "\"media\":{"
                "\"type\":\"\","
                "\"value\":{"
                    "\"playbackSource\":\"\","
                    "\"playbackSourceId\":\"\","
                    "\"trackName\":\"\","
                    "\"trackId\":\"\","
                    "\"trackNumber\":\"\","
                    "\"artist\":\"\","
                    "\"artistId\":\"\","
                    "\"album\":\"\","
                    "\"albumId\":\"\","
                    "\"coverUrls\":{\"tiny\":\"\",\"small\":\"\",\"medium\":\"\",\"large\":\"\"},"
                    "\"coverId\":\"\","
                    "\"mediaProvider\":\"\","
                    "\"mediaType\":\"TRACK\","
                    "\"durationInMilliseconds\":0"
                "}"
            "}"
        "}]"
    "}";
// clang-format on

/**
 * Method to create an adapter state struct response to getState();
 *
 * @return an adpater state partially filled with test values
 */
static AdapterState createAdapterState() {
    AdapterSessionState sessionState;
    sessionState.loggedIn = false;
    sessionState.userName = PLAYER_USER_NAME;
    sessionState.playerId = PLAYER_ID;

    AdapterPlaybackState playbackState;
    playbackState.playerId = PLAYER_ID;
    playbackState.state = PLAYER_STATE;
    playbackState.trackName = PLAYER_TRACK;

    AdapterState adapterState;
    adapterState.sessionState = sessionState;
    adapterState.playbackState = playbackState;
    return adapterState;
}

/// Message Id for testing.
static const std::string MESSAGE_ID_TEST("MessageId_Test");

/// Dialog Request Id for testing.
static const std::string DIALOG_REQUEST_ID_TEST("DialogId_Test");

/// String to identify log entries originating from this file.
static const std::string TAG("ExternalMediaPlayerTest");

/// Music service provider id 1.
static const std::string MSP_NAME1("MSP_PROVIDER1");

/// Music service provider id 2.
static const std::string MSP_NAME2("MSP_PROVIDER2");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Mock class of ExternalMediaAdapterInterface.
class MockExternalMediaPlayerAdapter : public ExternalMediaAdapterInterface {
public:
    /*
     * Method that adheres to the AdapterCreateFunc interface to create an adapter. This method create a mock
     * instances and assigns it to a class static to keep the mock class simple.
     *
     * @param mediaPlayer The mediaPlayer instance to be used to play Spotify content.
     * @param speakerManager A @c SpeakerManagerInterface to perform volume changes requested by ESDK.
     * @param messageSender The object to use for sending events.
     * @param focusManager The focusManager used to acquire/release channel.
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param externalMediaPlayer The instance of the @c ExternalMediaPlayer managing the adapter.
     * @return A @c std::shared_ptr to the new @c ExternalMediaAdapter instance.
     */
    static std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaAdapterInterface> getInstance(
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExternalMediaPlayerInterface> externalMediaPlayer);

    static std::shared_ptr<MockExternalMediaPlayerAdapter> m_currentActiveMediaPlayerAdapter;

    MOCK_METHOD0(doShutdown, void());
    MOCK_METHOD0(init, void());
    MOCK_METHOD0(deInit, void());
    MOCK_METHOD4(
        handleLogin,
        void(
            const std::string& accessToken,
            const std::string& userName,
            bool forceLogin,
            std::chrono::milliseconds tokenRefreshInterval));
    MOCK_METHOD0(handleLogout, void());
    MOCK_METHOD3(handlePlay, void(std::string& playContextToken, int64_t index, std::chrono::milliseconds offset));
    MOCK_METHOD1(handlePlayControl, void(RequestType requestType));
    MOCK_METHOD1(handleSeek, void(std::chrono::milliseconds offset));
    MOCK_METHOD1(handleAdjustSeek, void(std::chrono::milliseconds deltaOffset));
    MOCK_METHOD1(handleSetVolume, void(int8_t volume));
    MOCK_METHOD1(handleSetMute, void(bool));
    MOCK_METHOD0(getState, AdapterState());

private:
    /// MockExternalMediaPlayerAdapter private constructor.
    MockExternalMediaPlayerAdapter();
};

/// Static instance of MockMediaPlayerAdapter.
std::shared_ptr<MockExternalMediaPlayerAdapter> MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter;

MockExternalMediaPlayerAdapter::MockExternalMediaPlayerAdapter() :
        ExternalMediaAdapterInterface("MockExternalMediaPlayerAdapter") {
}

std::shared_ptr<avsCommon::sdkInterfaces::externalMediaPlayer::ExternalMediaAdapterInterface>
MockExternalMediaPlayerAdapter::getInstance(
    std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExternalMediaPlayerInterface> externalMediaPlayer) {
    MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter =
        std::shared_ptr<MockExternalMediaPlayerAdapter>(new MockExternalMediaPlayerAdapter());
    return MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter;
}

class MockExternalMediaPlayerObserver : public ExternalMediaPlayerObserverInterface {
public:
    static std::shared_ptr<MockExternalMediaPlayerObserver> getInstance();
    MOCK_METHOD2(
        onLoginStateProvided,
        void(const std::string&, const avsCommon::sdkInterfaces::externalMediaPlayer::ObservableSessionProperties));
    MOCK_METHOD2(
        onPlaybackStateProvided,
        void(
            const std::string&,
            const avsCommon::sdkInterfaces::externalMediaPlayer::ObservablePlaybackStateProperties));

private:
    /**
     * Constructor
     */
    MockExternalMediaPlayerObserver();
};

std::shared_ptr<MockExternalMediaPlayerObserver> MockExternalMediaPlayerObserver::getInstance() {
    return std::shared_ptr<MockExternalMediaPlayerObserver>(new MockExternalMediaPlayerObserver());
}

MockExternalMediaPlayerObserver::MockExternalMediaPlayerObserver() {
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
    const std::string& playerId) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\","
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
    const std::string& playerId) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\""
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
static std::string createPlayPayloadNoContext(int index, int64_t offsetInMilliseconds, const std::string& playerId) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\""
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
    int64_t offsetInMilliseconds) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":\"" + std::to_string(offsetInMilliseconds) + "\","
            "\"index\":\"" + std::to_string(index) + "\""
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
    const std::string& playerId) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"offsetInMilliseconds\":" + std::to_string(offsetInMilliseconds) + ","
            "\"playerId\":\"" + playerId + "\""
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
static std::string createPlayPayloadNoOffset(const std::string& playContext, int index, const std::string& playerId) {
    // clang-format off
    const std::string PLAY_PAYLOAD_TEST =
        "{"
            "\"playbackContextToken\":\"" + playContext + "\","
            "\"playerId\":\"" + playerId + "\","
            "\"index\":\"" + std::to_string(index) + "\""
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

    /// @c ExternalMediaPlayer to test
    std::shared_ptr<ExternalMediaPlayer> m_externalMediaPlayer;

    /// Player to send the audio to.
    std::shared_ptr<MockMediaPlayer> m_mockMediaPlayer;

    /// @c SpeakerInterface to manage volume changes of individual speakers.
    std::shared_ptr<MockSpeakerInterface> m_mockSpeakerInterface;

    /// @c SpeakerManager to manage volume changes across speakers.
    std::shared_ptr<MockSpeakerManager> m_mockSpeakerManager;

    /// @c ContextManager to provide state and update state.
    std::shared_ptr<MockContextManager> m_mockContextManager;

    /// @c FocusManager to request focus to the DIALOG channel.
    std::shared_ptr<MockFocusManager> m_mockFocusManager;

    /// A directive handler result to send the result to.
    std::unique_ptr<MockDirectiveHandlerResult> m_mockDirectiveHandlerResult;

    /// A message sender used to send events to AVS.
    std::shared_ptr<MockMessageSender> m_mockMessageSender;

    /// An exception sender used to send exception encountered events to AVS.
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionSender;

    /// A playback router to notify when @c ExternalMediaPlayer becomes active.
    std::shared_ptr<MockPlaybackRouter> m_mockPlaybackRouter;

    /// Attachment manager used to create a reader.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// Promise to be fulfilled when @c setState is called.
    std::promise<void> m_wakeSetStatePromise;

    /// Future to notify when @c setState is called.
    std::future<void> m_wakeSetStateFuture;
};

ExternalMediaPlayerTest::ExternalMediaPlayerTest() :
        m_wakeSetStatePromise{},
        m_wakeSetStateFuture{m_wakeSetStatePromise.get_future()} {
}

void ExternalMediaPlayerTest::SetUp() {
    m_mockSpeakerInterface =
        std::make_shared<NiceMock<MockSpeakerInterface>>(SpeakerInterface::Type::AVS_SPEAKER_VOLUME);
    m_mockSpeakerManager = std::make_shared<NiceMock<MockSpeakerManager>>();
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockFocusManager = std::make_shared<NiceMock<MockFocusManager>>();
    m_mockContextManager = std::make_shared<NiceMock<MockContextManager>>();
    m_mockExceptionSender = std::make_shared<NiceMock<MockExceptionEncounteredSender>>();
    m_mockMediaPlayer = MockMediaPlayer::create();
    m_mockPlaybackRouter = std::make_shared<NiceMock<MockPlaybackRouter>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);

    ExternalMediaPlayer::AdapterMediaPlayerMap adapterMediaPlayerMap;
    ExternalMediaPlayer::AdapterSpeakerMap adapterSpeakerMap;
    ExternalMediaPlayer::AdapterCreationMap adapterMap;

    adapterMediaPlayerMap.insert(std::make_pair(MSP_NAME1, m_mockMediaPlayer));
    adapterSpeakerMap.insert(std::make_pair(MSP_NAME1, m_mockSpeakerInterface));
    adapterMap.insert(std::make_pair(MSP_NAME1, &MockExternalMediaPlayerAdapter::getInstance));

    m_externalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter);

    m_mockDirectiveHandlerResult = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    ASSERT_TRUE(m_externalMediaPlayer);
}

void ExternalMediaPlayerTest::TearDown() {
    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), doShutdown());

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
TEST_F(ExternalMediaPlayerTest, testCreateWithNullPointers) {
    /// Have an empty map of adapters  and mediaPlayers
    ExternalMediaPlayer::AdapterCreationMap adapterMap;
    ExternalMediaPlayer::AdapterMediaPlayerMap adapterMediaPlayerMap;
    ExternalMediaPlayer::AdapterSpeakerMap adapterSpeakerMap;

    auto testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        nullptr,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        nullptr,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        nullptr,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        m_mockFocusManager,
        nullptr,
        m_mockExceptionSender,
        m_mockPlaybackRouter);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        nullptr,
        m_mockPlaybackRouter);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);

    testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        nullptr);
    EXPECT_EQ(testExternalMediaPlayer, nullptr);
}

/**
 * Method to test successful creation of ExternalMediaPlayer capability agent
 * even if the creation of adapters fails.
 */
TEST_F(ExternalMediaPlayerTest, testCreateWithAdapterCreationFailures) {
    /// Have an empty map of adapters  and mediaPlayers
    ExternalMediaPlayer::AdapterCreationMap adapterMap;
    ExternalMediaPlayer::AdapterMediaPlayerMap adapterMediaPlayerMap;
    ExternalMediaPlayer::AdapterSpeakerMap adapterSpeakerMap;

    auto testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter);
    ASSERT_TRUE(testExternalMediaPlayer);
    testExternalMediaPlayer->shutdown();

    // Create an adapter MSP_PROVIDER2 but do not create a mediaPlayer for it.
    adapterMap.clear();
    adapterMediaPlayerMap.clear();
    adapterMediaPlayerMap.insert(std::make_pair(MSP_NAME1, m_mockMediaPlayer));
    adapterMap.insert(std::make_pair(MSP_NAME2, &MockExternalMediaPlayerAdapter::getInstance));
    testExternalMediaPlayer = ExternalMediaPlayer::create(
        adapterMediaPlayerMap,
        adapterSpeakerMap,
        adapterMap,
        m_mockSpeakerManager,
        m_mockMessageSender,
        m_mockFocusManager,
        m_mockContextManager,
        m_mockExceptionSender,
        m_mockPlaybackRouter);

    ASSERT_TRUE(testExternalMediaPlayer);
    testExternalMediaPlayer->shutdown();
}

/**
 * Test getConfiguration on an ExternalMediaPlayer. The operation succeeds.
 */
TEST_F(ExternalMediaPlayerTest, testGetConfiguration) {
    auto configuration = m_externalMediaPlayer->getConfiguration();
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    // TODO: ARC-227 Verify default values
    ASSERT_EQ(configuration[PLAY_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[LOGIN_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[LOGOUT_DIRECTIVE], neitherNonBlockingPolicy);
    ASSERT_EQ(configuration[RESUME_DIRECTIVE], audioNonBlockingPolicy);
    ASSERT_EQ(configuration[PAUSE_DIRECTIVE], audioNonBlockingPolicy);
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

/**
 * Test session state information on an ExternalMediaPlayer .
 */
TEST_F(ExternalMediaPlayerTest, testCallingProvideSessionState) {
    EXPECT_CALL(
        *(m_mockContextManager.get()), setState(SESSION_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillOnce(DoAll(
            // need to include all four arguments, but only care about jsonState
            Invoke([this](
                       const avs::NamespaceAndName& namespaceAndName,
                       const std::string& jsonState,
                       const avs::StateRefreshPolicy& refreshPolicy,
                       const unsigned int stateRequestToken) { verifyState(jsonState, IDLE_SESSION_STATE); }),
            InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState)));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState());

    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test playback state information on an ExternalMediaPlayer.
 */
TEST_F(ExternalMediaPlayerTest, testCallingProvidePlaybackState) {
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

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState());

    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test payload with parse error in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, testPlayParserError) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createPlayPayloadWithParseError("XXX", 0, 0, "Spotify"), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test PLAY payload without an adapter in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, testPlayNoAdapter) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPlayPayload("XXX", 0, 0, "Spotify"), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test PLAY payload without play context in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, testPlayNoPlayContext) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createPlayPayloadNoContext(0, 0, MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test PLAY payload without playerId in ExternalMediaPlayer. This should fail.
 */
TEST_F(ExternalMediaPlayerTest, testPlayNoPlayerId) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPlayPayloadNoPlayerId("XXX", 0, 0), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test PLAY payload without offsetin ExternalMediaPlayer. This should succeed.
 */
TEST_F(ExternalMediaPlayerTest, testPlayNoOffset) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createPlayPayloadNoOffset("XXX", 0, MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlay(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test PLAY payload without index in ExternalMediaPlayer. This should succeed.
 */
TEST_F(ExternalMediaPlayerTest, testPlayNoIndex) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PLAY_DIRECTIVE.nameSpace, PLAY_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createPlayPayloadNoIndex("XXX", 0, MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlay(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful logout.
 */
TEST_F(ExternalMediaPlayerTest, testLogout) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        LOGOUT_DIRECTIVE.nameSpace, LOGOUT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleLogout());
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful login.
 */
TEST_F(ExternalMediaPlayerTest, testLogin) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        LOGIN_DIRECTIVE.nameSpace, LOGIN_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "",
        avsMessageHeader,
        createLoginPayload("XXX", "msploginuser", 1000, false, MSP_NAME1),
        m_attachmentManager,
        "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleLogin(_, _, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test observers of session state are correctly notified
 */
TEST_F(ExternalMediaPlayerTest, testLoginStateChangeObserverIsNotified) {
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
    EXPECT_CALL(*(observer), onLoginStateProvided(PLAYER_ID, observableSessionProperties)).Times(1);

    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test observers of playback state are correctly notified
 */
TEST_F(ExternalMediaPlayerTest, testPlaybackStateChangeObserverIsNotified) {
    // add a mock observer
    auto observer = MockExternalMediaPlayerObserver::getInstance();
    m_externalMediaPlayer->addObserver(observer);

    EXPECT_CALL(
        *(m_mockContextManager.get()),
        setState(PLAYBACK_STATE, _, StateRefreshPolicy::ALWAYS, PROVIDE_STATE_TOKEN_TEST))
        .Times(1)
        .WillRepeatedly(InvokeWithoutArgs(this, &ExternalMediaPlayerTest::wakeOnSetState));

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), getState())
        .WillRepeatedly(Return(createAdapterState()));

    ObservablePlaybackStateProperties observablePlaybackStateProperties{PLAYER_STATE, PLAYER_TRACK};
    EXPECT_CALL(*(observer), onPlaybackStateProvided(PLAYER_ID, observablePlaybackStateProperties)).Times(1);

    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test that after removal login observers are not called anymore
 */
TEST_F(ExternalMediaPlayerTest, testLoginStateChangeObserverRemoval) {
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
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    this->resetWakeOnSetState();

    m_externalMediaPlayer->removeObserver(observer);

    EXPECT_CALL(*(observer), onLoginStateProvided(_, _)).Times(0);
    m_externalMediaPlayer->provideState(SESSION_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test that after removal playback state observers are not called anymore
 */
TEST_F(ExternalMediaPlayerTest, testPlaybackStateChangeObserverRemoval) {
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
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
    this->resetWakeOnSetState();

    m_externalMediaPlayer->removeObserver(observer);

    EXPECT_CALL(*(observer), onPlaybackStateProvided(_, _)).Times(0);
    m_externalMediaPlayer->provideState(PLAYBACK_STATE, PROVIDE_STATE_TOKEN_TEST);
    ASSERT_TRUE(std::future_status::ready == m_wakeSetStateFuture.wait_for(WAIT_TIMEOUT));
}

/**
 * Test successful resume.
 */
TEST_F(ExternalMediaPlayerTest, testPlay) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        RESUME_DIRECTIVE.nameSpace, RESUME_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());
    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful pause.
 */
TEST_F(ExternalMediaPlayerTest, testPause) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PAUSE_DIRECTIVE.nameSpace, PAUSE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful next.
 */
TEST_F(ExternalMediaPlayerTest, testNext) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        NEXT_DIRECTIVE.nameSpace, NEXT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful previous.
 */
TEST_F(ExternalMediaPlayerTest, testPrevious) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        PREVIOUS_DIRECTIVE.nameSpace, PREVIOUS_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful StarOver.
 */
TEST_F(ExternalMediaPlayerTest, testStartOver) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        STARTOVER_DIRECTIVE.nameSpace, STARTOVER_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful rewind.
 */
TEST_F(ExternalMediaPlayerTest, testRewind) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        REWIND_DIRECTIVE.nameSpace, REWIND_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful fast-forward.
 */
TEST_F(ExternalMediaPlayerTest, testFastForward) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        FASTFORWARD_DIRECTIVE.nameSpace, FASTFORWARD_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful EnableRepeatOne.
 */
TEST_F(ExternalMediaPlayerTest, testEnableRepeatOne) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ENABLEREPEATONE_DIRECTIVE.nameSpace, ENABLEREPEATONE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful EnableRepeat.
 */
TEST_F(ExternalMediaPlayerTest, testEnableRepeat) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ENABLEREPEAT_DIRECTIVE.nameSpace, ENABLEREPEAT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful DisableRepeat.
 */
TEST_F(ExternalMediaPlayerTest, testDisableRepeat) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        DISABLEREPEAT_DIRECTIVE.nameSpace, DISABLEREPEAT_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful EnableShuffle.
 */
TEST_F(ExternalMediaPlayerTest, testEnableShuffle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ENABLESHUFFLE_DIRECTIVE.nameSpace, ENABLESHUFFLE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful DisableRepeat.
 */
TEST_F(ExternalMediaPlayerTest, testDisableShuffle) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        DISABLESHUFFLE_DIRECTIVE.nameSpace, DISABLESHUFFLE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful Favorite.
 */
TEST_F(ExternalMediaPlayerTest, testFavorite) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        FAVORITE_DIRECTIVE.nameSpace, FAVORITE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful UnFavorite.
 */
TEST_F(ExternalMediaPlayerTest, testUnfavorite) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        UNFAVORITE_DIRECTIVE.nameSpace, UNFAVORITE_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handlePlayControl(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test incorrect directive.
 */
TEST_F(ExternalMediaPlayerTest, testIncorrectDirective) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        FAVORITE_DIRECTIVE.nameSpace, PREVIOUS_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createPayloadWithPlayerId(MSP_NAME1), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test Seek failure passing incorrect field in payload.
 */
TEST_F(ExternalMediaPlayerTest, testSeekFailure) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        SEEK_DIRECTIVE.nameSpace, SEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createSeekPayload(100, MSP_NAME1, true), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test successful Seek.
 */
TEST_F(ExternalMediaPlayerTest, testSeekSuccess) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        SEEK_DIRECTIVE.nameSpace, SEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createSeekPayload(100, MSP_NAME1, false), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleSeek(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test  AdjustSeek failure incorrect field in payload.
 */
TEST_F(ExternalMediaPlayerTest, testAdjustSeekFailure) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ADJUSTSEEK_DIRECTIVE.nameSpace, ADJUSTSEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive =
        AVSDirective::create("", avsMessageHeader, createSeekPayload(100, MSP_NAME1, false), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test  AdjustSeek failure passing in an incorrect offset.
 */
TEST_F(ExternalMediaPlayerTest, testAdjustSeekFailure2) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ADJUSTSEEK_DIRECTIVE.nameSpace, ADJUSTSEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(86400014, MSP_NAME1, true), m_attachmentManager, "");

    EXPECT_CALL(*(m_mockExceptionSender.get()), sendExceptionEncountered(_, _, _));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setFailed(_));

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

/**
 * Test  AdjustSeek successful passing in correct payload and offset.
 */
TEST_F(ExternalMediaPlayerTest, testAdjustSeekSuccess) {
    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(
        ADJUSTSEEK_DIRECTIVE.nameSpace, ADJUSTSEEK_DIRECTIVE.name, MESSAGE_ID_TEST, DIALOG_REQUEST_ID_TEST);

    std::shared_ptr<AVSDirective> directive = AVSDirective::create(
        "", avsMessageHeader, createSeekPayload(86400000, MSP_NAME1, true), m_attachmentManager, "");

    EXPECT_CALL(*(MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter), handleAdjustSeek(_));
    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted());

    m_externalMediaPlayer->CapabilityAgent::preHandleDirective(directive, std::move(m_mockDirectiveHandlerResult));
    m_externalMediaPlayer->CapabilityAgent::handleDirective(MESSAGE_ID_TEST);
}

}  // namespace test
}  // namespace externalMediaPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
