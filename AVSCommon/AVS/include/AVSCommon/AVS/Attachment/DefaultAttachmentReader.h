/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_DEFAULTATTACHMENTREADER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_DEFAULTATTACHMENTREADER_H_

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>
#include "AttachmentReader.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A class that provides functionality to read data from an @c Attachment.
 *
 * @note This class is not thread-safe beyond the thread-safety provided by the underlying SharedDataStream object.
 */
template <typename SDSType>
class DefaultAttachmentReader : public AttachmentReader {
public:
    /**
     * Create an AttachmentReader.
     *
     * @param policy The policy this reader should adhere to.
     * @param sds The underlying @c SharedDataStream which this object will use.
     * @param offset If being constructed from an existing @c SharedDataStream, the index indicates where to read from.
     *     This parameter defaults to 0, indicating no offset from the specified reference.
     * @param reference The position in the stream @c offset is applied to. This parameter defaults to @c ABSOLUTE,
     *     indicating offset is relative to the very beginning of the Attachment.
     * @param resetOnOverrun If overrun is detected on @c read, whether to close the attachment (default behavior) or
     *     to reset the read position to where current write position is (and skip all the bytes in between).
     * @return Returns a new AttachmentReader, or nullptr if the operation failed.
     */
    static std::unique_ptr<AttachmentReader> create(
        typename SDSType::Reader::Policy policy,
        std::shared_ptr<SDSType> sds,
        typename SDSType::Index offset = 0,
        typename SDSType::Reader::Reference reference = SDSType::Reader::Reference::ABSOLUTE,
        bool resetOnOverrun = false);

    /**
     * Destructor.
     */
    ~DefaultAttachmentReader();

    /// @name AttachmentReader methods.
    /// @{
    std::size_t read(
        void* buf,
        std::size_t numBytes,
        ReadStatus* readStatus,
        std::chrono::milliseconds timeoutMs = std::chrono::milliseconds(0)) override;

    void close(ClosePoint closePoint = ClosePoint::AFTER_DRAINING_CURRENT_BUFFER) override;

    bool seek(uint64_t offset) override;

    uint64_t getNumUnreadBytes() override;

    /// @}
private:
    /**
     * Constructor.
     *
     * @param policy The @c ReaderPolicy of this object.
     * @param sds The underlying @c SharedDataStream which this object will use.
     * @param resetOnOverrun If overrun is detected on @c read, whether to close the attachment (default behavior) or
     *     to reset the read position to where current write position is (and skip all the bytes in between).
     */
    DefaultAttachmentReader(typename SDSType::Reader::Policy policy, std::shared_ptr<SDSType> sds, bool resetOnOverrun);

    /// Log tag
    static const std::string TAG;

    /// The underlying @c SharedDataStream reader.
    std::shared_ptr<typename SDSType::Reader> m_reader;

    // On @c read overrun, Whether to close the attachment, or reset it to catch up with the write
    bool m_resetOnOverrun;
};

template <typename SDSType>
const std::string DefaultAttachmentReader<SDSType>::TAG = "DefaultAttachmentReader";

template <typename SDSType>
std::unique_ptr<AttachmentReader> DefaultAttachmentReader<SDSType>::create(
    typename SDSType::Reader::Policy policy,
    std::shared_ptr<SDSType> sds,
    typename SDSType::Index offset,
    typename SDSType::Reader::Reference reference,
    bool resetOnOverrun) {
    auto reader =
        std::unique_ptr<DefaultAttachmentReader>(new DefaultAttachmentReader<SDSType>(policy, sds, resetOnOverrun));
    if (!reader->m_reader) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "createFailed").d("reason", "object not fully created"));
        return nullptr;
    }

    if (!reader->m_reader->seek(offset, reference)) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "ConstructorFailed").d("reason", "seek failed"));
        return nullptr;
    }

    return std::unique_ptr<AttachmentReader>(reader.release());
}

template <typename SDSType>
DefaultAttachmentReader<SDSType>::~DefaultAttachmentReader() {
    close();
}

