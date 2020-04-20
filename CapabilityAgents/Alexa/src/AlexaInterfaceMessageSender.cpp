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

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

#include "Alexa/AlexaInterfaceMessageSender.h"
#include "Alexa/AlexaInterfaceConstants.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG{"AlexaInterfaceMessageSender"};

/// Name of response events.
static const std::string EVENT_NAME_STATE_REPORT_STRING = "StateReport";

/// Name of response events.
static const std::string EVENT_NAME_RESPONSE_STRING = "Response";

/// Name of error response events.
static const std::string EVENT_NAME_ERROR_RESPONSE_STRING = "ErrorResponse";

/// Name of error response events.
static const std::string EVENT_NAME_DEFERRED_RESPONSE_STRING = "DeferredResponse";

/// Name of error response events.
static const std::string EVENT_NAME_CHANGE_REPORT_STRING = "ChangeReport";

/// The estimated deferral key.
static const std::string ESTIMATED_DEFERRAL_KEY_STRING = "estimatedDeferralInSeconds";

/// The cause key.
static const std::string CAUSE_KEY_STRING = "cause";

/// The change key.
static const std::string CHANGE_KEY_STRING = "change";

/// The message key in the event.
static const std::string MESSAGE_KEY_STRING = "message";

/// The properties key.
static const std::string PROPERTIES_KEY_STRING = "properties";

/// The timeOfSample key.
static const std::string TIME_OF_SAMPLE_KEY_STRING = "timeOfSample";

/// The type key.
static const std::string TYPE_KEY_STRING = "type";

/// The type key.
static const std::string UNCERTAINTY_IN_MILLISECONDS_KEY_STRING = "uncertaintyInMilliseconds";

/// The value key.
static const std::string VALUE_KEY_STRING = "value";

/// Map of AlexaInterfaceMessageSender::ErrorResponseTypes to their string representations.
static const std::map<AlexaInterfaceMessageSender::ErrorResponseType, std::string> errorTypeMap = {
    {AlexaInterfaceMessageSender::ErrorResponseType::ALREADY_IN_OPERATION, "ALREADY_IN_OPERATION"},
    {AlexaInterfaceMessageSender::ErrorResponseType::BRIDGE_UNREACHABLE, "BRIDGE_UNREACHABLE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::ENDPOINT_BUSY, "ENDPOINT_BUSY"},
    {AlexaInterfaceMessageSender::ErrorResponseType::ENDPOINT_LOW_POWER, "ENDPOINT_LOW_POWER"},
    {AlexaInterfaceMessageSender::ErrorResponseType::ENDPOINT_UNREACHABLE, "ENDPOINT_UNREACHABLE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::EXPIRED_AUTHORIZATION_CREDENTIAL,
     "EXPIRED_AUTHORIZATION_CREDENTIAL"},
    {AlexaInterfaceMessageSender::ErrorResponseType::FIRMWARE_OUT_OF_DATE, "FIRMWARE_OUT_OF_DATE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::HARDWARE_MALFUNCTION, "HARDWARE_MALFUNCTION"},
    {AlexaInterfaceMessageSender::ErrorResponseType::INSUFFICIENT_PERMISSIONS, "INSUFFICIENT_PERMISSIONS"},
    {AlexaInterfaceMessageSender::ErrorResponseType::INTERNAL_ERROR, "INTERNAL_ERROR"},
    {AlexaInterfaceMessageSender::ErrorResponseType::INVALID_AUTHORIZATION_CREDENTIAL,
     "INVALID_AUTHORIZATION_CREDENTIAL"},
    {AlexaInterfaceMessageSender::ErrorResponseType::INVALID_DIRECTIVE, "INVALID_DIRECTIVE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::INVALID_VALUE, "INVALID_VALUE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::NO_SUCH_ENDPOINT, "NO_SUCH_ENDPOINT"},
    {AlexaInterfaceMessageSender::ErrorResponseType::NOT_CALIBRATED, "NOT_CALIBRATED"},
    {AlexaInterfaceMessageSender::ErrorResponseType::NOT_SUPPORTED_IN_CURRENT_MODE, "NOT_SUPPORTED_IN_CURRENT_MODE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::NOT_IN_OPERATION, "NOT_IN_OPERATION"},
    {AlexaInterfaceMessageSender::ErrorResponseType::POWER_LEVEL_NOT_SUPPORTED, "POWER_LEVEL_NOT_SUPPORTED"},
    {AlexaInterfaceMessageSender::ErrorResponseType::RATE_LIMIT_EXCEEDED, "RATE_LIMIT_EXCEEDED"},
    {AlexaInterfaceMessageSender::ErrorResponseType::TEMPERATURE_VALUE_OUT_OF_RANGE, "TEMPERATURE_VALUE_OUT_OF_RANGE"},
    {AlexaInterfaceMessageSender::ErrorResponseType::VALUE_OUT_OF_RANGE, "VALUE_OUT_OF_RANGE"}};

/// Map of AlexaStateChangeCauseType to their string representations.
static const std::map<AlexaStateChangeCauseType, std::string> causeTypeMap = {
    {AlexaStateChangeCauseType::ALEXA_INTERACTION, "ALEXA_INTERACTION"},
    {AlexaStateChangeCauseType::APP_INTERACTION, "APP_INTERACTION"},
    {AlexaStateChangeCauseType::PHYSICAL_INTERACTION, "PHYSICAL_INTERACTION"},
    {AlexaStateChangeCauseType::PERIODIC_POLL, "PERIODIC_POLL"},
    {AlexaStateChangeCauseType::RULE_TRIGGER, "RULE_TRIGGER"},
    {AlexaStateChangeCauseType::VOICE_INTERACTION, "VOICE_INTERACTION"}};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<AlexaInterfaceMessageSender> AlexaInterfaceMessageSender::create(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "contextManagerNull"));
        return nullptr;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "messageSenderNull"));
        return nullptr;
    }

    auto sender =
        std::shared_ptr<AlexaInterfaceMessageSender>(new AlexaInterfaceMessageSender(contextManager, messageSender));
    if (!sender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "outOfMemory"));
        return nullptr;
    }

    if (!sender->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        sender.reset();
        return nullptr;
    }

    return sender;
}

