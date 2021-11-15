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

#ifndef ALEXA_CLIENT_SDK_ACSDKALERTS_INCLUDE_ACSDKALERTS_ALERTSCHEDULER_H_
#define ALEXA_CLIENT_SDK_ACSDKALERTS_INCLUDE_ACSDKALERTS_ALERTSCHEDULER_H_

#include "acsdkAlerts/Storage/AlertStorageInterface.h"

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/AVS/MixingBehavior.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <Settings/DeviceSettingsManager.h>
#include <acsdkAlertsInterfaces/AlertObserverInterface.h>

#include <list>
#include <set>
#include <string>

namespace alexaClientSDK {
namespace acsdkAlerts {

/**
 * This class handles the management of AVS alerts.  This is essentially a time-ordered queue, where a timer is
 * set for the alert which must activate soonest.  As alerts are added or removed, this timer must be reset.
 */
class AlertScheduler : public acsdkAlertsInterfaces::AlertObserverInterface {
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
     * @param metricRecorder The metric recorder.
     */
    AlertScheduler(
        std::shared_ptr<storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<renderer::RendererInterface> alertRenderer,
        std::chrono::seconds alertPastDueTimeLimitSeconds,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    /// @name AlertObserverInterface function.
    /// @{
    void onAlertStateChange(const AlertInfo& alertInfo) override;
    /// @}

    /**
     * Initialization.
     *
     * @note This function must be called before other use of an object this class.
     *
     * @param observer An observer which we will notify of all alert state changes.
     * @param m_settingsManager A settingsManager object that manages alarm volume ramp setting.
     * @param startAlertSchedulingOnInitialization Whether to start scheduling alerts after client initialization. If
     * this is set to false, no alert scheduling will occur until onSystemClockSynchronized is called.
     * @return Whether initialization was successful.
     */
    bool initialize(
        const std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface>& observer,
        const std::shared_ptr<settings::DeviceSettingsManager>& settingsManager,
        bool startAlertSchedulingOnInitialization = true);

    /**
     * Schedule an alert for rendering.
     *
     * @param alert The alert to be scheduled.
     * @return Whether the alert was successfully scheduled.
     */
    bool scheduleAlert(std::shared_ptr<Alert> alert);

    /**
     * Save the currently active alert as an offline stopped alert
     * @param alertToken the alert token of the offline stopped alert
     * @param scheduledTime the scheduled time of the offline stopped alert
     * @param eventTime the time the alert stopped
     * @return Whether the offline alert was successfully saved.
     */
    bool saveOfflineStoppedAlert(
        const std::string& alertToken,
        const std::string& scheduledTime,
        const std::string& eventTime);

    /**
     * Get list of offline stopped alerts
     *
     * @param alertContainer rapidJson array to load alerts into
     * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
     * rapidjson::Value objects.
     */
    bool getOfflineStoppedAlerts(rapidjson::Value* alertContainer, rapidjson::Document::AllocatorType& allocator);

    /**
     * Reload alerts from database, then update expired alerts and set a timer for the next alert if desired. If there
     * is an active alert, it will not interrupted or modified in any way.
     *
     * @param settingsManager A settingsManager object used to load alerts.
     * @param shouldScheduleAlerts Whether to start scheduling alerts.
     * @return Whether the alerts were reloaded from database successfully.
     */
    bool reloadAlertsFromDatabase(
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager,
        bool shouldScheduleAlerts);

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
     * Delete multiple alerts from the schedule by their tokens. All existing alerts are deleted with all-or-none rule.
     * In case of failure no actual deletion is made. Missing alert is not treated as an error.
     *
     * @param tokenList List of tokens of the alerts to be deleted
     * @return true if all alerts has been deleted, false if any of the deletion failed.
     */
    bool deleteAlerts(const std::list<std::string>& tokenList);

    /**
     * Delete an offline stopped alert from list of offline stopped alerts
     *
     * @param token The alertToken of the alert to be deleted
     * @param id The alert Id of the alert to be deleted
     */
    void deleteOfflineStoppedAlert(const std::string& token, int id);

    /**
     * Utility function to determine if an alert is currently active.
     *
     * @param alert The alert being queried.
     * @return Whether the alert is active.
     */
    bool isAlertActive(std::shared_ptr<Alert> alert);

    /**
     * Gets the current active alert.
     *
     * @return the current active alert, else nullptr.
     */
    std::shared_ptr<Alert> getActiveAlert();

    /**
     * Update our state of channel focus.
     *
     * @param focusState The state we now have.
     * @param behavior The mixing behavior.
     */
    void updateFocus(avsCommon::avs::FocusState focusState, avsCommon::avs::MixingBehavior behavior);

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

    /**
     * Utility method to get list of all alerts being tracked by @c AlertScheduler
     *
     * @return list of all alerts being tracked by @c AlertScheduler
     */
    std::list<std::shared_ptr<Alert>> getAllAlerts();

private:
    /**
     * A handler function which will be called by our internal executor when a managed alert changes state.
     *
     * @param alertInfo The information of the alert.
     */
    void executeOnAlertStateChange(const AlertInfo& alertInfo);

    /**
     * Update an alert with the new Dynamic Data (scheduled time, assets). This function cannot update an active alert
     * (use snooze instead).
     *
     * @param alert The alert to be rescheduled. The alert MUST be inactive.
     * @param newScheduledTime The new scheduled time.
     * @param newAssetConfiguration The new asset configuration.
     * @note The caller should validate the new schedule which should not be more than 30 minutes in the past.
     * @return Whether the alert was successfully rescheduled.
     */
    bool updateAlert(
        const std::shared_ptr<Alert>& alert,
        const std::string& newScheduledTime,
        const Alert::AssetConfiguration& newAssetConfiguration);

    /**
     * A utility function which wraps the executor submission to notify our observer.
     *
     * @param alertInfo The information of the alert.
     */
    void notifyObserver(const acsdkAlertsInterfaces::AlertObserverInterface::AlertInfo& alertInfo);

    /**
     * A handler function which will be called by our internal executor when a managed alert changes state.
     *
     * @param alertInfo The information of the alert.
     */
    void executeNotifyObserver(const acsdkAlertsInterfaces::AlertObserverInterface::AlertInfo& alertInfo);

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
     * @param alertInfo The information of the alert.
     */
    void onAlertReady(const acsdkAlertsInterfaces::AlertObserverInterface::AlertInfo& alertInfo);

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
     * A utility function to retreive the currently active alert.  This function requires @c m_mutex be locked.
     *
     * @return The currently active alert, otherwise nullptr.
     */
    std::shared_ptr<Alert> getActiveAlertLocked() const;

    /**
     * A utility function to deactivate the currently active alert.  This function requires @c m_mutex be locked.
     *
     * @param The reason the alert is being stopped.
     */
    void deactivateActiveAlertHelperLocked(Alert::StopReason reason);

    /**
     * A handler function to erase the alert from the alert storage database and then to notify it's observers
     * that the alert has been deleted.
     *
     * @param alert  The alert to be erased.
     */
    void eraseAlert(std::shared_ptr<Alert> alert);

    /// This is used to safely access the time utilities.
    avsCommon::utils::timing::TimeUtils m_timeUtils;

    /**
     * Our observer.  Once initialized, this is only accessed within executor functions, so does not need mutex
     * protection.
     */
    std::shared_ptr<acsdkAlertsInterfaces::AlertObserverInterface> m_observer;

    /// The settings manager used to retrieve the value of alarm volume ramp setting.
    std::shared_ptr<settings::DeviceSettingsManager> m_settingsManager;

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
    /// The current behavior state for the Alerts channel.
    avsCommon::avs::MixingBehavior m_mixingBehavior;
    /// The alert, if any, which is currently active.
    std::shared_ptr<Alert> m_activeAlert;
    /// All alerts which are scheduled to occur, ordered ascending by time.
    std::set<std::shared_ptr<Alert>, acsdkAlerts::TimeComparator> m_scheduledAlerts;

    /// The timer for the next alert to go off, if one is not already active.
    avsCommon::utils::timing::Timer m_scheduledAlertTimer;

    /// Whether to schedule alerts. m_scheduledAlertTimer will not be set while this is false.
    bool m_shouldScheduleAlerts;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /**
     * The @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKALERTS_INCLUDE_ACSDKALERTS_ALERTSCHEDULER_H_
