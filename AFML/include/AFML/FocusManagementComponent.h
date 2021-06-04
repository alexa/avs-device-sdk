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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUSMANAGEMENTCOMPONENT_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUSMANAGEMENTCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/VisualFocusAnnotation.h>
#include <InterruptModel/InterruptModel.h>

namespace alexaClientSDK {
namespace afml {

/**
 * Definition of a Manufactory component for focus management.
 */
using FocusManagementComponent = acsdkManufactory::Component<
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::VisualFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<interruptModel::InterruptModel>>>;

/**
 * Creates an manufactory component that exports a shared pointer to @c Annotated @c FocusManagers.
 *
 * @return A component.
 */
FocusManagementComponent getComponent();

}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUSMANAGEMENTCOMPONENT_H_
