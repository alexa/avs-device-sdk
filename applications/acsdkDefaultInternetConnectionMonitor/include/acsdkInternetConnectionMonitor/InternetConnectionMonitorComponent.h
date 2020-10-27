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

#ifndef ACSDKINTERNETCONNECTIONMONITOR_INTERNETCONNECTIONMONITORCOMPONENT_H_
#define ACSDKINTERNETCONNECTIONMONITOR_INTERNETCONNECTIONMONITORCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>

namespace alexaClientSDK {
namespace acsdkInternetConnectionMonitor {

/**
 * Definition of the Manufactory Component for the default implementation of InternetConnectionMonitorInterface.
 */
using InternetConnectionMonitorComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>>>;

/**
 * Get the default @c Manufactory component for creating instances of InternetConnectionMonitorInterface.
 *
 * @return Te default @c Manufactory component for creating instances of InternetConnectionMonitorInterface.
 */
InternetConnectionMonitorComponent getComponent();

}  // namespace acsdkInternetConnectionMonitor
}  // namespace alexaClientSDK

#endif  // ACSDKINTERNETCONNECTIONMONITOR_INTERNETCONNECTIONMONITORCOMPONENT_H_
