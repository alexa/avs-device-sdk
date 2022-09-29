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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORSTATETRACKERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORSTATETRACKERINTERFACE_H_

#include <memory>
#include <string>
#include <vector>

#include "PresentationOrchestratorStateObserverInterface.h"
#include "PresentationOrchestratorWindowObserverInterface.h"

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {

/**
 * The PresentationOrchestratorStateTrackerInterface tracks windows and presentations, the active presentation state
 * will be reported to the VisualActivityTracker in addition to any registered observers.
 */
class PresentationOrchestratorStateTrackerInterface {
public:
    virtual ~PresentationOrchestratorStateTrackerInterface() = default;

    /**
     * Acquires, or reacquires the given window ID
     * To ensure accurate state reporting it is recommended this function is called whenever a window is foregrounded
     * @param clientId The identifier for the client making this request
     * @param windowId The window ID which is being acquired
     * @param metadata The metadata for this request
     */
    virtual void acquireWindow(
        const std::string& clientId,
        const std::string& windowId,
        PresentationMetadata metadata) = 0;

    /**
     * Updates the presentation metadata, a previous acquireWindow call should already have been made
     * This function does not move this client to the front of the window stack
     * @param clientId The identifier for the client making this request
     * @param windowId The window ID to update
     * @param metadata The new metadata
     */
    virtual void updatePresentationMetadata(
        const std::string& clientId,
        const std::string& windowId,
        PresentationMetadata metadata) = 0;

    /**
     * Releases a previously acquired window
     * @param clientId The identifier for the client making this request
     * @param windowId The window ID which is being released
     */
    virtual void releaseWindow(const std::string& clientId, const std::string& windowId) = 0;

    /**
     * Retrieve the interface for the current focused presentation
     * @return The interface, or an empty string if nothing is active
     */
    virtual std::string getFocusedInterface() = 0;

    /**
     * Retrieve the current focused windowId
     * @return The window ID, or an empty string if nothing is active
     */
    virtual std::string getFocusedWindowId() = 0;

    /**
     * Sets the window instances that the orchestrator should track.
     * Will remove/update any existing orchestrator windows that are not in the set or have changed.
     * Will add any window instances in the set that are not currently handled by orchestrator.
     *
     * @param windows Array of window instances
     */
    virtual void setWindows(const std::vector<PresentationOrchestratorWindowInstance>& windows) = 0;

    /**
     * Adds a window instance to the orchestrator
     * @param window The window instance information
     * @return true if the window was added, false if the addition failed
     */
    virtual bool addWindow(const PresentationOrchestratorWindowInstance& window) = 0;

    /**
     * Removes a window instance from this orchestrator
     * @param windowId The id of the window to remove
     */
    virtual void removeWindow(const std::string& windowId) = 0;

    /**
     * Updates an existing window instance
     * @param window The window instance information, the window id must already exist
     */
    virtual void updateWindow(const PresentationOrchestratorWindowInstance& window) = 0;

    /**
     * Retrieves information for all window instances tracked by orchestrator
     * @return vector of window information
     */
    virtual std::vector<PresentationOrchestratorWindowInfo> getWindowInformation() = 0;

    /**
     * Adds an observer to be notified of window changes
     * @param observer The observer to add
     */
    virtual void addWindowObserver(std::weak_ptr<PresentationOrchestratorWindowObserverInterface> observer) = 0;

    /**
     * Removes a window observer that was previously added
     * @param observer The observer to remove
     */
    virtual void removeWindowObserver(std::weak_ptr<PresentationOrchestratorWindowObserverInterface> observer) = 0;

    /**
     * Adds an observer to be notified of state changes
     * @param observer The observer to add
     */
    virtual void addStateObserver(std::weak_ptr<PresentationOrchestratorStateObserverInterface> observer) = 0;

    /**
     * Removes an observer that was previously added
     * @param observer The observer to remove
     */
    virtual void removeStateObserver(std::weak_ptr<PresentationOrchestratorStateObserverInterface> observer) = 0;

    /**
     * Sets the interface which will be reported if no other clients have acquired a window
     * @param interfaceName The interface to report in VisualActivityTracker
     */
    virtual void setDeviceInterface(std::string interfaceName) = 0;

    /**
     * Releases the device interface which was previously set with setDeviceInterface
     */
    virtual void releaseDeviceInterface() = 0;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORSTATETRACKERINTERFACE_H_
