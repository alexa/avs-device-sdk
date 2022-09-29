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

#include "acsdkAlerts/AlertScheduler.h"

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

namespace alexaClientSDK {
namespace acsdkAlerts {

using namespace acsdkAlertsInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::timing;
using namespace avsCommon::utils::error;
using namespace avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
static const std::string TAG("AlertScheduler");

/// Metric Activity Name Prefix for ALERT metric source
static const std::string ALERT_METRIC_SOURCE_PREFIX = "ALERT-";

/// Metric constants related to Alerts
static const std::string ALERT_SCHEDULING_FAILED = "alertSchedulingFailed";
static const std::string ALERT_PAST_DUE_DURING_SCHEDULING = "alertpastDueWhileScheduling";
static const std::string ACTIVE_ALERT_RELOADED_DURING_SCHEDULING = "activeAlertReloadedDuringScheduling";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Submits a metric for a given count and name
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events
 * @param eventName The name of the metric event
 * @param count The count for metric event
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const std::string& eventName,
    int count) {
    if (!metricRecorder) {
        return;
    }

    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(ALERT_METRIC_SOURCE_PREFIX + eventName)
                           .addDataPoint(DataPointCounterBuilder{}.setName(eventName).increment(count).build())
                           .build();

    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric."));
        return;
    }
    recordMetric(metricRecorder, metricEvent);
}

AlertScheduler::AlertScheduler(
    std::shared_ptr<storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<renderer::RendererInterface> alertRenderer,
    std::chrono::seconds alertPastDueTimeLimit,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_alertStorage{alertStorage},
        m_alertRenderer{alertRenderer},
        m_alertPastDueTimeLimit{alertPastDueTimeLimit},
        m_focusState{avsCommon::avs::FocusState::NONE},
        m_mixingBehavior{avsCommon::avs::MixingBehavior::UNDEFINED},
        m_shouldScheduleAlerts{false},
        m_metricRecorder{metricRecorder} {
}

void AlertScheduler::onAlertStateChange(const AlertObserverInterface::AlertInfo& alertInfo) {
    ACSDK_DEBUG5(LX("onAlertStateChange")
                     .d("alertToken", alertInfo.token)
                     .d("alertType", alertInfo.type)
                     .d("state", alertInfo.state)
                     .d("reason", alertInfo.reason));
    m_executor.execute([this, alertInfo]() { executeOnAlertStateChange(alertInfo); });
}

bool AlertScheduler::initialize(
    const std::shared_ptr<AlertObserverInterface>& observer,
    const std::shared_ptr<settings::DeviceSettingsManager>& settingsManager,
    bool startAlertSchedulingOnInitialization) {
    m_shouldScheduleAlerts = startAlertSchedulingOnInitialization;

    if (!observer) {
        ACSDK_ERROR(LX("initializeFailed").m("observer was nullptr."));
        return false;
    }

    m_observer = observer;

    if (!m_alertStorage->open()) {
        ACSDK_INFO(LX("initialize").m("Couldn't open database.  Creating."));
        if (!m_alertStorage->createDatabase()) {
            ACSDK_ERROR(LX("initializeFailed").m("Could not create database."));
            return false;
        }
    }

    if (!reloadAlertsFromDatabase(settingsManager, m_shouldScheduleAlerts)) {
        ACSDK_ERROR(LX("initializeFailed").m("Could not reload alerts from database."));
        return false;
    }
    return true;
}

bool AlertScheduler::scheduleAlert(std::shared_ptr<Alert> alert) {
    ACSDK_DEBUG5(LX("scheduleAlert").d("token", alert->getToken()));
    int64_t unixEpochNow = 0;
    if (!m_timeUtils.getCurrentUnixTime(&unixEpochNow)) {
        ACSDK_ERROR(LX("scheduleAlertFailed").d("reason", "could not get current unix time."));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (alert->isPastDue(unixEpochNow, m_alertPastDueTimeLimit)) {
        ACSDK_ERROR(LX("scheduleAlertFailed").d("reason", "parsed alert is past-due.  Ignoring."));
        return false;
    }

    auto oldAlert = getAlertLocked(alert->getToken());
    if (oldAlert) {
        ACSDK_DEBUG5(LX("oldAlert").d("token", oldAlert->getToken()));

        // if the alert is already active, its a no-op
        bool alertIsCurrentlyActive = m_activeAlert && (m_activeAlert->getToken() == oldAlert->getToken());
        if (alertIsCurrentlyActive) {
            return false;
        }

        return updateAlert(oldAlert, alert->getScheduledTime_ISO_8601(), alert->getAssetConfiguration());
    }

    // it's a new alert.
    if (!m_alertStorage->store(alert)) {
        ACSDK_ERROR(LX("scheduleAlertFailed").d("reason", "could not store alert in database."));
        return false;
    }
    alert->setRenderer(m_alertRenderer);
    alert->setObserver(this);
    m_scheduledAlerts.insert(alert);

    if (!m_activeAlert) {
        setTimerForNextAlertLocked();
    }

    return true;
}

bool AlertScheduler::saveOfflineStoppedAlert(
    const std::string& alertToken,
    const std::string& scheduledTime,
    const std::string& eventTime) {
    ACSDK_DEBUG1(LX(__func__).d("token", alertToken).d("scheduledTime", scheduledTime).d("eventTime", eventTime));
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_alertStorage->storeOfflineAlert(alertToken, scheduledTime, eventTime)) {
        ACSDK_ERROR(LX("saveOfflineStoppedAlertFailed").d("reason", "could not store alert in database."));
        return false;
    }
    return true;
}

bool AlertScheduler::getOfflineStoppedAlerts(
    rapidjson::Value* alertContainer,
    rapidjson::Document::AllocatorType& allocator) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_alertStorage->loadOfflineAlerts(alertContainer, allocator)) {
        ACSDK_ERROR(LX("Unable to load alerts from offline database"));
        return false;
    }
    return true;
}

bool AlertScheduler::reloadAlertsFromDatabase(
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager,
    bool shouldScheduleAlerts) {
    m_shouldScheduleAlerts = shouldScheduleAlerts;

    int64_t unixEpochNow = 0;
    if (!m_timeUtils.getCurrentUnixTime(&unixEpochNow)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "could not get current unix time."));
        submitMetric(m_metricRecorder, ALERT_SCHEDULING_FAILED, 1);
        return false;
    } else {
        submitMetric(m_metricRecorder, ALERT_SCHEDULING_FAILED, 0);
    }

    std::vector<std::shared_ptr<Alert>> alerts;

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_scheduledAlertTimer.isActive()) {
        m_scheduledAlertTimer.stop();
    }
    m_scheduledAlerts.clear();
    m_alertStorage->load(&alerts, settingsManager);

    if (m_shouldScheduleAlerts) {
        int alertPastDueDuringSchedulingCount = 0;
        int activeAlertReloadedDuringSchedulingCount = 0;

        for (auto& alert : alerts) {
            // If the alert is active, we want to avoid modifying it so that it stays active
            bool alertIsCurrentlyActive = m_activeAlert && (m_activeAlert->getToken() == alert->getToken());
            if (!alertIsCurrentlyActive) {
                if (alert->isPastDue(unixEpochNow, m_alertPastDueTimeLimit)) {
                    notifyObserver(alert->createAlertInfo(AlertObserverInterface::State::PAST_DUE));
                    ACSDK_DEBUG5(LX(ALERT_PAST_DUE_DURING_SCHEDULING).d("alertId", alert->getToken()));
                    ++alertPastDueDuringSchedulingCount;
                    eraseAlert(alert);
                } else {
                    // if the alert was active when the system last powered down, then re-init the state to set
                    if (Alert::State::ACTIVE == alert->getState()) {
                        alert->reset();
                        m_alertStorage->modify(alert);
                        ACSDK_DEBUG5(LX(ACTIVE_ALERT_RELOADED_DURING_SCHEDULING).d("alertId", alert->getToken()));
                        ++activeAlertReloadedDuringSchedulingCount;
                    }

                    alert->setRenderer(m_alertRenderer);
                    alert->setObserver(this);

                    m_scheduledAlerts.insert(alert);
                    notifyObserver(alert->createAlertInfo(AlertObserverInterface::State::SCHEDULED_FOR_LATER));
                }
            }
        }

        // If we currently have an active alert, we don't want to set a timer for the next one yet
        if (!m_activeAlert) {
            setTimerForNextAlertLocked();
        }
        submitMetric(m_metricRecorder, ALERT_PAST_DUE_DURING_SCHEDULING, alertPastDueDuringSchedulingCount);
        submitMetric(
            m_metricRecorder, ACTIVE_ALERT_RELOADED_DURING_SCHEDULING, activeAlertReloadedDuringSchedulingCount);
    } else {
        for (auto& alert : alerts) {
            alert->setRenderer(m_alertRenderer);
            alert->setObserver(this);
            m_scheduledAlerts.insert(alert);
        }
    }

    return true;
}

