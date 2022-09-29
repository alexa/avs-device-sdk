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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATIONORCHESTRATORCLIENT_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATIONORCHESTRATORCLIENT_H_

#include <memory>
#include <string>
#include <unordered_map>

#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorClientInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>

#include "acsdk/PresentationOrchestratorClient/private/MultiWindowManagerInterface.h"
#include "acsdk/PresentationOrchestratorClient/private/PresentationLifespanToTimeoutMapper.h"
#include "acsdk/PresentationOrchestratorClient/private/WindowManager.h"

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

/**
 * The PresentationOrchestratorClient class.
 * Takes window requests from clients and manages the lifespan and status of presentations across windows.
 * Notifies the PresentationOrchestratorStateTracker when state changes occur to ensure state is reported accurately.
 */
class PresentationOrchestratorClient
        : public presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface
        , public presentationOrchestratorInterfaces::PresentationOrchestratorInterface
        , public presentationOrchestratorInterfaces::PresentationOrchestratorWindowObserverInterface
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public MultiWindowManagerInterface
        , public std::enable_shared_from_this<PresentationOrchestratorClient> {
public:
    /**
     * Create an instance of @c PresentationOrchestratorClient
     * @param clientId Id for PO Client
     * @param stateTracker Pointer to @c PresentationOrchestratorStateTrackerInterface
     * @param visualTimeoutManager Pointer to @c VisualTimeoutManagerInterface
     * @return @c PresentationOrchestratorClient instance
     */
    static std::shared_ptr<PresentationOrchestratorClient> create(
        const std::string& clientId,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface> stateTracker,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager);

    /// @c PresentationOrchestratorClientInterface functions
    /// @{
    presentationOrchestratorInterfaces::PresentationRequestToken requestWindow(
        const std::string& windowId,
        presentationOrchestratorInterfaces::PresentationOptions options,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer) override;
    /// @}

    /// @c PresentationOrchestratorInterface functions
    /// @{
    void clearPresentations() override;
    bool navigateBack() override;
    /// @}

    /// @c PresentationOrchestratorWindowObserverInterface functions
    void onWindowAdded(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance) override;
    void onWindowModified(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance) override;
    void onWindowRemoved(const std::string& windowId) override;
    /// @}

    /// @c MultiWindowManagerInterface functions
    /// @{
    void prepareToForegroundWindow(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstanceToForeground,
        ForegroundWindowCallback foregroundWindowCallback) override;

    void updateForegroundWindow() override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Set the executor used as the worker thread
     * @param executor The @c Executor to set
     * @note This function should only be used for testing purposes. No call to any other method should
     * be done prior to this call.
     */
    void setExecutor(const std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor>& executor);

private:
    /**
     * Constructor
     * @param clientId Id for PO Client
     * @param stateTracker Pointer to @c PresentationOrchestratorStateTrackerInterface
     * @param visualTimeoutManager Pointer to @c VisualTimeoutManagerInterface
     */
    PresentationOrchestratorClient(
        const std::string& clientId,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface> stateTracker,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager);

    /**
     * Internal method to process request of window and presentation
     * @param requestToken the unique identifier for this request, this will be included when observer functions are
     * called. onPresentationAvailable will be called on the observer interface once the presentation is available for
     * use.
     * @param windowId The window ID being requested
     * @param options The presentation options for this request
     * @param observer The observer for this window - all state changes for this presentation will be reported to this
     */
    void executeRequestWindow(
        const presentationOrchestratorInterfaces::PresentationRequestToken& requestToken,
        const std::string& windowId,
        presentationOrchestratorInterfaces::PresentationOptions options,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer);

    /**
     * Handle any prerequisite tasks for windows other than the specified window instance, before its foregrounded.
     * These may include unfocusing a window or clearing presentations from windows with higher zOrderIndex
     * @param windowInstanceToForeground The window instance to be foregrounded
     * @param foregroundWindowCallback The callback to be invoked once the specified window instance can be foregrounded
     */
    void executePrepareToForegroundWindow(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstanceToForeground,
        ForegroundWindowCallback foregroundWindowCallback);

    /**
     * Internal method to process modifications in specified window instance
     * @param windowInstance The updated window instance
     */
    void executeOnWindowModified(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance);

    /**
     * Internal method to process removal of a specified window
     * @param windowId ID of window being removed
     */
    void executeOnWindowRemoved(const std::string& windowId);

    /**
     * Internal method to retrieve the @c WindowManager which owns the foreground focused presentation
     * @return shared_ptr to foreground @c WindowManager, or nullptr if no @c WindowManager has foreground focus
     */
    std::shared_ptr<WindowManager> executeGetFocusedWindowManager();

    /**
     * A counter used to increment the token when a new window is requested.
     * Incrementing should be atomic operation and variable must be used only in worker thread.
     */
    std::atomic<presentationOrchestratorInterfaces::PresentationRequestToken> m_requestTokenCounter;

    /// Map of windowId to Window manager instances
    std::unordered_map<std::string, std::shared_ptr<WindowManager>> m_windowIdToManager;

    /// Pointer to @c PresentationOrchestratorStateTrackerInterface
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface> m_stateTracker;

    /// Pointer to @c VisualTimeoutManagerInterface
    std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> m_visualTimeoutManager;

    /// Pointer to @c PresentationLifespanToTimeoutMapper
    std::shared_ptr<PresentationLifespanToTimeoutMapper> m_lifespanToTimeoutMapper;

    /// Id for PO Client
    std::string m_clientId;

    /**
     * Worker thread for the @c PresentationOrchestratorClient
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATIONORCHESTRATORCLIENT_H_
