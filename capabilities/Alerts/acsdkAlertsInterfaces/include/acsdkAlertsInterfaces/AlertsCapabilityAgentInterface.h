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

#ifndef ACSDKALERTSINTERFACES_ALERTSCAPABILITYAGENTINTERFACE_H_
#define ACSDKALERTSINTERFACES_ALERTSCAPABILITYAGENTINTERFACE_H_

#include "acsdkAlertsInterfaces/AlertObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkAlertsInterfaces {

/**
 * An interface for Alerts Capability Agent actions that the application may need to access.
 */
class AlertsCapabilityAgentInterface {
public:
    /**
     * Adds an observer to be notified of alert status changes.
     *
     * @param observer The observer to add.
     */
    virtual void addObserver(std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> observer) = 0;

    /**
     * Removes an observer from being notified of alert status changes.
     *
     * @param observer The observer to remove.
     */
    virtual void removeObserver(std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> observer) = 0;

    /**
     * A function that allows an application to clear all alerts from storage.  This may be useful for a scenario
     * where a user logs out of a device, and another user will log in.  As the first user logs out, their pending
     * alerts should not go off.
     */
    virtual void removeAllAlerts() = 0;

    /**
     * This function provides a way for application code to request this object stop any active alert as the result
     * of a user action, such as pressing a physical 'stop' button on the device.
     */
    virtual void onLocalStop() = 0;

    /**
     * Destructor.
     */
    virtual ~AlertsCapabilityAgentInterface() = default;
};

}  // namespace acsdkAlertsInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALERTSINTERFACES_ALERTSCAPABILITYAGENTINTERFACE_H_
