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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONOBSERVERINTERFACE_H_

#include <memory>
#include <string>

#include "PresentationInterface.h"
#include "PresentationTypes.h"

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
/**
 * Interface to be implemented by observers interested in state changes to a @c PresentationInterface
 */
class PresentationObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~PresentationObserverInterface() = default;

    /**
     * Called when the presentation is available for use
     * @param id The identifier for this presentation, as provided when the window was initially requested
     * @param presentation The pointer used for presentation control
     */
    virtual void onPresentationAvailable(
        PresentationRequestToken id,
        std::shared_ptr<PresentationInterface> presentation) = 0;

    /**
     * Called when the presentation state has changed
     * @param id The identifier for this presentation, as provided when the window was initially requested
     * @param newState The new presentation state
     */
    virtual void onPresentationStateChanged(PresentationRequestToken id, PresentationState newState) = 0;

    /**
     * Called by the presentation orchestrator to perform custom back navigation on a presentation if supported
     * @param id The identifier for the presentation, as provided when the window was initially requested
     * @return true if the observer handled back navigation, false if the presentation orchestrator should handle back
     */
    virtual bool onNavigateBack(PresentationRequestToken id) = 0;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONOBSERVERINTERFACE_H_
