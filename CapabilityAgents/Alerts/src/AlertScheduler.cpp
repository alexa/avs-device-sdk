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

#include "Alerts/AlertScheduler.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

using namespace avsCommon::avs;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::timing;

/// String to identify log entries originating from this file.
static const std::string TAG("AlertScheduler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

AlertScheduler::AlertScheduler(
    std::shared_ptr<storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<renderer::RendererInterface> alertRenderer,
    std::chrono::seconds alertPastDueTimeLimit) :
        m_alertStorage{alertStorage},
        m_alertRenderer{alertRenderer},
        m_alertPastDueTimeLimit{alertPastDueTimeLimit},
        m_focusState{avsCommon::avs::FocusState::NONE} {
}

void AlertScheduler::onAlertStateChange(const std::string& alertToken, State state, const std::string& reason) {
    ACSDK_DEBUG9(LX("onAlertStateChange").d("alertToken", alertToken).d("state", state).d("reason", reason));
    m_executor.submit([this, alertToken, state, reason]() { executeOnAlertStateChange(alertToken, state, reason); });
}

bool AlertScheduler::initialize(std::shared_ptr<AlertObserverInterface> observer) {
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

    int64_t unixEpochNow = 0;
    if (!m_timeUtils.getCurrentUnixTime(&unixEpochNow)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "could not get current unix time."));
        return false;
    }

    std::vector<std::shared_ptr<Alert>> alerts;

    std::unique_lock<std::mutex> lock(m_mutex);
    m_alertStorage->load(&alerts);

    for (auto& alert : alerts) {
        if (alert->isPastDue(unixEpochNow, m_alertPastDueTimeLimit)) {
            std::string alertToken = alert->getToken();
            notifyObserver(alertToken, AlertObserverInterface::State::PAST_DUE);
            m_alertStorage->erase(alert);
        } else {
            // if it was active when the system last powered down, then re-init the state to set
            if (Alert::State::ACTIVE == alert->getState()) {
                alert->reset();
                m_alertStorage->modify(alert);
            }

            alert->setRenderer(m_alertRenderer);
            alert->setObserver(this);

            m_scheduledAlerts.insert(alert);
        }
    }

    lock.unlock();

    setTimerForNextAlert();
    return true;
}

bool AlertScheduler::scheduleAlert(std::shared_ptr<Alert> alert) {
    ACSDK_DEBUG9(LX("scheduleAlert"));
    int64_t unixEpochNow = 0;
    if (!m_timeUtils.getCurrentUnixTime(&unixEpochNow)) {
        ACSDK_ERROR(LX("scheduleAlertFailed").d("reason", "could not get current unix time."));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (getAlertLocked(alert->getToken())) {
        // This is the best default behavior.  If we send SetAlertFailed for a duplicate Alert,
        // then AVS will follow up with a DeleteAlert Directive - just to ensure the client does not
        // have a bad version of the Alert hanging around.  We already have the Alert, so let's return true,
        // so that SetAlertSucceeded will be sent back to AVS.
        ACSDK_INFO(LX("scheduleAlert").m("Duplicate SetAlert from AVS."));
        return true;
    }

    if (alert->isPastDue(unixEpochNow, m_alertPastDueTimeLimit)) {
        ACSDK_ERROR(LX("scheduleAlertFailed").d("reason", "parsed alert is past-due.  Ignoring."));
        return false;
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
    ACSDK_DEBUG9(LX("deleteAlert").d("alertToken", alertToken));
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_activeAlert && m_activeAlert->getToken() == alertToken) {
        deactivateActiveAlertHelperLocked(Alert::StopReason::AVS_STOP);
        return true;
    }

    auto alert = getAlertLocked(alertToken);

    if (!alert) {
        ACSDK_ERROR(LX("handleDeleteAlertFailed").m("could not find alert in map").d("token", alertToken));
        return false;
    }

    if (!m_alertStorage->erase(alert)) {
        ACSDK_ERROR(LX("handleDeleteAlertFailed").m("Could not erase alert from database").d("token", alertToken));
    }

    m_scheduledAlerts.erase(alert);
    setTimerForNextAlertLocked();

    return true;
}

bool AlertScheduler::isAlertActive(std::shared_ptr<Alert> alert) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isAlertActiveLocked(alert);
}

