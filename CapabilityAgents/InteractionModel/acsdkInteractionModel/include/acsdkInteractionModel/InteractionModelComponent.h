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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODEL_INCLUDE_ACSDKINTERACTIONMODEL_INTERACTIONMODELCOMPONENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODEL_INCLUDE_ACSDKINTERACTIONMODEL_INTERACTIONMODELCOMPONENT_H_

#include <memory>

#include <acsdkInteractionModelInterfaces/InteractionModelNotifierInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>

namespace alexaClientSDK {
namespace acsdkInteractionModel {

/**
 * Manufactory Component definition for the @c InteractionModelNotifierInterface.
 */
using InteractionModelComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>>;
/**
 * Get the @c Manufactory component for the acsdkInteractionModel Notifier, Capability Agent, and related.
 *
 * @return The @c Manufactory component for acsdkInteractionModel.
 */
InteractionModelComponent getComponent();

}  // namespace acsdkInteractionModel
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODEL_INCLUDE_ACSDKINTERACTIONMODEL_INTERACTIONMODELCOMPONENT_H_