bool AlertScheduler::updateAlert(
    const std::shared_ptr<Alert>& alert,
    const std::string& newScheduledTime,
    const Alert::AssetConfiguration& newAssetConfiguration) {
    ACSDK_DEBUG5(LX(__func__).d("token", alert->getToken()).m("updateAlert"));
    // Remove old alert.
    m_scheduledAlerts.erase(alert);

    // Re-insert the alert and update timer before exiting this function.
    FinallyGuard guard{[this, &alert] {
        m_scheduledAlerts.insert(alert);
        if (!m_activeAlert) {
            setTimerForNextAlertLocked();
        }
    }};

    auto oldScheduledTime = alert->getScheduledTime_ISO_8601();
    auto oldAssetConfiguration = alert->getAssetConfiguration();
    if (!alert->updateScheduledTime(newScheduledTime)) {
        ACSDK_ERROR(LX("updateAlertFailed").m("Update alert time failed."));
        return false;
    }

    if (!alert->setAssetConfiguration(newAssetConfiguration)) {
        ACSDK_ERROR(LX("updateAlertFailed").m("Update asset configuration failed."));
        alert->updateScheduledTime(oldScheduledTime);
        return false;
    }

    if (!m_alertStorage->modify(alert)) {
        ACSDK_ERROR(LX("updateAlertFailed").d("reason", "could not update alert in database."));
        alert->updateScheduledTime(oldScheduledTime);
        alert->setAssetConfiguration(oldAssetConfiguration);
        return false;
    }

    return true;
}

bool AlertScheduler::snoozeAlert(const std::string& alertToken, const std::string& updatedTime_ISO_8601) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_activeAlert || m_activeAlert->getToken() != alertToken) {
        ACSDK_ERROR(LX("snoozeAlertFailed").m("alert is not active.").d("token", alertToken));
        return false;
    }

    m_activeAlert->snooze(updatedTime_ISO_8601);

    return true;
}

bool AlertScheduler::deleteAlert(const std::string& alertToken) {
    ACSDK_DEBUG5(LX("deleteAlert").d("alertToken", alertToken));
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_activeAlert && m_activeAlert->getToken() == alertToken) {
        deactivateActiveAlertHelperLocked(Alert::StopReason::AVS_STOP);
        return true;
    }

    auto alert = getAlertLocked(alertToken);

    if (!alert) {
        ACSDK_WARN(LX(__func__).d("Alert does not exist", alertToken));
        return true;
    }

    eraseAlert(alert);

    m_scheduledAlerts.erase(alert);

    setTimerForNextAlertLocked();

    return true;
}

void AlertScheduler::deleteOfflineStoppedAlert(const std::string& token, int id) {
    ACSDK_DEBUG1(LX("deleteOfflineStoppedAlert").d("alertToken", token));
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_alertStorage->eraseOffline(token, id)) {
        ACSDK_ERROR(LX(__func__).m("Could not erase alert from offline database").d("token", token));
        return;
    }
}

bool AlertScheduler::deleteAlerts(const std::list<std::string>& tokenList) {
    ACSDK_DEBUG5(LX(__func__));

    bool deleteActiveAlert = false;
    std::list<std::shared_ptr<Alert>> alertsToBeRemoved;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& alertToken : tokenList) {
        if (m_activeAlert && m_activeAlert->getToken() == alertToken) {
            deleteActiveAlert = true;
            alertsToBeRemoved.push_back(m_activeAlert);
            ACSDK_DEBUG3(LX(__func__).m("Active alert is going to be deleted."));
            continue;
        }

        auto alert = getAlertLocked(alertToken);
        if (!alert) {
            ACSDK_WARN(LX(__func__).d("Alert is missing", alertToken));
            continue;
        }

        alertsToBeRemoved.push_back(alert);
    }

    if (!m_alertStorage->bulkErase(alertsToBeRemoved)) {
        ACSDK_ERROR(LX("deleteAlertsFailed").d("reason", "Could not erase alerts from database"));
        return false;
    }

    if (deleteActiveAlert) {
        deactivateActiveAlertHelperLocked(Alert::StopReason::AVS_STOP);
        m_activeAlert.reset();
    }

    for (auto& alert : alertsToBeRemoved) {
        m_scheduledAlerts.erase(alert);
        notifyObserver(alert->createAlertInfo(AlertObserverInterface::State::DELETED));
    }

    setTimerForNextAlertLocked();

    return true;
}

