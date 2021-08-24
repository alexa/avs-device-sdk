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

#include <chrono>

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <Settings/SettingEventMetadata.h>

#include "acsdkDoNotDisturb/DNDMessageRequest.h"
#include "acsdkDoNotDisturb/DNDSettingProtocol.h"
#include "acsdkDoNotDisturb/DoNotDisturbCapabilityAgent.h"

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

/// A timeout for an HTTP response when sending change events.
static const std::chrono::seconds HTTP_RESPONSE_TIMEOUT(15);

std::shared_ptr<DoNotDisturbCapabilityAgent> DoNotDisturbCapabilityAgent::createDoNotDisturbCapabilityAgent(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
    const std::shared_ptr<settings::storage::DeviceSettingStorageInterface>& settingsStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
    const acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>& endpointCapabilitiesRegistrar,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    ACSDK_DEBUG5(LX(__func__));

    if (!exceptionSender || !messageSender || !settingsStorage || !shutdownNotifier || !endpointCapabilitiesRegistrar ||
        !connectionManager) {
        ACSDK_ERROR(LX("createDoNotDisturbCapabilityAgentFailed")
                        .d("isExceptionSenderNull", !exceptionSender)
                        .d("isMessageSenderNull", !messageSender)
                        .d("isSettingsStorageNull", !settingsStorage)
                        .d("isShutdownNotifierNull", !shutdownNotifier)
                        .d("isEndpointCapabilitiesRegistrarNull", !endpointCapabilitiesRegistrar)
                        .d("isConnectionManagerNull", !connectionManager));
        return nullptr;
    }

    auto dndCA = create(messageSender, settingsStorage, exceptionSender, connectionManager, metricRecorder);
    if (!dndCA) {
        ACSDK_ERROR(LX("createDoNotDisturbCapabilityAgentFailed").m("null DoNotDisturb CapabilityAgent"));
        return nullptr;
    }

    shutdownNotifier->addObserver(dndCA);
    endpointCapabilitiesRegistrar->withCapability(dndCA, dndCA);
    connectionManager->addConnectionStatusObserver(dndCA);

    return dndCA;
}

std::shared_ptr<DoNotDisturbCapabilityAgent> DoNotDisturbCapabilityAgent::create(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingsStorage) {
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "messageSenderNull"));
        return nullptr;
    }

    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "exceptionEncounteredSenderNull"));
        return nullptr;
    }

    if (!settingsStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "settingsStorageNull"));
        return nullptr;
    }

    auto dndCA = create(messageSender, settingsStorage, exceptionEncounteredSender);

    if (!dndCA) {
        ACSDK_ERROR(LX("createFailed").d("reason", "null DoNotDisturb CapabilityAgent"));
        return nullptr;
    }

    return dndCA;
}

std::shared_ptr<DoNotDisturbCapabilityAgent> DoNotDisturbCapabilityAgent::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
    const std::shared_ptr<settings::storage::DeviceSettingStorageInterface>& settingsStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    auto dndCA = std::shared_ptr<DoNotDisturbCapabilityAgent>(
        new DoNotDisturbCapabilityAgent(exceptionSender, messageSender, connectionManager));

    if (!dndCA->initialize(settingsStorage, metricRecorder)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initialization failed"));
        return nullptr;
    }

    return dndCA;
}

DoNotDisturbCapabilityAgent::DoNotDisturbCapabilityAgent(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<AVSConnectionManagerInterface> connectionManager) :
        CapabilityAgent{NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"acsdkDoNotDisturb"},
        m_messageSender{messageSender},
        m_connectionManager{connectionManager},
        m_isConnected{false},
        m_hasOfflineChanges{false} {
    generateCapabilityConfiguration();
}

std::shared_ptr<settings::DoNotDisturbSetting> DoNotDisturbCapabilityAgent::getDoNotDisturbSetting() const {
    return m_dndModeSetting;
}

