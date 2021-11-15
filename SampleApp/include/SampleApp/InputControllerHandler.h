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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INPUTCONTROLLERHANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INPUTCONTROLLERHANDLER_H_

#include <memory>

#include <acsdkInputControllerInterfaces/InputControllerHandlerInterface.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * Sample implementation of an @c InputControllerHandlerInterface.
 */
class InputControllerHandler : public acsdkInputControllerInterfaces::InputControllerHandlerInterface {
public:
    /**
     * Create a InputControllerHandler object.
     *
     * @return A pointer to a new InputControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<InputControllerHandler> create();

    /// @name InputControllerHandlerInterface methods
    /// @{
    virtual InputConfigurations getConfiguration() override;
    virtual bool onInputChange(const std::string& input) override;
    /// @}

private:
    /**
     * Constructor.
     */
    InputControllerHandler() = default;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_INPUTCONTROLLERHANDLER_H_
