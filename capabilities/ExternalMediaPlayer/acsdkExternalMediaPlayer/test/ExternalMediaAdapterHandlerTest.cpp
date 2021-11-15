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

/// @file ExternalMediaAdapterHandler.cpp

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdkExternalMediaPlayer/ExternalMediaPlayer.h"
#include "acsdkExternalMediaPlayer/ExternalMediaAdapterHandler.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterInterface.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {
namespace test {

using PlayParams = acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface::PlayParams;
using namespace avsCommon::avs;

using namespace ::testing;

static const std::string PLAYER_ID = "testPlayerId";
static const std::string PLAY_CONTEXT_TOKEN = "testContextToken";
static const std::string SKILL_TOKEN = "testSkillToken";
static const std::string SESSION_ID = "testSessionId";
static const std::string NAVIGATION_NONE = "NONE";
static const std::string PLAYBACK_TARGET = "testPlaybackTarget";
static const PlayRequestor PLAY_REQUESTOR{.type = "ALERT", .id = "123"};
static const std::chrono::milliseconds PLAY_OFFSET{100};

#ifdef MEDIA_PORTABILITY_ENABLED
static const std::string MEDIA_SESSION_ID = "testMediaSessionId";
static const std::string CORRELATION_TOKEN = "testCorrelationToken";
#endif

class MockExternalMediaPlayer : public ExternalMediaPlayer::ExternalMediaPlayerInterface {
public:
    MOCK_METHOD1(setPlayerInFocus, void(const std::string& playerInFocus));
    MOCK_METHOD2(
        updateDiscoveredPlayers,
        void(
            const std::vector<acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo>& addedPlayers,
            const std::unordered_set<std::string>& removedPlayers));
    MOCK_METHOD1(
        addAdapterHandler,
        void(std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler));
    MOCK_METHOD1(
        removeAdapterHandler,
        void(std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler));
    MOCK_METHOD1(
        addObserver,
        void(const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer));
    MOCK_METHOD1(
        removeObserver,
        void(const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer));
};

/// Mock class for ExternalMediaPlayerAdapterHandler
class MockExternalMediaAdapterHandler : public ExternalMediaAdapterHandler {
public:
    MOCK_METHOD1(handleAuthorization, bool(const AuthorizedPlayerInfo& authorizedPlayer));
    MOCK_METHOD5(
        handleLogin,
        bool(
            const std::string& localPlayerId,
            const std::string& accessToken,
            const std::string& userName,
            bool forceLogin,
            std::chrono::milliseconds tokenRefreshInterval));
    MOCK_METHOD1(handleLogout, bool(const std::string& localPlayerId));
    MOCK_METHOD1(handlePlay, bool(const PlayParams& params));
#ifdef MEDIA_PORTABILITY_ENABLED
    MOCK_METHOD5(
#else
    MOCK_METHOD3(
#endif
        handlePlayControl,
        bool(
            const std::string& localPlayerId,
            acsdkExternalMediaPlayerInterfaces::RequestType requestType,
#ifdef MEDIA_PORTABILITY_ENABLED
            const std::string& mediaSessionId,
            const std::string& correlationToken,
#endif
            const std::string& playbackTarget));
    MOCK_METHOD2(handleSeek, bool(const std::string& localPlayerId, std::chrono::milliseconds offset));
    MOCK_METHOD2(handleAdjustSeek, bool(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset));
    MOCK_METHOD2(
        handleGetAdapterState,
        bool(const std::string& localPlayerId, acsdkExternalMediaPlayerInterfaces::AdapterState& state));
    MOCK_METHOD1(handleSetVolume, void(int8_t volume));
    MOCK_METHOD1(handleSetMute, void(bool mute));
    void reportMockPlayers();
    MockExternalMediaAdapterHandler();
};

void MockExternalMediaAdapterHandler::reportMockPlayers() {
    acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo playerInfo;
    playerInfo.localPlayerId = PLAYER_ID;
    reportDiscoveredPlayers({playerInfo});
};

MockExternalMediaAdapterHandler::MockExternalMediaAdapterHandler() : ExternalMediaAdapterHandler{"mock"} {
}

class ExternalMediaPlayerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    void authorizePlayer();

    /// @c ExternalMediaPlayer for testing purposes
    std::shared_ptr<MockExternalMediaPlayer> m_externalMediaPlayer;

