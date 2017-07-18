/*
 * AlertsCapabilityAgent.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "Alerts/AlertsCapabilityAgent.h"

#include "Alerts/Alarm.h"
#include "Alerts/Storage/SQLiteAlertStorage.h"
#include "Alerts/Timer.h"
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

using namespace avsCommon::avs;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::timing;
using namespace avsCommon::sdkInterfaces;
using namespace rapidjson;

/// The value for Type which we need for json parsing.
static const std::string KEY_TYPE = "type";

/// The value of the SetAlert Directive.
static const std::string DIRECTIVE_NAME_SET_ALERT = "SetAlert";
/// The value of the DeleteAlert Directive.
static const std::string DIRECTIVE_NAME_DELETE_ALERT = "DeleteAlert";

/// The key in our config file to find the root of settings for this Capability Agent.
static const std::string ALERTS_CAPABILITY_AGENT_CONFIGURATION_ROOT_KEY = "alertsCapabilityAgent";
/// The key in our config file to find the database file path.
static const std::string ALERTS_CAPABILITY_AGENT_DB_FILE_PATH_KEY = "databaseFilePath";
/// The key in our config file to find the alarm default sound file path.
static const std::string ALERTS_CAPABILITY_AGENT_ALARM_AUDIO_FILE_PATH_KEY = "alarmSoundFilePath";
/// The key in our config file to find the alarm short sound file path.
static const std::string ALERTS_CAPABILITY_AGENT_ALARM_SHORT_AUDIO_FILE_PATH_KEY = "alarmShortSoundFilePath";
/// The key in our config file to find the timer default sound file path.
static const std::string ALERTS_CAPABILITY_AGENT_TIMER_AUDIO_FILE_PATH_KEY = "timerSoundFilePath";
/// The key in our config file to find the timer short sound file path.
static const std::string ALERTS_CAPABILITY_AGENT_TIMER_SHORT_AUDIO_FILE_PATH_KEY = "timerShortSoundFilePath";

/// The value of the SetAlertSucceeded Event name.
static const std::string SET_ALERT_SUCCEEDED_EVENT_NAME = "SetAlertSucceeded";
/// The value of the SetAlertFailed Event name.
static const std::string SET_ALERT_FAILED_EVENT_NAME = "SetAlertFailed";
/// The value of the DeleteAlertSucceeded Event name.
static const std::string DELETE_ALERT_SUCCEEDED_EVENT_NAME = "DeleteAlertSucceeded";
/// The value of the DeleteAlertFailed Event name.
static const std::string DELETE_ALERT_FAILED_EVENT_NAME = "DeleteAlertFailed";
/// The value of the AlertStarted Event name.
static const std::string ALERT_STARTED_EVENT_NAME = "AlertStarted";
/// The value of the AlertStopped Event name.
static const std::string ALERT_STOPPED_EVENT_NAME = "AlertStopped";
/// The value of the AlertEnteredForeground Event name.
static const std::string ALERT_ENTERED_FOREGROUND_EVENT_NAME = "AlertEnteredForeground";
/// The value of the AlertEnteredBackground Event name.
static const std::string ALERT_ENTERED_BACKGROUND_EVENT_NAME = "AlertEnteredBackground";
/// The value of the Alerts Context Namespace.

static const std::string AVS_CONTEXT_HEADER_NAMESPACE_VALUE_KEY = "Alerts";
/// The value of the Alerts Context Names.
static const std::string AVS_CONTEXT_HEADER_NAME_VALUE_KEY = "AlertsState";
/// The value of the Alerts Context allAlerts node.
static const std::string AVS_CONTEXT_ALL_ALERTS_TOKEN_KEY = "allAlerts";
/// The value of the Alerts Context activeAlerts node.
static const std::string AVS_CONTEXT_ACTIVE_ALERTS_TOKEN_KEY = "activeAlerts";
/// The value of the Alerts Context token key.
static const std::string AVS_CONTEXT_ALERT_TOKEN_KEY = "token";
/// The value of the Alerts Context type key.
static const std::string AVS_CONTEXT_ALERT_TYPE_KEY = "type";
/// The value of the Alerts Context scheduled time key.
static const std::string AVS_CONTEXT_ALERT_SCHEDULED_TIME_KEY = "scheduledTime";
/// The value of Token text in an Event we may send.

static const std::string EVENT_PAYLOAD_TOKEN_KEY = "token";
/// The value of Token text in an Directive we may receive.
static const std::string DIRECTIVE_PAYLOAD_TOKEN_KEY = "token";

/// An empty dialogRequestId.
static const std::string EMPTY_DIALOG_REQUEST_ID = "";

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alerts";
/// The SetAlert directive signature.
static const avsCommon::avs::NamespaceAndName SET_ALERT{NAMESPACE, "SetAlert"};
/// The DeleteAlert directive signature.
static const avsCommon::avs::NamespaceAndName DELETE_ALERT{NAMESPACE, "DeleteAlert"};
/// The activityId string used with @c FocusManager by @c AlertsCapabilityAgent.
static const std::string ACTIVITY_ID = "Alerts.AlertStarted";

/// String to identify log entries originating from this file.
static const std::string TAG("AlertsCapabilityAgent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * A utility function to query if a file exists.
 *
 * TODO - make this more portable and dependable.
 * https://issues.labcollab.net/browse/ACSDK-380
 *
 * @param filePath The path to the file being queried about.
 * @return Whether the file exists and is accessible.
 */
