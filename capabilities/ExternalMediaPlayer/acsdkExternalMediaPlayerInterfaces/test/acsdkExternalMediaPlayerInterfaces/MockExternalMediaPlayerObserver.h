/*;
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

#ifndef AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYEROBSERVER_H_
#define AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYEROBSERVER_H_

#include <acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerObserverInterface.h>

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {
namespace test {

/// Mock class for the ExternalMediaPlayerObserverInterface
class MockExternalMediaPlayerObserver : public ExternalMediaPlayerObserverInterface {
public:
    static std::shared_ptr<MockExternalMediaPlayerObserver> getInstance();
    MOCK_METHOD2(
        onLoginStateProvided,
        void(const std::string&, const acsdkExternalMediaPlayerInterfaces::ObservableSessionProperties));
    MOCK_METHOD2(
        onPlaybackStateProvided,
        void(const std::string&, const acsdkExternalMediaPlayerInterfaces::ObservablePlaybackStateProperties));

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

}  // namespace test
}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_EXTERNALMEDIAPLAYER_ACSDKEXTERNALMEDIAPLAYERINTERFACES_TEST_ACSDKEXTERNALMEDIAPLAYERINTERFACES_MOCKEXTERNALMEDIAPLAYEROBSERVER_H_
