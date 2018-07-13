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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDIRECTIVE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDIRECTIVE_H_

#include <memory>
#include <string>

#include "Attachment/AttachmentManagerInterface.h"
#include "AVSMessage.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

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
     * @param attachmentContextId The contextId required to get attachments from the AttachmentManager.
     * @return The created AVSDirective object or @c nullptr if creation failed.
     */
    static std::unique_ptr<AVSDirective> create(
        const std::string& unparsedDirective,
        std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        const std::string& payload,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
        const std::string& attachmentContextId);

    /**
     * Returns a reader for the attachment associated with this directive.
     *
     * @param contentId The contentId associated with the attachment.
     * @param readerPolicy The policy with which to create the @c AttachmentReader.
     * @return An attachment reader or @c nullptr if no attachment was found with the given @c contentId.
     */
    std::unique_ptr<avsCommon::avs::attachment::AttachmentReader> getAttachmentReader(
        const std::string& contentId,
        utils::sds::ReaderPolicy readerPolicy) const;

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
     * @param attachmentContextId The contextId required to get attachments from the AttachmentManager.
     */
    AVSDirective(
        const std::string& unparsedDirective,
        std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        const std::string& payload,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
        const std::string& attachmentContextId);

    /// The unparsed directive JSON string from AVS.
    const std::string m_unparsedDirective;
    /// The attachmentManager.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> m_attachmentManager;
    /// The contextId needed to acquire the right attachment from the attachmentManager.
    std::string m_attachmentContextId;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDIRECTIVE_H_
