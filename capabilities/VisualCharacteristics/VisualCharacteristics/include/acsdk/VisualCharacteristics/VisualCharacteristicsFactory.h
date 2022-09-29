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

#ifndef ACSDK_VISUALCHARACTERISTICS_VISUALCHARACTERISTICSFACTORY_H_
#define ACSDK_VISUALCHARACTERISTICS_VISUALCHARACTERISTICSFACTORY_H_

#include <memory>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateObserverInterface.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace visualCharacteristics {

class VisualCharacteristicsFactory {
public:
    /**
     * Struct containing the interfaces exposed by the @c VisualCharacteristics component
     */
    struct VisualCharacteristicsExports {
        /// The instance of @c VisualCharacteristicsInterface provided by @c VisualCharacteristics
        std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsInterface> visualCharacteristicsInterface;

        /// Instance of @c RequiresShutdown used for cleaning up during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;

        /// Interface providing CapabilitiesDelegate access to the version and configurations of the capabilities.
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface> capabilityConfigurationInterface;

        /// Instance of @c PresentationOrchestratorStateObserverInterface used to register VisualCharacteristics as an
        /// observer of the Presentation Orchestrator
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface>
            presentationOrchestratorStateObserverInterface;
    };

    /**
     * Creates an instance of the @c VisualCharacteristics component
     * @param contextManager Instance of the context manager to retrieve context
     * @param exceptionEncounteredSender Instance of the exception encountered sender
     * @return The exports exposed by the VisualCharacteristics components
     */
    static avsCommon::utils::Optional<VisualCharacteristicsExports> create(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionEncounteredSender);
};

}  // namespace visualCharacteristics
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALCHARACTERISTICS_VISUALCHARACTERISTICSFACTORY_H_
