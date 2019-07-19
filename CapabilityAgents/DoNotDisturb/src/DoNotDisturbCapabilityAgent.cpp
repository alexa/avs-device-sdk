/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <Settings/SettingEventMetadata.h>

#include "DoNotDisturbCA/DoNotDisturbCapabilityAgent.h"
#include "DoNotDisturbCA/DNDMessageRequest.h"
#include "DoNotDisturbCA/DNDSettingProtocol.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace doNotDisturb {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::logger;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"DoNotDisturbCapabilityAgent"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The Alexa.DoNotDisturb interface namespace.
static const std::string NAMESPACE = "Alexa.DoNotDisturb";

/// The @c SetDoNotDisturb directive identifier.
static const NamespaceAndName DIRECTIVE_SETDONOTDISTURB{NAMESPACE, "SetDoNotDisturb"};

/// The @c DoNotDisturbChanged event identifier.
static const NamespaceAndName EVENT_DONOTDISTURBCHANGED{NAMESPACE, "DoNotDisturbChanged"};

/// The @c ReportDoNotDisturb event identifier.
static const NamespaceAndName EVENT_REPORTDONOTDISTURB{NAMESPACE, "ReportDoNotDisturb"};

/// AVS Interface type.
static const std::string DND_JSON_INTERFACE_TYPE = "AlexaInterface";

/// AVS Interface name.
static const std::string DND_JSON_INTERFACE_NAME = "Alexa.DoNotDisturb";

/// AVS interface version.
static const std::string DND_JSON_INTERFACE_VERSION = "1.0";

/// Name for "enabled" JSON branch.
static constexpr char JSON_KEY_ENABLED[] = "enabled";

std::shared_ptr<DoNotDisturbCapabilityAgent> DoNotDisturbCapabilityAgent::create(
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager,
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingsStorage) {
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "messageSenderNull"));
        return nullptr;
    }

    if (!customerDataManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "customerDataManagerNull"));
        return nullptr;
    }

    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "exceptionEncounteredSenderNull"));
        return nullptr;
    }

    if (!settingsManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "settingsManagerNull"));
        return nullptr;
    }

    if (!settingsStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "settingsStorageNull"));
        return nullptr;
    }

    auto dndCA = std::shared_ptr<DoNotDisturbCapabilityAgent>(new DoNotDisturbCapabilityAgent(
        customerDataManager, exceptionEncounteredSender, messageSender, settingsManager));

    if (!dndCA->initialize(settingsStorage)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initialization failed."));
        return nullptr;
    }

    return dndCA;
}

DoNotDisturbCapabilityAgent::DoNotDisturbCapabilityAgent(
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"DoNotDisturbCA"},
        CustomerDataHandler{customerDataManager},
        m_messageSender{messageSender},
        m_settingsManager{settingsManager},
        m_isConnected{false},
        m_hasOfflineChanges{false} {
    generateCapabilityConfiguration();
}

bool DoNotDisturbCapabilityAgent::initialize(
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingsStorage) {
    // TODO: ACSDK-2089 - Clear the data when SettingsAPI allows. There will be no need to keep settings storage in CA.
    m_settingsStorage = settingsStorage;

    const settings::SettingEventMetadata metadata = {
        .eventNamespace = NAMESPACE,
        .eventChangedName = EVENT_DONOTDISTURBCHANGED.name,
        .eventReportName = EVENT_REPORTDONOTDISTURB.name,
        .settingName = JSON_KEY_ENABLED,
    };
    auto protocol = DNDSettingProtocol::create(metadata, shared_from_this(), settingsStorage);
    m_dndModeSetting = settings::Setting<bool>::create(false, std::move(protocol));
    return m_settingsManager->addSetting<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(m_dndModeSetting);
}

void DoNotDisturbCapabilityAgent::generateCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, DND_JSON_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, DND_JSON_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, DND_JSON_INTERFACE_VERSION});

    m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));
}

avsCommon::avs::DirectiveHandlerConfiguration DoNotDisturbCapabilityAgent::getConfiguration() const {
    ACSDK_DEBUG5(LX(__func__));

    DirectiveHandlerConfiguration configuration;
    configuration[DIRECTIVE_SETDONOTDISTURB] = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    return configuration;
}

void DoNotDisturbCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void DoNotDisturbCapabilityAgent::preHandleDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    // No-op
}

/**
 * Parses a directive payload JSON and returns a parsed document object.
 *
 * @param payload JSON string to parse.
 * @param[out] document Pointer to a parsed document.
 * @return True if parsing was successful, false otherwise.
 */
static bool parseDirectivePayload(const std::string& payload, Document* document) {
    ACSDK_DEBUG5(LX(__func__));
    if (!document) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed").d("reason", "nullDocument"));
        return false;
    }

    ParseResult result = document->Parse(payload);
    if (!result) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return false;
    }

    return true;
}