static bool fileExists(const std::string &fileName) {
    std::ifstream is(fileName);
    return is.good();
}

/**
 * Utility function to tell if an Alert should be rendered, or discarded.
 *
 * @param alert The Alert being considered.
 * @param currentUnixTime The current time, expressed in Unix Epoch time.
 * @return Whether the alert is past-due, or should be scheduled for rendering.
 */
static bool isAlertPastDue(const std::shared_ptr<Alert> &alert, int64_t currentUnixTime) {
    int64_t cutoffTime = currentUnixTime - (ALERT_PAST_DUE_CUTOFF_MINUTES.count() * 60);

    return (alert->getScheduledTime_Unix() < cutoffTime);
}

/**
 * Utility function to construct a rapidjson array of alert details, representing all the alerts currently managed.
 *
 * @param alerts All the alerts being managed by this Capability Agent.
 * @param activeAlert The currently active alert, if any.
 * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the array.
 */
static rapidjson::Value buildAllAlertsContext(
        const std::set<std::shared_ptr<Alert>, Alert::TimeComparator> &alerts,
        std::shared_ptr<Alert> activeAlert,
        Document::AllocatorType &allocator) {
    rapidjson::Value alertArray(rapidjson::kArrayType);

    for (auto &alert : alerts) {
        rapidjson::Value alertJson;
        alertJson.SetObject();
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TOKEN_KEY), alert->getToken(), allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TYPE_KEY), alert->getTypeName(), allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_SCHEDULED_TIME_KEY), alert->getScheduledTime_ISO_8601(),
                allocator);

        alertArray.PushBack(alertJson, allocator);
    }

    if (activeAlert) {
        rapidjson::Value alertJson;
        alertJson.SetObject();
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TOKEN_KEY), activeAlert->getToken(), allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TYPE_KEY), activeAlert->getTypeName(), allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_SCHEDULED_TIME_KEY), activeAlert->getScheduledTime_ISO_8601(),
                allocator);

        alertArray.PushBack(alertJson, allocator);
    }

    return alertArray;
}

/**
 * Utility function to construct a rapidjson array of alert details, representing all the currently active alerts.
 *
 * @param activeAlert The currently active alert, which may be nullptr if no alert is active.
 * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the array.
 */
static rapidjson::Value buildActiveAlertsContext(std::shared_ptr<Alert> activeAlert,
                                                 Document::AllocatorType &allocator) {
    rapidjson::Value alertArray(rapidjson::kArrayType);

    if (activeAlert) {
        rapidjson::Value alertJson;
        alertJson.SetObject();
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TOKEN_KEY), activeAlert->getToken(), allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TYPE_KEY), activeAlert->getTypeName(), allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_SCHEDULED_TIME_KEY), activeAlert->getScheduledTime_ISO_8601(),
                allocator);

        alertArray.PushBack(alertJson, allocator);
    }

    return alertArray;
}

/**
 * Utility function to create a Context string for the given alert inputs.
 *
 * @param alerts All the alerts being managed by this Capability Agent.
 * @param activeAlert The currently active alert, which may be nullptr if no alert is active.
 * @return The Context string for the given alert inputs.
 */
static std::string getContextString(const std::set<std::shared_ptr<Alert>, Alert::TimeComparator> &alerts,
                                    const std::shared_ptr<Alert> &activeAlert) {
    rapidjson::Document state(kObjectType);
    rapidjson::Document::AllocatorType &alloc = state.GetAllocator();

    auto allAlertsJsonValue = buildAllAlertsContext(alerts, activeAlert, alloc);
    auto activeAlertsJsonValue = buildActiveAlertsContext(activeAlert, alloc);

    state.AddMember(StringRef(AVS_CONTEXT_ALL_ALERTS_TOKEN_KEY), allAlertsJsonValue, alloc);
    state.AddMember(StringRef(AVS_CONTEXT_ACTIVE_ALERTS_TOKEN_KEY), activeAlertsJsonValue, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!state.Accept(writer)) {
        ACSDK_ERROR(LX("getContextStringFailed").d("reason", "writerRefusedJsonObject"));
        return "";
    }

    return buffer.GetString();
}