bool AlertScheduler::isAlertActive(std::shared_ptr<Alert> alert) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isAlertActiveLocked(alert);
}

std::shared_ptr<Alert> AlertScheduler::getActiveAlert() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getActiveAlertLocked();
}

void AlertScheduler::updateFocus(avsCommon::avs::FocusState focusState, avsCommon::avs::MixingBehavior behavior) {
    ACSDK_DEBUG5(LX("updateFocus").d("focusState", focusState).d("mixingBehavior", behavior));
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_focusState == focusState) {
        return;
    }

    m_focusState = focusState;
    m_mixingBehavior = behavior;

    switch (m_focusState) {
        case FocusState::FOREGROUND:
            if (m_activeAlert) {
                m_activeAlert->setFocusState(m_focusState, m_mixingBehavior);
                notifyObserver(m_activeAlert->createAlertInfo(AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND));
            } else {
                activateNextAlertLocked();
            }
            return;

        case FocusState::BACKGROUND:
            if (m_activeAlert) {
                m_activeAlert->setFocusState(m_focusState, m_mixingBehavior);
                notifyObserver(m_activeAlert->createAlertInfo(AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND));
            } else {
                activateNextAlertLocked();
            }
            return;

        case FocusState::NONE:
            deactivateActiveAlertHelperLocked(Alert::StopReason::LOCAL_STOP);
            return;
    }

    ACSDK_ERROR(LX("updateFocusFailed").d("unhandledState", focusState));
}

avsCommon::avs::FocusState AlertScheduler::getFocusState() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_focusState;
}

AlertScheduler::AlertsContextInfo AlertScheduler::getContextInfo() {
    std::lock_guard<std::mutex> lock(m_mutex);

    AlertScheduler::AlertsContextInfo alertContexts;
    for (const auto& alert : m_scheduledAlerts) {
        alertContexts.scheduledAlerts.push_back(alert->getContextInfo());
    }

    if (m_activeAlert) {
        alertContexts.scheduledAlerts.push_back(m_activeAlert->getContextInfo());
        alertContexts.activeAlerts.push_back(m_activeAlert->getContextInfo());
    }

    return alertContexts;
}

void AlertScheduler::onLocalStop() {
    ACSDK_DEBUG5(LX("onLocalStop"));
    std::lock_guard<std::mutex> lock(m_mutex);
    deactivateActiveAlertHelperLocked(Alert::StopReason::LOCAL_STOP);
}

void AlertScheduler::clearData(Alert::StopReason reason) {
    ACSDK_INFO(LX("clearData").d("reason", Alert::stopReasonToString(reason)));
    std::lock_guard<std::mutex> lock(m_mutex);

    deactivateActiveAlertHelperLocked(reason);

    if (m_scheduledAlertTimer.isActive()) {
        m_scheduledAlertTimer.stop();
    }

    for (const auto& alert : m_scheduledAlerts) {
        notifyObserver(alert->createAlertInfo(AlertObserverInterface::State::DELETED));
    }

    m_scheduledAlerts.clear();
    m_alertStorage->clearDatabase();
}

void AlertScheduler::shutdown() {
    // These variables may call other functions here in the process of stopping / shutting down.  They are
    // also internally thread safe, so the mutex is not required to invoke these calls.
    m_executor.shutdown();
    m_scheduledAlertTimer.stop();

    m_observer.reset();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_alertStorage.reset();
    m_alertRenderer.reset();
    m_activeAlert.reset();
    for (auto& alert : m_scheduledAlerts) {
        alert->setRenderer(nullptr);
    }
    m_scheduledAlerts.clear();
}

