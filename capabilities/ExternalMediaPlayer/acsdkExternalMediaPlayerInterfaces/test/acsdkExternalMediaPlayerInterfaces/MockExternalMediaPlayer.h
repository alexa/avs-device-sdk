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

#ifndef AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYER_H_
#define AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYER_H_

#include <gmock/gmock.h>

#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerInterface.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {
namespace test {

using namespace avsCommon::avs;

using namespace ::testing;

/// Mock class or ExternalMediaPlayer
class MockExternalMediaPlayer : public ExternalMediaPlayerInterface {
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

}  // namespace test
}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYER_H_