std::shared_ptr<AlertsCapabilityAgent> AlertsCapabilityAgent::create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<renderer::RendererInterface> renderer,
        std::shared_ptr<storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<AlertObserverInterface> observer) {

    auto alertsCA = std::shared_ptr<AlertsCapabilityAgent>(new AlertsCapabilityAgent(
            messageSender, focusManager, contextManager,
            exceptionEncounteredSender, renderer, alertStorage, observer));

    if (!alertsCA->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initializion error."));
        return nullptr;
    }

    return alertsCA;
}

avsCommon::avs::DirectiveHandlerConfiguration AlertsCapabilityAgent::getConfiguration() const {
    avsCommon::avs::DirectiveHandlerConfiguration configuration;
    configuration[SET_ALERT] = avsCommon::avs::BlockingPolicy::NON_BLOCKING;
    configuration[DELETE_ALERT] = avsCommon::avs::BlockingPolicy::NON_BLOCKING;
    return configuration;
}

void AlertsCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    auto info = createDirectiveInfo(directive, nullptr);
    m_caExecutor.submit([this, info]() { executeHandleDirectiveImmediately(info); });
}

void AlertsCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_ERROR(LX("preHandleDirective").m("unexpected call."));
}

void AlertsCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_ERROR(LX("handleDirective").m("unexpected call."));
}

void AlertsCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_ERROR(LX("cancelDirective").m("unexpected call."));
}

void AlertsCapabilityAgent::onDeregistered() {
    std::lock_guard<std::mutex> lock(m_mutex);
    deactivateActiveAlertHelper(Alert::StopReason::SHUTDOWN);
    m_scheduledAlerts.clear();
}

void AlertsCapabilityAgent::enableSendEvents() {
    m_sendEventsEnabled = true;
    filterPastDueAlerts();
}

void AlertsCapabilityAgent::disableSendEvents() {
    m_sendEventsEnabled = false;
}

void AlertsCapabilityAgent::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isConnected = (Status::CONNECTED == status);
}

void AlertsCapabilityAgent::onFocusChanged(avsCommon::avs::FocusState focusState) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_focusState == focusState) {
        return;
    }

    m_focusState = focusState;

    switch (m_focusState) {
        case FocusState::FOREGROUND:
            if (m_activeAlert) {
                m_activeAlert->setFocusState(m_focusState);

                std::string token = m_activeAlert->getToken();

                m_caExecutor.submit(
                        [this, token]() { executeSendAlertFocusChangeEvent(token, FocusState::FOREGROUND); }
                );

                m_caExecutor.submit([this, token]() {
                    executeNotifyObserver(token, AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND);
                });

            } else {
                activateNextAlertLocked();
            }
            break;

        case FocusState::BACKGROUND:
            if (m_activeAlert) {
                m_activeAlert->setFocusState(m_focusState);

                std::string token = m_activeAlert->getToken();

                m_caExecutor.submit(
                        [this, token]() { executeSendAlertFocusChangeEvent(token, FocusState::BACKGROUND); }
                );

                m_caExecutor.submit([this, token]() {
                    executeNotifyObserver(token, AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND);
                });

            } else {
                activateNextAlertLocked();
            }
            break;

        case FocusState::NONE:
            deactivateActiveAlertHelper(Alert::StopReason::LOCAL_STOP);
            break;
    }
}

void AlertsCapabilityAgent::executeSendAlertFocusChangeEvent(
        const std::string &alertToken, avsCommon::avs::FocusState focusState) {
    switch (focusState) {
        case FocusState::FOREGROUND:
            sendEvent(ALERT_ENTERED_FOREGROUND_EVENT_NAME, alertToken);
            break;

        case FocusState::BACKGROUND:
            sendEvent(ALERT_ENTERED_BACKGROUND_EVENT_NAME, alertToken);
            break;

        case FocusState::NONE:
            // no-op
            break;
    }
}

void AlertsCapabilityAgent::executeNotifyObserver(
        const std::string &alertToken, AlertObserverInterface::State state, const std::string &reason) {
    if (m_observer) {
        m_observer->onAlertStateChange(alertToken, state, reason);
    }
}

