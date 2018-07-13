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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENTWRITER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENTWRITER_H_

#include "AVSCommon/Utils/SDS/InProcessSDS.h"
#include "AVSCommon/Utils/SDS/Writer.h"

#include "AttachmentWriter.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that provides functionality to write data to an @c Attachment.
 *
 * @note This class is not thread-safe beyond the thread-safety provided by the underlying SharedDataStream object.
 */
class InProcessAttachmentWriter : public AttachmentWriter {
public:
    /// Type aliases for convenience.
    using SDSType = avsCommon::utils::sds::InProcessSDS;
    using SDSTypeWriter = SDSType::Writer;

    /**
     * Create an InProcessAttachmentWriter.
     *
     * @param sds The underlying @c SharedDataStream which this object will use.
     * @param policy The policy of the new Writer.
     * @return Returns a new InProcessAttachmentWriter, or nullptr if the operation failed.
     */
    static std::unique_ptr<InProcessAttachmentWriter> create(
        std::shared_ptr<SDSType> sds,
        SDSTypeWriter::Policy policy = SDSTypeWriter::Policy::ALL_OR_NOTHING);

    /**
     * Destructor.
     */
    ~InProcessAttachmentWriter();

    std::size_t write(
        const void* buf,
        std::size_t numBytes,
        WriteStatus* writeStatus,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) override;

    void close() override;

protected:
    /**
     * Constructor.
     *
     * @param sds The underlying @c SharedDataStream which this object will use.
     * @param policy The policy of the new Writer.
     */
    InProcessAttachmentWriter(
        std::shared_ptr<SDSType> sds,
        SDSTypeWriter::Policy policy = SDSTypeWriter::Policy::ALL_OR_NOTHING);

    /// The underlying @c SharedDataStream reader.
    std::shared_ptr<SDSTypeWriter> m_writer;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENTWRITER_H_
