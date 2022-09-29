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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORSTATEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORSTATEOBSERVERINTERFACE_H_

#include <string>

#include "PresentationOrchestratorTypes.h"

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
/**
 * Interface to be implemented by observers interested in changes to the active presentation in a window
 */
class PresentationOrchestratorStateObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~PresentationOrchestratorStateObserverInterface() = default;

    /**
     * Called when the active presentation in a window has changed
     * @param windowId The window ID which was updated
     * @param metadata The new focus metadata
     */
    virtual void onStateChanged(const std::string& windowId, const PresentationMetadata& metadata) = 0;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORSTATEOBSERVERINTERFACE_H_
