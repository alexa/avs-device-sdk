/*
 * AVSDirective.cpp
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

#include "AVSCommon/AVSDirective.h"

#include <AVSUtils/Logging/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {

using namespace avsUtils;

std::unique_ptr<AVSDirective> AVSDirective::create(
        const std::string& unparsedDirective,
        std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        const std::string& payload,
        std::shared_ptr<acl::AttachmentManagerInterface> attachmentManager) {
    if (!avsMessageHeader) {
        Logger::log("AVSDirective::create - message header was nullptr.");
        return nullptr;
    }

    if (!attachmentManager) {
        Logger::log("AVSDirective::create - attachmentManager was nullptr.");
        return nullptr;
    }
    return std::unique_ptr<AVSDirective>(new AVSDirective(unparsedDirective, avsMessageHeader, payload,
        attachmentManager));
}

std::future<std::shared_ptr<std::iostream>> AVSDirective::getAttachmentReader(const std::string& contentId) const {
    return m_attachmentManager->createAttachmentReader(contentId);
}

AVSDirective::AVSDirective(const std::string& unparsedDirective, std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        const std::string& payload, std::shared_ptr<acl::AttachmentManagerInterface> attachmentManager) :
            AVSMessage{avsMessageHeader, payload},
            m_unparsedDirective{unparsedDirective},
            m_attachmentManager{attachmentManager} {
}

std::string AVSDirective::getUnparsedDirective() const {
    return m_unparsedDirective;
}

} // namespace alexaClientSDK
} // namespace avsCommon
