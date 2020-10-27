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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_COMPONENT_INCLUDE_CAPTIONS_CAPTIONSCOMPONENT_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_COMPONENT_INCLUDE_CAPTIONS_CAPTIONSCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <Captions/CaptionManagerInterface.h>
#include <Captions/CaptionPresenterInterface.h>

namespace alexaClientSDK {
namespace captions {

/**
 * Definition of a Manufactory component for Captions.
 */
using CaptionsComponent = acsdkManufactory::Component<
    std::shared_ptr<CaptionManagerInterface>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>>;

/**
 * Creates an manufactory component that exports a shared pointer to an implementation of @c CaptionManagerInterface.
 *
 * @return A component.
 */
CaptionsComponent getComponent();

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_COMPONENT_INCLUDE_CAPTIONS_CAPTIONSCOMPONENT_H_