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

#ifndef AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYERADAPTER_H_
#define AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYERADAPTER_H_

#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterInterface.h>
#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>

#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {
namespace test {

/// Mock class of ExternalMediaAdapterInterface.
class MockExternalMediaPlayerAdapter : public ExternalMediaAdapterInterface {
public:
    /*
     * Method that adheres to the AdapterCreateFunc interface to create an
     * adapter. This method create a mock instances and assigns it to a class
     * static to keep the mock class simple.
     *
     * @param metricRecorder The metricRecorder instance to be used to record
     * metrics
     * @param mediaPlayer The mediaPlayer instance to be used to play Spotify
     * content.
     * @param speakerManager A @c SpeakerManagerInterface to perform volume
     * changes requested by ESDK.
     * @param messageSender The object to use for sending events.
     * @param focusManager The focusManager used to acquire/release channel.
     * @param contextManager The AVS Context manager used to generate system
     * context for events.
     * @param externalMediaPlayer The instance of the @c ExternalMediaPlayer
     * managing the adapter.
     * @return A @c std::shared_ptr to the new @c ExternalMediaAdapter instance.
     */
    static std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface> getInstance(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> speaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface> externalMediaPlayer);

    /// MockExternalMediaPlayerAdapter private constructor.
    MockExternalMediaPlayerAdapter();

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
    MOCK_METHOD1(handlePlay, void(const HandlePlayParams& params));
    MOCK_METHOD2(handlePlayControl, void(RequestType requestType, const std::string& playbackTarget));
    MOCK_METHOD1(handleSeek, void(std::chrono::milliseconds offset));
    MOCK_METHOD1(handleAdjustSeek, void(std::chrono::milliseconds deltaOffset));
    MOCK_METHOD3(
        handleAuthorized,
        void(bool authorized, const std::string& playerId, const std::string& defaultSkillToken));
    MOCK_METHOD1(handleSetVolume, void(int8_t volume));
    MOCK_METHOD1(handleSetMute, void(bool));
    MOCK_METHOD0(getState, AdapterState());
    MOCK_METHOD0(getOffset, std::chrono::milliseconds());
};

/// Static instance of MockMediaPlayerAdapter.
std::shared_ptr<MockExternalMediaPlayerAdapter> MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter;

MockExternalMediaPlayerAdapter::MockExternalMediaPlayerAdapter() :
        RequiresShutdown("MockExternalMediaPlayerAdapter"),
        ExternalMediaAdapterInterface("MockExternalMediaPlayerAdapter") {
}

std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface> MockExternalMediaPlayerAdapter::
    getInstance(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface> speaker,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface> externalMediaPlayer) {
    MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter =
        std::shared_ptr<MockExternalMediaPlayerAdapter>(new MockExternalMediaPlayerAdapter());
    return MockExternalMediaPlayerAdapter::m_currentActiveMediaPlayerAdapter;
}
}  // namespace test
}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYERADAPTER_H_
