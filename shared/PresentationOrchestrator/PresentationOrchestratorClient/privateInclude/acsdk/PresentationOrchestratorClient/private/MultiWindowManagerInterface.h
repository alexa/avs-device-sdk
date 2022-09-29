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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_MULTIWINDOWMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_MULTIWINDOWMANAGERINTERFACE_H_

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorTypes.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

/**
 * Interface used by @c WindowManager to invoke operations on windows other than its internally managed window
 */
class MultiWindowManagerInterface {
public:
    /**
     * Destructor
     */
    virtual ~MultiWindowManagerInterface() = default;

    /// Callback definition for readability
    using ForegroundWindowCallback = std::function<void()>;

    /**
     * Invoke operations on windows other than the specified window instance being foregrounded.
     * These may include unfocusing a window or clearing presentations from windows with higher zOrderIndex.
     * Ensure that this call does not lead to a deadlock between the @c WindowManager and implementation of @c
     * MultiWindowManagerInterface
     * @param windowInstanceToForeground The window instance to be foregrounded
     * @param foregroundWindowCallback The callback to be invoked once the specified window instance can be foregrounded
     */
    virtual void prepareToForegroundWindow(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstanceToForeground,
        ForegroundWindowCallback foregroundWindowCallback) = 0;

    /**
     * Called when a state change has occurred to a previously foregrounded Window which may affect the state of other
     * windows.
     * Will result in the next window which should have foreground focus (if any) being updated to the foreground state.
     */
    virtual void updateForegroundWindow() = 0;
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_MULTIWINDOWMANAGERINTERFACE_H_