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
#include "InternetConnectionMonitorMock.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;

static const string TAG{"MAPLiteAuthDelegateMock"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

InternetConnectionMonitorMock::~InternetConnectionMonitorMock() = default;

std::shared_ptr<InternetConnectionMonitorMock> InternetConnectionMonitorMock::create() {
    ACSDK_INFO(LX(("create")));
    auto internetConnectionMonitor = shared_ptr<InternetConnectionMonitorMock>(new InternetConnectionMonitorMock());
    return internetConnectionMonitor;
}

void InternetConnectionMonitorMock::addInternetConnectionObserver(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer) {
    ACSDK_INFO(LX(("addInternetConnectionObserver")));

    observer->onConnectionStatusChanged(true);
}
void InternetConnectionMonitorMock::removeInternetConnectionObserver(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer) {
    ACSDK_INFO(LX(("removeInternetConnectionObserver")));
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK