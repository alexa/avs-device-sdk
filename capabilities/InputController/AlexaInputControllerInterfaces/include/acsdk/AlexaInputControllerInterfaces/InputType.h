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

#ifndef ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTTYPE_H_
#define ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTTYPE_H_

#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace alexaInputControllerInterfaces {

/*
 * Enumeration definitions for all supported input values as defined at
 * https://developer.amazon.com/en-US/docs/alexa/device-apis/alexa-inputcontroller.html#input-property
 */
enum class Input {
    /// Auxiliary Input 1
    AUX_1,
    /// Auxiliary Input 2
    AUX_2,
    /// Auxiliary Input 3
    AUX_3,
    /// Auxiliary Input 4
    AUX_4,
    /// Auxiliary Input 5
    AUX_5,
    /// Auxiliary Input 6
    AUX_6,
    /// Auxiliary Input 7
    AUX_7,
    /// BluRay Input
    BLURAY,
    /// Cable Input
    CABLE,
    /// CD Input
    CD,
    /// COAX Input 1
    COAX_1,
    /// COAX Input 2
    COAX_2,
    /// Composite Input 1
    COMPOSITE_1,
    /// DVD Input
    DVD,
    /// Game Input
    GAME,
    /// High Definition Radio Input
    HD_RADIO,
    /// HDMI Input 1
    HDMI_1,
    /// HDMI Input 2
    HDMI_2,
    /// HDMI Input 3
    HDMI_3,
    /// HDMI Input 4
    HDMI_4,
    /// HDMI Input 5
    HDMI_5,
    /// HDMI Input 6
    HDMI_6,
    /// HDMI Input 7
    HDMI_7,
    /// HDMI Input 8
    HDMI_8,
    /// HDMI Input 9
    HDMI_9,
    /// HDMI Input 10
    HDMI_10,
    /// HDMI Audio Relay Channel Input
    HDMI_ARC,
    /// Input 1
    INPUT_1,
    /// Input 2
    INPUT_2,
    /// Input 3
    INPUT_3,
    /// Input 4
    INPUT_4,
    /// Input 5
    INPUT_5,
    /// Input 6
    INPUT_6,
    /// Input 7
    INPUT_7,
    /// Input 8
    INPUT_8,
    /// Input 9
    INPUT_9,
    /// Input 10
    INPUT_10,
    /// IPod Input
    IPOD,
    /// Line Input 1
    LINE_1,
    /// Line Input 2
    LINE_2,
    /// Line Input 3
    LINE_3,
    /// Line Input 4
    LINE_4,
    /// Line Input 5
    LINE_5,
    /// Line Input 6
    LINE_6,
    /// Line Input 7
    LINE_7,
    /// Media Player Input
    MEDIA_PLAYER,
    /// Optical Input 1
    OPTICAL_1,
    /// Optical Input 2
    OPTICAL_2,
    /// Phono Input
    PHONO,
    /// PlayStation Input
    PLAYSTATION,
    /// PlayStation 3 Input
    PLAYSTATION_3,
    /// PlayStation 4 Input
    PLAYSTATION_4,
    /// Satellite Input
    SATELLITE,
    /// SmartCast Input
    SMARTCAST,
    /// Tuner Input
    TUNER,
    /// Television Input
    TV,
    /// USB Digital to Analog Converter Input
    USB_DAC,
    /// Video Input 1
    VIDEO_1,
    /// Video Input 2
    VIDEO_2,
    /// Video Input 3
    VIDEO_3,
    /// XBox Input
    XBOX
};

/**
 * Convert an @c Input enum to its corresponding string value. Note that any @c input that does not map
 * to @c Input enum value will return an empty string.
 *
 * @param input The Input value to convert.
 * @return The corresponding string value for @c input. Returns an empty string if @c input does not map
 * to a @c Input enum.
 */
std::string inputToString(Input input);

/**
 * Convert a string to a corresponding @c Input enum value. Note that any @c inputString that does not
 * map to @c Input enum value will return an @c Optional object with no value.
 *
 * @param inputString The input string to convert.
 * @return An @c Optional object, with the value as the corresponding @c Input enum value for @c inputString
 * if there is a match. Otherwise, return an @c Optional object with no value.
 */
avsCommon::utils::Optional<Input> stringToInput(const std::string& inputString);

}  // namespace alexaInputControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTTYPE_H_
