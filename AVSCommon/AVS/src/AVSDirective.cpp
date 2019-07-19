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

#include "AVSCommon/AVS/AVSDirective.h"
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include "AVSCommon/Utils/Logger/Logger.h"

#include <rapidjson/document.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json::jsonUtils;

using namespace rapidjson;

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

/// String to identify log entries originating from this file.
static const std::string TAG("AvsDirective");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Utility function to attempt a JSON parse of the passed in string, and inform the caller if this succeeded or failed.
 *
 * @param unparsedDirective The unparsed JSON string which should represent an AVS Directive.
 * @param[out] document The rapidjson Document which will be set to the parsed structure if the parse succeeds.
 * @return Whether the parse was successful.
 */
static bool parseDocument(const std::string& unparsedDirective, Document* document) {
    if (!document) {
        ACSDK_ERROR(LX("parseDocumentFailed").m("nullptr document"));
        return false;
    }

    if (!parseJSON(unparsedDirective, document)) {
        ACSDK_ERROR(LX("parseDocumentFailed").d("uparsedDirective", unparsedDirective));
        return false;
    }

    return true;
}

/**
 * Utility function to parse the header from a rapidjson document structure.
 *
 * @param document The constructed document tree
 * @param [out] parseStatus An out parameter to express if the parse was successful
 * @return A pointer to an AVSMessageHeader object, or nullptr if the parse fails.
 */
static std::shared_ptr<AVSMessageHeader> parseHeader(const Document& document, AVSDirective::ParseStatus* parseStatus) {
    if (!parseStatus) {
        ACSDK_ERROR(LX("parseHeaderFailed").m("nullptr parseStatus"));
        return nullptr;
    }

    *parseStatus = AVSDirective::ParseStatus::SUCCESS;

    // Get iterators to child nodes.
    Value::ConstMemberIterator directiveIt;
    if (!findNode(document, JSON_MESSAGE_DIRECTIVE_KEY, &directiveIt)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_DIRECTIVE_KEY;
        return nullptr;
    }

    Value::ConstMemberIterator headerIt;
    if (!findNode(directiveIt->value, JSON_MESSAGE_HEADER_KEY, &headerIt)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_HEADER_KEY;
        return nullptr;
    }

    // Now, extract values.
    std::string avsNamespace;
    if (!retrieveValue(headerIt->value, JSON_MESSAGE_NAMESPACE_KEY, &avsNamespace)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_NAMESPACE_KEY;
        return nullptr;
    }

    std::string avsName;
    if (!retrieveValue(headerIt->value, JSON_MESSAGE_NAME_KEY, &avsName)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_NAME_KEY;
        return nullptr;
    }

    std::string avsMessageId;
    if (!retrieveValue(headerIt->value, JSON_MESSAGE_ID_KEY, &avsMessageId)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_MESSAGE_ID_KEY;
        return nullptr;
    }

    // This is an optional header field - it's ok if it is not present.
    // Avoid jsonUtils::retrieveValue because it logs a missing value as an ERROR.
    std::string avsDialogRequestId;
    auto it = headerIt->value.FindMember(JSON_MESSAGE_DIALOG_REQUEST_ID_KEY);
    if (it != headerIt->value.MemberEnd()) {
        convertToValue(it->value, &avsDialogRequestId);
    }

    return std::make_shared<AVSMessageHeader>(avsNamespace, avsName, avsMessageId, avsDialogRequestId);
}

/**
 * Utility function to parse the payload from a rapidjson document structure.
 *
 * @param document The constructed document tree
 * @param [out] parseStatus An out parameter to express if the parse was successful
 * @return The payload content if it is available.
 */
static std::string parsePayload(const Document& document, AVSDirective::ParseStatus* parseStatus) {
    if (!parseStatus) {
        ACSDK_ERROR(LX("parsePayloadFailed").m("nullptr parseStatus"));
        return "";
    }

    Value::ConstMemberIterator directiveIt;
    if (!findNode(document, JSON_MESSAGE_DIRECTIVE_KEY, &directiveIt)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_DIRECTIVE_KEY;
        return "";
    }

    std::string payload;
    if (!retrieveValue(directiveIt->value, JSON_MESSAGE_PAYLOAD_KEY, &payload)) {
        *parseStatus = AVSDirective::ParseStatus::ERROR_MISSING_PAYLOAD_KEY;
        return "";
    }

    *parseStatus = AVSDirective::ParseStatus::SUCCESS;
    return payload;
}

std::pair<std::unique_ptr<AVSDirective>, AVSDirective::ParseStatus> AVSDirective::create(
    const std::string& unparsedDirective,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    const std::string& attachmentContextId) {
    std::pair<std::unique_ptr<AVSDirective>, ParseStatus> result;
    result.second = ParseStatus::SUCCESS;

    Document document;
    if (!parseDocument(unparsedDirective, &document)) {
        ACSDK_ERROR(LX("createFailed").m("failed to parse JSON"));
        result.second = ParseStatus::ERROR_INVALID_JSON;
        return result;
    }

    auto header = parseHeader(document, &(result.second));
    if (ParseStatus::SUCCESS != result.second) {
        ACSDK_ERROR(LX("createFailed").m("failed to parse header"));
        return result;
    }

    auto payload = parsePayload(document, &(result.second));
    if (ParseStatus::SUCCESS != result.second) {
        ACSDK_ERROR(LX("createFailed").m("failed to parse payload"));
        return result;
    }

    result.first = std::unique_ptr<AVSDirective>(
        new AVSDirective(unparsedDirective, header, payload, attachmentManager, attachmentContextId));

    return result;
}

std::unique_ptr<AVSDirective> AVSDirective::create(
    const std::string& unparsedDirective,
    std::shared_ptr<AVSMessageHeader> avsMessageHeader,
    const std::string& payload,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    const std::string& attachmentContextId) {
    if (!avsMessageHeader) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageHeader"));
        return nullptr;
    }
    if (!attachmentManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAttachmentManager"));
        return nullptr;
    }
    return std::unique_ptr<AVSDirective>(
        new AVSDirective(unparsedDirective, avsMessageHeader, payload, attachmentManager, attachmentContextId));
}

std::unique_ptr<AttachmentReader> AVSDirective::getAttachmentReader(
    const std::string& contentId,
    sds::ReaderPolicy readerPolicy) const {
    auto attachmentId = m_attachmentManager->generateAttachmentId(m_attachmentContextId, contentId);
    return m_attachmentManager->createReader(attachmentId, readerPolicy);
}

AVSDirective::AVSDirective(
    const std::string& unparsedDirective,
    std::shared_ptr<AVSMessageHeader> avsMessageHeader,
    const std::string& payload,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    const std::string& attachmentContextId) :
        AVSMessage{avsMessageHeader, payload},
        m_unparsedDirective{unparsedDirective},
        m_attachmentManager{attachmentManager},
        m_attachmentContextId{attachmentContextId} {
}

std::string AVSDirective::getUnparsedDirective() const {
    return m_unparsedDirective;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
