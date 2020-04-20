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

#include "AVSCommon/AVS/Attachment/DefaultAttachmentReader.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentReader.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

std::unique_ptr<InProcessAttachmentReader> InProcessAttachmentReader::create(
    SDSTypeReader::Policy policy,
    std::shared_ptr<SDSType> sds,
    SDSTypeIndex offset,
    SDSTypeReader::Reference reference,
    bool resetOnOverrun) {
    auto readerImpl =
        DefaultAttachmentReader<SDSType>::create(policy, std::move(sds), offset, reference, resetOnOverrun);
    if (!readerImpl) {
        return nullptr;
    }
    return std::unique_ptr<InProcessAttachmentReader>(new InProcessAttachmentReader(std::move(readerImpl)));
}

InProcessAttachmentReader::InProcessAttachmentReader(std::unique_ptr<AttachmentReader> reader) :
        m_delegate(std::move(reader)) {
}

std::size_t InProcessAttachmentReader::read(
    void* buf,
    std::size_t numBytes,
    ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
    return m_delegate->read(buf, numBytes, readStatus, timeoutMs);
}

void InProcessAttachmentReader::close(ClosePoint closePoint) {
    m_delegate->close(closePoint);
}

bool InProcessAttachmentReader::seek(uint64_t offset) {
    return m_delegate->seek(offset);
}

uint64_t InProcessAttachmentReader::getNumUnreadBytes() {
    return m_delegate->getNumUnreadBytes();
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
