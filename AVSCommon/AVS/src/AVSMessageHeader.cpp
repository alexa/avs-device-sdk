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

#include <sstream>

#include "AVSCommon/AVS/AVSMessageHeader.h"
#include "AVSCommon/AVS/EventBuilder.h"
#include "AVSCommon/Utils/JSON/JSONGenerator.h"
#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// The message id key in the header of the event
static const std::string MESSAGE_ID_KEY_STRING = "messageId";

/// The dialog request Id key in the header of the event.
static const std::string DIALOG_REQUEST_ID_KEY_STRING = "dialogRequestId";

/// The event correlation token key.
static const std::string EVENT_CORRELATION_KEY_STRING = "eventCorrelationToken";

/// The correlation token key.
static const std::string CORRELATION_TOKEN_KEY_STRING = "correlationToken";

/// The payload version key.
static const std::string PAYLOAD_VERSION_KEY_STRING = "payloadVersion";

/// The instance key.
static const std::string INSTANCE_KEY_STRING = "instance";

AVSMessageHeader::AVSMessageHeader(
    const std::string& avsNamespace,
    const std::string& avsName,
    const std::string& avsMessageId,
    const std::string& avsDialogRequestId,
    const std::string& correlationToken,
    const std::string& eventCorrelationToken,
    const std::string& payloadVersion,
    const std::string& instance) :
        m_namespace{avsNamespace},
        m_name{avsName},
        m_messageId{avsMessageId},
        m_dialogRequestId{avsDialogRequestId},
        m_correlationToken{correlationToken},
        m_eventCorrelationToken{eventCorrelationToken},
        m_payloadVersion{payloadVersion},
        m_instance{instance} {
}

AVSMessageHeader AVSMessageHeader::createAVSEventHeader(
    const std::string& avsNamespace,
    const std::string& avsName,
    const std::string& avsDialogRequestId,
    const std::string& correlationToken,
    const std::string& payloadVersion,
    const std::string& instance) {
    auto& newId = avsCommon::utils::uuidGeneration::generateUUID();
    return AVSMessageHeader(
        avsNamespace, avsName, newId, avsDialogRequestId, correlationToken, newId, payloadVersion, instance);
}

std::string AVSMessageHeader::getNamespace() const {
    return m_namespace;
}

std::string AVSMessageHeader::getName() const {
    return m_name;
}

std::string AVSMessageHeader::getMessageId() const {
    return m_messageId;
}

std::string AVSMessageHeader::getDialogRequestId() const {
    return m_dialogRequestId;
}

std::string AVSMessageHeader::getCorrelationToken() const {
    return m_correlationToken;
}

std::string AVSMessageHeader::getEventCorrelationToken() const {
    return m_eventCorrelationToken;
}

std::string AVSMessageHeader::getAsString() const {
    std::stringstream stream;
    stream << "namespace:" << m_namespace;
    stream << ",name:" << m_name;
    stream << ",messageId:" << m_messageId;
    stream << ",dialogRequestId:" << m_dialogRequestId;
    stream << ",correlationToken:" << m_correlationToken;
    stream << ",eventCorrelationToken:" << m_eventCorrelationToken;
    stream << ",payloadVersion:" << m_payloadVersion;
    stream << ",instance:" << m_instance;
    return stream.str();
}

std::string AVSMessageHeader::getPayloadVersion() const {
    return m_payloadVersion;
}

std::string AVSMessageHeader::getInstance() const {
    return m_instance;
}

std::string AVSMessageHeader::toJson() const {
    utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.addMember(constants::NAMESPACE_KEY_STRING, m_namespace);
    jsonGenerator.addMember(constants::NAME_KEY_STRING, m_name);
    jsonGenerator.addMember(MESSAGE_ID_KEY_STRING, m_messageId);

    if (!m_dialogRequestId.empty()) {
        jsonGenerator.addMember(DIALOG_REQUEST_ID_KEY_STRING, m_dialogRequestId);
    }

    if (!m_correlationToken.empty()) {
        jsonGenerator.addMember(CORRELATION_TOKEN_KEY_STRING, m_correlationToken);
    }

    if (!m_eventCorrelationToken.empty()) {
        jsonGenerator.addMember(EVENT_CORRELATION_KEY_STRING, m_eventCorrelationToken);
    }

    if (!m_payloadVersion.empty()) {
        jsonGenerator.addMember(PAYLOAD_VERSION_KEY_STRING, m_payloadVersion);
    }

    if (!m_instance.empty()) {
        jsonGenerator.addMember(INSTANCE_KEY_STRING, m_instance);
    }
    return jsonGenerator.toString();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
