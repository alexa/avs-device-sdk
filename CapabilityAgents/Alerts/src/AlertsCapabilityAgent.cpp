/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "AVSCommon/AVS/CapabilityConfiguration.h"
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Timing/TimeUtils.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

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

/// Alerts capability constants
/// Alerts interface type
static const std::string ALERTS_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// Alerts interface name
static const std::string ALERTS_CAPABILITY_INTERFACE_NAME = "Alerts";
/// Alerts interface version
static const std::string ALERTS_CAPABILITY_INTERFACE_VERSION = "1.3";

/// The value for Type which we need for json parsing.
static const std::string KEY_TYPE = "type";

// ==== Directives ===

/// The value of the SetAlert Directive.
static const std::string DIRECTIVE_NAME_SET_ALERT = "SetAlert";
/// The value of the DeleteAlert Directive.
static const std::string DIRECTIVE_NAME_DELETE_ALERT = "DeleteAlert";
/// The value of the DeleteAlerts Directive.
static const std::string DIRECTIVE_NAME_DELETE_ALERTS = "DeleteAlerts";
/// The value of the SetVolume Directive.
static const std::string DIRECTIVE_NAME_SET_VOLUME = "SetVolume";
/// The value of the AdjustVolume Directive.
static const std::string DIRECTIVE_NAME_ADJUST_VOLUME = "AdjustVolume";

// ==== Events ===

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
/// The value of the VolumeChanged Event name.
static const std::string ALERT_VOLUME_CHANGED_EVENT_NAME = "VolumeChanged";
/// The value of the DeleteAlertsSucceeded Event name.
static const std::string ALERT_DELETE_ALERTS_SUCCEEDED_EVENT_NAME = "DeleteAlertsSucceeded";
/// The value of the DeleteAlertsFailed Event name.
static const std::string ALERT_DELETE_ALERTS_FAILED_EVENT_NAME = "DeleteAlertsFailed";

// ==== Other constants ===

/// The value of the event payload key for a single token.
static const std::string EVENT_PAYLOAD_TOKEN_KEY = "token";
/// The value of the event payload key for multiple tokens.
static const std::string EVENT_PAYLOAD_TOKENS_KEY = "tokens";
/// The value of Token text in a Directive we may receive.
static const std::string DIRECTIVE_PAYLOAD_TOKEN_KEY = "token";
/// The value of Token list key in a Directive we may receive.
static const std::string DIRECTIVE_PAYLOAD_TOKENS_KEY = "tokens";
/// The value of volume key in a Directive we may receive.
static const std::string DIRECTIVE_PAYLOAD_VOLUME = "volume";

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

/// The value of the volume state info volume key.
static const std::string AVS_PAYLOAD_VOLUME_KEY = "volume";

/// An empty dialogRequestId.
static const std::string EMPTY_DIALOG_REQUEST_ID = "";

/// The namespace for this capability agent.
static const std::string NAMESPACE = "Alerts";
/// The SetAlert directive signature.
static const avsCommon::avs::NamespaceAndName SET_ALERT{NAMESPACE, DIRECTIVE_NAME_SET_ALERT};
/// The DeleteAlert directive signature.
static const avsCommon::avs::NamespaceAndName DELETE_ALERT{NAMESPACE, DIRECTIVE_NAME_DELETE_ALERT};
/// The DeleteAlerts directive signature.
static const avsCommon::avs::NamespaceAndName DELETE_ALERTS{NAMESPACE, DIRECTIVE_NAME_DELETE_ALERTS};
/// The SetVolume directive signature.
static const avsCommon::avs::NamespaceAndName SET_VOLUME{NAMESPACE, DIRECTIVE_NAME_SET_VOLUME};
/// The AdjustVolume directive signature.
static const avsCommon::avs::NamespaceAndName ADJUST_VOLUME{NAMESPACE, DIRECTIVE_NAME_ADJUST_VOLUME};

/// String to identify log entries originating from this file.
static const std::string TAG("AlertsCapabilityAgent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Creates the alerts capability configuration.
 *
 * @return The alerts capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getAlertsCapabilityConfiguration();

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
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
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
        speakerManager,
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

    focusManager->addObserver(alertsCA);
    connectionManager->addConnectionStatusObserver(alertsCA);
    speakerManager->addSpeakerManagerObserver(alertsCA);

    return alertsCA;
}

avsCommon::avs::DirectiveHandlerConfiguration AlertsCapabilityAgent::getConfiguration() const {
    auto audioNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUM_AUDIO, false);
    auto neitherNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    avsCommon::avs::DirectiveHandlerConfiguration configuration;
    configuration[SET_ALERT] = neitherNonBlockingPolicy;
    configuration[DELETE_ALERT] = neitherNonBlockingPolicy;
    configuration[DELETE_ALERTS] = neitherNonBlockingPolicy;
    configuration[SET_VOLUME] = audioNonBlockingPolicy;
    configuration[ADJUST_VOLUME] = audioNonBlockingPolicy;
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

void AlertsCapabilityAgent::onFocusChanged(const std::string& channelName, avsCommon::avs::FocusState newFocus) {
    m_executor.submit([this, channelName, newFocus]() { executeOnFocusManagerFocusChanged(channelName, newFocus); });
}

void AlertsCapabilityAgent::onAlertStateChange(
    const std::string& alertToken,
    const std::string& alertType,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG9(LX("onAlertStateChange")
                     .d("alertToken", alertToken)
                     .d("alertType", alertType)
                     .d("state", state)
                     .d("reason", reason));
    m_executor.submit([this, alertToken, alertType, state, reason]() {
        executeOnAlertStateChange(alertToken, alertType, state, reason);
    });
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
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
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
        m_speakerManager{speakerManager},
        m_contextManager{contextManager},
        m_isConnected{false},
        m_alertScheduler{alertStorage, alertRenderer, ALERT_PAST_DUE_CUTOFF_MINUTES},
        m_alertsAudioFactory{alertsAudioFactory},
        m_contentChannelIsActive{false},
        m_commsChannelIsActive{false},
        m_alertIsSounding{false} {
    m_capabilityConfigurations.insert(getAlertsCapabilityConfiguration());
}

std::shared_ptr<CapabilityConfiguration> getAlertsCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, ALERTS_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, ALERTS_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, ALERTS_CAPABILITY_INTERFACE_VERSION});

    return std::make_shared<CapabilityConfiguration>(configMap);
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

    // Initialize stored value for AVS_ALERTS_VOLUME speaker settings
    if (!getAlertVolumeSettings(&m_lastReportedSpeakerSettings)) {
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
    ACSDK_DEBUG5(LX(__func__));
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

bool AlertsCapabilityAgent::handleDeleteAlerts(
    const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
    const rapidjson::Document& payload) {
    ACSDK_DEBUG5(LX(__func__));

    std::list<std::string> alertTokens;

    auto tokensPayload = payload.FindMember(DIRECTIVE_PAYLOAD_TOKENS_KEY.c_str());
    if (tokensPayload == payload.MemberEnd()) {
        ACSDK_ERROR(LX("handleDeleteAlertsFailed").d("reason", "Cannot find tokens in payload"));
        return false;
    }

    if (!tokensPayload->value.IsArray()) {
        ACSDK_ERROR(LX("handleDeleteAlertsFailed")
                        .d("reason", "value is expected to be an array")
                        .d("key", DIRECTIVE_PAYLOAD_TOKENS_KEY.c_str()));
        return false;
    }

    auto tokenArray = tokensPayload->value.GetArray();
    for (rapidjson::SizeType i = 0; i < tokenArray.Size(); i++) {
        std::string token;
        if (!convertToValue(tokenArray[i], &token)) {
            ACSDK_WARN(LX("handleDeleteAlertsFailed").d("reason", "invalid token in payload"));
            continue;
        }
        alertTokens.push_back(token);
    }

    if (!m_alertScheduler.deleteAlerts(alertTokens)) {
        sendBulkEvent(ALERT_DELETE_ALERTS_FAILED_EVENT_NAME, alertTokens, true);
        return false;
    }

    sendBulkEvent(ALERT_DELETE_ALERTS_SUCCEEDED_EVENT_NAME, alertTokens, true);
    updateContextManager();

    return true;
}

bool AlertsCapabilityAgent::handleSetVolume(
    const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
    const rapidjson::Document& payload) {
    ACSDK_DEBUG5(LX(__func__));
    int64_t volumeValue = 0;
    if (!retrieveValue(payload, DIRECTIVE_PAYLOAD_VOLUME, &volumeValue)) {
        ACSDK_ERROR(LX("handleSetVolumeFailed").m("Could not find volume in the payload."));
        return false;
    }

    setNextAlertVolume(volumeValue);

    return true;
}

bool AlertsCapabilityAgent::handleAdjustVolume(
    const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
    const rapidjson::Document& payload) {
    ACSDK_DEBUG5(LX(__func__));
    int64_t adjustValue = 0;
    if (!retrieveValue(payload, DIRECTIVE_PAYLOAD_VOLUME, &adjustValue)) {
        ACSDK_ERROR(LX("handleAdjustVolumeFailed").m("Could not find volume in the payload."));
        return false;
    }

    SpeakerInterface::SpeakerSettings speakerSettings;
    m_speakerManager->getSpeakerSettings(SpeakerInterface::Type::AVS_ALERTS_VOLUME, &speakerSettings);
    int64_t volume = adjustValue + speakerSettings.volume;

    setNextAlertVolume(volume);

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

void AlertsCapabilityAgent::sendBulkEvent(
    const std::string& eventName,
    const std::list<std::string>& tokenList,
    bool isCertified) {
    rapidjson::Document payload(kObjectType);
    rapidjson::Document::AllocatorType& alloc = payload.GetAllocator();

    rapidjson::Value jsonTokenList(kArrayType);

    for (auto& token : tokenList) {
        jsonTokenList.PushBack(StringRef(token), alloc);
    }

    payload.AddMember(StringRef(EVENT_PAYLOAD_TOKENS_KEY), jsonTokenList, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("sendBulkEventFailed").m("Could not construct payload."));
        return;
    }

    auto jsonEventString = buildJsonEventString(eventName, EMPTY_DIALOG_REQUEST_ID, buffer.GetString()).second;

    if (isCertified) {
        m_certifiedSender->sendJSONMessage(jsonEventString);
    } else {
        if (!m_isConnected) {
            ACSDK_WARN(LX(__func__).m("Not connected to AVS.  Not sending Event.").d("event details", jsonEventString));
        } else {
            auto request = std::make_shared<MessageRequest>(jsonEventString);
            m_messageSender->sendMessage(request);
        }
    }
}

void AlertsCapabilityAgent::updateAVSWithLocalVolumeChanges(int8_t volume, bool forceUpdate) {
    if (!forceUpdate && volume == m_lastReportedSpeakerSettings.volume) {
        // Current speaker volume corresponds to what AVS has
        ACSDK_DEBUG7(LX("updateAVSWithLocalVolumeChanges").d("Alerts volume already set to this value", volume));
        return;
    }

    m_lastReportedSpeakerSettings.volume = volume;

    rapidjson::Document payload(kObjectType);
    rapidjson::Document::AllocatorType& alloc = payload.GetAllocator();

    payload.AddMember(StringRef(AVS_PAYLOAD_VOLUME_KEY), volume, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("updateAVSWithLocalVolumeChangesFailed").m("Could not construct payload."));
        return;
    }

    auto jsonEventString =
        buildJsonEventString(ALERT_VOLUME_CHANGED_EVENT_NAME, EMPTY_DIALOG_REQUEST_ID, buffer.GetString()).second;

    m_certifiedSender->sendJSONMessage(jsonEventString);
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
    } else if (DIRECTIVE_NAME_DELETE_ALERTS == directiveName) {
        handleDeleteAlerts(directive, payload);
    } else if (DIRECTIVE_NAME_SET_VOLUME == directiveName) {
        handleSetVolume(directive, payload);
    } else if (DIRECTIVE_NAME_ADJUST_VOLUME == directiveName) {
        handleAdjustVolume(directive, payload);
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

void AlertsCapabilityAgent::executeOnFocusManagerFocusChanged(
    const std::string& channelName,
    avsCommon::avs::FocusState focusState) {
    bool stateIsActive = focusState != FocusState::NONE;

    if (FocusManagerInterface::CONTENT_CHANNEL_NAME == channelName) {
        m_contentChannelIsActive = stateIsActive;
    } else if (FocusManagerInterface::COMMUNICATIONS_CHANNEL_NAME == channelName) {
        m_commsChannelIsActive = stateIsActive;
    } else {
        return;
    }

    if (m_alertIsSounding) {
        if (!m_commsChannelIsActive && !m_contentChannelIsActive) {
            // All lower channels of interest are stopped playing content. Return alert volume to base value
            // if needed.
            SpeakerInterface::SpeakerSettings speakerSettings;
            if (!getAlertVolumeSettings(&speakerSettings)) {
                ACSDK_ERROR(LX("executeOnFocusChangedFailed").d("reason", "Failed to get speaker settings."));
                return;
            }

            if (speakerSettings.volume > m_lastReportedSpeakerSettings.volume) {
                // Alert is sounding with volume higher than Base Volume. Assume that it was adjusted because of
                // content being played and reset it to the base one. Keep lower values, though.
                m_speakerManager->setVolume(
                    SpeakerInterface::Type::AVS_ALERTS_VOLUME, m_lastReportedSpeakerSettings.volume);
            }
        }
    }
}

void AlertsCapabilityAgent::executeOnAlertStateChange(
    const std::string& alertToken,
    const std::string& alertType,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG1(LX("executeOnAlertStateChange").d("alertToken", alertToken).d("state", state).d("reason", reason));

    bool alertIsActive = false;

    switch (state) {
        case AlertObserverInterface::State::READY:
            acquireChannel();
            break;

        case AlertObserverInterface::State::STARTED:
            sendEvent(ALERT_STARTED_EVENT_NAME, alertToken, true);
            updateContextManager();
            alertIsActive = true;
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
            alertIsActive = true;
            sendEvent(ALERT_ENTERED_FOREGROUND_EVENT_NAME, alertToken);
            break;

        case AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND:
            alertIsActive = true;
            sendEvent(ALERT_ENTERED_BACKGROUND_EVENT_NAME, alertToken);
            break;
    }

    if (alertIsActive) {
        // Alert is going to go off
        m_alertIsSounding = true;
        // Check if there are lower channels with content being played and increase alert volume if needed.
        if (m_contentChannelIsActive || m_commsChannelIsActive) {
            SpeakerInterface::SpeakerSettings contentSpeakerSettings;
            if (getSpeakerVolumeSettings(&contentSpeakerSettings)) {
                if (m_lastReportedSpeakerSettings.volume < contentSpeakerSettings.volume) {
                    // Adjust alerts volume to be at least as loud as content volume
                    m_speakerManager->setVolume(
                        SpeakerInterface::Type::AVS_ALERTS_VOLUME, contentSpeakerSettings.volume);
                }
            }
        }
    } else {
        if (m_alertIsSounding) {
            // Alert has just switched from STARTED to something else, since it could not transition from STARTED to
            // READY we may treat it as it is stopping.

            // Reset Active Alerts Volume volume to the Base Alerts Volume when alert stops
            m_alertIsSounding = false;
            m_speakerManager->setVolume(
                SpeakerInterface::Type::AVS_ALERTS_VOLUME, m_lastReportedSpeakerSettings.volume);
        }
    }

    m_executor.submit([this, alertToken, alertType, state, reason]() {
        executeNotifyObservers(alertToken, alertType, state, reason);
    });
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
    const std::string& alertType,
    AlertObserverInterface::State state,
    const std::string& reason) {
    ACSDK_DEBUG1(LX("executeNotifyObservers")
                     .d("alertToken", alertToken)
                     .d("alertType", alertType)
                     .d("state", state)
                     .d("reason", reason));
    for (auto observer : m_observers) {
        observer->onAlertStateChange(alertToken, alertType, state, reason);
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

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> AlertsCapabilityAgent::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void AlertsCapabilityAgent::onSpeakerSettingsChanged(
    const SpeakerManagerObserverInterface::Source& source,
    const SpeakerInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& settings) {
    m_executor.submit([this, settings, type]() { executeOnSpeakerSettingsChanged(type, settings); });
}

bool AlertsCapabilityAgent::getAlertVolumeSettings(
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* speakerSettings) {
    if (!m_speakerManager->getSpeakerSettings(SpeakerInterface::Type::AVS_ALERTS_VOLUME, speakerSettings).get()) {
        ACSDK_ERROR(LX("getAlertSpeakerSettingsFailed").d("reason", "Failed to get speaker settings"));
        return false;
    }
    return true;
}

bool AlertsCapabilityAgent::getSpeakerVolumeSettings(
    avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* speakerSettings) {
    if (!m_speakerManager->getSpeakerSettings(SpeakerInterface::Type::AVS_SPEAKER_VOLUME, speakerSettings).get()) {
        ACSDK_ERROR(LX("getContentSpeakerSettingsFailed").d("reason", "Failed to get speaker settings"));
        return false;
    }
    return true;
}

void AlertsCapabilityAgent::setNextAlertVolume(int64_t volume) {
    if (volume < speakerConstants::AVS_SET_VOLUME_MIN) {
        volume = speakerConstants::AVS_SET_VOLUME_MIN;
        ACSDK_DEBUG7(LX(__func__).m("Requested volume is lower than allowed minimum, using minimum instead."));
    } else if (volume > speakerConstants::AVS_SET_VOLUME_MAX) {
        volume = speakerConstants::AVS_SET_VOLUME_MAX;
        ACSDK_DEBUG7(LX(__func__).m("Requested volume is higher than allowed maximum, using maximum instead."));
    }

    ACSDK_DEBUG5(LX(__func__).d("New Alerts volume", volume));

    m_speakerManager
        ->setVolume(avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME, static_cast<int8_t>(volume))
        .get();

    // Always notify AVS of volume changes here
    updateAVSWithLocalVolumeChanges(static_cast<int8_t>(volume), true);
}

void AlertsCapabilityAgent::executeOnSpeakerSettingsChanged(
    const SpeakerInterface::Type& type,
    const SpeakerInterface::SpeakerSettings& speakerSettings) {
    if (SpeakerInterface::Type::AVS_ALERTS_VOLUME == type && !m_alertIsSounding) {
        updateAVSWithLocalVolumeChanges(speakerSettings.volume, false);
    }
}

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
