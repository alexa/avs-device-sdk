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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATION_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATION_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationObserverInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>
#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

/// Forward declaration for @c WindowManager class
class WindowManager;

/**
 * Class which represents an instance of a PresentationInterface returned by the Presentation Orchestrator
 */
class Presentation
        : public presentationOrchestratorInterfaces::PresentationInterface
        , public std::enable_shared_from_this<Presentation> {
public:
    /**
     * Create an instance of @c Presentation
     * @param observer Observer to notify of presentation availability and state changes
     * @param options presentation options
     * @param requestToken the unique identifier included when @c PresentationObserverInterface functions are called
     * @param state presentation state
     * @param visualTimeoutManager Pointer to @c VisualTimeoutManagerInterface
     * @param windowManager Pointer to @c WindowManager
     * @param lifespanToTimeoutMapper Pointer to @c PresentationLifespanToTimeoutMapper
     * @return @c Presentation instance
     */
    static std::shared_ptr<Presentation> create(
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer,
        presentationOrchestratorInterfaces::PresentationOptions options,
        presentationOrchestratorInterfaces::PresentationRequestToken requestToken,
        presentationOrchestratorInterfaces::PresentationState state,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager,
        std::shared_ptr<WindowManager> windowManager);

    /// @c PresentationInterface functions
    /// @{
    void dismiss() override;
    void foreground() override;
    void setMetadata(const std::string& metadata) override;
    void setLifespan(const presentationOrchestratorInterfaces::PresentationLifespan& lifespan) override;
    void startTimeout() override;
    void stopTimeout() override;
    void setTimeout(const std::chrono::milliseconds& timeout) override;
    presentationOrchestratorInterfaces::PresentationState getState() override;
    /// @}

    /**
     * Getter for metadata
     * @return metadata for the presentation
     */
    std::string getMetadata() const;

    /**
     * Getter for interface
     * @return interface associated with the presentation
     */
    std::string getInterface() const;

    /**
     * Getter for request token
     * @return token specified when presentation was requested
     */
    presentationOrchestratorInterfaces::PresentationRequestToken getRequestToken() const;

    /**
     * Getter for presentation lifespan
     * @return presentation lifespan
     */
    presentationOrchestratorInterfaces::PresentationLifespan getLifespan() const;

    /**
     * Method to update the presentation state
     * @param newState Presentation state
     */
    void setState(presentationOrchestratorInterfaces::PresentationState newState);

    /**
     * Method to perform custom back navigation on the presentation if supported
     * @return true if custom back navigation on presentation was handled, false otherwise
     */
    bool navigateBack();

private:
    /**
     * Constructor
     * @param observer Observer to notify of presentation availability and state changes
     * @param options presentation options
     * @param requestToken the unique identifier included when @c PresentationObserverInterface functions are called
     * @param state presentation state
     * @param visualTimeoutManager Pointer to @c VisualTimeoutManagerInterface
     * @param windowManager Pointer to @c WindowManager
     * @param customTimeout A custom timeout value specified in initial options post validation, -1 to use default
     */
    Presentation(
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> observer,
        presentationOrchestratorInterfaces::PresentationOptions options,
        presentationOrchestratorInterfaces::PresentationRequestToken requestToken,
        presentationOrchestratorInterfaces::PresentationState state,
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> visualTimeoutManager,
        std::shared_ptr<WindowManager> windowManager,
        std::chrono::milliseconds customTimeout);

    /**
     * Internal method to start the timeout assuming that the lock for m_options has already been acquired
     */
    void startTimeoutLocked();

    /**
     * Internal method to set the timeout assuming that the lock for m_options has already been acquired
     * @param timeout timeout value to be set
     */
    void setTimeoutLocked(const std::chrono::milliseconds& timeout);

    /**
     * Internal method to validate timeout value
     * @param timeout timeout value
     * @return true if timeout value is valid, false otherwise
     */
    static bool validateTimeout(const std::chrono::milliseconds& timeout);

    /// Pointer to @c PresentationObserverInterface for notifying presentation state change
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationObserverInterface> m_observer;

    /// Tracks current @c PresentationOptions associated with presentation
    presentationOrchestratorInterfaces::PresentationOptions m_options;

    /// Mutex to serialize access to m_options
    mutable std::mutex m_optionsMutex;

    /// Tracks current state of presentation
    std::atomic<presentationOrchestratorInterfaces::PresentationState> m_state;

    /// Token specified when presentation became available
    const presentationOrchestratorInterfaces::PresentationRequestToken m_requestToken;

    /// Tracks timeoutId from latest request made to @c VisualTimeoutManagerInterface
    std::atomic<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface::VisualTimeoutId> m_lastTimeoutId;

    /// Pointer to @c VisualTimeoutManagerInterface for requesting/stopping timeout
    std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface> m_visualTimeoutManager;

    /// Pointer to @c WindowManager for invoking operations on it's worker thread and presentation stack
    std::shared_ptr<WindowManager> m_windowManager;

    /// Custom timeout post validation specified in initial options. Not expected to change once initialized
    const std::chrono::milliseconds m_customTimeout;
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATION_H_
