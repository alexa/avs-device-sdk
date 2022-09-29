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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_H_

#include <memory>
#include <vector>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace speakerManagerComponent {

/// @addtogroup Lib_acsdkSpeakerManagerComponent
/// @{

/**
 * @brief Component for @c SpeakerManagerInterface.
 *
 * Definition of a Manufactory component for the avsCommon::sdkInterfaces::SpeakerManagerInterface.
 */
using SpeakerManagerComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>;

/**
 * @brief Create component for @c SpeakerManagerInterface.
 *
 * Creates an manufactory component that exports a shared pointer to an implementation of @c SpeakerManagerInterface.
 *
 * @return A component.
 */
SpeakerManagerComponent getSpeakerManagerComponent() noexcept;

/// @}

}  // namespace speakerManagerComponent
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERCOMPONENT_H_
