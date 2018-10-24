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

#include "AVSCommon/AVS/Attachment/InProcessAttachmentReader.h"
#include "AVSCommon/Utils/Logger/Logger.h"

using namespace alexaClientSDK::avsCommon::utils;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/// String to identify log entries originating from this file.
static const std::string TAG("InProcessAttachmentReader");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<InProcessAttachmentReader> InProcessAttachmentReader::create(
    SDSTypeReader::Policy policy,
    std::shared_ptr<SDSType> sds,
    SDSTypeIndex offset,
    SDSTypeReader::Reference reference,
    bool resetOnOverrun) {
    auto reader =
        std::unique_ptr<InProcessAttachmentReader>(new InProcessAttachmentReader(policy, sds, resetOnOverrun));

    if (!reader->m_reader) {
        ACSDK_ERROR(LX("createFailed").d("reason", "object not fully created"));
        return nullptr;
    }

    if (!reader->m_reader->seek(offset, reference)) {
        ACSDK_ERROR(LX("ConstructorFailed").d("reason", "seek failed"));
        return nullptr;
    }

    return reader;
}

InProcessAttachmentReader::InProcessAttachmentReader(
    SDSTypeReader::Policy policy,
    std::shared_ptr<SDSType> sds,
    bool resetOnOverrun) :
        m_resetOnOverrun{resetOnOverrun} {
    if (!sds) {
        ACSDK_ERROR(LX("ConstructorFailed").d("reason", "SDS parameter is nullptr"));
        return;
    }

    m_reader = sds->createReader(policy);

    if (!m_reader) {
        ACSDK_ERROR(LX("ConstructorFailed").d("reason", "could not create an SDS reader"));
        return;
    }
}

InProcessAttachmentReader::~InProcessAttachmentReader() {
    close();
}

std::size_t InProcessAttachmentReader::read(
    void* buf,
    std::size_t numBytes,
    ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
    if (!readStatus) {
        ACSDK_ERROR(LX("readFailed").d("reason", "read status is nullptr"));
        return 0;
    }

    if (!m_reader) {
        ACSDK_INFO(LX("readFailed").d("reason", "closed or uninitialized SDS"));
        *readStatus = ReadStatus::CLOSED;
        return 0;
    }

    if (timeoutMs.count() < 0) {
        ACSDK_ERROR(LX("readFailed").d("reason", "negative timeout"));
        *readStatus = ReadStatus::ERROR_INTERNAL;
        return 0;
    }

    *readStatus = ReadStatus::OK;

    if (0 == numBytes) {
        return 0;
    }

    auto wordSize = m_reader->getWordSize();
    if (numBytes < wordSize) {
        ACSDK_ERROR(LX("readFailed").d("reason", "bytes requested smaller than SDS word size"));
        *readStatus = ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE;
        return 0;
    }

    std::size_t bytesRead = 0;
    auto numWords = numBytes / wordSize;

    auto readResult = m_reader->read(buf, numWords, timeoutMs);

    /*
     * Convert SDS return code accordingly:
     *
     * < 0 : Error code.
     *   0 : The underlying SDS is closed.
     * > 0 : The number of bytes read.
     */

    if (readResult < 0) {
        switch (readResult) {
            // This means the writer has overwritten the reader.
            case SDSType::Reader::Error::OVERRUN:
                if (m_resetOnOverrun) {
                    // An attachment's read position will be reset to current writer position.
                    // Subsequent reads will deliver data from current writer position onward.
                    *readStatus = ReadStatus::OK_OVERRUN_RESET;
                    ACSDK_DEBUG5(LX("readFailed").d("reason", "memory overrun by writer"));
                    m_reader->seek(0, SDSTypeReader::Reference::BEFORE_WRITER);
                } else {
                    // An attachment cannot recover from this.
                    *readStatus = ReadStatus::ERROR_OVERRUN;
                    ACSDK_ERROR(LX("readFailed").d("reason", "memory overrun by writer"));
                    close();
                }
                break;

            // This means there is still an active writer, but no data.  A read would block if the policy was blocking.
            case SDSType::Reader::Error::WOULDBLOCK:
                *readStatus = ReadStatus::OK_WOULDBLOCK;
                break;

            // This means there is still an active writer, but no data.  A read call timed out waiting for data.
            case SDSType::Reader::Error::TIMEDOUT:
                *readStatus = ReadStatus::OK_TIMEDOUT;
                break;
        }

        // If the status was not updated, then there's an error code from SDS we may not be handling.
        if (ReadStatus::OK == *readStatus) {
            ACSDK_ERROR(LX("readFailed").d("reason", "unhandled error code").d("code", readResult));
            *readStatus = ReadStatus::ERROR_INTERNAL;
        }

    } else if (0 == readResult) {
        *readStatus = ReadStatus::CLOSED;
        ACSDK_DEBUG0(LX("readFailed").d("reason", "SDS is closed"));
    } else {
        bytesRead = static_cast<size_t>(readResult) * wordSize;
    }

    return bytesRead;
}

void InProcessAttachmentReader::close(ClosePoint closePoint) {
    if (m_reader) {
        switch (closePoint) {
            case ClosePoint::IMMEDIATELY:
                m_reader->close();
                return;
            case ClosePoint::AFTER_DRAINING_CURRENT_BUFFER:
                m_reader->close(0, SDSType::Reader::Reference::BEFORE_WRITER);
                return;
        }
    }
}

bool InProcessAttachmentReader::seek(uint64_t offset) {
    if (m_reader) {
        return m_reader->seek(offset);
    }
    return false;
}

uint64_t InProcessAttachmentReader::getNumUnreadBytes() {
    if (m_reader) {
        return m_reader->tell(utils::sds::InProcessSDS::Reader::Reference::BEFORE_WRITER);
    }

    ACSDK_ERROR(LX("getNumUnreadBytesFailed").d("reason", "noReader"));
    return 0;
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