void DoNotDisturbCapabilityAgent::handleDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        const std::string directiveName = info->directive->getName();

        Document payload(kObjectType);
        if (!parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (directiveName == DIRECTIVE_SETDONOTDISTURB.name) {
            if (!handleSetDoNotDisturbDirective(info, payload)) {
                return;
            }
        } else {
            ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "Unknown directive."));
            sendExceptionEncounteredAndReportFailed(
                info, "Unexpected Directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (info->result) {
            info->result->setCompleted();
        }
        removeDirective(info->directive->getMessageId());
    });
}

void DoNotDisturbCapabilityAgent::cancelDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    if (info && info->directive) {
        removeDirective(info->directive->getMessageId());
    }
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> DoNotDisturbCapabilityAgent::
    getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void DoNotDisturbCapabilityAgent::doShutdown() {
    m_settingsManager->removeSetting<settings::DeviceSettingsIndex::DO_NOT_DISTURB>(m_dndModeSetting);
    m_dndModeSetting.reset();
}

void DoNotDisturbCapabilityAgent::clearData() {
    // TODO: ACSDK-2089 - Clear the data when SettingsAPI allows.
    std::string settingKey = NAMESPACE + "::" + JSON_KEY_ENABLED;
    m_settingsStorage->deleteSetting(settingKey);
}

bool DoNotDisturbCapabilityAgent::handleSetDoNotDisturbDirective(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    rapidjson::Document& document) {
    bool state = false;

    if (!retrieveValue(document, JSON_KEY_ENABLED, &state)) {
        std::string errorMessage = "'enabled' value not specified for SetDoNotDisturb";
        ACSDK_ERROR(LX("handleSetDoNotDisturbDirectiveFailed").m(errorMessage));
        sendExceptionEncounteredAndReportFailed(info, errorMessage);
        return false;
    }

    m_dndModeSetting->setAvsChange(state);

    return true;
}

std::shared_future<MessageRequestObserverInterface::Status> DoNotDisturbCapabilityAgent::sendDNDEvent(
    const std::string& eventName,
    const std::string& value) {
    const std::string EMPTY_DIALOG_REQUEST_ID;
    JsonGenerator payload;
    if (!payload.addRawJsonMember(JSON_KEY_ENABLED, value)) {
        ACSDK_ERROR(LX("sendEventFailed").d("reason", "failedToAddValueToPayload"));
        std::promise<MessageRequestObserverInterface::Status> promise;
        promise.set_value(MessageRequestObserverInterface::Status::INTERNAL_ERROR);
        return promise.get_future();
    }
    auto jsonEventString = buildJsonEventString(eventName, EMPTY_DIALOG_REQUEST_ID, payload.toString()).second;
    auto request = std::make_shared<DNDMessageRequest>(jsonEventString);

    m_messageSender->sendMessage(request);
    return request->getCompletionFuture();
}

std::shared_future<bool> DoNotDisturbCapabilityAgent::sendChangedEvent(const std::string& value) {
    std::promise<bool> promise;
    {
        std::lock_guard<std::mutex> guard(m_connectedStateMutex);
        if (!m_isConnected) {
            m_hasOfflineChanges = true;
            promise.set_value(false);
            return promise.get_future();
        }
        m_hasOfflineChanges = false;
    }

    // Sequentialize event processing so that no directive or another event would be handled while we sending this event
    m_executor.submit([this, value]() {
        MessageRequestObserverInterface::Status status = sendDNDEvent(EVENT_DONOTDISTURBCHANGED.name, value).get();
        bool isSucceeded = MessageRequestObserverInterface::Status::SUCCESS == status ||
                           MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT == status;

        if (!isSucceeded) {
            sendDNDEvent(EVENT_REPORTDONOTDISTURB.name, m_dndModeSetting->get() ? "true" : "false");
        }
    });
    promise.set_value(true);
    return promise.get_future();
}

std::shared_future<bool> DoNotDisturbCapabilityAgent::sendReportEvent(const std::string& value) {
    std::promise<bool> promise;
    if (!m_isConnected) {
        promise.set_value(false);
        return promise.get_future();
    }
    m_executor.submit([this, value]() { sendDNDEvent(EVENT_REPORTDONOTDISTURB.name, value); });

    promise.set_value(true);
    return promise.get_future();
}

void DoNotDisturbCapabilityAgent::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    std::lock_guard<std::mutex> guard(m_connectedStateMutex);

    m_isConnected = status == ConnectionStatusObserverInterface::Status::CONNECTED;
    if (m_isConnected) {
        std::string modeString = m_dndModeSetting->get() ? "true" : "false";
        if (m_hasOfflineChanges) {
            // Reapply the change that was not delivered to the AVS while SDK was offline. Use the final value.
            m_dndModeSetting->setLocalChange(m_dndModeSetting->get());
        } else {
            sendReportEvent(modeString);
        }
    }
}

}  // namespace doNotDisturb
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
