/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTSCHEDULER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTSCHEDULER_H_

#include "Alerts/Storage/AlertStorageInterface.h"
#include "Alerts/AlertObserverInterface.h"

#include <AVSCommon/AVS/FocusState.h>

#include <set>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

/**
 * This class handles the management of AVS alerts.  This is essentially a time-ordered queue, where a timer is
 * set for the alert which must activate soonest.  As alerts are added or removed, this timer must be reset.
 */
class AlertScheduler : public AlertObserverInterface {
public:
    /**
     * A utility structure to facilitate sending Context to AVS.
     */
    struct AlertsContextInfo {
        /// All alerts that are scheduled.
        std::vector<Alert::ContextInfo> scheduledAlerts;
        /// All active alerts.
        std::vector<Alert::ContextInfo> activeAlerts;
    };

    /**
     * Constructor.
     *
     * @param alertStorage The storage object where alerts can be saved, modified and deleted.
     * @param alertRenderer The object which will handle user-perceivable effects upon alert activation.
     * @param alertPastDueTimeLimitSeconds The threshold in seconds, beyond which alerts will be considered past-due
     * and discarded.
     */
    AlertScheduler(
        std::shared_ptr<storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<renderer::RendererInterface> alertRenderer,
        std::chrono::seconds alertPastDueTimeLimitSeconds);

    void onAlertStateChange(const std::string& alertToken, State state, const std::string& reason = "") override;

    /**
     * Initialization.
     *
     * @note This function must be called before other use of an object this class.
     *
     * @param observer An observer which we will notify of all alert state changes.
     * @return Whether initialization was successful.
     */
    bool initialize(std::shared_ptr<AlertObserverInterface> observer);

    /**
     * Schedule an alert for rendering.
     *
     * @param alert The alert to be scheduled.
     * @return Whether the alert was successfully scheduled.
     */
    bool scheduleAlert(std::shared_ptr<Alert> alert);

    /**
     * Snooze an active alert to re-activate at a new specified time.  The alert, if active, will be de-activated
     * and re-scheduled for the new time.
     *
     * @param alertToken The AVS token identifying the alert.
     * @param updatedTime_ISO_8601 The new time in ISO-8601 format.
     * @return Whether we successfully snoozed the alert.
     */
    bool snoozeAlert(const std::string& alertToken, const std::string& updatedTime_ISO_8601);

    /**
     * Delete an alert from the schedule.
     *
     * @param alertToken The AVS token identifying the alert.
     * @return Whether we successfully deleted the alert.
     */
    bool deleteAlert(const std::string& alertToken);

    /**
     * Utility function to determine if an alert is currently active.
     *
     * @param alert The alert being queried.
     * @return Whether the alert is active.
     */
    bool isAlertActive(std::shared_ptr<Alert> alert);

    /**
     * Update our state of channel focus.
     *
     * @param focusState The state we now have.
     */
    void updateFocus(avsCommon::avs::FocusState focusState);

    /**
     * Provide our current channel focus state.
     *
     * @return Our current channel focus state.
     */
    avsCommon::avs::FocusState getFocusState();

    /**
     * Collects Context data for all alerts being managed.
     *
     * @return An AlertsContextInfo structure, containing all data needed.
     */
    AlertScheduler::AlertsContextInfo getContextInfo();

    /**
     * Handle a local stop.
     */
    void onLocalStop();

    /**
     * Clear all data being managed.  This includes database storage.
     *
     * @param reason What triggered the data to be cleared.
     */
    void clearData(Alert::StopReason reason = Alert::StopReason::SHUTDOWN);

    /**
     * Handle shutdown.
     */
    void shutdown();

private:
    /**
     * A handler function which will be called by our internal executor when a managed alert changes state.
     *
     * @param alertToken The AVS token identifying the alert.
     * @param state The state of the alert.
     * @param reason The reason the the state changed, if applicable.
     */
    void executeOnAlertStateChange(std::string alertToken, State state, std::string reason);

    /**
     * A utility function which wraps the executor submission to notify our observer.
     *
     * @param alertToken The AVS token identifying the alert.
     * @param state The state of the alert.
     * @param reason The reason the the state changed, if applicable.
     */
    void notifyObserver(
        const std::string& alertToken,
        AlertObserverInterface::State state,
        const std::string& reason = "");

    /**
     * A handler function which will be called by our internal executor when a managed alert changes state.
     *
     * @param alertToken The AVS token identifying the alert.
     * @param state The state of the alert.
     * @param reason The reason the the state changed, if applicable.
     */
    void executeNotifyObserver(std::string alertToken, AlertObserverInterface::State state, std::string reason = "");

    /**
     * Utility function to set the timer for the next scheduled alert.  This function requires @c m_mutex be locked.
     */
    void setTimerForNextAlertLocked();

    /**
     * Utility function to set the timer for the next scheduled alert.
     */
    void setTimerForNextAlert();

    /**
     * Utility function to activate the next scheduled alert.  This function requires @c m_mutex be locked.
     */
    void activateNextAlertLocked();

    /**
     * Utility function to be called when an alert is ready to activate.
     *
     * @param alertToken The AVS token of the alert that should become active.
     */
    void onAlertReady(const std::string& alertToken);

    /**
     * Utility function to query if a given alert is active.  This function requires @c m_mutex be locked.
     *
     * @param alert The alert to be queried.
     * @return Whether the alert is active or not.
     */
    bool isAlertActiveLocked(std::shared_ptr<Alert> alert) const;

    /**
     * A utility function to retreive a scheduled alert given its token.  This function requires @c m_mutex be locked.
     *
     * @param token The token for the alert.
     * @return The scheduled Alert, otherwise nullptr.
     */
    std::shared_ptr<Alert> getAlertLocked(const std::string& token) const;

    /**
     * A utility function to deactivate the currently active alert.  This function requires @c m_mutex be locked.
     *
     * @param The reason the alert is being stopped.
     */
    void deactivateActiveAlertHelperLocked(Alert::StopReason reason);

    /// This is used to safely access the time utilities.
    avsCommon::utils::timing::TimeUtils m_timeUtils;

    /**
     * Our observer.  Once initialized, this is only accessed within executor functions, so does not need mutex
     * protection.
     */
    std::shared_ptr<AlertObserverInterface> m_observer;

    /// Mutex for accessing all variables besides the observer.
    std::mutex m_mutex;

    /// The AlertStorage object.
    std::shared_ptr<storage::AlertStorageInterface> m_alertStorage;
    /// The AlertRenderer object.
    std::shared_ptr<renderer::RendererInterface> m_alertRenderer;

    /// The maximum time-limit in seconds for which an alert will be valid beyond its scheduled time.
    std::chrono::seconds m_alertPastDueTimeLimit;
    /// The current focus state for the Alerts channel.
    avsCommon::avs::FocusState m_focusState;

    /// The alert, if any, which is currently active.
    std::shared_ptr<Alert> m_activeAlert;
    /// All alerts which are scheduled to occur, ordered ascending by time.
    std::set<std::shared_ptr<Alert>, alerts::TimeComparator> m_scheduledAlerts;

    /// The timer for the next alert to go off, if one is not already active.
    avsCommon::utils::timing::Timer m_scheduledAlertTimer;

    /**
     * The @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_ALERTSCHEDULER_H_