void AlertScheduler::executeOnAlertStateChange(const AlertObserverInterface::AlertInfo& alertInfo) {
    ACSDK_DEBUG1(LX("executeOnAlertStateChange")
                     .d("alertToken", alertInfo.token)
                     .d("state", alertInfo.state)
                     .d("reason", alertInfo.reason));
    std::lock_guard<std::mutex> lock(m_mutex);

    switch (alertInfo.state) {
        case State::READY:
            notifyObserver(alertInfo);
            break;

        case State::STARTED:
            if (m_activeAlert && Alert::State::ACTIVATING == m_activeAlert->getState()) {
                m_activeAlert->setStateActive();
                m_alertStorage->modify(m_activeAlert);
                notifyObserver(alertInfo);

                AlertInfo infoCopy = alertInfo;
                // In addition to notifying that an alert started, need to notify what focus state the alert is in.
                if (FocusState::FOREGROUND == m_focusState) {
                    infoCopy.state = AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND;
                    notifyObserver(infoCopy);
                } else {
                    infoCopy.state = AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND;
                    notifyObserver(infoCopy);
                }
            }
            break;

        case State::STOPPED:
            if (m_activeAlert && m_activeAlert->getToken() == alertInfo.token) {
                notifyObserver(alertInfo);
                eraseAlert(m_activeAlert);
                m_activeAlert.reset();
            } else {
                auto alert = getAlertLocked(alertInfo.token);
                if (alert) {
                    notifyObserver(alertInfo);
                    ACSDK_DEBUG(
                        (LX("erasing a stopped Alert that is no longer active").d("alertToken", alertInfo.token)));
                    eraseAlert(alert);
                } else {
                    notifyObserver(alertInfo);
                }
            }
            setTimerForNextAlertLocked();

            break;

        case State::COMPLETED:
            if (m_activeAlert) {
                notifyObserver(alertInfo);
            }
            eraseAlert(m_activeAlert);
            m_activeAlert.reset();
            setTimerForNextAlertLocked();

            break;

        case State::SNOOZED:
            m_alertStorage->modify(m_activeAlert);
            m_scheduledAlerts.insert(m_activeAlert);
            m_activeAlert.reset();
            notifyObserver(alertInfo);
            setTimerForNextAlertLocked();

            break;

        case State::PAST_DUE:
            // An alert should never send this state.
            // Instead, this class generates it to inform higher level observers.
            break;

        case State::FOCUS_ENTERED_FOREGROUND:
            // An alert should never send this state.
            // Instead, this class generates it to inform higher level observers.
            break;

        case State::FOCUS_ENTERED_BACKGROUND:
            // An alert should never send this state.
            // Instead, this class generates it to inform higher level observers.
            break;

        case State::SCHEDULED_FOR_LATER:
            // An alert should never send this state.
            // Instead, this class generates it to inform higher level observers.
            break;

        case State::DELETED:
            // An alert should never send this state.
            // Instead, this class generates it to inform higher level observers.
            break;

        case State::ERROR:

            // clear out the alert that had the error, to avoid degenerate repeated alert behavior.
            if (m_activeAlert && m_activeAlert->getToken() == alertInfo.token) {
                eraseAlert(m_activeAlert);
                m_activeAlert.reset();
                setTimerForNextAlertLocked();
            } else {
                auto alert = getAlertLocked(alertInfo.token);
                if (alert) {
                    ACSDK_DEBUG(
                        (LX("erasing Alert with an error that is no longer active").d("alertToken", alertInfo.token)));
                    eraseAlert(alert);
                    m_scheduledAlerts.erase(alert);
                    setTimerForNextAlertLocked();
                }
            }

            notifyObserver(alertInfo);

            break;
    }
}

void AlertScheduler::notifyObserver(const AlertObserverInterface::AlertInfo& alertInfo) {
    ACSDK_DEBUG5(LX("notifyObserver")
                     .d("alertToken", alertInfo.token)
                     .d("alertType", alertInfo.type)
                     .d("state", alertInfo.state)
                     .d("reason", alertInfo.reason));
    m_executor.execute([this, alertInfo]() { executeNotifyObserver(alertInfo); });
}

void AlertScheduler::executeNotifyObserver(const AlertObserverInterface::AlertInfo& alertInfo) {
    m_observer->onAlertStateChange(alertInfo);
}

void AlertScheduler::deactivateActiveAlertHelperLocked(Alert::StopReason reason) {
    if (m_activeAlert) {
        m_activeAlert->deactivate(reason);
    }
}

void AlertScheduler::setTimerForNextAlert() {
    std::lock_guard<std::mutex> lock(m_mutex);
    setTimerForNextAlertLocked();
}

