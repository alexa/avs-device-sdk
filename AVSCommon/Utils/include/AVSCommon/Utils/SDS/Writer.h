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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_WRITER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_WRITER_H_

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <limits>
#include <cstring>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"

#include "SharedDataStream.h"
#include "WriterPolicy.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {

/**
 * This is a nested class in @c SharedDataStream which provides an interface for writing (producing) data to the
 * stream.
 *
 * @note This class is primarily intended to be used from a single thread.  The @c Writer as a whole is thread-safe in
 * the sense that @c Writer and @c Readers can all live in different threads, but individual member functions of a
 * @c Writer instance should not be called from multiple threads except where specifically noted in function
 * documentation below.
 */
template <typename T>
class SharedDataStream<T>::Writer {
public:
    /// Specifies the policy to use for writing to the stream.
    using Policy = WriterPolicy;

    /**
     * Enumerates error codes which may be returned by @c write().
     *
     * @note Using a weakly-typed enum so errors can be compared to @c write() return values without casting.
     */
    struct Error {
        enum {
            /// Returned when @c close() has been previously called on the @c Writer.
            CLOSED = 0,
            /// Returned when policy is @c Policy::ALL_OR_NOTHING and the @c write() would overwrrite unconsumed data.
            WOULDBLOCK = -1,
            /// Returned when a @c write() parameter is invalid.
            INVALID = -2,
            /// Returned when policy is @c Policy::BLOCKING and no space becomes available before the specified timeout.
            TIMEDOUT = -3,
        };
    };

    /**
     * Constructs a new @c Writer which produces data for the provided @c SharedDataStream.
     *
     * @param policy The policy to use for reading from the stream.
     * @param stream The @c BufferLayout to use for writing stream data.
     */
    Writer(Policy policy, std::shared_ptr<BufferLayout> bufferLayout);

    /// This destructor detaches the @c Writer from a @c BufferLayout.
    ~Writer();

