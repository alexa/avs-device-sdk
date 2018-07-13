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
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace avsCommon::utils;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("AvsDirective");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

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
