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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_CAPTURESTATE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_CAPTURESTATE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/// Enumeration defining the states of audio input initiator.
enum class CaptureState {
    /// Start Capture.
    START,
    /// Stop Capture. Only supported for PRESS_AND_HOLD. TAP will stop capturing only when StopCapture directive
    /// is received by device.
    /// @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/speechrecognizer.html#stopcapture
    STOP,
    /// Guard option for unknown received state.
    UNKNOWN
};

/**
 * This function converts the provided string to an @c CaptureState.
 *
 * @param string The string to convert to @c CaptureState.
 * @return The @c CaptureState.
 */
inline CaptureState captureStateFromString(const std::string& string) {
    if ("START" == string) {
        return CaptureState::START;
    } else if ("STOP" == string) {
        return CaptureState::STOP;
    } else {
        return CaptureState::UNKNOWN;
    }
}

/**
 * This function converts a CaptureState to a string.
 *
 * @param state the input @c CaptureState.
 * @return the string for the input CaptureState.
 */
inline std::string captureStateToString(CaptureState state) {
    switch (state) {
        case CaptureState::START:
            return "START";
        case CaptureState::STOP:
            return "STOP";
        default:
            return "UNKNOWN";
    }
}

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_CAPTURESTATE_H_
