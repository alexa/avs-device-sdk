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

#ifndef AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYER_TEST_ACSDKEXTERNALMEDIAPLAYER_MOCKEXTERNALMEDIAADAPTERHANDLER_H_
#define AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYER_TEST_ACSDKEXTERNALMEDIAPLAYER_MOCKEXTERNALMEDIAADAPTERHANDLER_H_

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {
namespace test {

static const std::string PLAYER_ID = "testPlayerId";

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
    MOCK_METHOD3(
        handlePlayControl,
        bool(
            const std::string& localPlayerId,
            acsdkExternalMediaPlayerInterfaces::RequestType requestType,
            const std::string& playbackTarget));
    MOCK_METHOD2(handleSeek, bool(const std::string& localPlayerId, std::chrono::milliseconds offset));
    MOCK_METHOD2(handleAdjustSeek, bool(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset));
    MOCK_METHOD2(
        handleGetAdapterState,
        bool(const std::string& localPlayerId, acsdkExternalMediaPlayerInterfaces::AdapterState& state));
    MOCK_METHOD1(handleSetVolume, void(int8_t volume));
    MOCK_METHOD1(handleSetMute, void(bool mute));
    void reportMockPlayers(const std::string& localPlayerId);
    MockExternalMediaAdapterHandler();
};

void MockExternalMediaAdapterHandler::reportMockPlayers(const std::string& localPlayerId) {
    acsdkExternalMediaPlayerInterfaces::DiscoveredPlayerInfo playerInfo;

    playerInfo.localPlayerId = localPlayerId;
    reportDiscoveredPlayers({playerInfo});
};

MockExternalMediaAdapterHandler::MockExternalMediaAdapterHandler() : ExternalMediaAdapterHandler{"mock"} {
}

}  // namespace test
}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYER_TEST_ACSDKEXTERNALMEDIAPLAYER_MOCKEXTERNALMEDIAADAPTERHANDLER_H_
