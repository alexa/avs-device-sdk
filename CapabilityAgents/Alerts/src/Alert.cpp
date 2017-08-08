/*
* Alert.cpp
*
* Copyright 2017 Amazon.com, Inc. or its affiliates.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Alerts/Alert.h"

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <time.h>

#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

using namespace avsCommon::avs;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace avsCommon::utils::timing;
using namespace renderer;
using namespace rapidjson;

/// String for lookup of the token value in a parsed JSON document.
static const std::string KEY_TOKEN = "token";

/// String for lookup of the scheduled time value in a parsed JSON document.
static const std::string KEY_SCHEDULED_TIME = "scheduledTime";

/// We won't allow an alert to render more than 1 hour.
const std::chrono::seconds MAXIMUM_ALERT_RENDERING_TIME = std::chrono::hours(1);

/// String to identify log entries originating from this file.
static const std::string TAG("Alert");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::string Alert::stateToString(Alert::State state) {
    switch (state) {
        case Alert::State::UNSET:
            return "UNSET";
        case Alert::State::SET:
            return "SET";
        case Alert::State::ACTIVATING:
            return "ACTIVATING";
        case Alert::State::ACTIVE:
            return "ACTIVE";
        case Alert::State::SNOOZING:
            return "SNOOZING";
        case Alert::State::SNOOZED:
            return "SNOOZED";
        case Alert::State::STOPPING:
            return "STOPPING";
        case Alert::State::STOPPED:
            return "STOPPED";
        case Alert::State::COMPLETED:
            return "COMPLETED";
    }

    ACSDK_ERROR(LX("stateToStringFailed").m("Unhandled case:" + std::to_string(static_cast<int>(state))));

    return "UNKNOWN_STATE";
}

std::string Alert::stopReasonToString(Alert::StopReason stopReason) {
    switch (stopReason) {
        case Alert::StopReason::UNSET:
            return "UNSET";
        case Alert::StopReason::AVS_STOP:
            return "AVS_STOP";
        case Alert::StopReason::LOCAL_STOP:
            return "LOCAL_STOP";
        case Alert::StopReason::SHUTDOWN:
            return "SHUTDOWN";
    }

    ACSDK_ERROR(LX("stopReasonToStringFailed")
            .m("Unhandled case:" + std::to_string(static_cast<int>(stopReason))));

    return "UNKNOWN_STOP_REASON";
}

std::string Alert::parseFromJsonStatusToString(Alert::ParseFromJsonStatus parseFromJsonStatus) {
    switch (parseFromJsonStatus) {
        case Alert::ParseFromJsonStatus::OK:
            return "OK";
        case Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY:
            return "MISSING_REQUIRED_PROPERTY";
        case Alert::ParseFromJsonStatus::INVALID_VALUE:
            return "INVALID_VALUE";
    }

    ACSDK_ERROR(LX("parseFromJsonStatusToStringFailed")
            .m("Unhandled case:" + std::to_string(static_cast<int>(parseFromJsonStatus))));

    return "UNKNOWN_PARSE_FROM_JSON_STATUS";
}

Alert::Alert() :
        m_dbId{0}, m_scheduledTime_Unix{0}, m_state{State::SET},
        m_rendererState{RendererObserverInterface::State::UNSET}, m_stopReason{StopReason::UNSET},
        m_focusState{avsCommon::avs::FocusState::NONE}, m_observer{nullptr}, m_hasTimerExpired{false} {

}

Alert::~Alert() {

}

Alert::ParseFromJsonStatus Alert::parseFromJson(const rapidjson::Value & payload, std::string* errorMessage) {
    if (!retrieveValue(payload, KEY_TOKEN, &m_token)) {
        ACSDK_ERROR(LX("parseFromJsonFailed").m("could not parse token."));
        *errorMessage = "missing property: " + KEY_TOKEN;
        return ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY;
    }

    if (!retrieveValue(payload, KEY_SCHEDULED_TIME, &m_scheduledTime_ISO_8601)) {
        ACSDK_ERROR(LX("parseFromJsonFailed").m("could not parse scheduled time."));
        *errorMessage = "missing property: " + KEY_SCHEDULED_TIME;
        return ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY;
    }

    if (!convert8601TimeStringToUnix(m_scheduledTime_ISO_8601, &m_scheduledTime_Unix)) {
        ACSDK_ERROR(LX("parseFromJsonFailed")
                .m("could not convert time to unix.").d("parsed time string", m_scheduledTime_ISO_8601));
        return ParseFromJsonStatus::INVALID_VALUE;
    }

    return ParseFromJsonStatus::OK;
}

void Alert::setRenderer(std::shared_ptr<renderer::RendererInterface> renderer) {
    m_renderer = renderer;
}

void Alert::setObserver(AlertObserverInterface* observer) {
    m_observer = observer;
}

void Alert::setFocusState(FocusState focusState) {
    if (focusState == m_focusState) {
        return;
    }

    m_focusState = focusState;

    if (State::ACTIVE == m_state) {
        m_renderer->stop();
        startRenderer();
    }
}

bool Alert::setStateActive() {
    if (State::ACTIVATING != m_state) {
        ACSDK_ERROR(LX("setStateActiveFailed").d("current state", stateToString(m_state)));
        return false;
    }

    m_state = State::ACTIVE;
    return true;
}

void Alert::reset() {
    m_state = Alert::State::SET;
}

void Alert::activate() {
    if (Alert::State::ACTIVATING == m_state || Alert::State::ACTIVE == m_state) {
        ACSDK_ERROR(LX("activateFailed").m("Alert is already active."));
        return;
    }

    m_state = Alert::State::ACTIVATING;

    if (!m_maxLengthTimer.isActive()) {
        if (!m_maxLengthTimer.start(
                MAXIMUM_ALERT_RENDERING_TIME, std::bind(&Alert::onMaxTimerExpiration, this)).valid()) {
            ACSDK_ERROR(LX("executeStartFailed").d("reason", "startTimerFailed"));
        }
    }

    startRenderer();
}

void Alert::deActivate(StopReason reason) {
    m_state = Alert::State::STOPPING;
    m_stopReason = reason;
    m_maxLengthTimer.stop();
    m_renderer->stop();
}

void Alert::onRendererStateChange(RendererObserverInterface::State state, const std::string & reason) {
    switch (state) {
        case RendererObserverInterface::State::UNSET:
            // no-op
            break;

        case RendererObserverInterface::State::STARTED:
            if (State::ACTIVATING == m_state && m_observer) {

                // NOTE : we don't update state to ACTIVE here.  We leave it as ACTIVATED, allowing our owning
                // object to set the state to active when it chooses to.

                m_observer->onAlertStateChange(m_token, AlertObserverInterface::State::STARTED);
            }
            break;

        case RendererObserverInterface::State::STOPPED:
            if (m_hasTimerExpired) {
                m_state = State::COMPLETED;
                if (m_observer) {
                    m_observer->onAlertStateChange(m_token, AlertObserverInterface::State::COMPLETED);
                }
            } else {
                if (Alert::State::STOPPING == m_state) {
                    m_state = State::STOPPED;
                    if (m_observer) {
                        m_observer->onAlertStateChange(m_token, AlertObserverInterface::State::STOPPED);
                    }
                } else if (Alert::State::SNOOZING == m_state) {
                    m_state = State::SNOOZED;
                    if (m_observer) {
                        m_observer->onAlertStateChange(m_token, AlertObserverInterface::State::SNOOZED);
                    }
                }
            }
            break;

        case RendererObserverInterface::State::ERROR:
            if (m_observer) {
                m_observer->onAlertStateChange(m_token, AlertObserverInterface::State::ERROR, reason);
            }

            break;
    }
}

std::string Alert::getToken() const {
    return m_token;
}

int64_t Alert::getScheduledTime_Unix() const {
    return m_scheduledTime_Unix;
}

std::string Alert::getScheduledTime_ISO_8601() const {
    return m_scheduledTime_ISO_8601;
}

Alert::State Alert::getState() const {
    return m_state;
}

int Alert::getId() const {
    return m_dbId;
}

void Alert::printDiagnostic() {
    ACSDK_INFO(LX(" ** Alert | id:" + std::to_string(m_dbId)));
    ACSDK_INFO(LX("          | type:" + getTypeName()));
    ACSDK_INFO(LX("          | token:" + m_token));
    ACSDK_INFO(LX("          | scheduled time (8601):" + m_scheduledTime_ISO_8601));
    ACSDK_INFO(LX("          | scheduled time (Unix):" + std::to_string(m_scheduledTime_Unix)));
    ACSDK_INFO(LX("          | state:" + stateToString(m_state)));
}

void Alert::startRenderer() {
    std::string fileName = getDefaultAudioFilePath();
    if (avsCommon::avs::FocusState::BACKGROUND == m_focusState) {
        fileName = getDefaultShortAudioFilePath();
    }

    m_renderer->setObserver(this);
    m_renderer->start(fileName);
}

void Alert::snooze(const std::string & updatedScheduledTime_ISO_8601) {
    int64_t unixTime = 0;
    if (!convert8601TimeStringToUnix(updatedScheduledTime_ISO_8601, &unixTime)) {
        ACSDK_ERROR(LX("setScheduledTime_ISO_8601Failed")
                .m("could not convert time string").d("value", updatedScheduledTime_ISO_8601));
        return;
    }

    m_scheduledTime_ISO_8601 = updatedScheduledTime_ISO_8601;
    m_scheduledTime_Unix = unixTime;

    m_state = State::SNOOZING;
    m_renderer->stop();
}

Alert::StopReason Alert::getStopReason() const {
    return m_stopReason;
}

void Alert::onMaxTimerExpiration() {
    m_state = Alert::State::STOPPING;
    m_hasTimerExpired = true;
    m_renderer->stop();
}

} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK