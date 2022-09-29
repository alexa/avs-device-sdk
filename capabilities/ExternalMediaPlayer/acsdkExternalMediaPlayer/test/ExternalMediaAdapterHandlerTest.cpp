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

#include "acsdkExternalMediaPlayer/ExternalMediaAdapterHandler.h"
#include "acsdkExternalMediaPlayer/MockExternalMediaAdapterHandler.h"

#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/MockExternalMediaPlayer.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {
namespace test {

using namespace acsdkExternalMediaPlayerInterfaces;
using namespace acsdkExternalMediaPlayerInterfaces::test;
using namespace avsCommon::avs;
using PlayParams = ExternalMediaAdapterHandlerInterface::PlayParams;

using namespace ::testing;

static const std::string PLAY_CONTEXT_TOKEN = "testContextToken";
static const std::string SKILL_TOKEN = "testSkillToken";
static const std::string SESSION_ID = "testSessionId";
static const std::string PLAYBACK_TARGET = "testPlaybackTarget";
static const PlayRequestor PLAY_REQUESTOR{.type = "ALERT", .id = "123"};
static const std::chrono::milliseconds PLAY_OFFSET{100};

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
    m_externalMediaPlayerAdapterHandler->reportMockPlayers(PLAYER_ID);
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
        handlePlayControl(PLAYER_ID, acsdkExternalMediaPlayerInterfaces::RequestType::NONE, PLAYBACK_TARGET));
    m_externalMediaPlayerAdapterHandler->playControl(
        PLAYER_ID, acsdkExternalMediaPlayerInterfaces::RequestType::NONE, PLAYBACK_TARGET);
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