template <typename SDSType>
std::size_t DefaultAttachmentReader<SDSType>::read(
    void* buf,
    std::size_t numBytes,
    AttachmentReader::ReadStatus* readStatus,
    std::chrono::milliseconds timeoutMs) {
    if (!readStatus) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "readFailed").d("reason", "read status is nullptr"));
        return 0;
    }

    if (!buf) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "readFailed").d("reason", "buf is nullptr"));
        *readStatus = ReadStatus::ERROR_INTERNAL;
        return 0;
    }

    if (!m_reader) {
        ACSDK_INFO(utils::logger::LogEntry(TAG, "readFailed").d("reason", "closed or uninitialized SDS"));
        *readStatus = ReadStatus::CLOSED;
        return 0;
    }

    if (timeoutMs.count() < 0) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "readFailed").d("reason", "negative timeout"));
        *readStatus = ReadStatus::ERROR_INTERNAL;
        return 0;
    }

    *readStatus = ReadStatus::OK;

    if (0 == numBytes) {
        return 0;
    }

    const auto wordSize = m_reader->getWordSize();
    if (numBytes < wordSize) {
        ACSDK_ERROR(
            utils::logger::LogEntry(TAG, "readFailed").d("reason", "bytes requested smaller than SDS word size"));
        *readStatus = ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE;
        return 0;
    }

    std::size_t bytesRead = 0;
    const auto numWords = numBytes / wordSize;

    const auto readResult = m_reader->read(buf, numWords, timeoutMs);

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
                    ACSDK_DEBUG5(utils::logger::LogEntry(TAG, "readFailed").d("reason", "memory overrun by writer"));
                    m_reader->seek(0, SDSType::Reader::Reference::BEFORE_WRITER);
                } else {
                    // An attachment cannot recover from this.
                    *readStatus = ReadStatus::ERROR_OVERRUN;
                    ACSDK_ERROR(utils::logger::LogEntry(TAG, "readFailed").d("reason", "memory overrun by writer"));
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
            ACSDK_ERROR(
                utils::logger::LogEntry(TAG, "readFailed").d("reason", "unhandled error code").d("code", readResult));
            *readStatus = ReadStatus::ERROR_INTERNAL;
        }

    } else if (0 == readResult) {
        *readStatus = ReadStatus::CLOSED;
        ACSDK_DEBUG0(utils::logger::LogEntry(TAG, "readFailed").d("reason", "SDS is closed"));
    } else {
        bytesRead = static_cast<size_t>(readResult) * wordSize;
    }

    return bytesRead;
}

template <typename SDSType>
void DefaultAttachmentReader<SDSType>::close(AttachmentReader::ClosePoint closePoint) {
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

template <typename SDSType>
bool DefaultAttachmentReader<SDSType>::seek(uint64_t offset) {
    if (m_reader) {
        return m_reader->seek(offset);
    }
    return false;
}

template <typename SDSType>
uint64_t DefaultAttachmentReader<SDSType>::getNumUnreadBytes() {
    if (m_reader) {
        return m_reader->tell(SDSType::Reader::Reference::BEFORE_WRITER);
    }

    ACSDK_ERROR(utils::logger::LogEntry(TAG, "getNumUnreadBytesFailed").d("reason", "noReader"));
    return 0;
}

template <typename SDSType>
DefaultAttachmentReader<SDSType>::DefaultAttachmentReader(
    typename SDSType::Reader::Policy policy,
    std::shared_ptr<SDSType> sds,
    bool resetOnOverrun) :
        m_resetOnOverrun{resetOnOverrun} {
    if (!sds) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "ConstructorFailed").d("reason", "SDS parameter is nullptr"));
        return;
    }

    m_reader = sds->createReader(policy);

    if (!m_reader) {
        ACSDK_ERROR(utils::logger::LogEntry(TAG, "ConstructorFailed").d("reason", "could not create an SDS reader"));
        return;
    }
}

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_DEFAULTATTACHMENTREADER_H_
