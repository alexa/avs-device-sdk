/*
 * AttachmentManager.cpp
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

#include <AVSUtils/Logging/Logger.h>
#include "ACL/AttachmentManager.h"
#include "AVSUtils/Memory/Memory.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsUtils;

AttachmentManager::AttachmentManager(std::chrono::minutes timeout): m_timeout(timeout) {
}

std::future<std::shared_ptr<std::iostream>> AttachmentManager::createAttachmentReader(const std::string& attachmentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    createAttachmentPromiseHelperLocked(attachmentId);
    return m_attachments[attachmentId].get_future();
}

void AttachmentManager::createAttachment(const std::string& attachmentId, std::shared_ptr<std::iostream> attachment) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // TODO: [ACSDK-210] Move the clean AttachmentId logic into acl::MimeParser;
    // If the attachmentId is empty, then we'll get "<>" with length of 2. So to be a valid attachmentId, it needs have
    // length larger than 2.
    if (attachmentId.size() <= 2) {
        avsUtils::Logger::log("The attachmentId is not valid, attachment can't be created.");
        return;
    }

    std::string cleanAttachmentId = attachmentId;
    if ( ('<' == attachmentId[0]) && ('>' == attachmentId[attachmentId.size() - 1]) ) {
        cleanAttachmentId = attachmentId.substr(1, attachmentId.size() - 2);
    }

    createAttachmentPromiseHelperLocked(cleanAttachmentId);
    auto currentTime = std::chrono::steady_clock::now();
    m_timeStamps.insert(std::make_pair(currentTime, cleanAttachmentId));

    for (auto iter = m_timeStamps.begin(); iter != m_timeStamps.end();) {
        auto timeGap = std::chrono::duration_cast<std::chrono::minutes>(currentTime - iter->first);
        if (timeGap >= m_timeout) {
            m_attachments.erase(iter->second);
            iter = m_timeStamps.erase(iter);
        } else {
            // The time stamp is sorted in map from the oldest to the latest. So once we find the lifespan of the
            // attachment is within the timeout, we can just break out the loop.
            break;
        }
    }
    // Add this check in case the m_timeout value is 0.
    if (m_attachments.find(cleanAttachmentId) != m_attachments.end()) {
        m_attachments[cleanAttachmentId].set_value(attachment);
    }
}

void AttachmentManager::releaseAttachment(const std::string& attachmentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_attachments.erase(attachmentId);
}

void AttachmentManager::createAttachmentPromiseHelperLocked(const std::string& attachmentId) {
    if (m_attachments.find(attachmentId) == m_attachments.end()) {
        m_attachments[attachmentId] = std::promise<std::shared_ptr<std::iostream>>();
    }
}

} // namespace acl
} // namespace alexaClientSDK
