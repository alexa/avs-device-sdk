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

#include "Alerts/AlertsCapabilityAgent.h"

#include "Alerts/Alarm.h"
#include "Alerts/Reminder.h"
#include "Alerts/Storage/SQLiteAlertStorage.h"
#include "Alerts/Timer.h"
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/File/FileUtils.h>
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
using namespace avsCommon::utils::file;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::timing;
using namespace avsCommon::sdkInterfaces;
using namespace certifiedSender;
using namespace rapidjson;

/// The value for Type which we need for json parsing.
static const std::string KEY_TYPE = "type";

/// The value of the SetAlert Directive.
static const std::string DIRECTIVE_NAME_SET_ALERT = "SetAlert";
/// The value of the DeleteAlert Directive.
static const std::string DIRECTIVE_NAME_DELETE_ALERT = "DeleteAlert";

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

static const std::string EVENT_PAYLOAD_TOKEN_KEY = "token";
/// The value of Token text in an Directive we may receive.
static const std::string DIRECTIVE_PAYLOAD_TOKEN_KEY = "token";

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

/// An empty dialogRequestId.
static const std::string EMPTY_DIALOG_REQUEST_ID = "";

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alerts";
/// The SetAlert directive signature.
static const avsCommon::avs::NamespaceAndName SET_ALERT{NAMESPACE, "SetAlert"};
/// The DeleteAlert directive signature.
static const avsCommon::avs::NamespaceAndName DELETE_ALERT{NAMESPACE, "DeleteAlert"};

/// String to identify log entries originating from this file.
static const std::string TAG("AlertsCapabilityAgent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Utility function to construct a rapidjson array of alert details, representing all the alerts currently managed.
 *
 * @param alertsInfo All the alerts being managed by this Capability Agent.
 * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the array.
 */
static rapidjson::Value buildAllAlertsContext(
    const std::vector<Alert::ContextInfo>& alertsInfo,
    Document::AllocatorType& allocator) {
    rapidjson::Value alertArray(rapidjson::kArrayType);

    for (const auto& info : alertsInfo) {
        rapidjson::Value alertJson;
        alertJson.SetObject();
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TOKEN_KEY), info.token, allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TYPE_KEY), info.type, allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_SCHEDULED_TIME_KEY), info.scheduledTime_ISO_8601, allocator);

        alertArray.PushBack(alertJson, allocator);
    }

    return alertArray;
}

/**
 * Utility function to construct a rapidjson array of alert details, representing all the currently active alerts.
 *
 * @param alertsInfo The currently active alert, which may be nullptr if no alert is active.
 * @param allocator The rapidjson allocator, required for the results of this function to be mergable with other
 * rapidjson::Value objects.
 * @return The rapidjson::Value representing the array.
 */
static rapidjson::Value buildActiveAlertsContext(
    const std::vector<Alert::ContextInfo>& alertsInfo,
    Document::AllocatorType& allocator) {
    rapidjson::Value alertArray(rapidjson::kArrayType);

    if (!alertsInfo.empty()) {
        auto& info = alertsInfo[0];
        rapidjson::Value alertJson;
        alertJson.SetObject();
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TOKEN_KEY), info.token, allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_TYPE_KEY), info.type, allocator);
        alertJson.AddMember(StringRef(AVS_CONTEXT_ALERT_SCHEDULED_TIME_KEY), info.scheduledTime_ISO_8601, allocator);

        alertArray.PushBack(alertJson, allocator);
    }

    return alertArray;
}

std::shared_ptr<AlertsCapabilityAgent> AlertsCapabilityAgent::create(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> alertsAudioFactory,
    std::shared_ptr<renderer::RendererInterface> alertRenderer,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager) {
    auto alertsCA = std::shared_ptr<AlertsCapabilityAgent>(new AlertsCapabilityAgent(
        messageSender,
        certifiedMessageSender,
        focusManager,
        contextManager,
        exceptionEncounteredSender,
        alertStorage,
        alertsAudioFactory,
        alertRenderer,
        dataManager));

    if (!alertsCA->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initialization error."));
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
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "directive is nullptr."));
    }
    auto info = createDirectiveInfo(directive, nullptr);
    m_executor.submit([this, info]() { executeHandleDirectiveImmediately(info); });
}

void AlertsCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    // intentional no-op.
}

void AlertsCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "info is nullptr."));
    }
    m_executor.submit([this, info]() { executeHandleDirectiveImmediately(info); });
}

void AlertsCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    // intentional no-op.
}

void AlertsCapabilityAgent::onDeregistered() {
    // intentional no-op.
}

void AlertsCapabilityAgent::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    m_executor.submit([this, status, reason]() { executeOnConnectionStatusChanged(status, reason); });
}

void AlertsCapabilityAgent::onFocusChanged(avsCommon::avs::FocusState focusState) {
    ACSDK_DEBUG9(LX("onFocusChanged").d("focusState", focusState));
    m_executor.submit([this, focusState]() { executeOnFocusChanged(focusState); });
}

void AlertsCapabilityAgent::onAlertStateChange(
    const std::string& alertToken,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG9(LX("onAlertStateChange").d("alertToken", alertToken).d("state", state).d("reason", reason));
    m_executor.submit([this, alertToken, state, reason]() { executeOnAlertStateChange(alertToken, state, reason); });
}

void AlertsCapabilityAgent::addObserver(std::shared_ptr<AlertObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    m_executor.submit([this, observer]() { executeAddObserver(observer); });
}

void AlertsCapabilityAgent::removeObserver(std::shared_ptr<AlertObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    m_executor.submit([this, observer]() { executeRemoveObserver(observer); });
}

void AlertsCapabilityAgent::removeAllAlerts() {
    m_executor.submit([this]() { executeRemoveAllAlerts(); });
}

void AlertsCapabilityAgent::onLocalStop() {
    ACSDK_DEBUG9(LX("onLocalStop"));
    m_executor.submitToFront([this]() { executeOnLocalStop(); });
}

AlertsCapabilityAgent::AlertsCapabilityAgent(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> alertsAudioFactory,
    std::shared_ptr<renderer::RendererInterface> alertRenderer,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager) :
        CapabilityAgent("Alerts", exceptionEncounteredSender),
        RequiresShutdown("AlertsCapabilityAgent"),
        CustomerDataHandler(dataManager),
        m_messageSender{messageSender},
        m_certifiedSender{certifiedMessageSender},
        m_focusManager{focusManager},
        m_contextManager{contextManager},
        m_isConnected{false},
        m_alertScheduler{alertStorage, alertRenderer, ALERT_PAST_DUE_CUTOFF_MINUTES},
        m_alertsAudioFactory{alertsAudioFactory} {
}

void AlertsCapabilityAgent::doShutdown() {
    m_executor.shutdown();
    releaseChannel();
    m_messageSender.reset();
    m_certifiedSender.reset();
    m_focusManager.reset();
    m_contextManager.reset();
    m_observers.clear();
    m_alertScheduler.shutdown();
}

bool AlertsCapabilityAgent::initialize() {
    if (!initializeAlerts()) {
        ACSDK_ERROR(LX("initializeFailed").m("Could not initialize alerts."));
        return false;
    }

    updateContextManager();

    return true;
}

bool AlertsCapabilityAgent::initializeAlerts() {
    return m_alertScheduler.initialize(shared_from_this());
}

