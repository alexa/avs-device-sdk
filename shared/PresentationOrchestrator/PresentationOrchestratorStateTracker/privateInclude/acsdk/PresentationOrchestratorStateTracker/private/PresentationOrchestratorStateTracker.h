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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORSTATETRACKER_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORSTATETRACKER_PRIVATE_PRESENTATIONORCHESTRATORSTATETRACKER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORSTATETRACKER_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORSTATETRACKER_PRIVATE_PRESENTATIONORCHESTRATORSTATETRACKER_H_

#include <chrono>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/Notifier/Notifier.h>
#include <AFML/ActivityTrackerInterface.h>
#include <AVSCommon/SDKInterfaces/VisualFocusAnnotation.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace presentationOrchestratorStateTracker {
class PresentationOrchestratorStateTracker
        : public presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface
        , public avsCommon::utils::RequiresShutdown
        , public notifier::Notifier<presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface>
        , public notifier::Notifier<
              presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface> {
public:
    /**
     * Creates an instance of the PresentationOrchestratorStateTracker
     * @param visualActivityTracker Instance of the VisualActivityTrackerInterface to update with state updates
     * @return Instance of the PresentationOrchestratorStateTracker if creation was successfull, nullptr otherwise
     */
    static std::shared_ptr<PresentationOrchestratorStateTracker> create(
        const std::shared_ptr<afml::ActivityTrackerInterface>& visualActivityTracker);

    /// @c PresentationOrchestratorStateTrackerInterface functions
    /// @{
    void acquireWindow(
        const std::string& clientId,
        const std::string& windowId,
        presentationOrchestratorInterfaces::PresentationMetadata metadata) override;
    void updatePresentationMetadata(
        const std::string& clientId,
        const std::string& windowId,
        presentationOrchestratorInterfaces::PresentationMetadata metadata) override;
    void releaseWindow(const std::string& clientId, const std::string& windowId) override;
    std::string getFocusedInterface() override;
    std::string getFocusedWindowId() override;
    void setWindows(const std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance>&
                        windows) override;
    bool addWindow(const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& window) override;
    void removeWindow(const std::string& windowId) override;
    void updateWindow(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& window) override;
    std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInfo> getWindowInformation() override;
    void addWindowObserver(
        std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface> observer)
        override;
    void removeWindowObserver(
        std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface> observer)
        override;
    void addStateObserver(
        std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface> observer)
        override;
    void removeStateObserver(
        std::weak_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateObserverInterface> observer)
        override;
    void setDeviceInterface(std::string interfaceName) override;
    void releaseDeviceInterface() override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Set the executor used as the worker thread
     * @param executor The @c Executor to set
     * @note This function should only be used for testing purposes. No call to any other method should
     * be done prior to this call.
     */
    void setExecutor(const std::shared_ptr<avsCommon::utils::threading::Executor>& executor);

private:
    /// Constructor
    explicit PresentationOrchestratorStateTracker(
        std::shared_ptr<afml::ActivityTrackerInterface> visualActivityTracker);

    /// Internal struct holding presentation metadata
    struct PresentationMetadata {
        /// The client ID
        std::string clientId;

        /// The presentation metadata
        presentationOrchestratorInterfaces::PresentationMetadata metadata;

        /// The last time at which this presentation was acquired
        std::chrono::time_point<std::chrono::steady_clock> acquiredTime;

        /// Default constructor
        PresentationMetadata() = default;

        /// Constructor
        PresentationMetadata(
            std::string clientId,
            presentationOrchestratorInterfaces::PresentationMetadata metadata,
            std::chrono::time_point<std::chrono::steady_clock> acquiredTime = std::chrono::steady_clock::now()) :
                clientId(std::move(clientId)),
                metadata(std::move(metadata)),
                acquiredTime(std::move(acquiredTime)) {
        }
    };

    /// Internal struct defining a window and its state
    struct Window {
        /// The window configuration
        presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance configuration;

        /// The Mapping from client ID to the presentations iterator inside the presentationStack
        std::unordered_map<std::string, std::list<PresentationMetadata>::iterator> clientIdToPresentation;

        /// The stack of presentations
        std::list<PresentationMetadata> presentationStack;

        /// Constructor
        explicit Window(presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance config) :
                configuration(std::move(config)) {
        }
    };

    /**
     * Notify all the state observers when a window change has occurred
     * @param windowId The window ID which was updated
     * @param metadata The new metadata
     */
    void executeNotifyStateObservers(
        const std::string& windowId,
        const presentationOrchestratorInterfaces::PresentationMetadata& metadata);

    /**
     * Updates the focused presentation, if necessary
     * @param changedWindow The window ID which triggered the change
     */
    void executeUpdateFocusedPresentation(const Window& changedWindow);

    /**
     * Updates the visual activity tracker
     */
    void executeUpdateVisualActivityTracker();

    /**
     * Executor - Sets the window instances that the orchestrator should track.
     * Will remove/update any existing orchestrator windows that are not in the set or have changed.
     * Will add any window instances in the set that are not currently handled by orchestrator.
     *
     * @param windows Array of window instances
     */
    void executeSetWindows(
        const std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance>& windows);

    /**
     * Executor - Adds a window instance to the orchestrator
     * @param window The window instance information
     */
    bool executeAddWindow(const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& window);

    /**
     * Executor - Removes a window instance from this orchestrator
     * @param windowId The id of the window to remove
     */
    void executeRemoveWindow(const std::string& windowId);

    /**
     * Executor - Updates an existing window instance
     * @param window The window instance information, the window id must already exist
     */
    void executeUpdateWindow(const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& window);

    /**
     * Gets the interface of the currently focused presentation
     * @return The interface name
     */
    std::string executeGetFocusedInterface();

    /**
     * Gets the currently focused windowId
     * @return The window id
     */
    std::string executeGetFocusedWindowId();

    /// The visual activity tracker
    std::shared_ptr<afml::ActivityTrackerInterface> m_visualActivityTracker;

    /// Map from window ID to window
    std::unordered_map<std::string, Window> m_windows;

    /// The currently focused window ID and presentation
    std::pair<std::string, PresentationMetadata> m_focusedWindowAndPresentation;

    /// The device interface as set by setDeviceInterface
    std::string m_deviceInterface;

    /// The worker thread for the @c PresentationOrchestratorStateTracker.
    std::shared_ptr<avsCommon::utils::threading::Executor> m_executor;
};
}  // namespace presentationOrchestratorStateTracker
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORSTATETRACKER_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORSTATETRACKER_PRIVATE_PRESENTATIONORCHESTRATORSTATETRACKER_H_
