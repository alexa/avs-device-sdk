/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTOBSERVERINTERFACE_H_

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
        /// The alert is ready to start, and is waiting for channel focus.
        READY,
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
     * @param alertType The type of the alert.
     * @param state The state of the alert.
     * @param reason The reason for the state change.
     */
    virtual void onAlertStateChange(
        const std::string& alertToken,
        const std::string& alertType,
        State state,
        const std::string& reason = "") = 0;

    /**
     * Convert a @c State to a @c std::string.
     *
     * @param state The @c State to convert.
     * @return The string representation of @c state.
     */
    static std::string stateToString(State state);
};

inline std::string AlertObserverInterface::stateToString(State state) {
    switch (state) {
        case State::READY:
            return "READY";
        case State::STARTED:
            return "STARTED";
        case State::STOPPED:
            return "STOPPED";
        case State::SNOOZED:
            return "SNOOZED";
        case State::COMPLETED:
            return "COMPLETED";
        case State::PAST_DUE:
            return "PAST_DUE";
        case State::FOCUS_ENTERED_FOREGROUND:
            return "FOCUS_ENTERED_FOREGROUND";
        case State::FOCUS_ENTERED_BACKGROUND:
            return "FOCUS_ENTERED_BACKGROUND";
        case State::ERROR:
            return "ERROR";
    }
    return "unknown State";
}

/**
 * Write a @c State value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param state The @c State value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AlertObserverInterface::State& state) {
    return stream << AlertObserverInterface::stateToString(state);
}

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTOBSERVERINTERFACE_H_
