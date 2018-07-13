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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTMANAGERINTERFACE_H_

#include <chrono>
#include <string>
#include <memory>

#include "AVSCommon/AVS/Attachment/Attachment.h"
#include "AVSCommon/AVS/Attachment/AttachmentReader.h"
#include "AVSCommon/AVS/Attachment/AttachmentWriter.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * This class allows the decoupling of attachment readers and writers from the management of attachments.
 *
 * This class is thread safe.
 */
class AttachmentManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AttachmentManagerInterface() = default;

    /**
     * Creates an attachmentId given two particular @c strings - the contextId and the contentId.
     * Generally, contextId allows disambiguation when two attachment contentIds are not guaranteed to be unique.
     * This function provides a consistent way for different parts of application code to combine contextId and
     * contentId into a single string.  Clearly, both the reader and writer of a given attachment need to call this
     * function with the same two strings.
     *
     * As an example of usage, if an application has several sources of attachments, for example two audio providers,
     * then one pair of contextId / contentId strings might be: { "AudioProvider1", "Attachment1" }.  If the other
     * audio provider creates an attachment, then the pair: { "AudioProvider2", "Attachment1" } allows the
     * contextId to disambiguate what happens to be identical contentIds.
     *
     * If this function is called with one or both strings being empty, then the combine will not be performed.
     * In the case of both strings being empty, an empty string will be returned.  If only one string is non-empty,
     * then that string will be returned.
     *
     * @param contextId The contextId, which generally reflects the source of the @c Attachment.
     * @param contentId The contentId, which is considered unique when paired with a particular contextId.
     * @return The combined strings, which may be then used as a single attachmentId, per the logic outlined above.
     */
    virtual std::string generateAttachmentId(const std::string& contextId, const std::string& contentId) const = 0;

    /**
     * Sets the timeout parameter which is used to ensure unused attachments are eventually cleaned up.  This
     * time is specified in minutes.  An unused attachment is defined as an attachment for which only a reader or
     * writer was created.  Such an Attachment is waiting to be either produced or consumed.
     *
     * If this function is not called, then the timeout is set to a default value specified by the implementation.
     *
     * The timeout cannot be set lower than an implementation specific minimum, since too low a timeout
     * could cause attachments to be removed before both reader and writer have had time to request it.
     *
     * @param timeoutMinutes The timeout, expressed in minutes.  If this is less than the minimum, then the
     * setting will not be updated, and the function will return false.
     * @return Whether the timeout was set ok.
     */
    virtual bool setAttachmentTimeoutMinutes(std::chrono::minutes timeoutMinutes) = 0;

    /**
     * Returns a pointer to an @c AttachmentWriter.
     * @note Calls to @c createReader and @c createWriter may occur in any order.
     *
     * @param attachmentId The id of the @c Attachment.
     * @param policy The WriterPolicy that the AttachmentWriter should adhere to.
     * @return An @c AttachmentWriter.
     */
    virtual std::unique_ptr<AttachmentWriter> createWriter(
        const std::string& attachmentId,
        utils::sds::WriterPolicy policy = avsCommon::utils::sds::WriterPolicy::ALL_OR_NOTHING) = 0;

    /**
     * Returns a pointer to an @c AttachmentReader.
     * @note Calls to @c createReader and @c createWriter may occur in any order.
     *
     * @param attachmentId The id of the @c Attachment.
     * @param policy The @c AttachmentReader policy, which determines the semantics of the @c AttachmentReader.read().
     * @return An @c AttachmentReader.
     */
    virtual std::unique_ptr<AttachmentReader> createReader(
        const std::string& attachmentId,
        utils::sds::ReaderPolicy policy) = 0;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTMANAGERINTERFACE_H_