void AlertScheduler::updateFocus(avsCommon::avs::FocusState focusState) {
    ACSDK_DEBUG9(LX("updateFocus").d("focusState", focusState));
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_focusState == focusState) {
        return;
    }

    m_focusState = focusState;

    switch (m_focusState) {
        case FocusState::FOREGROUND:
            if (m_activeAlert) {
                m_activeAlert->setFocusState(m_focusState);
                auto token = m_activeAlert->getToken();
                notifyObserver(token, AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND);
            } else {
                activateNextAlertLocked();
            }
            return;

        case FocusState::BACKGROUND:
            if (m_activeAlert) {
                m_activeAlert->setFocusState(m_focusState);
                auto token = m_activeAlert->getToken();
                notifyObserver(token, AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND);
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
    ACSDK_DEBUG9(LX("onLocalStop"));
    std::lock_guard<std::mutex> lock(m_mutex);
    deactivateActiveAlertHelperLocked(Alert::StopReason::LOCAL_STOP);
}

void AlertScheduler::clearData(Alert::StopReason reason) {
    ACSDK_DEBUG9(LX("clearData"));
    std::lock_guard<std::mutex> lock(m_mutex);

    deactivateActiveAlertHelperLocked(reason);

    if (m_scheduledAlertTimer.isActive()) {
        m_scheduledAlertTimer.stop();
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
    m_scheduledAlerts.clear();
}

void AlertScheduler::executeOnAlertStateChange(std::string alertToken, State state, std::string reason) {
    ACSDK_DEBUG9(LX("executeOnAlertStateChange").d("alertToken", alertToken).d("state", state).d("reason", reason));
    std::lock_guard<std::mutex> lock(m_mutex);

    switch (state) {
        case State::READY:
            notifyObserver(alertToken, state, reason);
            break;

        case State::STARTED:
            if (m_activeAlert && Alert::State::ACTIVATING == m_activeAlert->getState()) {
                m_activeAlert->setStateActive();
                m_alertStorage->modify(m_activeAlert);
                notifyObserver(alertToken, state, reason);
            }
            break;

        case State::STOPPED:
            m_alertStorage->erase(m_activeAlert);
            m_activeAlert.reset();

            notifyObserver(alertToken, state, reason);
            setTimerForNextAlertLocked();

            break;

        case State::COMPLETED:
            m_alertStorage->erase(m_activeAlert);
            m_activeAlert.reset();

            notifyObserver(alertToken, state, reason);
            setTimerForNextAlertLocked();

            break;

        case State::SNOOZED:
            m_alertStorage->modify(m_activeAlert);
            m_scheduledAlerts.insert(m_activeAlert);
            m_activeAlert.reset();
            m_alertRenderer->setObserver(nullptr);

            notifyObserver(alertToken, state, reason);
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

        case State::ERROR:

            // clear out the alert that had the error, to avoid degenerate repeated alert behavior.

            if (m_activeAlert && m_activeAlert->getToken() == alertToken) {
                m_alertStorage->erase(m_activeAlert);
                m_activeAlert.reset();
                setTimerForNextAlertLocked();
            } else {
                auto alert = getAlertLocked(alertToken);
                if (alert) {
                    m_alertStorage->erase(alert);
                    m_scheduledAlerts.erase(alert);
                    setTimerForNextAlertLocked();
                }
            }

            notifyObserver(alertToken, state, reason);

            break;
    }
}

void AlertScheduler::notifyObserver(
    const std::string& alertToken,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG9(LX("notifyObserver").d("alertToken", alertToken).d("state", state).d("reason", reason));
    m_executor.submit([this, alertToken, state, reason]() { executeNotifyObserver(alertToken, state, reason); });
}

void AlertScheduler::executeNotifyObserver(
    std::string alertToken,
    AlertObserverInterface::State state,
    std::string reason) {
    m_observer->onAlertStateChange(alertToken, state, reason);
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
    ACSDK_DEBUG9(LX("setTimerForNextAlertLocked"));
    if (m_scheduledAlertTimer.isActive()) {
        m_scheduledAlertTimer.stop();
    }

    if (m_activeAlert) {
        ACSDK_INFO(LX("executeScheduleNextAlertForRendering").m("An alert is already active."));
        return;
    }

    if (m_scheduledAlerts.empty()) {
        ACSDK_DEBUG9(LX("executeScheduleNextAlertForRendering").m("no work to do."));
        return;
    }

    auto alert = (*m_scheduledAlerts.begin());

    int64_t timeNow;
    if (!m_timeUtils.getCurrentUnixTime(&timeNow)) {
        ACSDK_ERROR(LX("executeScheduleNextAlertForRenderingFailed").d("reason", "could not get current unix time."));
        return;
    }

    std::chrono::seconds secondsToWait{alert->getScheduledTime_Unix() - timeNow};

    if (secondsToWait < std::chrono::seconds::zero()) {
        secondsToWait = std::chrono::seconds::zero();
    }

    if (secondsToWait == std::chrono::seconds::zero()) {
        auto token = alert->getToken();
        notifyObserver(token, AlertObserverInterface::State::READY);
    } else {
        // start the timer for the next alert.
        if (!m_scheduledAlertTimer
                 .start(secondsToWait, std::bind(&AlertScheduler::onAlertReady, this, alert->getToken()))
                 .valid()) {
            ACSDK_ERROR(LX("executeScheduleNextAlertForRenderingFailed").d("reason", "startTimerFailed"));
        }
    }
}

void AlertScheduler::onAlertReady(const std::string& alertToken) {
    ACSDK_DEBUG9(LX("onAlertReady").d("alertToken", alertToken));
    notifyObserver(alertToken, AlertObserverInterface::State::READY);
}

void AlertScheduler::activateNextAlertLocked() {
    ACSDK_DEBUG9(LX("activateNextAlertLocked"));
    if (m_activeAlert) {
        ACSDK_ERROR(LX("activateNextAlertLockedFailed").d("reason", "An alert is already active."));
        return;
    }

    if (m_scheduledAlerts.empty()) {
        return;
    }

    m_activeAlert = *(m_scheduledAlerts.begin());
    m_scheduledAlerts.erase(m_scheduledAlerts.begin());

    m_activeAlert->setFocusState(m_focusState);
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

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