void AlertsCapabilityAgent::onAlertStateChange(const std::string &alertToken, AlertObserverInterface::State state,
                                               const std::string &reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_activeAlert || alertToken != m_activeAlert->getToken()) {
        ACSDK_ERROR(LX("onAlertStateChangeFailed").m("state change for alert which is not active."));
        return;
    }

    std::string alertTokenCopy = alertToken;
    m_caExecutor.submit([this, alertTokenCopy, state, reason]() {
        executeNotifyObserver(alertTokenCopy, state, reason);
    });

    switch (state) {

        case AlertObserverInterface::State::STARTED:
            if (Alert::State::ACTIVATING == m_activeAlert->getState()) {
                sendEvent(ALERT_STARTED_EVENT_NAME, m_activeAlert->getToken());
                m_activeAlert->setStateActive();
                m_alertStorage->modify(m_activeAlert);
                updateContextManagerLocked();
            }
            break;

        case AlertObserverInterface::State::SNOOZED:
            m_activeAlert->reset();
            m_alertStorage->modify(m_activeAlert);
            m_scheduledAlerts.insert(m_activeAlert);
            m_activeAlert.reset();
            m_alertRenderer->setObserver(nullptr);
            releaseChannel();

            updateContextManagerLocked();
            scheduleNextAlertForRendering();

            break;

        case AlertObserverInterface::State::STOPPED:
            // NOTE: Only send AlertStopped Event if local stop.  Otherwise this is done during DeleteAlert handling.
            if (Alert::StopReason::LOCAL_STOP == m_activeAlert->getStopReason()) {
                sendEvent(ALERT_STOPPED_EVENT_NAME, m_activeAlert->getToken());
            }

            m_alertStorage->erase(m_activeAlert);
            m_activeAlert.reset();
            m_alertRenderer->setObserver(nullptr);
            releaseChannel();

            updateContextManagerLocked();
            scheduleNextAlertForRendering();

            break;

        case AlertObserverInterface::State::COMPLETED:
            sendEvent(ALERT_STOPPED_EVENT_NAME, m_activeAlert->getToken());
            m_alertStorage->erase(m_activeAlert);
            m_activeAlert.reset();
            m_alertRenderer->setObserver(nullptr);
            releaseChannel();

            updateContextManagerLocked();
            scheduleNextAlertForRendering();

            break;

        case AlertObserverInterface::State::ERROR:
            // let's at least get the next alert set up for rendering.
            m_alertStorage->erase(m_activeAlert);
            m_activeAlert.reset();
            m_alertRenderer->setObserver(nullptr);
            releaseChannel();

            updateContextManagerLocked();
            scheduleNextAlertForRendering();

            break;

        case AlertObserverInterface::State::PAST_DUE:
        case AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND:
        case AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND:
            // no-op
            break;
    }
}

void AlertsCapabilityAgent::onLocalStop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    deactivateActiveAlertHelper(Alert::StopReason::LOCAL_STOP);
}

void AlertsCapabilityAgent::removeAllAlerts() {
    std::unique_lock<std::mutex> lock(m_mutex);
    deactivateActiveAlertHelper(Alert::StopReason::SHUTDOWN);
    m_scheduledAlerts.clear();

    lock.unlock();

    m_alertStorage->clearDatabase();
}

AlertsCapabilityAgent::AlertsCapabilityAgent(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<renderer::RendererInterface> renderer,
        std::shared_ptr<storage::AlertStorageInterface> alertStorage,
        std::shared_ptr<AlertObserverInterface> observer)
        : CapabilityAgent("Alerts", exceptionEncounteredSender),
          m_messageSender{messageSender},
          m_focusManager{focusManager},
          m_contextManager{contextManager},
          m_alertStorage{alertStorage},
          m_alertRenderer{renderer},
          m_isConnected{false}, m_sendEventsEnabled{false}, m_focusState{avsCommon::avs::FocusState::NONE},
          m_observer{observer} {

}

