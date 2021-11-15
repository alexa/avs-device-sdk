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

#ifndef ACSDKINPUTCONTROLLERINTERFACES_INPUTCONTROLLERHANDLERINTERFACE_H_
#define ACSDKINPUTCONTROLLERINTERFACES_INPUTCONTROLLERHANDLERINTERFACE_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace alexaClientSDK {
namespace acsdkInputControllerInterfaces {

/**
 * An interface to handle input changes from InputController.
 */
class InputControllerHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~InputControllerHandlerInterface() = default;

    /**
     * Alias to a type used for defining the inputs.  The key of the map is the input, and the set is the
     * friendlyNames associated with the input.  For more information, please refer to the Alexa.InputController API.
     *
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/inputcontroller.html
     */
    using InputFriendlyNameType = std::unordered_map<std::string, std::unordered_set<std::string>>;

    /// The configuration of the inputs on the device.
    struct InputConfigurations {
        /// Inputs and its friendly names of the device.
        InputFriendlyNameType inputs;
    };

    /**
     * A function to get the input configuration of the device.
     *
     * @return The @c InputConfigurations of the device.
     */
    virtual InputConfigurations getConfiguration() = 0;

    /**
     * A callback function to request the change of the input on the device.  The @c InputController does not remember
     * the previous input, so this callback will be called whenever AVS notifies a change in input.  Also, during
     * initialization, the application is responsible for remembering the previous input, as the @c InputController
     * does not notify the application of the previous input with this callback.
     *
     * @param input The selected input on the product.  The input is guaranteed to be one of the inputs as specified in
     * the @c InputConfigurations from @c getConfiguration().
     * @return A bool indicate if the change in input is successful or not.
     *
     */
    virtual bool onInputChange(const std::string& input) = 0;
};

}  // namespace acsdkInputControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKINPUTCONTROLLERINTERFACES_INPUTCONTROLLERHANDLERINTERFACE_H_
