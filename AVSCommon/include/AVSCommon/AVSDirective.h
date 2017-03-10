/*
 * AVSDirective.h
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_INCLUDE_AVS_COMMON_AVS_DIRECTIVE_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_INCLUDE_AVS_COMMON_AVS_DIRECTIVE_H_

#include <memory>
#include <string>

#include <ACL/Message.h>
#include <ACL/AttachmentManagerInterface.h>

#include "AVSCommon/AVSMessage.h"

namespace alexaClientSDK {
namespace avsCommon {

/**
 * A class representation of the AVS directive.
 */
class AVSDirective : public AVSMessage {
public:
    /**
     * Create an AVSDirective object with the given @c avsMessageHeader, @c payload and @c attachmentManager.
     *
     * @param unparsedDirective The unparsed directive JSON string from AVS.
     * @param avsMessageHeader The header fields of the directive.
     * @param payload The payload of the directive.
     * @param attachmentManager The attachment manager.
     * @return The created AVSDirective object or @c nullptr if creation failed.
     */
    static std::unique_ptr<AVSDirective> create(const std::string& unparsedDirective,
        std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        const std::string& payload,
        std::shared_ptr<acl::AttachmentManagerInterface> attachmentManager);

    /**
     * Returns a reader for the attachment associated with this directive.
     *
     * @param contentId The contentId associated with the attachment.
     * @return An attachment reader or @c nullptr if no attachment was found with the given @c contentId.
     */
    std::future<std::shared_ptr<std::iostream>> getAttachmentReader(const std::string& contentId) const;

    /**
     * Returns the underlying unparsed directive.
     */
    std::string getUnparsedDirective() const;

private:
    /**
     * Constructor.
     *
     * @param unparsedDirective The unparsed directive JSON string from AVS.
     * @param avsMessageHeader The object representation of an AVS message header.
     * @param payload The payload of an AVS message.
     * @param attachmentManager The attachment manager object.
     */
    AVSDirective(const std::string& unparsedDirective,
        std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        const std::string& payload,
        std::shared_ptr<acl::AttachmentManagerInterface> attachmentManager);

    /// The unparsed directive JSON string from AVS.
    const std::string m_unparsedDirective;
    /// Object knows how to find the attachment based on the attachmentId.
    std::shared_ptr<acl::AttachmentManagerInterface> m_attachmentManager;
};

} // namespace avsCommon
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_AVS_COMMON_INCLUDE_AVS_COMMON_AVS_DIRECTIVE_H_
