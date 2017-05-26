/*
 * MessageInterpreter.cpp
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

#include "ADSL/MessageInterpreter.h"

#include <AVSCommon/JSON/JSONUtils.h>
#include <AVSCommon/AVSMessageHeader.h>
#include <AVSCommon/AVSDirective.h>

#include <AVSUtils/Logging/Logger.h>

namespace alexaClientSDK {
namespace adsl {

using namespace avsCommon;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsUtils;
using namespace acl;

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
    exceptionEncounteredSender->sendExceptionEncountered(unparsedMessage,
        avsCommon::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
        errorDescription);
}

MessageInterpreter::MessageInterpreter(
        std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<AttachmentManagerInterface> attachmentManager):
        m_exceptionEncounteredSender{exceptionEncounteredSender},
        m_directiveSequencer{directiveSequencer}, m_attachmentManager{attachmentManager} {
}

void MessageInterpreter::receive(const std::string & contextId, const std::string & message) {

    std::string directiveBody;

    if (!lookupJsonValueHelper(message, message, JSON_MESSAGE_DIRECTIVE_KEY, &directiveBody)) {
        return;
    }

    std::string header;
    if (!lookupJsonValueHelper(message, directiveBody, JSON_MESSAGE_HEADER_KEY, &header)) {
        return;
    }

    std::string payload;
    if (!lookupJsonValueHelper(message, directiveBody, JSON_MESSAGE_PAYLOAD_KEY, &payload)) {
        return;
    }

    std::string avsNamespace;
    if (!lookupJsonValueHelper(message, header, JSON_MESSAGE_NAMESPACE_KEY, &avsNamespace)) {
        return;
    }

    std::string avsName;
    if (!lookupJsonValueHelper(message, header, JSON_MESSAGE_NAME_KEY, &avsName)) {
        return;
    }

    std::string avsMessageId;
    if (!lookupJsonValueHelper(message, header, JSON_MESSAGE_ID_KEY, &avsMessageId)) {
        return;
    }

    // This is an optional header field - it's ok if it is not present.
    std::string avsDialogRequestId;
    jsonUtils::lookupStringValue(header, JSON_MESSAGE_DIALOG_REQUEST_ID_KEY, &avsDialogRequestId);

    auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>(avsNamespace, avsName, avsMessageId, avsDialogRequestId);
    std::shared_ptr<AVSDirective> avsDirective = AVSDirective::create(message, avsMessageHeader,
                                                         payload, m_attachmentManager, contextId);
    if (!avsDirective) {
        const std::string errorDescription = "AVSDirective is nullptr, failed to send to DirectiveSequencer";
        Logger::log(errorDescription);
        sendExceptionEncounteredHelper(m_exceptionEncounteredSender, message, errorDescription);
        return;
    }
    m_directiveSequencer->onDirective(avsDirective);
}

bool MessageInterpreter::lookupJsonValueHelper(const std::string & avsMessage,
        const std::string& jsonMessageHeader,
        const std::string& lookupKey,
        std::string* outputValue) {
    if (!jsonUtils::lookupStringValue(jsonMessageHeader, lookupKey, outputValue)) {
        const std::string errorDescription = "Could not look up key from AVS JSON directive string: " + lookupKey;
        Logger::log(errorDescription);
        sendExceptionEncounteredHelper(m_exceptionEncounteredSender, avsMessage, errorDescription);
        return false;
    }
    return true;
}

} // namespace directiveSequencer
} // namespace alexaClientSDK
