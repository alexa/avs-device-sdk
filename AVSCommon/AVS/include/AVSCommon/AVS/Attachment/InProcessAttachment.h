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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENT_H_

#include "AVSCommon/AVS/Attachment/Attachment.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentReader.h"
#include "AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h"

#include "AVSCommon/Utils/SDS/InProcessSDS.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that represents an AVS attachment following an in-process memory management model.
 */
class InProcessAttachment : public Attachment {
public:
    /// Type aliases for convenience.
    using SDSType = avsCommon::utils::sds::InProcessSDS;
    using SDSBufferType = avsCommon::utils::sds::InProcessSDSTraits::Buffer;

    /// Default size of underlying SDS when created internally.
    static const int SDS_BUFFER_DEFAULT_SIZE_IN_BYTES = 0x100000;

    /**
     * Constructor.
     *
     * @param id The attachment id.
     * @param sds The underlying @c SharedDataStream object.  If not specified, then this class will create its own.
     */
    InProcessAttachment(const std::string& id, std::unique_ptr<SDSType> sds = nullptr);

    std::unique_ptr<AttachmentWriter> createWriter(
        InProcessAttachmentWriter::SDSTypeWriter::Policy policy =
            InProcessAttachmentWriter::SDSTypeWriter::Policy::ALL_OR_NOTHING) override;

    std::unique_ptr<AttachmentReader> createReader(InProcessAttachmentReader::SDSTypeReader::Policy policy) override;

private:
    // The sds from which we will create the reader and writer.
    std::shared_ptr<SDSType> m_sds;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENT_H_
