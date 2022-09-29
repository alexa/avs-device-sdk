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

#ifndef ACSDKALEXAKEYPADCONTROLLERINTERFACES_ALEXAKEYPADCONTROLLERINTERFACE_H_
#define ACSDKALEXAKEYPADCONTROLLERINTERFACES_ALEXAKEYPADCONTROLLERINTERFACE_H_

#include <set>
#include <string>

#include <AVSCommon/Utils/Optional.h>

#include "acsdkAlexaKeypadControllerInterfaces/Keystroke.h"

namespace alexaClientSDK {
namespace acsdkAlexaKeypadControllerInterfaces {

/**
 * The AlexaKeypadControllerInterface carries out keypad controller actions such as moving up, down, left, right or
 * scrolling through voice-control.
 *
 * An implementation of the AlexaKeypadControllerInterface. Sends information to device for sendKeystroke results.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class AlexaKeypadControllerInterface {
public:
    /**
     * Utility object used for reporting KeypadController response.
     *
     * @c type should be `SUCCESS` if there are no errors while executing keypad controller operation.
     * Otherwise, it should return the corresponding error response along with a log message @c errorMessage that would
     * be logged in the cloud, for the purposes of aiding debugging.
     */
    struct Response {
        /**
         * Enum for the different error types understood by the AlexaKeypadController capability agent.
         */
        enum class Type {
            /// Device processed command successfully without any errors
            SUCCESS,

            /// Indicates there is no additional information available when 'INFO' OR 'MORE' operation is invoked
            NO_INFORMATION_AVAILABLE,

            /// Indicates the element cannot be selected when 'SELECT' operation is invoked
            INVALID_SELECTION,

            /// Indicates the keystroke operation is not supported on the device.
            KEYSTROKE_NOT_SUPPORTED,

            /// An error occurred that can't be described by one of the other error types. For example, a runtime
            /// exception occurred. We recommend that you always send a more specific error type, if possible.
            INTERNAL_ERROR
        };

        /**
         * Default Constructor. Set the type to SUCCESS.
         */
        Response() : type{Type::SUCCESS} {};

        /// Response type for KeypadController handler responses.
        Type type;

        /// The error message for logging if the @c type is any other than SUCCESS, for the purposes of aiding
        /// debugging.
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AlexaKeypadControllerInterface() = default;

    /**
     * Execute keystroke operation on the device
     *
     * @param keystroke The @c Keystroke value user asked for
     * @return A @c Response to indicate whether keystroke event was successfully executed or
     * encountered an error while executing.
     */
    virtual Response handleKeystroke(Keystroke keystroke) = 0;

    /**
     * Returns all supported keystrokes the device is expected to understand.
     *
     * @return A @c Keystroke set object that contains supported key stroke properties. e.g. UP, DOWN, LEFT, RIGHT,
     * SELECT
     */
    virtual std::set<Keystroke> getSupportedKeys() = 0;
};

}  // namespace acsdkAlexaKeypadControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAKEYPADCONTROLLERINTERFACES_ALEXAKEYPADCONTROLLERINTERFACE_H_
