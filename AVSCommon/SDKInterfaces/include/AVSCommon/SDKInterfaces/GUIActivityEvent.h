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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_GUIACTIVITYEVENT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_GUIACTIVITYEVENT_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/// Enumeration of activity events used to indicate the state of a GUI
enum class GUIActivityEvent {
    /// GUI switched to active state (audio/video started).
    ACTIVATED,

    /// GUI become inactive.
    DEACTIVATED,

    /// Interrupt event (touch/scroll/etc).
    INTERRUPT,

    /// Guard option for unknown received state.
    UNKNOWN
};

/**
 * This function converts the provided string to an @c GUIActivityEvent.
 *
 * @param string The string to convert to @c GUIActivityEvent.
 * @return The @c GUIActivityEvent.
 */
inline GUIActivityEvent guiActivityEventFromString(const std::string& string) {
    if ("ACTIVATED" == string) {
        return GUIActivityEvent::ACTIVATED;
    } else if ("DEACTIVATED" == string) {
        return GUIActivityEvent::DEACTIVATED;
    } else if ("INTERRUPT" == string) {
        return GUIActivityEvent::INTERRUPT;
    } else {
        return GUIActivityEvent::UNKNOWN;
    }
}

/**
 * This function converts a GUIActivityEvent to a string.
 *
 * @param event the GUIActivityEvent to be converted.
 * @return the string representation of the GUIActivityEvent.
 */
inline std::string guiActivityEventToString(GUIActivityEvent event) {
    switch (event) {
        case GUIActivityEvent::ACTIVATED:
            return "ACTIVATED";
        case GUIActivityEvent::DEACTIVATED:
            return "DEACTIVATED";
        case GUIActivityEvent::INTERRUPT:
            return "INTERRUPT";
        case GUIActivityEvent::UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

/**
 * Write an @c GUIActivityEvent value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param state The @c GUIActivityEvent value.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const GUIActivityEvent& activityEvent) {
    return stream << guiActivityEventToString(activityEvent);
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_GUIACTIVITYEVENT_H_