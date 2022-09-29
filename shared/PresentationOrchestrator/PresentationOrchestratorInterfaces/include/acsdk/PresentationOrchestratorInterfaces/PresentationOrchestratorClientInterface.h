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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORCLIENTINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORCLIENTINTERFACE_H_

#include <memory>
#include <string>

#include "PresentationObserverInterface.h"
#include "PresentationTypes.h"

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {

class PresentationOrchestratorClientInterface {
public:
    virtual ~PresentationOrchestratorClientInterface() = default;

    /**
     * Request a GUI window for the given presentation
     * @param windowId The window ID being requested
     * @param options The options for this window request
     * @param observer The observer for this window - all state changes for this presentation will be reported to this
     * observer
     * @return the unique identifier for this request, this will be included when observer functions are called.
     * presentationAvailable will be called on the observer interface once the presentation is available for use.
     */
    virtual PresentationRequestToken requestWindow(
        const std::string& windowId,
        PresentationOptions options,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer) = 0;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORCLIENTINTERFACE_H_
