/*
 * AttachmentManagerInterface.h
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
#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ATTACHMENT_MANAGER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ATTACHMENT_MANAGER_INTERFACE_H_

#include <future>
#include <iostream>
#include <memory>
#include <string>

namespace alexaClientSDK {
namespace acl {

/// Interface class that manages how attachments are stored and retrieved.
class AttachmentManagerInterface {
public:
    /**
     * Create a future that stores an binary attachment. The attachment will become available once the writer has
     * finished writing the attachment and calls the @c createAttachment() function with the same attachment ID.
     *
     * This call must not block.
     *
     * @param attachmentId A unique ID that identifies an attachment.
     * @return The future that stores an attachment.
     */
    virtual std::future<std::shared_ptr<std::iostream>> createAttachmentReader(const std::string& attachmentId) = 0;

    /**
     * Create an attachment associated with the attachment ID.
     *
     * This call must not block.
     *
     * @param attachmentId A unique ID that identifies an attachment.
     * @param attachment The underlying binary attachment that needs to be stored and managed by
     *        an @c AttachmentManager.
     */
    virtual void createAttachment(const std::string& attachmentId, std::shared_ptr<std::iostream> attachment) = 0;

    /**
     * Remove the record of an attachment associated with the @c attachmentId.
     *
     * @param attachmentId The attachemntId.
     */
    virtual void releaseAttachment(const std::string& attachmentId) = 0;
};

} // namespace acl
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ATTACHMENT_MANAGER_INTERFACE_H_
