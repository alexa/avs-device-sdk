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

#include <iostream>
#include <memory>
#include <string>
#include <utility>

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
     * An enum to indicate the status of parsing an AVS Directive from a JSON string representation.
     */
    enum class ParseStatus {
        /// The parse was successful.
        SUCCESS,

        /// The parse failed due to invalid JSON formatting.
        ERROR_INVALID_JSON,

        /// The parse failed due to the directive key being missing.
        ERROR_MISSING_DIRECTIVE_KEY,

        /// The parse failed due to the header key being missing.
        ERROR_MISSING_HEADER_KEY,

        /// The parse failed due to the namespace key being missing.
        ERROR_MISSING_NAMESPACE_KEY,

        /// The parse failed due to the name key being missing.
        ERROR_MISSING_NAME_KEY,

        /// The parse failed due to the message id key being missing.
        ERROR_MISSING_MESSAGE_ID_KEY,

        /// The parse failed due to the message payload key being missing.
        ERROR_MISSING_PAYLOAD_KEY
    };

    /**
     * Creates an AVSDirective.
     *
     * @param unparsedDirective The unparsed AVS Directive JSON string.
     * @param attachmentManager The attachment manager.
     * @param attachmentContextId The contextId required to get attachments from the AttachmentManager.
     * @return A pair of an AVSDirective pointer and a parse status.  If the AVSDirective is nullptr, the status will
     * express the parse error.
     */
    static std::pair<std::unique_ptr<AVSDirective>, ParseStatus> create(
        const std::string& unparsedDirective,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
        const std::string& attachmentContextId);

    /**
     * Creates an AVSDirective.
     *
     * @param unparsedDirective The unparsed AVS Directive JSON string.
     * @param avsMessageHeader The header fields of the Directive.
     * @param payload The payload of the Directive.
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

/**
 * This function converts the provided @c ParseStatus to a string.
 *
 * @param status The @c ParseStatus to convert to a string.
 * @return The string conversion of the @c ParseStatus.
 */
inline std::string avsDirectiveParseStatusToString(AVSDirective::ParseStatus status) {
    switch (status) {
        case AVSDirective::ParseStatus::SUCCESS:
            return "SUCCESS";
        case AVSDirective::ParseStatus::ERROR_INVALID_JSON:
            return "ERROR_INVALID_JSON";
        case AVSDirective::ParseStatus::ERROR_MISSING_DIRECTIVE_KEY:
            return "ERROR_MISSING_DIRECTIVE_KEY";
        case AVSDirective::ParseStatus::ERROR_MISSING_HEADER_KEY:
            return "ERROR_MISSING_HEADER_KEY";
        case AVSDirective::ParseStatus::ERROR_MISSING_NAMESPACE_KEY:
            return "ERROR_MISSING_NAMESPACE_KEY";
        case AVSDirective::ParseStatus::ERROR_MISSING_NAME_KEY:
            return "ERROR_MISSING_NAME_KEY";
        case AVSDirective::ParseStatus::ERROR_MISSING_MESSAGE_ID_KEY:
            return "ERROR_MISSING_MESSAGE_ID_KEY";
        case AVSDirective::ParseStatus::ERROR_MISSING_PAYLOAD_KEY:
            return "ERROR_MISSING_PAYLOAD_KEY";
    }
    return "UNKNOWN_STATUS";
}

/**
 * Write a @c ParseStatus value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param status The @c ParseStatus value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AVSDirective::ParseStatus& status) {
    return stream << avsDirectiveParseStatusToString(status);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDIRECTIVE_H_
