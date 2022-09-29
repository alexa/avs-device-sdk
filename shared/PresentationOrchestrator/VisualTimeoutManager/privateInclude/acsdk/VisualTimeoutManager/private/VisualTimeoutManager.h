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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_PRIVATEINCLUDE_ACSDK_VISUALTIMEOUTMANAGER_PRIVATE_VISUALTIMEOUTMANAGER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_PRIVATEINCLUDE_ACSDK_VISUALTIMEOUTMANAGER_PRIVATE_VISUALTIMEOUTMANAGER_H_

#include <chrono>
#include <memory>
#include <string>
#include <ostream>
#include <unordered_set>

#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace visualTimeoutManager {

/**
 * Class which manages timeouts for visual experiences, tracks dialog and GUI events to reset timeout appropriately
 */
class VisualTimeoutManager
        : public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public presentationOrchestratorInterfaces::VisualTimeoutManagerInterface {
public:
    /**
     * Creates an instance of @c VisualTimeoutManager.
     *
     * @param timerDelegateFactory The optional TimerDelegateFactoryInterface object used to initialize timer for
     * testing.
     */
    static std::shared_ptr<VisualTimeoutManager> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface>
            timerDelegateFactory = nullptr);

    /// @c VisualTimeoutManagerInterface functions
    /// @{
    VisualTimeoutId requestTimeout(std::chrono::milliseconds delay, VisualTimeoutCallback timeoutCallback) override;
    bool stopTimeout(VisualTimeoutId timeoutId) override;
    /// @}

    /// @name DialogUXStateObserverInterface Functions
    /// @{
    void onDialogUXStateChanged(
        alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) override;
    /// @}

    /// @name ActivityEventObserverInterface Functions
    /// @{
    void onGUIActivityEventReceived(
        const std::string& source,
        const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent) override;
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
     * Enum to track the (internal) state of the VisualTimeout
     */
    enum class VisualTimeoutState {
        /**
         * Represents internal timer has started and timeout is active.
         */
        ACTIVE,

        /**
         * Timeout finished executing the callback.
         */
        FINISHED,

        /**
         * Initial state before a timeout is set to active.
         */
        INIT,

        /**
         * Timeout is stopped via call to StopTimeout API.
         * Timeout cannot be activated again from this state.
         * A new request is expected to replace the timeout.
         */
        STOPPED,

        /**
         * Timeout suspended due to change in device state.
         * Same timeout can be activated again from this state.
         */
        SUSPENDED
    };

    /**
     * Converts @c VisualTimeoutState to string
     * @param @c VisualTimeoutState value
     * @return string representation
     */
    static const std::string visualTimeoutStateToString(const VisualTimeoutState& state) {
        switch (state) {
            case VisualTimeoutState::ACTIVE:
                return "ACTIVE";
            case VisualTimeoutState::FINISHED:
                return "FINISHED";
            case VisualTimeoutState::INIT:
                return "INIT";
            case VisualTimeoutState::STOPPED:
                return "STOPPED";
            case VisualTimeoutState::SUSPENDED:
                return "SUSPENDED";
        }

        return std::string();
    }

    /**
     * Write a @c VisualTimeoutState value to an @c ostream as a string.     *
     * @param An ostream to send the @c VisualTimeoutState as a string.
     * @param The @c VisualTimeoutState to convert.
     * @return The stream.
     */
    friend std::ostream& operator<<(std::ostream& stream, const VisualTimeoutState& state);

    /**
     * Struct to group attributes for VisualTimeout
     */
    struct VisualTimeoutAttributes {
        std::chrono::milliseconds delay;
        VisualTimeoutCallback timeoutCallback;
        VisualTimeoutId timeoutId;
        VisualTimeoutState state;

        VisualTimeoutAttributes() :
                timeoutId{std::numeric_limits<VisualTimeoutId>::max()},
                state(VisualTimeoutState::INIT) {
        }

        VisualTimeoutAttributes(
            std::chrono::milliseconds delay,
            VisualTimeoutCallback timeoutCallback,
            VisualTimeoutId timeoutId) :
                delay{std::move(delay)},
                timeoutCallback{std::move(timeoutCallback)},
                timeoutId{timeoutId},
                state{VisualTimeoutState::INIT} {};
    };

    /**
     * Constructor.
     *
     * @param timerDelegateFactory The optional TimerDelegateFactoryInterface object used to initialize timer for
     * testing.
     */
    VisualTimeoutManager(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface>
            timerDelegateFactory = nullptr);

    /**
     * Internal function for starting the timer
     */
    void executeStartTimer();

    /**
     * Internal function to stop the timer
     * @param timeoutId Id corresponding to the timeout, returned by requestTimeout
     * @return true if the timer was stopped, false if this timer was not active
     */
    bool executeStopTimer(VisualTimeoutId timeoutId);

    /**
     * Internal function to handle the change in DialogUXState
     * @param newState new @c DialogUXState
     */
    void executeOnDialogUXStateChanged(
        alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState);

    /**
     * Internal function to act on @c ActivityEvent
     * @param source The source of the activity event.
     * @param activityEvent The activity event.
     */
    void executeOnGUIActivityEventReceived(
        const std::string& source,
        const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent);

    /**
     * Internal function to set the state in current timeout attributes
     * @param newState @c VisualTimeoutState to be set
     */
    void executeSetState(const VisualTimeoutState& newState);

    /**
     * Internal function to execute the callback associated with current timeout
     */
    void executeCallback();

    /// Timeout attributes specified in current request
    VisualTimeoutAttributes m_currentTimeoutAttributes;

    /// Internal timer responsible for submitting callbacks to the worker thread
    alexaClientSDK::avsCommon::utils::timing::Timer m_timer;

    /// Worker thread for the @c VisualTimeoutManager
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;

    /// The current state of DialogUX. Should only be used in worker thread
    DialogUXState m_dialogUxState;

    /// Set of sources which are currently reporting activity. Should only be used in worker thread
    std::unordered_set<std::string> m_activeSources;

    /**
     * A counter used to increment the timeout id when a new timeout is requested.
     * Incrementing should be atomic operation and variable must be used only in worker thread.
     */
    VisualTimeoutId m_timeoutIdCounter;
};
}  // namespace visualTimeoutManager
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_PRIVATEINCLUDE_ACSDK_VISUALTIMEOUTMANAGER_PRIVATE_VISUALTIMEOUTMANAGER_H_
