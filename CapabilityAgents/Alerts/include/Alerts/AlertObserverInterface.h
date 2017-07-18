/*
 * AlertObserverInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERT_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERT_OBSERVER_INTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

/**
 * An interface for observing state changes on an Alert object.
 */
class AlertObserverInterface {
public:
    /**
     * An enum class to represent the states an alert can be in.
     */
    enum class State {
        /// The alert has started.
        STARTED,
        /// The alert has stopped due to user or system intervention.
        STOPPED,
        /// The alert has snoozed.
        SNOOZED,
        /// The alert has completed on its own.
        COMPLETED,
        /// The alert has been determined to be past-due, and will not be rendered.
        PAST_DUE,
        /// The alert has entered the foreground.
        FOCUS_ENTERED_FOREGROUND,
        /// The alert has entered the background.
        FOCUS_ENTERED_BACKGROUND,
        /// The alert has encountered an error.
        ERROR
    };

    /**
     * Destructor.
     */
    virtual ~AlertObserverInterface() = default;

    /**
     * A callback function to notify an object that an alert has updated its state.
     *
     * @param alertToken The AVS token of the alert.
     * @param state The state of the alert.
     * @param reason The reason for the state change.
     */
    virtual void onAlertStateChange(const std::string & alertToken, State state, const std::string & reason = "") = 0;
};

} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERT_OBSERVER_INTERFACE_H_