void AlertScheduler::setTimerForNextAlertLocked() {
    if (m_shouldScheduleAlerts) {
        ACSDK_DEBUG5(LX("setTimerForNextAlertLocked"));
        if (m_scheduledAlertTimer.isActive()) {
            m_scheduledAlertTimer.stop();
        }

        if (m_activeAlert) {
            ACSDK_INFO(LX("executeScheduleNextAlertForRendering").m("An alert is already active."));
            return;
        }

        if (m_scheduledAlerts.empty()) {
            ACSDK_INFO(LX("executeScheduleNextAlertForRendering").m("no work to do."));
            return;
        }

        auto alert = (*m_scheduledAlerts.begin());

        int64_t timeNow;
        if (!m_timeUtils.getCurrentUnixTime(&timeNow)) {
            ACSDK_ERROR(
                LX("executeScheduleNextAlertForRenderingFailed").d("reason", "could not get current unix time."));
            return;
        }

        ACSDK_INFO((LX("executeScheduleNextAlertForRendering").d("scheduledTime", alert->getScheduledTime_Unix())));
        ACSDK_INFO((LX("executeScheduleNextAlertForRendering").d("time now", timeNow)));

        std::chrono::seconds secondsToWait{alert->getScheduledTime_Unix() - timeNow};

        if (secondsToWait < std::chrono::seconds::zero()) {
            secondsToWait = std::chrono::seconds::zero();
        }
        if (secondsToWait == std::chrono::seconds::zero()) {
            notifyObserver(alert->createAlertInfo(AlertObserverInterface::State::READY));
        } else {
            // start the timer for the next alert.
            if (!m_scheduledAlertTimer
                     .start(
                         secondsToWait,
                         std::bind(
                             &AlertScheduler::onAlertReady,
                             this,
                             alert->createAlertInfo(AlertObserverInterface::State::READY)))
                     .valid()) {
                ACSDK_ERROR(LX("executeScheduleNextAlertForRenderingFailed").d("reason", "startTimerFailed"));
            }
        }
    } else {
        ACSDK_INFO(LX("executeScheduleNextAlertForRenderingSkipped").d("reason", "m_shouldScheduleAlerts is false."));
    }
}

void AlertScheduler::onAlertReady(const AlertObserverInterface::AlertInfo& alertInfo) {
    ACSDK_DEBUG5(LX("onAlertReady").d("alertToken", alertInfo.token).d("alertType", alertInfo.type));
    notifyObserver(alertInfo);
}

void AlertScheduler::activateNextAlertLocked() {
    ACSDK_DEBUG5(LX("activateNextAlertLocked"));
    if (m_activeAlert) {
        ACSDK_ERROR(LX("activateNextAlertLockedFailed").d("reason", "An alert is already active."));
        return;
    }

    if (m_scheduledAlerts.empty()) {
        return;
    }

    m_activeAlert = *(m_scheduledAlerts.begin());
    m_scheduledAlerts.erase(m_scheduledAlerts.begin());

    m_activeAlert->setFocusState(m_focusState, m_mixingBehavior);
    m_activeAlert->activate();
}

bool AlertScheduler::isAlertActiveLocked(std::shared_ptr<Alert> alert) const {
    if (!m_activeAlert) {
        return false;
    }

    if (m_activeAlert->getToken() != alert->getToken()) {
        return false;
    }

    auto state = m_activeAlert->getState();
    return (Alert::State::ACTIVATING == state || Alert::State::ACTIVE == state);
}

std::shared_ptr<Alert> AlertScheduler::getAlertLocked(const std::string& token) const {
    for (auto& alert : m_scheduledAlerts) {
        if (token == alert->getToken()) {
            return alert;
        }
    }

    return nullptr;
}

std::shared_ptr<Alert> AlertScheduler::getActiveAlertLocked() const {
    return m_activeAlert;
}

std::list<std::shared_ptr<Alert>> AlertScheduler::getAllAlerts() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    auto list = std::list<std::shared_ptr<Alert>>(m_scheduledAlerts.begin(), m_scheduledAlerts.end());
    if (m_activeAlert) {
        list.push_back(m_activeAlert);
    }
    return list;
}

void AlertScheduler::eraseAlert(std::shared_ptr<Alert> alert) {
    ACSDK_DEBUG5(LX(__func__));
    if (!alert) {
        ACSDK_ERROR(LX("eraseAlertFailed").m("alert was nullptr"));
        return;
    }

    auto alertToken = alert->getToken();
    if (!m_alertStorage->erase(alert)) {
        ACSDK_ERROR(LX(__func__).m("Could not erase alert from database").d("token", alertToken));
        return;
    }
    notifyObserver(alert->createAlertInfo(AlertObserverInterface::State::DELETED));
}

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