bool AlertsCapabilityAgent::initialize() {
    auto configurationRoot = ConfigurationNode::getRoot()[ALERTS_CAPABILITY_AGENT_CONFIGURATION_ROOT_KEY];
    if (!configurationRoot) {
        ACSDK_ERROR(LX("initializeFailed").m("could not load AlertsCapabilityAgent configuration root."));
        return false;
    }

    if (!initializeDefaultSounds(configurationRoot)) {
        ACSDK_ERROR(LX("initializeFailed").m("Could not initialize default sounds."));
        return false;
    }

    if (!initializeAlerts(configurationRoot)) {
        ACSDK_ERROR(LX("initializeFailed").m("Could not initialize alerts."));
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    updateContextManagerLocked();
    lock.unlock();

    scheduleNextAlertForRendering();

    return true;
}

bool AlertsCapabilityAgent::initializeDefaultSounds(const ConfigurationNode &configurationRoot) {
    std::string alarmAudioFilePath;
    std::string alarmShortAudioFilePath;
    std::string timerAudioFilePath;
    std::string timerShortAudioFilePath;

    if (!configurationRoot.getString(ALERTS_CAPABILITY_AGENT_ALARM_AUDIO_FILE_PATH_KEY, &alarmAudioFilePath) ||
        alarmAudioFilePath.empty()) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not read alarm audio file path."));
        return false;
    }

    if (!fileExists(alarmAudioFilePath)) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not open alarm audio file."));
        return false;
    }

    if (!configurationRoot.getString(ALERTS_CAPABILITY_AGENT_ALARM_SHORT_AUDIO_FILE_PATH_KEY, &alarmShortAudioFilePath)
        || alarmShortAudioFilePath.empty()) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not read alarm short audio file path."));
        return false;
    }

    if (!fileExists(alarmAudioFilePath)) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not open alarm short audio file."));
        return false;
    }

    if (!configurationRoot.getString(ALERTS_CAPABILITY_AGENT_TIMER_AUDIO_FILE_PATH_KEY, &timerAudioFilePath) ||
        timerAudioFilePath.empty()) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not read timer audio file path."));
        return false;
    }

    if (!fileExists(alarmAudioFilePath)) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not open timer audio file."));
        return false;
    }

    if (!configurationRoot.getString(ALERTS_CAPABILITY_AGENT_TIMER_SHORT_AUDIO_FILE_PATH_KEY, &timerShortAudioFilePath)
        || timerShortAudioFilePath.empty()) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not read timer short audio file path."));
        return false;
    }

    if (!fileExists(alarmAudioFilePath)) {
        ACSDK_ERROR(LX("initializeDefaultSoundsFailed").m("could not open timer short audio file."));
        return false;
    }

    Alarm::setDefaultAudioFilePath(alarmAudioFilePath);
    Alarm::setDefaultShortAudioFilePath(alarmShortAudioFilePath);

    Timer::setDefaultAudioFilePath(timerAudioFilePath);
    Timer::setDefaultShortAudioFilePath(timerShortAudioFilePath);

    return true;
}

bool AlertsCapabilityAgent::initializeAlerts(const ConfigurationNode &configurationRoot) {
    std::string sqliteFilePath;

    if (!configurationRoot.getString(ALERTS_CAPABILITY_AGENT_DB_FILE_PATH_KEY, &sqliteFilePath) ||
        sqliteFilePath.empty()) {
        ACSDK_ERROR(LX("initializeAlertsFailed").m("could not load sqlite file path."));
        return false;
    }

    if (!m_alertStorage->open(sqliteFilePath)) {
        ACSDK_INFO(LX("initializeAlerts").m("database file does not exist.  Creating."));
        if (!m_alertStorage->createDatabase(sqliteFilePath)) {
            ACSDK_ERROR(LX("initializeAlertsFailed").m("Could not create database file."));
            return false;
        }
    }

    std::vector<std::shared_ptr<Alert>> alerts;
    m_alertStorage->load(&alerts);

    auto unixEpochNow = getCurrentUnixTime();

    for (auto &alert : alerts) {
        if (isAlertPastDue(alert, unixEpochNow)) {
            m_pastDueAlerts.push_back(alert);
        } else {
            alert->setRenderer(m_alertRenderer);
            alert->setObserver(this);

            // if it was active when the system last powered down, then re-init the state to set
            if (Alert::State::ACTIVE == alert->getState()) {
                alert->reset();
                m_alertStorage->modify(alert);
            }

            m_scheduledAlerts.insert(alert);
        }
    }

    return true;
}

void AlertsCapabilityAgent::executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info) {
    auto &directive = info->directive;

    rapidjson::Document payload;
    payload.Parse(directive->getPayload());

    if (payload.HasParseError()) {
        std::string errorMessage = "Unable to parse payload";
        ACSDK_ERROR(LX("executeHandleDirectiveImmediatelyFailed").m(errorMessage));
        sendProcessingDirectiveException(directive, errorMessage);
        return;
    }

    auto directiveName = directive->getName();
    std::string alertToken;

    if (DIRECTIVE_NAME_SET_ALERT == directiveName) {
        if (handleSetAlert(directive, payload, &alertToken)) {
            sendEvent(SET_ALERT_SUCCEEDED_EVENT_NAME, alertToken);
        } else {
            sendEvent(SET_ALERT_FAILED_EVENT_NAME, alertToken);
        }
    } else if (DIRECTIVE_NAME_DELETE_ALERT == directiveName) {
        if (handleDeleteAlert(directive, payload, &alertToken)) {
            sendEvent(DELETE_ALERT_SUCCEEDED_EVENT_NAME, alertToken);
        } else {
            sendEvent(DELETE_ALERT_FAILED_EVENT_NAME, alertToken);
        }
    }
}

