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

#include "AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("InProcessAttachmentWriter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<InProcessAttachmentWriter> InProcessAttachmentWriter::create(
    std::shared_ptr<SDSType> sds,
    SDSTypeWriter::Policy policy) {
    auto writer = std::unique_ptr<InProcessAttachmentWriter>(new InProcessAttachmentWriter(sds, policy));

    if (!writer->m_writer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "could not create instance"));
        return nullptr;
    }

    return writer;
}

InProcessAttachmentWriter::InProcessAttachmentWriter(std::shared_ptr<SDSType> sds, SDSTypeWriter::Policy policy) {
    if (!sds) {
        ACSDK_ERROR(LX("constructorFailed").d("reason", "SDS parameter is nullptr"));
        return;
    }
    m_writer = sds->createWriter(policy);
}

InProcessAttachmentWriter::~InProcessAttachmentWriter() {
    close();
}

std::size_t InProcessAttachmentWriter::write(
    const void* buff,
    std::size_t numBytes,
    WriteStatus* writeStatus,
    std::chrono::milliseconds timeout) {
    if (!writeStatus) {
        ACSDK_ERROR(LX("writeFailed").d("reason", "writeStatus is nullptr"));
        return 0;
    }

    if (!m_writer) {
        ACSDK_ERROR(LX("writeFailed").d("reason", "SDS is closed or uninitialized"));
        *writeStatus = WriteStatus::CLOSED;
        ACSDK_ERROR(LX("InProcessAttachmentWriter : SDS is closed!"));
        return 0;
    }

    *writeStatus = WriteStatus::OK;

    if (0 == numBytes) {
        return 0;
    }

    auto wordSize = m_writer->getWordSize();
    if (numBytes < wordSize) {
        ACSDK_ERROR(LX("writeFailed").d("reason", "number of bytes are smaller than the underlying word size"));
        *writeStatus = WriteStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE;
        return 0;
    }

    std::size_t bytesWritten = 0;
    auto numWords = numBytes / wordSize;
    auto writeResult = m_writer->write(buff, numWords, timeout);

    /*
     * Convert SDS return code accordingly:
     *
     * < 0 : error code.
     *   0 : the underlying SDS is closed.
     * > 0 : number of bytes written.
     */

    if (writeResult < 0) {
        switch (writeResult) {
            // This means the buffer is full, and we cannot write until a reader consumes data.
            case SDSType::Writer::Error::WOULDBLOCK:
                *writeStatus = WriteStatus::OK_BUFFER_FULL;
                break;

            // This means a parameter passed to the underlying SDS writer was invalid.
            case SDSType::Writer::Error::INVALID:
                ACSDK_ERROR(LX("AttachmentWriter has generated an internal error."));
                *writeStatus = WriteStatus::ERROR_INTERNAL;
                break;
            case SDSType::Writer::Error::TIMEDOUT:
                ACSDK_DEBUG9(LX("AttachmentWriter has timed out while attempting a write."));
                *writeStatus = WriteStatus::TIMEDOUT;
                break;
        }

        // If the status was not updated, then there's an error code from SDS we may not be handling.
        if (WriteStatus::OK == *writeStatus) {
            ACSDK_ERROR(LX("writeFailed")
                            .d("reason", "unhandled error code from underlying SDS")
                            .d("code", std::to_string(writeResult)));
            *writeStatus = WriteStatus::ERROR_INTERNAL;
        }

    } else if (0 == writeResult) {
        *writeStatus = WriteStatus::CLOSED;
        ACSDK_INFO(LX("writeFailed").d("reason", "underlying SDS is closed"));
        close();
    } else {
        bytesWritten = static_cast<size_t>(writeResult) * wordSize;
    }

    return bytesWritten;
}

void InProcessAttachmentWriter::close() {
    if (m_writer) {
        m_writer->close();
    }
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
