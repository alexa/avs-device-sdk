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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_NAVIGATIONEVENT_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_NAVIGATIONEVENT_H_

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/// Enumeration of navigation events that could be sent from GUI App to SampleApp.
enum class NavigationEvent {
    /// Exit button has been pressed.
    EXIT,

    /// Back button has been pressed.
    BACK,

    /// Guard option for unknown received state.
    UNKNOWN
};

/**
 * This function converts the provided string to an @c NavigationEvent.
 *
 * @param string The string to convert to @c NavigationEvent.
 * @return The @c NavigationEvent.
 */
inline NavigationEvent navigationEventFromString(const std::string& string) {
    if ("EXIT" == string) {
        return NavigationEvent::EXIT;
    } else if ("BACK" == string) {
        return NavigationEvent::BACK;
    } else {
        return NavigationEvent::UNKNOWN;
    }
}

/**
 * This function converts a NavigationEvent to string
 *
 * @param event the input NavigationEvent
 * @return the string for the input NavigationEvent
 */
inline std::string navigationEventToString(NavigationEvent event) {
    switch (event) {
        case NavigationEvent::BACK:
            return "BACK";
        case NavigationEvent::EXIT:
            return "EXIT";
        default:
            return "UNKNOWN";
    }
}

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_NAVIGATIONEVENT_H_
