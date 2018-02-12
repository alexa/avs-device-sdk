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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTMANAGER_H_

#include <mutex>
#include <unordered_map>

#include "AVSCommon/AVS/Attachment/AttachmentManagerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * This class allows the decoupling of attachment readers and writers from the management of attachments.
 *
 * This class is thread safe.
 *
 * A design principle of the @c Attachment class is that each attachment will have at most one reader and writer.
 *
 * Application code may query the manager for a reader and writer object at any time, and in any order.
 *
 * @note Resource management is currently implemented by a timeout approach.  This does have the following limitations:
 *
 *  @li An AttachmentReader or AttachmentWriter has reference to a shared buffer resource for the actual data.  This
 *    buffer will remain in existence until both the Reader and Writer have been destroyed.
 *  @li Therefore, application code should ensure that Readers and Writers are destroyed when no longer needed.
 *  @li The AttachmentManager will always satisfy a request to create a Reader or Writer - it will not currently
 *    enforce a maximum resource limit.
 *  @li ACSDK-254 will address this by enforcing such limits.  It should also be noted however, that a well
 *    behaving application may not observe much difference - the future implementation will forcibly close
 *    the oldest Attachment to make space for the new one.  For a system reading and writing a small set of attachments
 *    at any given time, the AttachmentManager should not need to step in and take such action.
 */
class AttachmentManager : public AttachmentManagerInterface {
public:
    /**
     * This is the default timeout value for attachments.  Any attachment which is inspected in the
     * @c removeExpiredAttachmentsLocked() call, and whose lifetime exceeds this value, will be released.
     */
    static constexpr std::chrono::minutes ATTACHMENT_MANAGER_TIMOUT_MINUTES_DEFAULT = std::chrono::hours(12);

    /**
     * This is the minimum timeout value for attachments.  @c setAttachmentTimeoutMinutes() will not accept a
     * value lower than this.
     */
    static constexpr std::chrono::minutes ATTACHMENT_MANAGER_TIMOUT_MINUTES_MINIMUM = std::chrono::minutes(1);

    /**
     * A local enumeration allowing the createReader call to act as a factory function for the underlying
     * attachments.  This enumeration need not include all specializations of the @c Attachment class, only the ones
     * that make sense for the @c AttachmentManager to manage.
     */
    enum class AttachmentType {
        /// This value corresponds to the @c InProcessAttachment class.
        IN_PROCESS
    };

    /**
     * Constructor.
     *
     * @param attachmentType The type of attachments which will be managed.
     */
    AttachmentManager(AttachmentType attachmentType);

    std::string generateAttachmentId(const std::string& contextId, const std::string& contentId) const override;

    bool setAttachmentTimeoutMinutes(std::chrono::minutes timeoutMinutes) override;

    std::unique_ptr<AttachmentWriter> createWriter(
        const std::string& attachmentId,
        utils::sds::WriterPolicy policy = avsCommon::utils::sds::WriterPolicy::ALL_OR_NOTHING) override;

    std::unique_ptr<AttachmentReader> createReader(const std::string& attachmentId, utils::sds::ReaderPolicy policy)
        override;

private:
    /**
     * A utility structure to encapsulate an @c Attachment, its creation time, and other appropriate data fields.
     */
    struct AttachmentManagementDetails {
        /**
         * Constructor.  This sets the creationTime to the time of instantiation.
         */
        AttachmentManagementDetails();

        /// The time this structure instance was created.
        std::chrono::steady_clock::time_point creationTime;
        /// The Attachment this object is managing.
        std::unique_ptr<Attachment> attachment;
    };

    /**
     * A utility function to acquire the details object for an attachment being managed.  This function
     * encapsulates logic to set up the object if it does not already exist, before returning it.
     *
     * @note The class mutex @c m_mutex must be locked before calling this function.
     *
     * @param attachmentId The attachment id for the attachment detail being requested.
     * @return The attachment detail object.
     */
    AttachmentManagementDetails& getDetailsLocked(const std::string& attachmentId);

    /**
     * A cleanup function, which will release an @c AttachmentManagementDetails from the map if either both a writer
     * and reader have been created, or if its lifetime has exceeded the timeout.
     * @note: @c m_mutex must be acquired before calling this function.
     */
    void removeExpiredAttachmentsLocked();

    /// The type of attachments that this manager will create.
    AttachmentType m_attachmentType;
    /// The timeout in minutes.  Any attachment whose lifetime exceeds this value will be released.
    std::chrono::minutes m_attachmentExpirationMinutes;
    /// The mutex to ensure the non-static public APIs are thread safe.
    std::mutex m_mutex;
    /// The map of attachment details.
    std::unordered_map<std::string, AttachmentManagementDetails> m_attachmentDetailsMap;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTMANAGER_H_