bool AlertsCapabilityAgent::handleSetAlert(const std::shared_ptr<avsCommon::avs::AVSDirective> &directive,
                                           const rapidjson::Document &payload,
                                           std::string *alertToken) {
    std::string alertType;

    if (!retrieveValue(payload, KEY_TYPE, &alertType)) {
        std::string errorMessage = "Alert type not specified for SetAlert";
        ACSDK_ERROR(LX("handleSetAlertFailed").m(errorMessage));
        sendProcessingDirectiveException(directive, errorMessage);
        return false;
    }

    std::shared_ptr<Alert> parsedAlert;

    if (Alarm::TYPE_NAME == alertType) {
        parsedAlert = std::make_shared<Alarm>();
    } else if (Timer::TYPE_NAME == alertType) {
        parsedAlert = std::make_shared<Timer>();
    }

    if (!parsedAlert) {
        ACSDK_ERROR(LX("handleSetAlertFailed").d("reason", "unknown alert type").d("type:", alertType));
        return false;
    }

    std::string errorMessage;

    auto parseStatus = parsedAlert->parseFromJson(payload, &errorMessage);
    if (Alert::ParseFromJsonStatus::MISSING_REQUIRED_PROPERTY == parseStatus) {
        sendProcessingDirectiveException(directive, "Missing required property.");
        return false;
    } else if (Alert::ParseFromJsonStatus::INVALID_VALUE == parseStatus) {
        sendProcessingDirectiveException(directive, "Invalid value.");
        return false;
    }

    *alertToken = parsedAlert->getToken();
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_activeAlert &&
        (m_activeAlert->getToken() == *alertToken) &&
        (Alert::State::ACTIVE == m_activeAlert->getState())) {

        snoozeAlertLocked(m_activeAlert, parsedAlert->getScheduledTime_ISO_8601());
        sendEvent(ALERT_STOPPED_EVENT_NAME, m_activeAlert->getToken());

        // We won't be getting callbacks in simple mode.  Handle all work here.
        if (Alert::isSimpleModeEnabled()) {
            m_activeAlert->reset();
            m_scheduledAlerts.insert(m_activeAlert);
            m_activeAlert.reset();
            m_alertRenderer->setObserver(nullptr);
            releaseChannel();
            updateContextManagerLocked();
            scheduleNextAlertForRendering();
        }

    } else {
        if (getScheduledAlertByTokenLocked(parsedAlert->getToken())) {
            // This is the best default behavior.  If we send SetAlertFailed for a duplicate Alert,
            // then AVS will follow up with a DeleteAlert Directive - just to ensure the client does not
            // have a bad version of the Alert hanging around.  We already have the Alert, so let's return true,
            // so that SetAlertSucceeded will be sent back to AVS.
            ACSDK_INFO(LX("handleSetAlert").m("Duplicate SetAlert from AVS."));
            return true;
        }

        auto unixEpochNow = getCurrentUnixTime();

        if (isAlertPastDue(parsedAlert, unixEpochNow)) {
            ACSDK_ERROR(LX("handleSetAlertFailed").d("reason", "parsed alert is past-due.  Ignoring."));
            return false;
        } else {
            // it's a new alert.
            if (!m_alertStorage->store(parsedAlert)) {
                ACSDK_ERROR(LX("handleSetAlertFailed").d("reason", "could not store alert in database."));
                return false;
            }
            parsedAlert->setRenderer(m_alertRenderer);
            parsedAlert->setObserver(this);
            m_scheduledAlerts.insert(parsedAlert);

            updateContextManagerLocked();

            if (!m_activeAlert) {
                scheduleNextAlertForRendering();
            }
        }
    }

    return true;
}

