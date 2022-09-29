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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORSTATETRACKER_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORSTATETRACKER_PRESENTATIONORCHESTRATORSTATETRACKERFACTORY_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORSTATETRACKER_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORSTATETRACKER_PRESENTATIONORCHESTRATORSTATETRACKERFACTORY_H_

#include <memory>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <AFML/ActivityTrackerInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace presentationOrchestratorStateTracker {

/**
 * Class which creates an instance of the @c PresentationOrchestratorStateTracker
 */
class PresentationOrchestratorStateTrackerFactory {
public:
    /// Struct returned by the create method which contains instances of the interfaces exposed by the
    /// @c PresentationOrchestratorStateTracker
    struct PresentationOrchestratorStateTrackerExports {
        /// The instance of @c PresentationOrchestratorStateTrackerInterface provided by the @c
        /// PresentationOrchestratorStateTracker
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
            presentationOrchestratorStateTrackerInterface;
        /// Instance of @c RequiresShutdown used for cleaning up during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Create an instance of the @c PresentationOrchestratorStateTracker
     * @param activityTrackerInterface The @c ActivityTrackerInterface to publish state changes to
     * @return The interfaces exposed by the @c PresentationOrchestratorStateTracker, or empty if creation failed
     */
    static avsCommon::utils::Optional<PresentationOrchestratorStateTrackerExports> create(
        const std::shared_ptr<afml::ActivityTrackerInterface>& activityTrackerInterface);
};

}  // namespace presentationOrchestratorStateTracker
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORSTATETRACKER_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORSTATETRACKER_PRESENTATIONORCHESTRATORSTATETRACKERFACTORY_H_