    /**
     * This function adds new data to the stream by copying it from the provided buffer.
     *
     * @param buf A buffer to copy the data from.
     * @param nWords The maximum number of @c wordSize words to copy.
     * @param timeout The maximum time to wait (if @c policy is @c BLOCKING) for space to write into.  If this parameter
     *     is zero, there is no timeout and blocking writes will wait forever.  If @c policy is not @C BLOCKING, this
     *     parameter is ignored.
     * @return The number of @c wordSize words copied, or zero if the stream has closed, or a
     *     negative @c Error code if the stream is still open, but no data could be written.
     *
     * @note A stream is closed for the @c Writer if @c Writer::close() has been called.
     *
     * @warning If @c policy is @c BLOCKING and @c timeout is 0, this function will only unblock when a @c Reader
     *     @c read()s some data or @c seek()s forward.  Applications which use this combination of parameters must use
     *     @c Readers to drain some data from the @c SharedDataStream if they need to unblock the @c Writer.
     */
    ssize_t write(const void* buf, size_t nWords, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * This function reports the current position of the @c Writer in the stream.
     *
     * @return The @c Writer's position (in @c wordSize words) in the stream.
     */
    Index tell() const;

    /**
     * This function closes the @c Writer, such that @c Readers will return 0 when they catch up with the @c Writer,
     * and subsequent calls to @c write() will return 0.
     */
    void close();

    /**
     * This function returns the word size (in bytes).  All @c SharedDataStream operations that work with data or
     * position in the stream are quantified in words.
     *
     * @return The size (in bytes) of words for this @c Writer's @c SharedDataStream.
     */
    size_t getWordSize() const;

    /**
     * Returns the text of an error code.
     *
     * @return Text of the specified error.
     */
    static std::string errorToString(Error error);

private:
    /**
     * The tag associated with log entries from this class.
     */
    static const std::string TAG;

    /// The @c Policy to use for writing to the stream.
    Policy m_policy;

    /// The @c BufferLayout to use for writing stream data.
    std::shared_ptr<BufferLayout> m_bufferLayout;

    /**
     * A flag indicating whether this writer has closed.  This flag prevents trying to disable the writer during
     * destruction after previously having closed the writer.  Usage of this flag must be locked by
     * @c Header::WriterEnabledMutex.
     */
    bool m_closed;
};

template <typename T>
const std::string SharedDataStream<T>::Writer::TAG = "SdsWriter";

template <typename T>
SharedDataStream<T>::Writer::Writer(Policy policy, std::shared_ptr<BufferLayout> bufferLayout) :
        m_policy{policy},
        m_bufferLayout{bufferLayout},
        m_closed{false} {
    // Note - SharedDataStream::createWriter() holds writerEnableMutex while calling this function.
    auto header = m_bufferLayout->getHeader();
    header->isWriterEnabled = true;
    header->writeEndCursor = header->writeStartCursor.load();
}

template <typename T>
SharedDataStream<T>::Writer::~Writer() {
    close();
}

template <typename T>
ssize_t SharedDataStream<T>::Writer::write(const void* buf, size_t nWords, std::chrono::milliseconds timeout) {
    if (nullptr == buf) {
        logger::acsdkError(logger::LogEntry(TAG, "writeFailed").d("reason", "nullBuffer"));
        return Error::INVALID;
    }
    if (0 == nWords) {
        logger::acsdkError(logger::LogEntry(TAG, "writeFailed").d("reason", "zeroNumWords"));
        return Error::INVALID;
    }

    auto header = m_bufferLayout->getHeader();
    if (!header->isWriterEnabled) {
        logger::acsdkError(logger::LogEntry(TAG, "writeFailed").d("reason", "writerDisabled"));
        return Error::CLOSED;
    }

    auto wordsToCopy = nWords;
    auto buf8 = static_cast<const uint8_t*>(buf);
    std::unique_lock<Mutex> backwardSeekLock(header->backwardSeekMutex, std::defer_lock);
    Index writeEnd = header->writeStartCursor + nWords;

    switch (m_policy) {
        case Policy::NONBLOCKABLE:
            // For NONBLOCKABLE, we can truncate the write if it won't fit in the buffer.
            if (nWords > m_bufferLayout->getDataSize()) {
                wordsToCopy = nWords = m_bufferLayout->getDataSize();
                writeEnd = header->writeStartCursor + nWords;
            }
            break;
        case Policy::ALL_OR_NOTHING:
            // For ALL_OR_NOTHING, we can't overwrite readers, and we can't truncate, but we might be able to discard
            // bytes that overflow if oldestUnconsumedCursor is in the future (e.g. there are readers waiting for future
            // data which has not been written yet).

            // Note - this check must be performed while locked to prevent a reader from backwards-seeking into the
            // write region between here and the writeEndCursor update below.
            backwardSeekLock.lock();
            if ((writeEnd >= header->oldestUnconsumedCursor) &&
                ((writeEnd - header->oldestUnconsumedCursor) > m_bufferLayout->getDataSize())) {
                return Error::WOULDBLOCK;
            }
            break;
        case Policy::BLOCKING:
            // For BLOCKING, we need to wait until there is room for at least one word.

            // Condition for returning from write: there is space for a write.
            auto predicate = [this, header] {
                return (header->writeStartCursor < header->oldestUnconsumedCursor) ||
                       (header->writeStartCursor - header->oldestUnconsumedCursor) < m_bufferLayout->getDataSize();
            };

            // Note - this check must be performed while locked to prevent a reader from backwards-seeking into the
            // write region between here and the writeEndCursor update below.
            backwardSeekLock.lock();

            // Wait for space to become available.
            if (std::chrono::milliseconds::zero() == timeout) {
                header->spaceAvailableConditionVariable.wait(backwardSeekLock, predicate);
            } else if (!header->spaceAvailableConditionVariable.wait_for(backwardSeekLock, timeout, predicate)) {
                return Error::TIMEDOUT;
            }

            // Figure out how much space we have.
            auto spaceAvailable = m_bufferLayout->getDataSize();
            if (header->writeStartCursor >= header->oldestUnconsumedCursor) {
                auto wordsToOverrun =
                    m_bufferLayout->getDataSize() - (header->writeStartCursor - header->oldestUnconsumedCursor);
                if (wordsToOverrun < spaceAvailable) {
                    spaceAvailable = wordsToOverrun;
                }
            }

            // For BLOCKING, we can truncate the write if it won't fit in the buffer.
            if (spaceAvailable < nWords) {
                wordsToCopy = nWords = spaceAvailable;
                writeEnd = header->writeStartCursor + nWords;
            }

            break;
    }

    header->writeEndCursor = writeEnd;

    // We've updated our end cursor, so we no longer need to hold off backward seeks.
    if (backwardSeekLock) {
        backwardSeekLock.unlock();
    }

    if (Policy::ALL_OR_NOTHING == m_policy) {
        // If we have more data than the SDS can hold and we're not going to be overwriting oldestUnconsumedCursor, we
        // can safely discard the initial data and just leave the trailing data in the buffer.
        if (wordsToCopy > m_bufferLayout->getDataSize()) {
            wordsToCopy = m_bufferLayout->getDataSize();
            buf8 += (nWords - wordsToCopy) * getWordSize();
        }
    }

    // Split it across the wrap.
    size_t beforeWrap = m_bufferLayout->wordsUntilWrap(header->writeStartCursor);
    if (beforeWrap > wordsToCopy) {
        beforeWrap = wordsToCopy;
    }
    size_t afterWrap = wordsToCopy - beforeWrap;

    // Copy the two segments.
    memcpy(m_bufferLayout->getData(header->writeStartCursor), buf8, beforeWrap * getWordSize());
    if (afterWrap > 0) {
        memcpy(
            m_bufferLayout->getData(header->writeStartCursor + beforeWrap),
            buf8 + beforeWrap * getWordSize(),
            afterWrap * getWordSize());
    }

    // Advance the write cursor.
    // Note: To prevent a race condition and ensure that readers which block on dataAvailableConditionVariable don't
    // miss a notify, we should always lock the dataAvailableConditionVariable mutex while moving writeStartCursor.  As
    // an optimization, we skip that lock for NONBLOCKABLE writers under the assumption that they will be writing
    // continuously, so a missed notification is not significant.
    // Note: As a further optimization, the lock could be omitted if no blocking readers are in use (ACSDK-251).
    std::unique_lock<Mutex> dataAvailableLock(header->dataAvailableMutex, std::defer_lock);
    if (Policy::NONBLOCKABLE != m_policy) {
        dataAvailableLock.lock();
    }
    header->writeStartCursor = header->writeEndCursor.load();
    if (Policy::NONBLOCKABLE != m_policy) {
        dataAvailableLock.unlock();
    }

    // Notify the reader(s).
    // Note: as an optimization, we could skip this if there are no blocking readers (ACSDK-251).
    header->dataAvailableConditionVariable.notify_all();

    return nWords;
}

template <typename T>
typename SharedDataStream<T>::Index SharedDataStream<T>::Writer::tell() const {
    return m_bufferLayout->getHeader()->writeStartCursor;
}

template <typename T>
void SharedDataStream<T>::Writer::close() {
    auto header = m_bufferLayout->getHeader();
    std::lock_guard<Mutex> lock(header->writerEnableMutex);
    if (m_closed) {
        return;
    }
    if (header->isWriterEnabled) {
        header->isWriterEnabled = false;

        std::unique_lock<Mutex> dataAvailableLock(header->dataAvailableMutex);

        header->hasWriterBeenClosed = true;

        header->dataAvailableConditionVariable.notify_all();
    }
    m_closed = true;
}

template <typename T>
size_t SharedDataStream<T>::Writer::getWordSize() const {
    return m_bufferLayout->getHeader()->wordSize;
}

template <typename T>
std::string SharedDataStream<T>::Writer::errorToString(Error error) {
    switch (error) {
        case Error::CLOSED:
            return "CLOSED";
        case Error::WOULDBLOCK:
            return "WOULDBLOCK";
        case Error::INVALID:
            return "INVALID";
        case Error::TIMEDOUT:
            return "TIMEDOUT";
    }
    logger::acsdkError(logger::LogEntry(TAG, "errorToStringFailed").d("reason", "invalidError").d("error", error));
    return "(unknown error " + to_string(error) + ")";
}

}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_WRITER_H_