bool AlertsCapabilityAgent::handleDeleteAlert(const std::shared_ptr<avsCommon::avs::AVSDirective> & directive,
                                              const rapidjson::Document & payload,
                                              std::string* alertToken) {
    if (!retrieveValue(payload, DIRECTIVE_PAYLOAD_TOKEN_KEY, alertToken)) {
        ACSDK_ERROR(LX("handleDeleteAlertFailed").m("Could not find token in the payload."));
        return false;
    }

    if (m_activeAlert && m_activeAlert->getToken() == *alertToken) {
        sendEvent(ALERT_STOPPED_EVENT_NAME, *alertToken);
        deactivateActiveAlertHelper(Alert::StopReason::AVS_STOP);
        return true;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto alert = getScheduledAlertByTokenLocked(*alertToken);

    if (!alert) {
        ACSDK_ERROR(LX("handleDeleteAlertFailed").m("could not find alert in map").d("token", *alertToken));
        return false;
    }

    if (!m_alertStorage->erase(alert)) {
        ACSDK_ERROR(LX("handleDeleteAlertFailed").m("Could not erase alert from database").d("token", *alertToken));
    }

    m_scheduledAlerts.erase(alert);
    updateContextManagerLocked();
    scheduleNextAlertForRendering();

    return true;
}

void AlertsCapabilityAgent::sendEvent(const std::string & eventName, const std::string & alertToken) {
    /**
     * TODO : ACSDK-393 to investigate if Events which cannot be sent right away should be stored somehow and
     * sent retrospectively once a connection is established.
     */

    if (!m_isConnected) {
        ACSDK_INFO(LX("sendEventFailed").m("Not connected - cannot send Event."));
        return;
    }

    if (!m_sendEventsEnabled) {
        ACSDK_INFO(LX("sendEventFailed").m("Not enabled to send events."));
        return;
    }

    rapidjson::Document payload(kObjectType);
    rapidjson::Document::AllocatorType& alloc = payload.GetAllocator();

    payload.AddMember(StringRef(EVENT_PAYLOAD_TOKEN_KEY), alertToken, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendEventFailed").m("Could not construct payload."));
        return;
    }

    auto jsonEventString = buildJsonEventString(eventName, EMPTY_DIALOG_REQUEST_ID, buffer.GetString()).second;

    auto request = std::make_shared<MessageRequest>(jsonEventString);
    m_messageSender->sendMessage(request);
}

void AlertsCapabilityAgent::sendProcessingDirectiveException(
        const std::shared_ptr<AVSDirective> & directive, const std::string & errorMessage) {
    auto unparsedDirective = directive->getUnparsedDirective();

    ACSDK_ERROR(LX("sendProcessingDirectiveException")
            .m("Could not parse directive.")
            .m(errorMessage)
            .m(unparsedDirective));

    m_exceptionEncounteredSender->sendExceptionEncountered(
            unparsedDirective, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, errorMessage);
}

void AlertsCapabilityAgent::updateContextManagerLocked() {
    std::string contextString = getContextString(m_scheduledAlerts, m_activeAlert);

    NamespaceAndName namespaceAndName{AVS_CONTEXT_HEADER_NAMESPACE_VALUE_KEY, AVS_CONTEXT_HEADER_NAME_VALUE_KEY};

    auto setStateSuccess = m_contextManager->setState(
            namespaceAndName, contextString, StateRefreshPolicy::NEVER);

    if (setStateSuccess != SetStateResult::SUCCESS) {
        ACSDK_ERROR(LX("updateContextManagerFailed")
                .m("Could not set the state on the contextManager")
                .d("result", static_cast<int>(setStateSuccess)));
    }
}

void AlertsCapabilityAgent::scheduleNextAlertForRendering() {
    m_alertSchedulerExecutor.submit([this] () { executeScheduleNextAlertForRendering(); });
}

void AlertsCapabilityAgent::executeScheduleNextAlertForRendering() {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_activeAlert) {
        ACSDK_INFO(LX("executeScheduleNextAlertForRendering").m("An alert is already active."));
        return;
    }

    if (m_scheduledAlerts.empty()) {
        ACSDK_INFO(LX("executeScheduleNextAlertForRendering").m("no work to do."));
        return;
    }

    auto alert = (*m_scheduledAlerts.begin());

    lock.unlock();

    if (m_scheduledAlertTimer.isActive()) {
        m_scheduledAlertTimer.stop();
    }

    int64_t timeNow = getCurrentUnixTime();

    std::chrono::seconds secondsToWait{alert->getScheduledTime_Unix() - timeNow};

    if (secondsToWait < std::chrono::seconds::zero()) {
        secondsToWait = std::chrono::seconds::zero();
    }

    if (secondsToWait == std::chrono::seconds::zero()) {
        if (!acquireChannel()) {
            ACSDK_ERROR(LX("executeScheduleNextAlertForRenderingFailed").m("Could not request channel."));
            return;
        }
    } else {
        releaseChannel();

        // start the timer for the next alert.
        if (!m_scheduledAlertTimer.start(
                secondsToWait, std::bind(&AlertsCapabilityAgent::acquireChannel, this)).valid()) {
            ACSDK_ERROR(LX("executeScheduleNextAlertForRenderingFailed").d("reason", "startTimerFailed"));
        }
    }
}

