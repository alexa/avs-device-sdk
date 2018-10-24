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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENTREADER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENTREADER_H_

#include "AVSCommon/Utils/SDS/InProcessSDS.h"
#include "AVSCommon/Utils/SDS/Reader.h"

#include "AttachmentReader.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that provides functionality to read data from an @c Attachment following an in-process memory management
 * model.
 *
 * @note This class is not thread-safe beyond the thread-safety provided by the underlying SharedDataStream object.
 */
class InProcessAttachmentReader : public AttachmentReader {
public:
    /// Type aliases for convenience.
    using SDSType = avsCommon::utils::sds::InProcessSDS;
    using SDSTypeIndex = avsCommon::utils::sds::InProcessSDS::Index;
    using SDSTypeReader = SDSType::Reader;

    /**
     * Create an InProcessAttachmentReader.
     *
     * @param policy The policy this reader should adhere to.
     * @param sds The underlying @c SharedDataStream which this object will use.
     * @param index If being constructed from an existing @c SharedDataStream, the index indicates where to read from.
     * @param reference The position in the stream @c offset is applied to.  This parameter defaults to 0, indicating
     *     no offset from the specified reference.
     * @param resetOnOverrun If overrun is detected on @c read, whether to close the attachment (default behavior) or
     *     to reset the read position to where current write position is (and skip all the bytes in between).
     * @return Returns a new InProcessAttachmentReader, or nullptr if the operation failed.  This parameter defaults
     *     to @c ABSOLUTE, indicating offset is relative to the very beginning of the Attachment.
     */
    static std::unique_ptr<InProcessAttachmentReader> create(
        SDSTypeReader::Policy policy,
        std::shared_ptr<SDSType> sds,
        SDSTypeIndex offset = 0,
        SDSTypeReader::Reference reference = SDSTypeReader::Reference::ABSOLUTE,
        bool resetOnOverrun = false);

    /**
     * Destructor.
     */
    ~InProcessAttachmentReader();

    std::size_t read(
        void* buf,
        std::size_t numBytes,
        ReadStatus* readStatus,
        std::chrono::milliseconds timeoutMs = std::chrono::milliseconds(0)) override;

    void close(ClosePoint closePoint = ClosePoint::AFTER_DRAINING_CURRENT_BUFFER) override;

    bool seek(uint64_t offset) override;

    uint64_t getNumUnreadBytes() override;

private:
    /**
     * Constructor.
     *
     * @param policy The @c ReaderPolicy of this object.
     * @param sds The underlying @c SharedDataStream which this object will use.
     * @param resetOnOverrun If overrun is detected on @c read, whether to close the attachment (default behavior) or
     *     to reset the read position to where current write position is (and skip all the bytes in between).
     */
    InProcessAttachmentReader(SDSTypeReader::Policy policy, std::shared_ptr<SDSType> sds, bool resetOnOverrun);

    /// The underlying @c SharedDataStream reader.
    std::shared_ptr<SDSTypeReader> m_reader;

    // On @c read overrun, Whether to close the attachment, or reset it to catch up with the write
    bool m_resetOnOverrun;
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_INPROCESSATTACHMENTREADER_H_
