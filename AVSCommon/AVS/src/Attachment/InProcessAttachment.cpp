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

#include "AVSCommon/AVS/Attachment/InProcessAttachment.h"
#include "AVSCommon/Utils/Memory/Memory.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

using namespace alexaClientSDK::avsCommon::utils::memory;

InProcessAttachment::InProcessAttachment(const std::string& id, std::unique_ptr<SDSType> sds) :
        Attachment(id),
        m_sds{std::move(sds)} {
    if (!m_sds) {
        auto buffSize = SDSType::calculateBufferSize(SDS_BUFFER_DEFAULT_SIZE_IN_BYTES);
        auto buff = std::make_shared<SDSBufferType>(buffSize);
        m_sds = SDSType::create(buff);
    }
}

std::unique_ptr<AttachmentWriter> InProcessAttachment::createWriter(
    InProcessAttachmentWriter::SDSTypeWriter::Policy policy) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_hasCreatedWriter) {
        return nullptr;
    }

    auto writer = InProcessAttachmentWriter::create(m_sds, policy);
    if (writer) {
        m_hasCreatedWriter = true;
    }

    return std::move(writer);
}

std::unique_ptr<AttachmentReader> InProcessAttachment::createReader(
    InProcessAttachmentReader::SDSTypeReader::Policy policy) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_hasCreatedReader) {
        return nullptr;
    }

    auto reader = InProcessAttachmentReader::create(policy, m_sds);
    if (reader) {
        m_hasCreatedReader = true;
    }

    return std::move(reader);
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