bool AlertsCapabilityAgent::handleSetAlert(
    const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
    const rapidjson::Document& payload,
    std::string* alertToken) {
    ACSDK_DEBUG9(LX("handleSetAlert"));
    std::string alertType;
    if (!retrieveValue(payload, KEY_TYPE, &alertType)) {
        std::string errorMessage = "Alert type not specified for SetAlert";
        ACSDK_ERROR(LX("handleSetAlertFailed").m(errorMessage));
        sendProcessingDirectiveException(directive, errorMessage);
        return false;
    }

    std::shared_ptr<Alert> parsedAlert;

    if (Alarm::TYPE_NAME == alertType) {
        parsedAlert = std::make_shared<Alarm>(m_alertsAudioFactory->alarmDefault(), m_alertsAudioFactory->alarmShort());
    } else if (Timer::TYPE_NAME == alertType) {
        parsedAlert = std::make_shared<Timer>(m_alertsAudioFactory->timerDefault(), m_alertsAudioFactory->timerShort());
    } else if (Reminder::TYPE_NAME == alertType) {
        parsedAlert =
            std::make_shared<Reminder>(m_alertsAudioFactory->reminderDefault(), m_alertsAudioFactory->reminderShort());
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

    if (m_alertScheduler.isAlertActive(parsedAlert)) {
        return m_alertScheduler.snoozeAlert(parsedAlert->getToken(), parsedAlert->getScheduledTime_ISO_8601());
    }

    if (!m_alertScheduler.scheduleAlert(parsedAlert)) {
        return false;
    }

    updateContextManager();

    return true;
}

bool AlertsCapabilityAgent::handleDeleteAlert(
    const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
    const rapidjson::Document& payload,
    std::string* alertToken) {
    ACSDK_DEBUG9(LX("handleDeleteAlert"));
    if (!retrieveValue(payload, DIRECTIVE_PAYLOAD_TOKEN_KEY, alertToken)) {
        ACSDK_ERROR(LX("handleDeleteAlertFailed").m("Could not find token in the payload."));
        return false;
    }

    if (!m_alertScheduler.deleteAlert(*alertToken)) {
        return false;
    }

    updateContextManager();

    return true;
}

void AlertsCapabilityAgent::sendEvent(const std::string& eventName, const std::string& alertToken, bool isCertified) {
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

    if (isCertified) {
        m_certifiedSender->sendJSONMessage(jsonEventString);
    } else {
        if (!m_isConnected) {
            ACSDK_WARN(
                LX("sendEvent").m("Not connected to AVS.  Not sending Event.").d("event details", jsonEventString));
        } else {
            auto request = std::make_shared<MessageRequest>(jsonEventString);
            m_messageSender->sendMessage(request);
        }
    }
}

void AlertsCapabilityAgent::sendProcessingDirectiveException(
    const std::shared_ptr<AVSDirective>& directive,
    const std::string& errorMessage) {
    auto unparsedDirective = directive->getUnparsedDirective();

    ACSDK_ERROR(
        LX("sendProcessingDirectiveException").m("Could not parse directive.").m(errorMessage).m(unparsedDirective));

    m_exceptionEncounteredSender->sendExceptionEncountered(
        unparsedDirective, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, errorMessage);
}

void AlertsCapabilityAgent::acquireChannel() {
    ACSDK_DEBUG9(LX("acquireChannel"));
    m_focusManager->acquireChannel(FocusManagerInterface::ALERTS_CHANNEL_NAME, shared_from_this(), NAMESPACE);
}

void AlertsCapabilityAgent::releaseChannel() {
    ACSDK_DEBUG9(LX("releaseChannel"));
    if (m_alertScheduler.getFocusState() != FocusState::NONE) {
        m_focusManager->releaseChannel(FocusManagerInterface::ALERTS_CHANNEL_NAME, shared_from_this());
    }
}

void AlertsCapabilityAgent::executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG1(LX("executeHandleDirectiveImmediately"));
    auto& directive = info->directive;

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
            sendEvent(SET_ALERT_SUCCEEDED_EVENT_NAME, alertToken, true);
        } else {
            sendEvent(SET_ALERT_FAILED_EVENT_NAME, alertToken, true);
        }
    } else if (DIRECTIVE_NAME_DELETE_ALERT == directiveName) {
        if (handleDeleteAlert(directive, payload, &alertToken)) {
            sendEvent(DELETE_ALERT_SUCCEEDED_EVENT_NAME, alertToken, true);
        } else {
            sendEvent(DELETE_ALERT_FAILED_EVENT_NAME, alertToken, true);
        }
    }
}

void AlertsCapabilityAgent::executeOnConnectionStatusChanged(const Status status, const ChangedReason reason) {
    ACSDK_DEBUG1(LX("executeOnConnectionStatusChanged").d("status", status).d("reason", reason));
    m_isConnected = (Status::CONNECTED == status);
}

void AlertsCapabilityAgent::executeOnFocusChanged(avsCommon::avs::FocusState focusState) {
    ACSDK_DEBUG1(LX("executeOnFocusChanged").d("focusState", focusState));
    m_alertScheduler.updateFocus(focusState);
}

