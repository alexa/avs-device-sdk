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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONMANAGER_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONMANAGER_H_

#include <vector>
#include <gmock/gmock.h>

#include <Captions/CaptionManager.h>
#include <Captions/CaptionManagerInterface.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

class MockCaptionManager
        : public CaptionFrameParseListenerInterface
        , public avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
        , public CaptionManagerInterface {
public:
    /// @name CaptionFrameParseListenerInterface methods
    /// @{
    MOCK_METHOD1(onParsed, void(const CaptionFrame&));
    ///@}

    /// @name CaptionManagerInterface methods
    /// @{
    MOCK_METHOD1(setCaptionPresenter, void(const std::shared_ptr<CaptionPresenterInterface>&));
    MOCK_METHOD2(onCaption, void(uint64_t sourceId, const captions::CaptionData&));
    MOCK_METHOD1(
        setMediaPlayers,
        void(const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>&));
    ///@}

    /// @name MediaPlayerObserverInterface methods
    /// @{
    MOCK_METHOD2(onPlaybackStarted, void(SourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState&));
    MOCK_METHOD2(onPlaybackFinished, void(SourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState&));
    MOCK_METHOD4(
        onPlaybackError,
        void(
            SourceId,
            const avsCommon::utils::mediaPlayer::ErrorType&,
            std::string,
            const avsCommon::utils::mediaPlayer::MediaPlayerState&));
    MOCK_METHOD2(onFirstByteRead, void(SourceId, const avsCommon::utils::mediaPlayer::MediaPlayerState&));
    /// @}
};

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_TEST_MOCKCAPTIONMANAGER_H_
