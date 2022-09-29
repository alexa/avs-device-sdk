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

#ifndef ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTCONTROLLERINTERFACE_H_
#define ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTCONTROLLERINTERFACE_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "InputControllerObserverInterface.h"
#include "InputType.h"

namespace alexaClientSDK {
namespace alexaInputControllerInterfaces {

/**
 * An interface to handle input changes from InputController.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class InputControllerInterface {
public:
    /**
     * Utility object used for reporting InputController handler response.
     */
    struct Response {
        /**
         * Enum for the different response types understood by the InputController capability agent.
         */
        enum class Type {
            /// InputController Request was handled successfully.
            SUCCESS,
            /// The directive contains a value that's not valid for the target endpoint. For example, an invalid input.
            FAILED_INVALID_VALUE,
            /// The number of allowed failed attempts to perform a InputController action has been exceeded.
            FAILED_TOO_MANY_FAILED_ATTEMPTS,
            /// Indicates the endpoint is unreachable or offline.
            FAILED_ENDPOINT_UNREACHABLE,
            /// Indicates that an error occurred that can't be described by one of the other error types.
            FAILED_INTERNAL_ERROR
        };

        /**
         * Default Constructor, set the response type to success.
         */
        Response() : type{Type::SUCCESS} {};

        /**
         * Constructor.
         * @param type The response type @c Type
         * @param errorMessage The error message if @c type is anything other than SUCCESS.
         *
         */
        Response(Type type, std::string errorMessage) : type{type}, errorMessage{std::move(errorMessage)} {};

        /// Response type for InputController handler responses.
        Type type;

        /// The error message for logging if the @c type is anything other than SUCCESS, for the purposes of
        /// aiding debugging.
        std::string errorMessage;
    };

    /**
     * Destructor.
     */
    virtual ~InputControllerInterface() = default;

    /**
     * A set of pairs to specify the supported inputs of the device.
     * The first element of a pair is the Input type, and the second element is the set of friendly names for that
     * input.
     * @note Each friendly name must be unique across inputs for this device
     */
    using SupportedInputs = std::vector<std::pair<Input, std::unordered_set<std::string>>>;

    /*
     * Return the inputs supported.
     *
     * @return the set of inputs the device supports.
     */
    virtual SupportedInputs getSupportedInputs() = 0;

    /**
     * Set the input.
     *
     * @param input The desired input of the endpoint, which is one of those that is available from the device.
     * @return whether the input was successfully set, or if an error was encountered in the process. @c
     * InputControllerInterface::Response.type should return SUCCESS if no errors were encountered. Otherwise, @c
     * InputControllerInterface::Response.type should contain the corresponding error code along with a log message in
     * @c InputControllerInterface::Response.errorMessage.
     */
    virtual Response setInput(Input input) = 0;

    /**
     * Get the current input.
     *
     * @return the current input
     */
    virtual Input getInput() = 0;

    /**
     * Adds a @c InputControllerObserverInterface observer.
     *
     * @note If InputController implementation has configured its instance's property as proactively reported,
     * then it is required to notify observers of @c InputControllerObserverInterface for any change in its property
     * state. This includes notifying the value when the device starts, if it is different from the last reported value.
     *
     * @param observer The pointer to the @c InputControllerObserverInterface.
     * @return @c true if the object supports observer notification and observer was successfully added; otherwise,
     * return @c false
     */
    virtual bool addObserver(std::weak_ptr<InputControllerObserverInterface> observer) = 0;

    /**
     * Removes a observer of @c InputControllerObserverInterface.
     *
     * @param observer The pointer to the @c InputControllerObserverInterface.
     */
    virtual void removeObserver(std::weak_ptr<InputControllerObserverInterface> observer) = 0;
};

}  // namespace alexaInputControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTCONTROLLERINTERFACE_H_
