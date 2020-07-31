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
/// JSON key to get the optional instance object of a message.
static const std::string JSON_MESSAGE_INSTANCE_KEY = "instance";
/// JSON key to get the payload version object of a message.
static const std::string JSON_MESSAGE_PAYLOAD_VERSION_KEY = "payloadVersion";
/// JSON key to get the correlationToken value of a header.
static const std::string JSON_CORRELATION_TOKEN_KEY = "correlationToken";
/// JSON key to get the eventCorrelationToken value of the header.
static const std::string JSON_EVENT_CORRELATION_TOKEN_KEY = "eventCorrelationToken";
/// JSON key to get the endpoint object of a message.
static const std::string JSON_ENDPOINT_KEY = "endpoint";
/// JSON key to get the endpointId value of the endpoint.
static const std::string JSON_ENDPOINT_ID_KEY = "endpointId";
/// JSON key to get the cookie value for the endpoint.
static const std::string JSON_ENDPOINT_COOKIE_KEY = "cookie";

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

    std::string instance;
    if (retrieveValue(headerIt->value, JSON_MESSAGE_INSTANCE_KEY, &instance)) {
        ACSDK_DEBUG5(LX(__func__).d(JSON_MESSAGE_INSTANCE_KEY, instance));
    }

    std::string payloadVersion;
    if (retrieveValue(headerIt->value, JSON_MESSAGE_PAYLOAD_VERSION_KEY, &payloadVersion)) {
        ACSDK_DEBUG5(LX(__func__).d(JSON_MESSAGE_PAYLOAD_VERSION_KEY, payloadVersion));
    }

    std::string correlationToken;
    if (retrieveValue(headerIt->value, JSON_CORRELATION_TOKEN_KEY, &correlationToken)) {
        ACSDK_DEBUG5(LX(__func__).d(JSON_CORRELATION_TOKEN_KEY, correlationToken));
    }

    std::string eventCorrelationToken;
    if (retrieveValue(headerIt->value, JSON_EVENT_CORRELATION_TOKEN_KEY, &eventCorrelationToken)) {
        ACSDK_DEBUG5(LX(__func__).d(JSON_EVENT_CORRELATION_TOKEN_KEY, eventCorrelationToken));
    }

    return std::make_shared<AVSMessageHeader>(
        avsNamespace,
        avsName,
        avsMessageId,
        avsDialogRequestId,
        correlationToken,
        eventCorrelationToken,
        payloadVersion,
        instance);
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

/**
 * Utility function to parse the endpoint attributes from a rapidjson document structure.
 *
 * @param document The constructed document tree
 * @return The endpoint attribute content if it is available; otherwise, and empty object.
 */
static utils::Optional<AVSMessageEndpoint> parseEndpoint(const Document& document) {
    Value::ConstMemberIterator directiveIt;
    if (!findNode(document, JSON_MESSAGE_DIRECTIVE_KEY, &directiveIt)) {
        ACSDK_ERROR(LX("parseEndpointFailed").d("reason", "noDirectiveKey"));
        return utils::Optional<AVSMessageEndpoint>();
    }

    Value::ConstMemberIterator endpointIt;
    if (!findNode(directiveIt->value, JSON_ENDPOINT_KEY, &endpointIt)) {
        ACSDK_DEBUG0(LX(__func__).m("noEndpoint"));
        return utils::Optional<AVSMessageEndpoint>();
    }

    std::string endpointId;
    if (!retrieveValue(endpointIt->value, JSON_ENDPOINT_ID_KEY, &endpointId)) {
        ACSDK_ERROR(LX(__func__).m("noEndpointId"));
        return utils::Optional<AVSMessageEndpoint>();
    }

    AVSMessageEndpoint messageEndpoint{endpointId};
    messageEndpoint.cookies = retrieveStringMap(endpointIt->value, JSON_ENDPOINT_COOKIE_KEY);

    ACSDK_DEBUG5(LX(__func__).sensitive("endpointId", endpointId));
    return utils::Optional<AVSMessageEndpoint>(messageEndpoint);
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

    auto endpoint = parseEndpoint(document);

    result.first = std::unique_ptr<AVSDirective>(
        new AVSDirective(unparsedDirective, header, payload, attachmentManager, attachmentContextId, endpoint));

    return result;
}

std::unique_ptr<AVSDirective> AVSDirective::create(
    const std::string& unparsedDirective,
    const std::shared_ptr<AVSMessageHeader> avsMessageHeader,
    const std::string& payload,
    const std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    const std::string& attachmentContextId,
    const utils::Optional<AVSMessageEndpoint>& endpoint) {
    if (!avsMessageHeader) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageHeader"));
        return nullptr;
    }
    if (!attachmentManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAttachmentManager"));
        return nullptr;
    }
    return std::unique_ptr<AVSDirective>(new AVSDirective(
        unparsedDirective, avsMessageHeader, payload, attachmentManager, attachmentContextId, endpoint));
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
    const std::string& attachmentContextId,
    const utils::Optional<AVSMessageEndpoint>& endpoint) :
        AVSMessage{avsMessageHeader, payload, endpoint},
        m_unparsedDirective{unparsedDirective},
        m_attachmentManager{attachmentManager},
        m_attachmentContextId{attachmentContextId} {
}

std::string AVSDirective::getUnparsedDirective() const {
    return m_unparsedDirective;
}

std::string AVSDirective::getAttachmentContextId() const {
    return m_attachmentContextId;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