AlexaInterfaceMessageSender::AlexaInterfaceMessageSender(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) :
        RequiresShutdown{"AlexaInterfaceMessageSender"},
        m_contextManager{contextManager},
        m_messageSender{messageSender} {
}

AlexaInterfaceMessageSender::~AlexaInterfaceMessageSender() {
    m_pendingChangeReports.clear();
    m_pendingResponses.clear();
    m_messageSender.reset();
    m_contextManager.reset();
}

bool AlexaInterfaceMessageSender::initialize() {
    m_contextManager->addContextManagerObserver(shared_from_this());
    return true;
}

void AlexaInterfaceMessageSender::doShutdown() {
    m_contextManager->removeContextManagerObserver(shared_from_this());
    m_executor.shutdown();
}

bool AlexaInterfaceMessageSender::sendStateReportEvent(
    const std::string& instance,
    const std::string& correlationToken,
    const AVSMessageEndpoint& endpoint) {
    return sendCommonResponseEvent(EVENT_NAME_STATE_REPORT_STRING, instance, correlationToken, endpoint);
}

bool AlexaInterfaceMessageSender::sendResponseEvent(
    const std::string& instance,
    const std::string& correlationToken,
    const AVSMessageEndpoint& endpoint,
    const std::string& jsonPayload) {
    return sendCommonResponseEvent(EVENT_NAME_RESPONSE_STRING, instance, correlationToken, endpoint, jsonPayload);
}

bool AlexaInterfaceMessageSender::sendErrorResponseEvent(
    const std::string& instance,
    const std::string& correlationToken,
    const AVSMessageEndpoint& endpoint,
    const AlexaInterfaceMessageSender::ErrorResponseType errorType,
    const std::string& errorMessage) {
    if (errorTypeMap.find(errorType) == errorTypeMap.end()) {
        ACSDK_ERROR(LX("sendErrorResponseEventFailed").d("reason", "invalidErrorType"));
        return false;
    }

    auto errorTypeString = errorTypeMap.at(errorType);
    AVSMessageHeader eventHeader = AVSMessageHeader::createAVSEventHeader(
        ALEXA_INTERFACE_NAME,
        EVENT_NAME_ERROR_RESPONSE_STRING,
        "",
        correlationToken,
        ALEXA_INTERFACE_VERSION,
        instance);

    json::JsonGenerator jsonGenerator;
    jsonGenerator.addMember(TYPE_KEY_STRING, errorTypeString);
    jsonGenerator.addMember(MESSAGE_KEY_STRING, errorMessage);
    jsonGenerator.finishObject();

    auto jsonString =
        buildJsonEventString(eventHeader, Optional<AVSMessageEndpoint>(endpoint), jsonGenerator.toString());
    sendEvent(jsonString);
    return true;
}

bool AlexaInterfaceMessageSender::sendDeferredResponseEvent(
    const std::string& instance,
    const std::string& correlationToken,
    const int estimatedDeferralInSeconds) {
    AVSMessageHeader eventHeader = AVSMessageHeader::createAVSEventHeader(
        ALEXA_INTERFACE_NAME,
        EVENT_NAME_DEFERRED_RESPONSE_STRING,
        "",
        correlationToken,
        ALEXA_INTERFACE_VERSION,
        instance);

    json::JsonGenerator jsonGenerator;
    jsonGenerator.addMember(ESTIMATED_DEFERRAL_KEY_STRING, estimatedDeferralInSeconds);
    jsonGenerator.finishObject();

    auto jsonString = buildJsonEventString(eventHeader, Optional<AVSMessageEndpoint>(), jsonGenerator.toString());
    sendEvent(jsonString);
    return true;
}

AlexaInterfaceMessageSender::ResponseData::ResponseData(
    const std::string& typeIn,
    const std::string& instanceIn,
    const std::string& correlationTokenIn,
    const AVSMessageEndpoint& endpointIn,
    const std::string& jsonPayloadIn) :
        type(typeIn),
        instance(instanceIn),
        correlationToken(correlationTokenIn),
        endpoint(endpointIn),
        jsonPayload(jsonPayloadIn) {
}

AlexaInterfaceMessageSender::ChangeReportData::ChangeReportData(
    const CapabilityTag& tagIn,
    const CapabilityState& stateIn,
    const AlexaStateChangeCauseType& causeIn) :
        tag(tagIn),
        state(stateIn),
        cause(causeIn) {
}

bool AlexaInterfaceMessageSender::sendCommonResponseEvent(
    const std::string& type,
    const std::string& instance,
    const std::string& correlationToken,
    const AVSMessageEndpoint& endpoint,
    const std::string& jsonPayload) {
    auto event = std::make_shared<ResponseData>(type, instance, correlationToken, endpoint, jsonPayload);
    m_executor.submit([this, event]() {
        // Start collecting context for this endpoint.
        auto token = m_contextManager->getContext(shared_from_this(), event->endpoint.endpointId);
        m_pendingResponses[token] = event;
    });

    return true;
}

void AlexaInterfaceMessageSender::completeResponseEvent(
    const std::shared_ptr<ResponseData>& event,
    const Optional<AVSContext>& context) {
    AVSMessageHeader eventHeader = AVSMessageHeader::createAVSEventHeader(
        ALEXA_INTERFACE_NAME, event->type, "", event->correlationToken, ALEXA_INTERFACE_VERSION, event->instance);

    auto jsonString = buildJsonEventString(
        eventHeader, Optional<AVSMessageEndpoint>(event->endpoint), event->jsonPayload, Optional<AVSContext>(context));
    sendEvent(jsonString);
}

void AlexaInterfaceMessageSender::completeChangeReportEvent(
    const std::shared_ptr<ChangeReportData>& event,
    const AVSContext& context) {
    if (causeTypeMap.find(event->cause) == causeTypeMap.end()) {
        ACSDK_ERROR(LX("completeChangeReportEventtFailed").d("reason", "invalidCauseType"));
        return;
    }
    auto causeTypeString = causeTypeMap.at(event->cause);
    std::string instance;
    if (event->tag.instance.hasValue()) {
        instance = event->tag.instance.value();
    }
    AVSMessageHeader eventHeader = AVSMessageHeader::createAVSEventHeader(
        ALEXA_INTERFACE_NAME, EVENT_NAME_CHANGE_REPORT_STRING, "", "", ALEXA_INTERFACE_VERSION, instance);

    // Remove the changed property from the context.
    auto prunedContext = context;
    prunedContext.removeState(event->tag);

    json::JsonGenerator jsonGenerator;
    jsonGenerator.startObject(CHANGE_KEY_STRING);
    {
        jsonGenerator.startObject(CAUSE_KEY_STRING);
        jsonGenerator.addMember(TYPE_KEY_STRING, causeTypeString);
        jsonGenerator.finishObject();

        jsonGenerator.startArray(PROPERTIES_KEY_STRING);
        jsonGenerator.startArrayElement();
        jsonGenerator.addMember(constants::NAMESPACE_KEY_STRING, event->tag.nameSpace);
        jsonGenerator.addMember(constants::NAME_KEY_STRING, event->tag.name);
        if (event->tag.instance.hasValue()) {
            jsonGenerator.addMember("instance", event->tag.instance.value());
        }

        jsonGenerator.addRawJsonMember(VALUE_KEY_STRING, event->state.valuePayload);
        jsonGenerator.addMember(TIME_OF_SAMPLE_KEY_STRING, event->state.timeOfSample.getTime_ISO_8601());
        jsonGenerator.addMember(UNCERTAINTY_IN_MILLISECONDS_KEY_STRING, event->state.uncertaintyInMilliseconds);
        jsonGenerator.finishArrayElement();
        jsonGenerator.finishArray();
    }
    jsonGenerator.finishObject();

    auto jsonString = buildJsonEventString(
        eventHeader,
        Optional<AVSMessageEndpoint>(event->tag.endpointId),
        jsonGenerator.toString(),
        Optional<AVSContext>(prunedContext));
    sendEvent(jsonString);
}

