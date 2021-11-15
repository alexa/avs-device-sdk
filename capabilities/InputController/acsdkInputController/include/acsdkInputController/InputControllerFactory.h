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

#ifndef ACSDKINPUTCONTROLLER_INPUTCONTROLLERFACTORY_H_
#define ACSDKINPUTCONTROLLER_INPUTCONTROLLERFACTORY_H_

#include <memory>

#include <acsdkInputControllerInterfaces/InputControllerHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkInputController {

/// This structure contains the interfaces to interact with the InputController Capability Agent.
struct InputControllerFactoryInterfaces {
    /// Interface for handling @c AVSDirectives.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;

    /// Interface providing CapabilitiesDelegate access to the version and configurations of the capabilities.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface>
        capabilityConfigurationInterface;
};

/**
 * Creates a new InputController capability agent
 *
 * @param handler The handler for InputController input updates.
 * @param exceptionSender The object to use for sending AVS Exception messages.
 * @return An @c Optional @c InputControllerFactoryInterfaces object.
 */
avsCommon::utils::Optional<InputControllerFactoryInterfaces> create(
    const std::shared_ptr<acsdkInputControllerInterfaces::InputControllerHandlerInterface>& handler,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);

}  // namespace acsdkInputController
}  // namespace alexaClientSDK

#endif  // ACSDKINPUTCONTROLLER_INPUTCONTROLLERFACTORY_H_