    /// The external media adapter handler
    std::shared_ptr<MockExternalMediaAdapterHandler> m_externalMediaPlayerAdapterHandler;
};

void ExternalMediaPlayerTest::SetUp() {
    m_externalMediaPlayer = std::make_shared<NiceMock<MockExternalMediaPlayer>>();
    m_externalMediaPlayerAdapterHandler = std::make_shared<MockExternalMediaAdapterHandler>();
    m_externalMediaPlayerAdapterHandler->setExternalMediaPlayer(m_externalMediaPlayer);
    m_externalMediaPlayerAdapterHandler->reportMockPlayers();
};

void ExternalMediaPlayerTest::TearDown() {
    m_externalMediaPlayerAdapterHandler->shutdown();
}

void ExternalMediaPlayerTest::authorizePlayer() {
    acsdkExternalMediaPlayerInterfaces::PlayerInfo playerInfo;
    playerInfo.localPlayerId = PLAYER_ID;
    playerInfo.playerSupported = true;
    m_externalMediaPlayerAdapterHandler->updatePlayerInfo({playerInfo});
}

/**
 * Test authorization passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleAuthorization) {
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleAuthorization(_))
        .Times(1)
        .WillOnce(Invoke([](const AuthorizedPlayerInfo& actual) {
            EXPECT_EQ(actual.localPlayerId, PLAYER_ID);
            EXPECT_EQ(actual.authorized, true);
            return true;
        }));
    authorizePlayer();
}

/**
 * Test login passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleLogin) {
    authorizePlayer();
    const std::string& accessToken = "token";
    const std::string& userName = "peci";
    const bool forceLogin = false;
    const std::chrono::milliseconds tokenRefreshInterval{234};
    EXPECT_CALL(
        *m_externalMediaPlayerAdapterHandler,
        handleLogin(PLAYER_ID, accessToken, userName, forceLogin, tokenRefreshInterval));
    m_externalMediaPlayerAdapterHandler->login(PLAYER_ID, accessToken, userName, forceLogin, tokenRefreshInterval);
}

/**
 * Test logout passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleLogout) {
    authorizePlayer();
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleLogout(PLAYER_ID));
    m_externalMediaPlayerAdapterHandler->logout(PLAYER_ID);
}

/**
 * Test play passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandlePlay) {
    authorizePlayer();
    const PlayParams params(
        PLAYER_ID,
        PLAY_CONTEXT_TOKEN,
        0,
        PLAY_OFFSET,
        SKILL_TOKEN,
        SESSION_ID,
        acsdkExternalMediaPlayerInterfaces::Navigation::NONE,
        false,
        PLAY_REQUESTOR,
#ifdef MEDIA_PORTABILITY_ENABLED
        MEDIA_SESSION_ID,
        CORRELATION_TOKEN,
#endif
        "");
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handlePlay(_));
    m_externalMediaPlayerAdapterHandler->play(params);
}

/**
 * Test play control passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandlePlayControl) {
    authorizePlayer();
    EXPECT_CALL(
        *m_externalMediaPlayerAdapterHandler,
        handlePlayControl(
            PLAYER_ID,
            acsdkExternalMediaPlayerInterfaces::RequestType::NONE,
#ifdef MEDIA_PORTABILITY_ENABLED
            MEDIA_SESSION_ID,
            CORRELATION_TOKEN,
#endif
            PLAYBACK_TARGET));
    m_externalMediaPlayerAdapterHandler->playControl(
        PLAYER_ID,
        acsdkExternalMediaPlayerInterfaces::RequestType::NONE,
#ifdef MEDIA_PORTABILITY_ENABLED
        MEDIA_SESSION_ID,
        CORRELATION_TOKEN,
#endif
        PLAYBACK_TARGET);
}

/**
 * Test seek passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleSeek) {
    authorizePlayer();
    std::chrono::milliseconds offset{500};
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleSeek(PLAYER_ID, offset));
    m_externalMediaPlayerAdapterHandler->seek(PLAYER_ID, offset);
}

/**
 * Test adjust seek passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleAdjustSeek) {
    authorizePlayer();
    std::chrono::milliseconds offset{500};
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleAdjustSeek(PLAYER_ID, offset));
    m_externalMediaPlayerAdapterHandler->adjustSeek(PLAYER_ID, offset);
}

/**
 * Test get adapter states passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleGetAdapterStates) {
    authorizePlayer();
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleGetAdapterState(PLAYER_ID, _));
    m_externalMediaPlayerAdapterHandler->getAdapterStates();
}

/**
 * Test get adapter states passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleGetAdapterState) {
    authorizePlayer();
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleGetAdapterState(PLAYER_ID, _));
    m_externalMediaPlayerAdapterHandler->getAdapterState(PLAYER_ID);
}

/**
 * Test speaker change passthrough
 */
TEST_F(ExternalMediaPlayerTest, testHandleSpeakerChange) {
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings settings;
    settings.volume = 76;
    settings.mute = true;
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleSetVolume(76));
    EXPECT_CALL(*m_externalMediaPlayerAdapterHandler, handleSetMute(true));
    m_externalMediaPlayerAdapterHandler->onSpeakerSettingsChanged(
        ExternalMediaAdapterHandler::Source::DIRECTIVE,
        avsCommon::sdkInterfaces::ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME,
        settings);
}

}  // namespace test
}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
