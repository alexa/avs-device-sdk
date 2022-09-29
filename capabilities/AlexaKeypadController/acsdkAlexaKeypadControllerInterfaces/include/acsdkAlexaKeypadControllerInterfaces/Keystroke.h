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

#ifndef ACSDKALEXAKEYPADCONTROLLERINTERFACES_KEYSTROKE_H_
#define ACSDKALEXAKEYPADCONTROLLERINTERFACES_KEYSTROKE_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkAlexaKeypadControllerInterfaces {

/**
 * The Keystroke enumeration carries out keypad controller actions such as moving up, down, left, right or scrolling
 * through voice-control.
 */
enum class Keystroke {
    /// Move up one unit
    UP,

    /// Move down one unit
    DOWN,

    /// Move left one unit
    LEFT,

    /// Move right one unit
    RIGHT,

    /// Select the element or item that has focus
    SELECT,

    /// Scroll in the up direction
    PAGE_UP,

    /// Scroll in the down direction
    PAGE_DOWN,

    /// Scroll in the left direction
    PAGE_LEFT,

    /// Scroll in the right direction
    PAGE_RIGHT,

    /// Jump to an additional detail page
    INFO,

    /// Display more information about the onscreen elment that has focus. For example, show content that's
    /// offscreen
    MORE,

    /// Simulate the behavior of the back button on the remote control. For example, navigate back to the previous
    /// screen.
    BACK

};

/**
 * Convert a @c Keystroke enum to its corresponding string value. Note that any @c keystroke that does not map
 * to @c Keystroke enum value will return an empty string.
 *
 * @param keystroke The keystroke value to convert.
 * @return The corresponding string value for @c keystroke. Returns an empty string if @c keystroke does not map
 * to a @c Keystroke enum.
 */
std::string keystrokeToString(Keystroke keystroke);

/**
 * Convert a string to a corresponding @c Keystroke enum value. Note that any @c keystrokeString that does not
 * map to @c Keystroke enum value will return an @c Optional object with no value.
 *
 * @param keystrokeString The keystroke string to convert.
 * @return An @c Optional object, with the value as the corresponding @c Keystroke enum value for @c keystrokeString
 * if there is a match. Otherwise, return an @c Optional object with no value.
 */
avsCommon::utils::Optional<Keystroke> stringToKeystroke(const std::string& keystrokeString);

}  // namespace acsdkAlexaKeypadControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAKEYPADCONTROLLERINTERFACES_KEYSTROKE_H_
