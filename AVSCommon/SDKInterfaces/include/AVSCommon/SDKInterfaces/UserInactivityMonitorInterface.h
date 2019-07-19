/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_USERINACTIVITYMONITORINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_USERINACTIVITYMONITORINTERFACE_H_

#include <chrono>

#include <AVSCommon/SDKInterfaces/UserInactivityMonitorObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface is used to notify an implementation of the user activity. Any component that interacts with the user
 * (e.g. AudioInputProcessor) should register an instance of this interface to signal when user interaction is detected
 * (e.g. SpeechStarted).
 *
 * This interface should also send the System.UserInactivityReport Event as defined here:
 * https://developer.amazon.com/docs/alexa-voice-service/system.html#userinactivityreport
 * and notify its observers when this occurs.
 */
class UserInactivityMonitorInterface {
public:
    /// Destructor.
    virtual ~UserInactivityMonitorInterface() = default;

    /// The function to be called when the user has become active.
    virtual void onUserActive() = 0;

    /**
     * Calculates how many seconds have elapsed since a user last interacted with the device.
     *
     * @return How many seconds have elapsed since a user last interacted with the device.
     */
    virtual std::chrono::seconds timeSinceUserActivity() = 0;

    /**
     * Adds an observer to be notified when the System.UserInactivityReport Event has been sent.
     *
     * @param observer The observer to be notified when the System.UserInactivityReport Event has been sent.
     */
    virtual void addObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorObserverInterface> observer) = 0;

    /**
     * Removes an observer from the collection of observers which will be notified when the System.UserInactivityReport
     * Event has been sent.
     *
     * @param observer The observer that should no longer be notified when the System.UserInactivityReport Event has
     * been sent.
     */
    virtual void removeObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorObserverInterface> observer) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_USERINACTIVITYMONITORINTERFACE_H_
