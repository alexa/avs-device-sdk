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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSCAPABILITYAGENTSTATE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSCAPABILITYAGENTSTATE_H_

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

#include <ostream>
#include <string>

enum class NotificationsCapabilityAgentState {
    // The capability agent is awaiting directives.
    IDLE,
    // A call to renderNotification() has been made, but the notification has not yet finished rendering.
    PLAYING,
    // A call to cancelNotificationRendering() has been made, but the notification has not yet been cancelled.
    CANCELING_PLAY,
    // The capability agent is shutting down.
    SHUTTING_DOWN,
    // The capability agent has finished shutting down.
    SHUTDOWN
};

inline std::string stateToString(NotificationsCapabilityAgentState state) {
    switch (state) {
        case NotificationsCapabilityAgentState::IDLE:
            return "IDLE";
        case NotificationsCapabilityAgentState::PLAYING:
            return "PLAYING";
        case NotificationsCapabilityAgentState::CANCELING_PLAY:
            return "CANCELING_PLAY";
        case NotificationsCapabilityAgentState::SHUTTING_DOWN:
            return "SHUTTING_DOWN";
        case NotificationsCapabilityAgentState::SHUTDOWN:
            return "SHUTDOWN";
    }
    return "Unknown NotificationsCapabilityAgent State";
}

inline std::ostream& operator<<(std::ostream& stream, const NotificationsCapabilityAgentState& state) {
    return stream << stateToString(state);
}

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSCAPABILITYAGENTSTATE_H_
