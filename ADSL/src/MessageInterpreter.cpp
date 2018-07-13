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

#include "ADSL/MessageInterpreter.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/AVS/AVSMessageHeader.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <rapidjson/document.h>
#include <AVSCommon/Utils/Metrics.h>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace adsl {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("MessageInterpreter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// JSON key to get the directive object of a message.
static const std::string JSON_MESSAGE_DIRECTIVE_KEY = "directive";
/// JSON key to get the header object of a message.
static const std::string JSON_MESSAGE_HEADER_KEY = "header";
/// JSON key to get the namespace value of a header.
static const std::string JSON_MESSAGE_NAMESPACE_KEY = "namespace";
/// JSON key to get the name value of a header.
static const std::string JSON_MESSAGE_NAME_KEY = "name";
/// JSON key to get the messageId value of a header.
static const std::string JSON_MESSAGE_ID_KEY = "messageId";
/// JSON key to get the dialogRequestId value of a header.
static const std::string JSON_MESSAGE_DIALOG_REQUEST_ID_KEY = "dialogRequestId";
/// JSON key to get the payload object of a message.
static const std::string JSON_MESSAGE_PAYLOAD_KEY = "payload";

/**
 * Utility function to handle sending exceptions encountered messages back to AVS.
 *
 * @param exceptionEncounteredSender The sender that can send exceptions encountered messages to AVS.
 * @param unparsedMessage The JSON message sent from AVS which we were unable to process.
 * @param errorDescription The description of the error encountered.
 */
static void sendExceptionEncounteredHelper(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    const std::string& unparsedMessage,
    const std::string& errorDescription) {
    exceptionEncounteredSender->sendExceptionEncountered(
        unparsedMessage, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, errorDescription);
}

MessageInterpreter::MessageInterpreter(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager) :
        m_exceptionEncounteredSender{exceptionEncounteredSender},
        m_directiveSequencer{directiveSequencer},
        m_attachmentManager{attachmentManager} {
}

void MessageInterpreter::receive(const std::string& contextId, const std::string& message) {
    Document document;

    if (!parseJSON(message, &document)) {
        const std::string error = "Parsing JSON Document failed";
        sendExceptionEncounteredHelper(m_exceptionEncounteredSender, message, error);
        return;
    }

    // Get iterator to child nodes
    Value::ConstMemberIterator directiveIt;
    if (!findNode(document, JSON_MESSAGE_DIRECTIVE_KEY, &directiveIt)) {
        sendParseValueException(JSON_MESSAGE_DIRECTIVE_KEY, message);
        return;
    }

    Value::ConstMemberIterator headerIt;
    if (!findNode(directiveIt->value, JSON_MESSAGE_HEADER_KEY, &headerIt)) {
        sendParseValueException(JSON_MESSAGE_HEADER_KEY, message);
        return;
    }

    // Retrieve values
    std::string payload;
    if (!retrieveValue(directiveIt->value, JSON_MESSAGE_PAYLOAD_KEY, &payload)) {
        sendParseValueException(JSON_MESSAGE_PAYLOAD_KEY, message);
        return;
    }

    std::string avsNamespace;
    if (!retrieveValue(headerIt->value, JSON_MESSAGE_NAMESPACE_KEY, &avsNamespace)) {
        sendParseValueException(JSON_MESSAGE_NAMESPACE_KEY, message);
        return;
    }

    std::string avsName;
    if (!retrieveValue(headerIt->value, JSON_MESSAGE_NAME_KEY, &avsName)) {
        sendParseValueException(JSON_MESSAGE_NAME_KEY, message);
        return;
    }

    std::string avsMessageId;
    if (!retrieveValue(headerIt->value, JSON_MESSAGE_ID_KEY, &avsMessageId)) {
        sendParseValueException(JSON_MESSAGE_ID_KEY, message);
        return;
    }

    // This is an optional header field - it's ok if it is not present.
    // Avoid jsonUtils::retrieveValue because it logs a missing value as an ERROR.
    std::string avsDialogRequestId;
    auto it = headerIt->value.FindMember(JSON_MESSAGE_DIALOG_REQUEST_ID_KEY);
    if (it != headerIt->value.MemberEnd()) {
        convertToValue(it->value, &avsDialogRequestId);
    } else {
        ACSDK_DEBUG(LX("receive").d("messageId", avsMessageId).m("No dialogRequestId attached to message."));
    }

    auto avsMessageHeader = std::make_shared<AVSMessageHeader>(avsNamespace, avsName, avsMessageId, avsDialogRequestId);
    std::shared_ptr<AVSDirective> avsDirective =
        AVSDirective::create(message, avsMessageHeader, payload, m_attachmentManager, contextId);
    if (!avsDirective) {
        const std::string errorDescription = "AVSDirective is nullptr, failed to send to DirectiveSequencer";
        ACSDK_ERROR(LX("receiveFailed").d("reason", "createAvsDirectiveFailed"));
        sendExceptionEncounteredHelper(m_exceptionEncounteredSender, message, errorDescription);
        return;
    }

    if (avsDirective->getName() == "StopCapture" || avsDirective->getName() == "Speak") {
        ACSDK_METRIC_MSG(TAG, avsDirective, Metrics::Location::ADSL_ENQUEUE);
    }

    m_directiveSequencer->onDirective(avsDirective);
}

void MessageInterpreter::sendParseValueException(const std::string& key, const std::string& json) {
    const std::string errorMessage = "reason=valueRetrievalFailed,key=" + key + ",payload=" + json;
    ACSDK_ERROR(LX("messageParsingFailed").m(errorMessage));
    sendExceptionEncounteredHelper(m_exceptionEncounteredSender, json, errorMessage);
}

}  // namespace adsl
}  // namespace alexaClientSDK