void AlexaInterfaceMessageSender::sendEvent(const std::string& eventJson) {
    auto request = std::make_shared<MessageRequest>(eventJson);
    request->addObserver(shared_from_this());
    m_messageSender->sendMessage(request);
}

void AlexaInterfaceMessageSender::onStateChanged(
    const CapabilityTag& identifier,
    const CapabilityState& state,
    const AlexaStateChangeCauseType cause) {
    auto event = std::make_shared<ChangeReportData>(identifier, state, cause);
    m_executor.submit([this, event]() {
        // Start collecting context for this endpoint.
        auto token = m_contextManager->getContext(shared_from_this(), event->tag.endpointId);
        m_pendingChangeReports[token] = event;
    });
}

void AlexaInterfaceMessageSender::onContextAvailable(
    const std::string& endpointId,
    const AVSContext& endpointContext,
    ContextRequestToken token) {
    m_executor.submit([this, endpointId, endpointContext, token]() {
        ACSDK_DEBUG(LX("onContextAvailable").sensitive("endpointId", endpointId));

        // Is this for a pending response event?
        if (m_pendingResponses.find(token) != m_pendingResponses.end()) {
            completeResponseEvent(m_pendingResponses[token], Optional<AVSContext>(endpointContext));
            m_pendingResponses.erase(token);
        } else if (m_pendingChangeReports.find(token) != m_pendingChangeReports.end()) {
            completeChangeReportEvent(m_pendingChangeReports[token], endpointContext);
            m_pendingChangeReports.erase(token);
        } else {
            ACSDK_ERROR(LX("onContextAvailable").d("reason", "unknownEvent").d("token", token));
        }
    });
}

void AlexaInterfaceMessageSender::onContextFailure(const ContextRequestError error, ContextRequestToken token) {
    m_executor.submit([this, error, token]() {
        ACSDK_ERROR(LX("executeOnContextFailure").d("error", error));

        // Is this for a pending response event?
        if (m_pendingResponses.find(token) != m_pendingResponses.end()) {
            // Try to send without context.
            completeResponseEvent(m_pendingResponses[token]);
            m_pendingResponses.erase(token);
            return;
        } else if (m_pendingChangeReports.find(token) != m_pendingChangeReports.end()) {
            // Cannot send an empty ChangeReport
            ACSDK_ERROR(LX("executeOnContextFailure").d("reason", "cannotSendWithoutContext").d("token", token));
            m_pendingChangeReports.erase(token);
        } else {
            ACSDK_ERROR(LX("executeOnContextFailure").d("reason", "unknownEvent").d("token", token));
        }
    });
}

void AlexaInterfaceMessageSender::onSendCompleted(MessageRequestObserverInterface::Status status) {
    if (status == MessageRequestObserverInterface::Status::SUCCESS ||
        status == MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED ||
        status == MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT) {
        ACSDK_DEBUG(LX("onSendCompleted").d("status", status));
        return;
    }
    ACSDK_ERROR(LX("onSendCompleted").m("sendFailed").d("status", status));
}

void AlexaInterfaceMessageSender::onExceptionReceived(const std::string& exceptionMessage) {
    ACSDK_ERROR(LX("onExceptionReceived").d("exception", exceptionMessage));
}

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
