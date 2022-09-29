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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRESENTATIONORCHESTRATORCLIENTFACTORY_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRESENTATIONORCHESTRATORCLIENTFACTORY_H_

#include <memory>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorClientInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {
class PresentationOrchestratorClientFactory {
public:
    /// Struct returned by the create method which contains instances of the interfaces exposed by the
    /// @c PresentationOrchestratorClient
    struct PresentationOrchestratorClientExports {
        /// The instance of @c PresentationOrchestratorClientInterface provided by the @c PresentationOrchestratorClient
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
            presentationOrchestratorClientInterface;
        /// The instance of @c PresentationOrchestratorInterface provided by the @c PresentationOrchestratorClient
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface>
            presentationOrchestratorInterface;
        /// Instance of @c RequiresShutdown used for cleaning up during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Creates an instance of the PresentationOrchestratorClient
     * @param stateTracker Instance of the @c PresentationOrchestratorStateTrackerInterface to use for state reporting
     * @param visualTimeoutManager Instance of the @c VisualTimeoutManagerInterface for managing timeouts
     * @return Optional containing the interfaces exported by the PresentationOrchestratorClient, or empty if creation
     * failed
     */
    static avsCommon::utils::Optional<PresentationOrchestratorClientExports> create(
        const std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>&
            stateTracker,
        const std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface>& visualTimeoutManager,
        const std::string& clientId = "default");
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRESENTATIONORCHESTRATORCLIENTFACTORY_H_
