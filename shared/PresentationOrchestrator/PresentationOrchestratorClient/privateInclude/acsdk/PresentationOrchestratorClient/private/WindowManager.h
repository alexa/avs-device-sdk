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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_WINDOWMANAGER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_WINDOWMANAGER_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationObserverInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorTypes.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>
#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>

#include "acsdk/PresentationOrchestratorClient/private/MultiWindowManagerInterface.h"
#include "acsdk/PresentationOrchestratorClient/private/Presentation.h"
#include "acsdk/PresentationOrchestratorClient/private/PresentationLifespanToTimeoutMapper.h"
#include "acsdk/PresentationOrchestratorClient/private/ReorderableUniqueStack.h"

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

/**
 * The @c WindowManager is responsible for managing a single window instance and all associated presentations
 */
class WindowManager
        : public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<WindowManager> {
public:
    /**
     * Create an instance of @c WindowManager
     * @param clientId PO client ID responsible for the window manager
     * @param windowInstance Window instance information managed by window manager
     * @param stateTracker Pointer to @c PresentationOrchestratorStateTrackerInterface
     * @param visualTimeoutManager Pointer to @c VisualTimeoutManagerInterface
     * @param multiWindowManager Pointer to @c MultiWindowManagerInterface
     * @param lifespanToTimeoutMapper Pointer to @c PresentationLifespanToTimeoutMapper
     * @return @c WindowManager instance
     */
    static std::shared_ptr<WindowManager> create(
        const std::string& clientId,
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface> stateTracker,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager,
        std::shared_ptr<MultiWindowManagerInterface> multiWindowManager,
        std::shared_ptr<PresentationLifespanToTimeoutMapper> lifespanToTimeoutMapper);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Acquire foreground state for presentation in this window
     * @param requestToken token associated with the window request
     * @param options presentation options
     * @param observer @c PresentationObserverInterface pointer for notifying of presentation availability
     */
    void acquire(
        const presentationOrchestratorInterfaces::PresentationRequestToken& requestToken,
        presentationOrchestratorInterfaces::PresentationOptions options,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer);

    /**
     * Clear/dismiss all presentations tracked by the window manager, ignoring visual focus behavior
     */
    void clearPresentations();

    /**
     * Method to navigate back in the presentation stack being managed for this window
     * @return true if the window manager handled back navigation, false otherwise
     */
    bool navigateBack();

    /**
     * Foreground specified presentation from the stack.
     * The presentation should already be present in the stack to be foregrounded.
     * @param presentation presentation to be foregrounded
     */
    void foregroundPresentation(std::shared_ptr<Presentation> presentation);

    /**
     * Set a window and its presentation to foreground focused.
     * PO State Tracker should already be tracking the window as focused before this method is called.
     */
    void foregroundWindow();

    /**
     * Check if a window and its presentation is in foreground and focused
     * @return boolean indicating if window is foreground focused
     */
    bool isForegroundFocused();

    /**
     *  Unfocus a window and its presentation while retaining in foreground
     */
    void unfocus();

    /**
     * Dismiss specified presentation from the stack
     * @param presentation presentation to be dismissed
     * @param isSelfDismiss true if presentation dismiss method was called, false for timeout and back navigation
     */
    void dismissPresentation(std::shared_ptr<Presentation> presentation, bool isSelfDismiss = false);

    /**
     * Method to handle update in presentation metadata
     * @param presentation presentation whose metadata was updated
     */
    void onPresentationMetadataUpdate(std::shared_ptr<Presentation> presentation);

    /**
     * Method to handle update in presentation lifespan
     * @param presentation presentation whose lifespan was updated
     */
    void onPresentationLifespanUpdate(std::shared_ptr<Presentation> presentation);

    /**
     * Method to handle update in window instance configuration
     * @param windowInstance updated window instance
     */
    void setWindowInstance(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance);

    /**
     * Getter for associated window instance
     * @return the window instance information
     */
    presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance getWindowInstance();

    /**
     * Return the timeout duration corresponding to a presentation lifespan
     * @param lifespan presentation lifespan
     * @return timeout duration
     */
    std::chrono::milliseconds getTimeoutDuration(
        const presentationOrchestratorInterfaces::PresentationLifespan& lifespan);

private:
    /**
     * Constructor
     * @param clientId PO client ID responsible for the window manager
     * @param windowInstance Window instance information managed by window manager
     * @param stateTracker Pointer to @c PresentationOrchestratorStateTrackerInterface
     * @param visualTimeoutManager Pointer to @c VisualTimeoutManagerInterface
     * @param multiWindowManager Pointer to @c MultiWindowManagerInterface
     * @param lifespanToTimeoutMapper Pointer to @c PresentationLifespanToTimeoutMapper
     */
    WindowManager(
        const std::string& clientId,
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& windowInstance,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface> stateTracker,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager,
        std::shared_ptr<MultiWindowManagerInterface> multiWindowManager,
        std::shared_ptr<PresentationLifespanToTimeoutMapper> lifespanToTimeoutMapper);

    /**
     * Internal method to process request for window and presentation foreground state
     * @param requestToken token associated with the window request
     * @param options presentation options
     * @param observer @c PresentationObserverInterface pointer for notifying of presentation availability
     */
    void executeAcquire(
        const presentationOrchestratorInterfaces::PresentationRequestToken& requestToken,
        presentationOrchestratorInterfaces::PresentationOptions options,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer);

    /**
     * Internal method to dismiss specified presentation
     * @param presentation presentation to be dismissed
     * @param isSelfDismiss true if presentation dismiss method was called, false for timeout and back navigation
     */
    void executeDismissPresentation(std::shared_ptr<Presentation> presentation, bool isSelfDismiss);

    /**
     * Internal method to handle back navigations
     * @return true if the window manager handled back navigation, false otherwise
     */
    bool executeNavigateBack();

    /**
     * Internal method to foreground presentation
     * @param presentation presentation to be foregrounded
     */
    void executeForegroundPresentation(std::shared_ptr<Presentation> presentation);

    /**
     * Internal method to process update in lifespan of a presentation tracked for associated window
     * @param presentation presentation whose lifespan was updated
     */
    void executeOnPresentationLifespanUpdate(std::shared_ptr<Presentation> presentation);

    /**
     * Internal method to process update in metadata of a presentation tracked for associated window
     * @param presentation presentation whose metadata was updated
     */
    void executeOnPresentationMetadataUpdate(std::shared_ptr<Presentation> presentation);

    /**
     * Internal method to set the window and its presentation to foreground focused state.
     * PO State Tracker should already be tracking the window as focused before this method is called.
     */
    void executeForegroundWindow();

    /**
     * Internal method to determine whether the window and its presentation is in foreground and focused
     * @return boolean indicating if window is foreground focused
     */
    bool executeIsForegroundFocused();

    /**
     * Internal method to clear all presentations tracked by associated window
     */
    void executeClearPresentations();

    /**
     * Internal method to unfocus the window while retaining it in foreground
     */
    void executeUnfocus();

    /**
     * Internal method to handle state change for presentation on top of stack.
     * Called in executor before foregrounding a new presentation.
     * @param nextPresentationLifespan lifespan of next presentation to be moved to top of stack
     */
    void executeTopPresentationStateChange(
        const presentationOrchestratorInterfaces::PresentationLifespan& nextPresentationLifespan);

    /// Presentation stack
    ReorderableUniqueStack<std::shared_ptr<Presentation>> m_presentationStack;

    /// Pointer to @c PresentationOrchestratorStateTrackerInterface
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface> m_stateTracker;

    /// PO client ID responsible for the window manager
    std::string m_clientId;

    /// Window instance for the manager
    presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance m_windowInstance;

    /// Pointer to @c VisualTimeoutManagerInterface
    std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> m_visualTimeoutManager;

    /// Pointer to @c MultiWindowManagerInterface
    std::shared_ptr<MultiWindowManagerInterface> m_multiWindowManager;

    /**
     * Pointer to @c PresentationLifespanToTimeoutMapper
     * @note This is not expected to undergo thread unsafe operations and hence used directly in main class thread
     */
    std::shared_ptr<PresentationLifespanToTimeoutMapper> m_lifespanToTimeoutMapper;

    /**
     * Worker thread for the @c WindowManager
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_WINDOWMANAGER_H_