void AlertsCapabilityAgent::executeOnAlertStateChange(
    const std::string& alertToken,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG1(LX("executeOnAlertStateChange").d("alertToken", alertToken).d("state", state).d("reason", reason));
    switch (state) {
        case AlertObserverInterface::State::READY:
            acquireChannel();
            break;

        case AlertObserverInterface::State::STARTED:
            sendEvent(ALERT_STARTED_EVENT_NAME, alertToken, true);
            updateContextManager();
            break;

        case AlertObserverInterface::State::SNOOZED:
            releaseChannel();
            updateContextManager();
            break;

        case AlertObserverInterface::State::STOPPED:
            sendEvent(ALERT_STOPPED_EVENT_NAME, alertToken, true);
            releaseChannel();
            updateContextManager();
            break;

        case AlertObserverInterface::State::COMPLETED:
            sendEvent(ALERT_STOPPED_EVENT_NAME, alertToken, true);
            releaseChannel();
            updateContextManager();
            break;

        case AlertObserverInterface::State::ERROR:
            releaseChannel();
            updateContextManager();
            break;

        case AlertObserverInterface::State::PAST_DUE:
            sendEvent(ALERT_STOPPED_EVENT_NAME, alertToken, true);
            break;

        case AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND:
            sendEvent(ALERT_ENTERED_FOREGROUND_EVENT_NAME, alertToken);
            break;

        case AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND:
            sendEvent(ALERT_ENTERED_BACKGROUND_EVENT_NAME, alertToken);
            break;
    }

    m_executor.submit([this, alertToken, state, reason]() { executeNotifyObservers(alertToken, state, reason); });
}

void AlertsCapabilityAgent::executeAddObserver(std::shared_ptr<AlertObserverInterface> observer) {
    ACSDK_DEBUG1(LX("executeAddObserver").d("observer", observer.get()));
    m_observers.insert(observer);
}

void AlertsCapabilityAgent::executeRemoveObserver(std::shared_ptr<AlertObserverInterface> observer) {
    ACSDK_DEBUG1(LX("executeRemoveObserver").d("observer", observer.get()));
    m_observers.erase(observer);
}

void AlertsCapabilityAgent::executeNotifyObservers(
    const std::string& alertToken,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG1(LX("executeNotifyObservers").d("alertToken", alertToken).d("state", state).d("reason", reason));
    for (auto observer : m_observers) {
        observer->onAlertStateChange(alertToken, state, reason);
    }
}

void AlertsCapabilityAgent::executeRemoveAllAlerts() {
    ACSDK_DEBUG1(LX("executeRemoveAllAlerts"));
    m_alertScheduler.clearData();
}

void AlertsCapabilityAgent::executeOnLocalStop() {
    ACSDK_DEBUG1(LX("executeOnLocalStop"));
    m_alertScheduler.onLocalStop();
}

void AlertsCapabilityAgent::updateContextManager() {
    std::string contextString = getContextString();

    NamespaceAndName namespaceAndName{AVS_CONTEXT_HEADER_NAMESPACE_VALUE_KEY, AVS_CONTEXT_HEADER_NAME_VALUE_KEY};

    auto setStateSuccess = m_contextManager->setState(namespaceAndName, contextString, StateRefreshPolicy::NEVER);

    if (setStateSuccess != SetStateResult::SUCCESS) {
        ACSDK_ERROR(LX("updateContextManagerFailed")
                        .m("Could not set the state on the contextManager")
                        .d("result", static_cast<int>(setStateSuccess)));
    }
}

std::string AlertsCapabilityAgent::getContextString() {
    rapidjson::Document state(kObjectType);
    rapidjson::Document::AllocatorType& alloc = state.GetAllocator();

    auto alertsContextInfo = m_alertScheduler.getContextInfo();
    auto allAlertsJsonValue = buildAllAlertsContext(alertsContextInfo.scheduledAlerts, alloc);
    auto activeAlertsJsonValue = buildActiveAlertsContext(alertsContextInfo.activeAlerts, alloc);

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

void AlertsCapabilityAgent::clearData() {
    auto result = m_executor.submit([this]() { m_alertScheduler.clearData(Alert::StopReason::LOG_OUT); });
    result.wait();
}

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
