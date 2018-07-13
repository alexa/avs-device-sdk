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

#include "Integration/TestExceptionEncounteredSender.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace avsCommon::utils::json;

/// JSON key to get the event object of a message.
static const std::string JSON_MESSAGE_EVENT_KEY = "event";
/// JSON key to get the directive object of a message.
static const std::string JSON_MESSAGE_DIRECTIVE_KEY = "directive";
/// JSON key to get the header object of a message.
static const std::string JSON_MESSAGE_HEADER_KEY = "header";
/// JSON key to get the namespace value of a header.
static const std::string JSON_MESSAGE_NAMESPACE_KEY = "namespace";
/// JSON key to get the name value of a header.
static const std::string JSON_MESSAGE_NAME_KEY = "name";
/// JSON key to get the messageId value of a header.
static const std::string JSON_MESSAGE_MESSAGE_ID_KEY = "messageId";
/// JSON key to get the dialogRequestId value of a header.
static const std::string JSON_MESSAGE_DIALOG_REQUEST_ID_KEY = "dialogRequestId";
/// JSON key to get the payload object of a message.
static const std::string JSON_MESSAGE_PAYLOAD_KEY = "payload";

void TestExceptionEncounteredSender::sendExceptionEncountered(
    const std::string& unparsedDirective,
    avs::ExceptionErrorType error,
    const std::string& message) {
    std::unique_lock<std::mutex> lock(m_mutex);
    ExceptionParams dp;
    dp.type = ExceptionParams::Type::EXCEPTION;
    dp.directive = parseDirective(
        unparsedDirective,
        std::make_shared<avsCommon::avs::attachment::AttachmentManager>(
            avsCommon::avs::attachment::AttachmentManager::AttachmentType::IN_PROCESS));
    dp.exceptionUnparsedDirective = unparsedDirective;
    dp.exceptionError = error;
    dp.exceptionMessage = message;
    m_queue.push_back(dp);
    m_wakeTrigger.notify_all();
}

std::shared_ptr<avsCommon::avs::AVSDirective> TestExceptionEncounteredSender::parseDirective(
    const std::string& rawJSON,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager) {
    std::string directiveJSON;
    std::string headerJSON;
    std::string payloadJSON;
    std::string nameSpace;
    std::string name;
    std::string messageId;
    std::string dialogRequestId;

    if (!jsonUtils::retrieveValue(rawJSON, JSON_MESSAGE_DIRECTIVE_KEY, &directiveJSON) ||
        !jsonUtils::retrieveValue(directiveJSON, JSON_MESSAGE_HEADER_KEY, &headerJSON) ||
        !jsonUtils::retrieveValue(directiveJSON, JSON_MESSAGE_PAYLOAD_KEY, &payloadJSON) ||
        !jsonUtils::retrieveValue(headerJSON, JSON_MESSAGE_NAMESPACE_KEY, &nameSpace) ||
        !jsonUtils::retrieveValue(headerJSON, JSON_MESSAGE_NAME_KEY, &name) ||
        !jsonUtils::retrieveValue(headerJSON, JSON_MESSAGE_MESSAGE_ID_KEY, &messageId)) {
        return nullptr;
    }

    jsonUtils::retrieveValue(headerJSON, JSON_MESSAGE_NAMESPACE_KEY, &dialogRequestId);

    auto header = std::make_shared<avsCommon::avs::AVSMessageHeader>(nameSpace, name, messageId, dialogRequestId);
    return avsCommon::avs::AVSDirective::create(rawJSON, header, payloadJSON, attachmentManager, "");
}

TestExceptionEncounteredSender::ExceptionParams TestExceptionEncounteredSender::waitForNext(
    const std::chrono::seconds duration) {
    ExceptionParams ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        ret.type = ExceptionParams::Type::TIMEOUT;
        return ret;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

TestExceptionEncounteredSender::ExceptionParams::ExceptionParams() : type{Type::UNSET} {
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
