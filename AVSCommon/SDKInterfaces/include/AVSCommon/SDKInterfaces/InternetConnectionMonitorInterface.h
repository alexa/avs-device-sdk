/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_INTERNETCONNECTIONMONITORINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_INTERNETCONNECTIONMONITORINTERFACE_H_

#include "AVSCommon/SDKInterfaces/InternetConnectionObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * An interface for classes responsible for monitoring and reporting internet connection status.
 */
class InternetConnectionMonitorInterface {
public:
    /**
     * Add an observer to be notified of internet connection changes.
     */
    virtual void addInternetConnectionObserver(std::shared_ptr<InternetConnectionObserverInterface> observer) = 0;

    /**
     * Remove an observer to be notified of internet connection changes.
     */
    virtual void removeInternetConnectionObserver(std::shared_ptr<InternetConnectionObserverInterface> observer) = 0;

    /**
     * Destructor.
     */
    virtual ~InternetConnectionMonitorInterface() = default;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_INTERNETCONNECTIONMONITORINTERFACE_H_
