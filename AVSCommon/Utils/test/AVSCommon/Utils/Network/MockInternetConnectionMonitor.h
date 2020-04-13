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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_NETWORK_MOCKINTERNETCONNECTIONMONITOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_NETWORK_MOCKINTERNETCONNECTIONMONITOR_H_

#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace network {
namespace test {

class MockInternetConnectionMonitor : public avsCommon::sdkInterfaces::InternetConnectionMonitorInterface {
public:
    MOCK_METHOD1(
        addInternetConnectionObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer));
    MOCK_METHOD1(
        removeInternetConnectionObserver,
        void(std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer));
};

}  // namespace test
}  // namespace network
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_NETWORK_MOCKINTERNETCONNECTIONMONITOR_H_
