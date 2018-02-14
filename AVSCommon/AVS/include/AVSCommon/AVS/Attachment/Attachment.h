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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENT_H_

#include <atomic>
#include <mutex>
#include <string>

#include "AVSCommon/AVS/Attachment/AttachmentReader.h"
#include "AVSCommon/AVS/Attachment/AttachmentWriter.h"
#include "AVSCommon/Utils/SDS/InProcessSDS.h"
#include "AVSCommon/Utils/SDS/ReaderPolicy.h"
#include "AVSCommon/Utils/SDS/WriterPolicy.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that represents an AVS attachment.
 */
class Attachment {
public:
    /**
     * Constructor.
     *
     * @param attachmentId The id for this attachment.
     */
    Attachment(const std::string& attachmentId);

    /**
     * Destructor.
     */
    virtual ~Attachment() = default;

    /**
     * Creates a writer object, with which the Attachment may be written to.
     *
     * @return a @unique_ptr to an AttachmentWriter.
     */

    virtual std::unique_ptr<AttachmentWriter> createWriter(
        utils::sds::WriterPolicy policy = utils::sds::WriterPolicy::ALL_OR_NOTHING) = 0;

    /**
     * Creates a reader object, with which the Attachment may be read from.
     *
     * @param The policy used to configure the reader.
     * @return a @unique_ptr to an AttachmentReader.
     * */
    virtual std::unique_ptr<AttachmentReader> createReader(utils::sds::ReaderPolicy policy) = 0;

    /**
     * An accessor to get the attachmentId.
     *
     * @return The attachment Id.
     */
    std::string getId() const;

    /**
     * Utility function to tell if a reader has been created for this object.
     *
     * @return Whether a reader has been created for this object.
     */
    bool hasCreatedReader();

    /**
     * Utility function to tell if a writer has been created for this object.
     *
     * @return Whether a writer has been created for this object.
     */
    bool hasCreatedWriter();

protected:
    /// The id for this attachment object.
    const std::string m_id;
    /// mutex to protext access to the createReader and createWriter API.
    std::mutex m_mutex;
    /// An atomic tracking variable to tell whether this object has created a writer.
    std::atomic<bool> m_hasCreatedWriter;
    /// An atomic tracking variable to tell whether this object has created a reader.
    std::atomic<bool> m_hasCreatedReader;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENT_H_
