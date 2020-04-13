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

#include "AVSCommon/AVS/AVSMessage.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

AVSMessage::AVSMessage(
    std::shared_ptr<AVSMessageHeader> avsMessageHeader,
    std::string payload,
    const utils::Optional<AVSMessageEndpoint>& endpoint) :
        m_header{avsMessageHeader},
        m_payload{std::move(payload)},
        m_endpoint{endpoint} {
}

std::string AVSMessage::getNamespace() const {
    return m_header->getNamespace();
}

std::string AVSMessage::getName() const {
    return m_header->getName();
}

std::string AVSMessage::getMessageId() const {
    return m_header->getMessageId();
}

std::string AVSMessage::getCorrelationToken() const {
    return m_header->getCorrelationToken();
}

std::string AVSMessage::getEventCorrelationToken() const {
    return m_header->getEventCorrelationToken();
}

std::string AVSMessage::getPayloadVersion() const {
    return m_header->getPayloadVersion();
}

std::string AVSMessage::getInstance() const {
    return m_header->getInstance();
}

std::string AVSMessage::getDialogRequestId() const {
    return m_header->getDialogRequestId();
}

std::string AVSMessage::getPayload() const {
    return m_payload;
}

std::shared_ptr<const AVSMessageHeader> AVSMessage::getHeader() const {
    return m_header;
}

std::string AVSMessage::getHeaderAsString() const {
    return m_header->getAsString();
}

utils::Optional<AVSMessageEndpoint> AVSMessage::getEndpoint() const {
    return m_endpoint;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