settings::SettingEventMetadata DoNotDisturbCapabilityAgent::getDoNotDisturbEventsMetadata() {
    return settings::SettingEventMetadata{
        NAMESPACE, EVENT_DONOTDISTURBCHANGED.name, EVENT_REPORTDONOTDISTURB.name, JSON_KEY_ENABLED};
}

bool DoNotDisturbCapabilityAgent::initialize(
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface> settingsStorage,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    auto metadata = getDoNotDisturbEventsMetadata();
    auto protocol = DNDSettingProtocol::create(metadata, shared_from_this(), settingsStorage, metricRecorder);
    m_dndModeSetting = settings::Setting<bool>::create(false, std::move(protocol));
    return m_dndModeSetting != nullptr;
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
    if (m_connectionManager) {
        m_connectionManager->removeConnectionStatusObserver(shared_from_this());
    }

    m_executor.waitForSubmittedTasks();
    m_executor.shutdown();
    m_dndModeSetting.reset();
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
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<std::promise<bool>> promise = std::make_shared<std::promise<bool>>();
    {
        std::lock_guard<std::mutex> guard(m_connectedStateMutex);
        if (!m_isConnected) {
            m_hasOfflineChanges = true;
            promise->set_value(false);
            return promise->get_future();
        }
        m_hasOfflineChanges = false;
    }

    // Avoid a race condition where doShutdown() resets the m_dndModeSetting pointer
    // while the lambda executes, causing a segfault if m_dndModeSetting is dereferenced.
    auto dndModeSetting = m_dndModeSetting;

    // Sequentialize event processing so that no directive or another event would be handled while we sending this event
    m_executor.submit([this, value, dndModeSetting, promise]() {
        auto future = sendDNDEvent(EVENT_DONOTDISTURBCHANGED.name, value);
        if (future.wait_for(HTTP_RESPONSE_TIMEOUT) == std::future_status::ready) {
            auto httpResponse = future.get();
            std::ostringstream oss;
            oss << httpResponse;
            ACSDK_DEBUG5(
                LX("sendChangedEventExecutor").d("eventName", EVENT_DONOTDISTURBCHANGED.name).d("status", oss.str()));

            switch (httpResponse) {
                case MessageRequestObserverInterface::Status::THROTTLED:
                case MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR:
                case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
                case MessageRequestObserverInterface::Status::TIMEDOUT:
                case MessageRequestObserverInterface::Status::INTERNAL_ERROR:
                    // retry once but don't wait for response
                    sendDNDEvent(EVENT_REPORTDONOTDISTURB.name, dndModeSetting->get() ? "true" : "false");
                    promise->set_value(true);
                    break;

                case MessageRequestObserverInterface::Status::SUCCESS:
                case MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED:
                case MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT:
                    promise->set_value(true);
                    break;

                case MessageRequestObserverInterface::Status::CANCELED:
                case MessageRequestObserverInterface::Status::BAD_REQUEST:
                case MessageRequestObserverInterface::Status::PENDING:
                case MessageRequestObserverInterface::Status::NOT_CONNECTED:
                case MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED:
                case MessageRequestObserverInterface::Status::PROTOCOL_ERROR:
                case MessageRequestObserverInterface::Status::REFUSED:
                case MessageRequestObserverInterface::Status::INVALID_AUTH:
                default:
                    promise->set_value(false);
                    break;
            }
        } else {
            ACSDK_WARN(LX("sendChangedEventExecutor").m("sendEventFailed").d("reason", "noHTTPResponse"));
            // retry once but don't wait for response
            sendDNDEvent(EVENT_REPORTDONOTDISTURB.name, dndModeSetting->get() ? "true" : "false");
            promise->set_value(true);
        }
    });
    return promise->get_future();
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

std::shared_future<bool> DoNotDisturbCapabilityAgent::sendStateReportEvent(const std::string& payload) {
    // Not supported.
    std::promise<bool> promise;
    promise.set_value(false);
    return promise.get_future();
}

void DoNotDisturbCapabilityAgent::cancel() {
    // TODO: ACSDK-2246: use SharedAVSSettingProtocol to implement sending logic.
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
