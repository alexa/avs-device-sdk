/*
 * AttachmentManager.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ATTACHMENT_MANAGER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ATTACHMENT_MANAGER_H_

#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>

#include "AttachmentManagerInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Class that manages how attachment is stored and retrieved.
 *
 * The class is thread safe.
 *
 * Attachments will be stored in attachment manager as pointers through function @c createAttachment, which will create
 * an attachmentId to attachment mapping in attachment manager. Before any new attachment is stored, the attachment
 * manager will scan through the attachments based on the chronological order when the attachment is stored, and release
 * those that has existed beyond the timeout limit. The attachment could be retrieved from the attachment manager
 * through @c createAttachmentReader function with the attachment ID.
 */
class AttachmentManager : public AttachmentManagerInterface {
public:
    /**
     * Constructor.
     *
     * @param timeout Time out in minutes to release the binary attachment from AVS. The default value is 720 minutes.
     *        The attachment with certain attachment ID will be release if there is no reader trying to get the
     *        attachment within the timeout limit.
     */
    AttachmentManager(std::chrono::minutes timeout = std::chrono::minutes(720));

    std::future<std::shared_ptr<std::iostream>> createAttachmentReader(const std::string& attachmentId) override;

    /**
     * @inheritDoc
     *
     * Before creating an new entry for the attachment, the previously stored attachment will be examined by the order
     * of the time stamp when they were created. All the expired attachments (has not been accessed within time out
     * limit) will be removed.
     */
    void createAttachment(const std::string& attachmentId, std::shared_ptr<std::iostream> attachment) override;

    void releaseAttachment(const std::string& attachmentId) override;

private:
    /**
     * Helper function that checks if there is already an attachment created with the @c attachmentId. If not, then
     * create one, otherwise do nothing. The function should only be used when the mutex is already held by the caller.
     *
     * @param attachmentId The attachment ID associated with the attachment.
     */
    void createAttachmentPromiseHelperLocked(const std::string& attachmentId);

    /// Maps an attachment ID to the attachment that can be retrieved in the future.
    std::map<std::string, std::promise<std::shared_ptr<std::iostream>>> m_attachments;
    /// Maps the time stamp to each attachment.
    std::multimap<std::chrono::steady_clock::time_point, std::string> m_timeStamps;
    /// Mutex used to serialize access to @c m_attachments and @c m_timeStamps.
    std::mutex m_mutex;
    /// Time out to release the attachment if there are no reader/writer to access it.
    std::chrono::minutes m_timeout;
};

} // namespace acl
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ATTACHMENT_MANAGER_H_