void AlertsCapabilityAgent::activateNextAlertLocked() {
    if (m_activeAlert) {
        ACSDK_ERROR(LX("activateNextAlertLockedFailed").d("reason", "An alert is already active."));
        return;
    }

    if (m_scheduledAlerts.empty()) {
        releaseChannel();
        return;
    }

    m_activeAlert = *(m_scheduledAlerts.begin());
    m_scheduledAlerts.erase(m_scheduledAlerts.begin());

    if (Alert::isSimpleModeEnabled()) {
        m_activeAlert->activateSimple(m_focusState);
        sendEvent(ALERT_STARTED_EVENT_NAME, m_activeAlert->getToken());
        m_alertStorage->modify(m_activeAlert);
        updateContextManagerLocked();
    } else {
        m_activeAlert->setFocusState(m_focusState);
        m_activeAlert->activate();
    }
}

std::shared_ptr<Alert> AlertsCapabilityAgent::getScheduledAlertByTokenLocked(const std::string & token) {
    for (auto & alert : m_scheduledAlerts) {
        if (token == alert->getToken()) {
            return alert;
        }
    }

    return nullptr;
}

bool AlertsCapabilityAgent::snoozeAlertLocked(std::shared_ptr<Alert> alert, const std::string & updatedTime) {
    // let's make sure we only snooze an alarm.  Other alert types can't be snoozed.
    if (alert->getTypeName() != Alarm::TYPE_NAME) {
        ACSDK_ERROR(LX("snoozeAlertFailed").d("reason", "attempt to snooze a non-alarm alert."));
        return false;
    }

    alert->snooze(updatedTime);

    if (!m_alertStorage->modify(alert)) {
        ACSDK_ERROR(LX("snoozeAlertFailed").d("reason", "could not modify alert in database for snooze."));
        return false;
    }

    updateContextManagerLocked();

    return true;
}

bool AlertsCapabilityAgent::acquireChannel() {
    if (m_activeAlert) {
        ACSDK_ERROR(LX("acquireChannelFailed").m("An alert is already active.  Not scheduling another."));
        return false;
    }

    if (m_scheduledAlerts.empty()) {
        ACSDK_ERROR(LX("acquireChannelFailed").m("no work to do."));
        return false;
    }

    if (FocusState::NONE == m_focusState) {
        m_focusManager->acquireChannel(FocusManagerInterface::ALERTS_CHANNEL_NAME, shared_from_this(), ACTIVITY_ID);
        return true;
    }

    return false;
}

void AlertsCapabilityAgent::releaseChannel() {
    if (FocusState::NONE != m_focusState) {
        m_focusManager->releaseChannel(FocusManagerInterface::ALERTS_CHANNEL_NAME, shared_from_this());
    }
}

void AlertsCapabilityAgent::filterPastDueAlerts() {
    auto unixEpochNow = getCurrentUnixTime();

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto & alert : m_scheduledAlerts) {
        if (isAlertPastDue(alert, unixEpochNow)) {
            m_pastDueAlerts.push_back(alert);
        }
    }

    for (auto & alert : m_pastDueAlerts) {
        std::string alertToken = alert->getToken();
        sendEvent(ALERT_STOPPED_EVENT_NAME, alertToken);

        m_caExecutor.submit([this, alertToken]() {
            executeNotifyObserver(alertToken, AlertObserverInterface::State::PAST_DUE, "");
        });

        m_scheduledAlerts.erase(alert);
        m_alertStorage->erase(alert);
    }
}

void AlertsCapabilityAgent::deactivateActiveAlertHelper(Alert::StopReason stopReason) {
    if (!m_activeAlert) {
        return;
    }

    m_activeAlert->deActivate(stopReason);

    // We won't get callbacks in simple mode.  Handle all work here.
    if (Alert::isSimpleModeEnabled()) {
        // NOTE: Only send AlertStopped Event if local stop.  Otherwise this is done during DeleteAlert handling.
        if (Alert::StopReason::LOCAL_STOP == stopReason) {
            sendEvent(ALERT_STOPPED_EVENT_NAME, m_activeAlert->getToken());
        }

        m_alertStorage->erase(m_activeAlert);
        m_activeAlert.reset();
        m_alertRenderer->setObserver(nullptr);
        releaseChannel();
        updateContextManagerLocked();
        scheduleNextAlertForRendering();
    }
}

} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK
