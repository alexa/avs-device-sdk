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

#include <vector>

#include "AVSCommon/AVS/Attachment/InProcessAttachment.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Memory/Memory.h"

#include "AVSCommon/AVS/Attachment/AttachmentManager.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::memory;

/// String to identify log entries originating from this file.
static const std::string TAG("AttachmentManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// The definition for these two static class members.
constexpr std::chrono::minutes AttachmentManager::ATTACHMENT_MANAGER_TIMOUT_MINUTES_DEFAULT;
constexpr std::chrono::minutes AttachmentManager::ATTACHMENT_MANAGER_TIMOUT_MINUTES_MINIMUM;

// Used within generateAttachmentId().
static const std::string ATTACHMENT_ID_COMBINING_SUBSTRING = ":";

AttachmentManager::AttachmentManagementDetails::AttachmentManagementDetails() :
        creationTime{std::chrono::steady_clock::now()} {
}

AttachmentManager::AttachmentManager(AttachmentType attachmentType) :
        m_attachmentType{attachmentType},
        m_attachmentExpirationMinutes{ATTACHMENT_MANAGER_TIMOUT_MINUTES_DEFAULT} {
}

std::string AttachmentManager::generateAttachmentId(const std::string& contextId, const std::string& contentId) const {
    if (contextId.empty() && contentId.empty()) {
        ACSDK_ERROR(LX("generateAttachmentIdFailed")
                        .d("reason", "contextId and contentId are empty")
                        .d("result", "empty string"));
        return "";
    }
    if (contextId.empty()) {
        ACSDK_WARN(LX("generateAttachmentIdWarning").d("reason", "contextId is empty").d("result", "contentId"));
        return contentId;
    }
    if (contentId.empty()) {
        ACSDK_WARN(LX("generateAttachmentIdWarning").d("reason", "contentId is empty").d("result", "contextId"));
        return contextId;
    }

    return contextId + ATTACHMENT_ID_COMBINING_SUBSTRING + contentId;
}

bool AttachmentManager::setAttachmentTimeoutMinutes(std::chrono::minutes minutes) {
    if (minutes < ATTACHMENT_MANAGER_TIMOUT_MINUTES_MINIMUM) {
        int minimumMinutes = ATTACHMENT_MANAGER_TIMOUT_MINUTES_MINIMUM.count();
        std::string minutePrintString = (1 == minimumMinutes) ? " minute" : " minutes";
        ACSDK_ERROR(LX("setAttachmentTimeoutError")
                        .d("reason", "timeout parameter less than minimum value")
                        .d("attemptedSetting", std::to_string(minimumMinutes) + minutePrintString));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_attachmentExpirationMinutes = minutes;
    return true;
}

AttachmentManager::AttachmentManagementDetails& AttachmentManager::getDetailsLocked(const std::string& attachmentId) {
    // This call ensures the details object exists, whether updated previously, or as a new object.
    auto& details = m_attachmentDetailsMap[attachmentId];

    // If it's a new object, the inner attachment has not yet been created.  Let's go do that.
    if (!details.attachment) {
        // Lack of default case will allow compiler to generate warnings if a case is unhandled.
        switch (m_attachmentType) {
            // The in-process attachment type.
            case AttachmentType::IN_PROCESS:
                details.attachment = make_unique<InProcessAttachment>(attachmentId);
                break;
        }

        // In code compiled with no warnings, the following test should never pass.
        // Still, let's make sure the right thing happens if it does.  Also, assign nullptr's so that application code
        // can detect the error and react accordingly.
        if (!details.attachment) {
            ACSDK_ERROR(LX("getDetailsLockedError").d("reason", "Unsupported attachment type"));
        }
    }

    return details;
}

std::unique_ptr<AttachmentWriter> AttachmentManager::createWriter(
    const std::string& attachmentId,
    utils::sds::WriterPolicy policy) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& details = getDetailsLocked(attachmentId);
    if (!details.attachment) {
        ACSDK_ERROR(LX("createWriterFailed").d("reason", "Could not access attachment"));
        return nullptr;
    }

    auto writer = details.attachment->createWriter(policy);
    removeExpiredAttachmentsLocked();
    return writer;
}

std::unique_ptr<AttachmentReader> AttachmentManager::createReader(
    const std::string& attachmentId,
    sds::ReaderPolicy policy) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& details = getDetailsLocked(attachmentId);
    if (!details.attachment) {
        ACSDK_ERROR(LX("createWriterFailed").d("reason", "Could not access attachment"));
        return nullptr;
    }

    auto reader = details.attachment->createReader(policy);
    removeExpiredAttachmentsLocked();
    return reader;
}

void AttachmentManager::removeExpiredAttachmentsLocked() {
    std::vector<std::string> idsToErase;
    auto now = std::chrono::steady_clock::now();

    for (auto& iter : m_attachmentDetailsMap) {
        auto& details = iter.second;

        /*
         * Our criteria for releasing an AttachmentManagementDetails object - either:
         *  - Both futures have been returned, which means the attachment now has a reader and writer.  Great!
         *  - Only the reader or writer future was returned, and the attachment has exceeded its lifetime limit.
         */

        auto attachmentLifetime = std::chrono::duration_cast<std::chrono::minutes>(now - details.creationTime);

        if ((details.attachment->hasCreatedReader() && details.attachment->hasCreatedWriter()) ||
            attachmentLifetime > m_attachmentExpirationMinutes) {
            idsToErase.push_back(iter.first);
        }
    }

    for (auto id : idsToErase) {
        m_attachmentDetailsMap.erase(id);
    }
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
