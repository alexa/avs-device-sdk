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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORWINDOWOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORWINDOWOBSERVERINTERFACE_H_

#include <string>

#include "PresentationOrchestratorTypes.h"

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
/**
 * Interface to be implemented by observers interested in changes to window configuration
 */
class PresentationOrchestratorWindowObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~PresentationOrchestratorWindowObserverInterface() = default;

    /**
     * A window was added to the window configuration
     * @param instance The window instance which was added
     */
    virtual void onWindowAdded(const PresentationOrchestratorWindowInstance& instance) = 0;

    /**
     * Called when a window instance has been modified
     * @param instance The modified window instance
     * @note the Window ID will never be changed
     */
    virtual void onWindowModified(const PresentationOrchestratorWindowInstance& instance) = 0;

    /**
     * Called when a window instance has been removed
     * @param windowId The window ID which has been removed
     */
    virtual void onWindowRemoved(const std::string& windowId) = 0;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORWINDOWOBSERVERINTERFACE_H_
