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

#ifndef AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_INTERNETCONNECTIONMONITORMOCK_H_
#define AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_INTERNETCONNECTIONMONITORMOCK_H_
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionObserverInterface.h>
#include <mutex>
#include <vector>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {
class InternetConnectionMonitorMock
        : public alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionMonitorInterface {
public:
    ~InternetConnectionMonitorMock() override;

    static std::shared_ptr<InternetConnectionMonitorMock> create();

    void addInternetConnectionObserver(
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer)
            override;
    void removeInternetConnectionObserver(
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer)
            override;

private:
    InternetConnectionMonitorMock() = default;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_INTERNETCONNECTIONMONITORMOCK_H_
