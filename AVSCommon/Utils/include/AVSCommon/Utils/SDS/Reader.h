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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_READER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_READER_H_

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <limits>
#include <cstring>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"
#include "SharedDataStream.h"
#include "ReaderPolicy.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {

/**
 * This is a nested class in SharedDataStream which provides an interface for reading (consuming) data from a stream.
 *
 * @note This class is primarily intended to be used from a single thread.  The @c Reader as a whole is thread-safe in
 * the sense that @c Writer and @c Readers can all live in different threads, but individual member functions of a
 * @c Reader instance should not be called from multiple threads except where specifically noted in function
 * documentation below.
 */
template <typename T>
class SharedDataStream<T>::Reader {
public:
    /// Specifies the policy to use for reading from the stream.
    using Policy = ReaderPolicy;

    /// Specifies a reference to measure @c seek()/@c tell()/@c close() offsets against.
    enum class Reference {
        /// The offset is from this @c Reader's current position: `(index = reader + offset)`.
        AFTER_READER,
        /// The offset is to this @c Reader's current position: `(index = reader - offset)`.
        BEFORE_READER,
        /// The offset is to the @c Writer's current position: `(index = writer - offset)`.
        BEFORE_WRITER,
        /// The offset is absolute: `(index = 0 + offset)`.
        ABSOLUTE
    };

    /**
     * Enumerates error codes which may be returned by @c read().
     *
     * @note Using a weakly-typed enum so errors can be compared to @c read() return values without casting.
     */
    struct Error {
        enum {
            /**
             * Returned when the stream is closed - either due to a @c Writer::close() call and no remaining buffered
             * data, or due to a @c Reader::close() call which has reached its close index.
             */
            CLOSED = 0,
            /// Returned when the data requested has been overwritten and is invalid.
            OVERRUN = -1,
            /// Returned when policy is @c Policy::NONBLOCKING and no data is available.
            WOULDBLOCK = -2,
            /// Returned when policy is @c Policy::BLOCKING and no data becomes available before the specified timeout.
            TIMEDOUT = -3,
            /// Returned when a @c read() parameter is invalid.
            INVALID = -4
        };
    };

    /**
     * Constructs a new @c Reader which consumes data from the provided @c SharedDataStream.  The caller must hold
     * @c Header::readerEnableMutex when constructing new Readers.
     *
     * @param policy The policy to use for reading from the stream.
     * @param bufferLayout The @c BufferLayout to use for reading stream data.
     * @param id The id of the reader, assigned by the @c SharedDataStream.
     */
    Reader(Policy policy, std::shared_ptr<BufferLayout> bufferLayout, uint8_t id);

    /// This destructor detaches the @c Reader from a @c BufferLayout.
    ~Reader();

    /**
     * This function consumes data from the stream and copies it to the provided buffer.
     *
     * @param buf A buffer to copy the consumed data to.  This buffer must be large enough to hold @c nWords
     *     (`nWords * wordSize` bytes).
     * @param nWords The maximum number of @c wordSize words to copy.
     * @param timeout The maximum time to wait (if @c policy is @c BLOCKING) for data.  If this parameter is zero,
     *     there is no timeout and blocking reads will wait forever.  If @c policy is @c NONBLOCKING, this parameter
     *     is ignored.
     * @return The number of @c wordSize words copied if data was consumed, or zero if the stream has closed, or a
     *     negative @c Error code if the stream is still open, but no data could be consumed.
     *
     * @note A stream is closed for the @c Reader if @c Reader::close() has been called on it, or if @c Writer::close()
     *     has been called and the @c Reader has consumed all remaining data left in the stream when the @c Writer
     *     closed.  In the special case of a new stream, where no @c Writer has been created, the stream is not
     *     considered to be closed for the @c Reader; attempts to @c read() will either block or return @c WOULDBLOCK,
     *     depending on the @c Policy.
     */
    ssize_t read(void* buf, size_t nWords, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * This function moves the @c Reader to the specified location in the stream.  If successful, subsequent calls to
     * @c read() will start from the new location.  For this function to succeed, the specified location *must* point
     * at data which has not been pushed out of the buffer; if the specified location points at old data which has
     * already been overwritten, the @c seek() call will fail.  If the specified location points at future data which
     * does not yet exist in the buffer, the @c seek() call will succeed.  If the @c seek() call fails, the @c Reader
     * position will remain unchanged.
     *
     * @param offset The position (in @c wordSize words) in the stream, relative to @c reference, to move the @c Reader
     *     to.
     * @param reference The position in the stream @c offset is applied to.
     * @return @c true if the specified position points at unconsumed data, else @c false.
     */
    bool seek(Index offset, Reference reference = Reference::ABSOLUTE);

    /**
     * This function reports the current position of the @c Reader in the stream.
     *
     * @param reference The position in the stream the return value is measured against.
     * @return The @c Reader's position (in @c wordSize words) in the stream relative to @c reference.
     *
     * @note For @c Reference::BEFORE_WRITER, if the read cursor points at a location in the future (after the writer),
     *     then the reader is not before the writer, so this function will return 0.
     */
    Index tell(Reference reference = Reference::ABSOLUTE) const;

    /**
     * This function sets the point at which the @c Reader's stream will close.  With the default parameters, this
     * function will close t he stream immediately, without reading any additional data.  To schedule the stream to
     * close once all the data which is currently in the buffer has been read, call
     * `close(0, Reference::BEFORE_WRITER)`. If another close point is desired, it can be specified using a different
     * @c offset and/or @c reference.
     *
     * @param offset The position (in @c wordSize words) in the stream, relative to @c reference, to close at.
     * @param reference The position in the stream the close point is measured against.
     *
     * @note This function can be called from any thread or process, and it will schedule the @c Reader to close,
     *     however it will *not* wake up a @c BLOCKING @c Reader which is already blocked waiting for data.  In the
     *     case of a blocked @c read(), that @c read() will return when it wakes up - either due to a timeout, or due
     *     to a @c Writer adding data to the stream.
     */
    void close(Index offset = 0, Reference reference = Reference::AFTER_READER);

    /**
     * This function returns the id assigned to this @c Reader.  If a @c Reader instance is not destroyed cleanly (e.g.
     * a @c Reader from another process that crashes), its id can be passed to @c SharedDataStream::reset() to free up
     * its resources.
     *
     * @return The id assigned to this @c Reader.
     */
    size_t getId() const;

    /**
     * This function returns the word size (in bytes).  All @c SharedDataStream operations that work with data or
     * position in the stream are quantified in words.
     *
     * @return The size (in bytes) of words for this @c Reader's @c SharedDataStream.
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

    /// The @c Policy to use for reading from the stream.
    Policy m_policy;

    /// The @c BufferLayout to use for reading stream data.
    std::shared_ptr<BufferLayout> m_bufferLayout;

    /**
     * The index in @c BufferLayout::getReaderCursorArray() and @c BufferLayout::getReaderCloseIndexArray() assigned to
     * this @c Reader.
     */
    uint8_t m_id;

    /// Pointer to this reader's cursor in BufferLayout::getReaderCursorArray().
    AtomicIndex* m_readerCursor;

    /// Pointer to this reader's close index in BufferLayout::getReaderCloseIndexArray().
    AtomicIndex* m_readerCloseIndex;
};

template <typename T>
const std::string SharedDataStream<T>::Reader::TAG = "SdsReader";

template <typename T>
SharedDataStream<T>::Reader::Reader(Policy policy, std::shared_ptr<BufferLayout> bufferLayout, uint8_t id) :
        m_policy{policy},
        m_bufferLayout{bufferLayout},
        m_id{id},
        m_readerCursor{&m_bufferLayout->getReaderCursorArray()[m_id]},
        m_readerCloseIndex{&m_bufferLayout->getReaderCloseIndexArray()[m_id]} {
    // Note - SharedDataStream::createReader() holds readerEnableMutex while calling this function.
    // Read new data only.
    // Note: It is important that new readers start with their cursor at the writer.  This allows
    // updateOldestUnconsumedCursor() to be thread-safe without holding readerEnableMutex.  See
    // updateOldestUnconsumedCursor() comments for further explanation.
    *m_readerCursor = m_bufferLayout->getHeader()->writeStartCursor.load();

    // Read indefinitely.
    *m_readerCloseIndex = std::numeric_limits<Index>::max();

    m_bufferLayout->enableReaderLocked(m_id);
}

template <typename T>
SharedDataStream<T>::Reader::~Reader() {
    // Note: We can't leave a reader with its cursor in the future; doing so can introduce a race condition in
    // updateOldestUnconsumedCursor().  See updateOldestUnconsumedCursor() comments for further explanation.
    seek(0, Reference::BEFORE_WRITER);

    std::lock_guard<Mutex> lock(m_bufferLayout->getHeader()->readerEnableMutex);
    m_bufferLayout->disableReaderLocked(m_id);
    m_bufferLayout->updateOldestUnconsumedCursor();
}

template <typename T>
ssize_t SharedDataStream<T>::Reader::read(void* buf, size_t nWords, std::chrono::milliseconds timeout) {
    if (nullptr == buf) {
        logger::acsdkError(logger::LogEntry(TAG, "readFailed").d("reason", "nullBuffer"));
        return Error::INVALID;
    }

    if (0 == nWords) {
        logger::acsdkError(logger::LogEntry(TAG, "readFailed").d("reason", "invalidNumWords").d("numWords", nWords));
        return Error::INVALID;
    }

    // Check if closed.
    auto readerCloseIndex = m_readerCloseIndex->load();
    if (*m_readerCursor >= readerCloseIndex) {
        return Error::CLOSED;
    }

    // Initial check for overrun.
    auto header = m_bufferLayout->getHeader();
    if ((header->writeEndCursor >= *m_readerCursor) &&
        (header->writeEndCursor - *m_readerCursor) > m_bufferLayout->getDataSize()) {
        return Error::OVERRUN;
    }

    std::unique_lock<Mutex> lock(header->dataAvailableMutex, std::defer_lock);
    if (Policy::BLOCKING == m_policy) {
        lock.lock();
    }

    // Figure out how much we can actually copy.
    size_t wordsAvailable = tell(Reference::BEFORE_WRITER);
    if (0 == wordsAvailable) {
        if (header->writeEndCursor > 0 && !header->isWriterEnabled) {
            return Error::CLOSED;
        } else if (Policy::NONBLOCKING == m_policy) {
            return Error::WOULDBLOCK;
        } else if (Policy::BLOCKING == m_policy) {
            // Condition for returning from read: the Writer has been closed or there is data to read
            auto predicate = [this, header] {
                return header->hasWriterBeenClosed || tell(Reference::BEFORE_WRITER) > 0;
            };

            if (std::chrono::milliseconds::zero() == timeout) {
                header->dataAvailableConditionVariable.wait(lock, predicate);
            } else if (!header->dataAvailableConditionVariable.wait_for(lock, timeout, predicate)) {
                return Error::TIMEDOUT;
            }
        }
        wordsAvailable = tell(Reference::BEFORE_WRITER);

        // If there is still no data, the writer has closed in the interim
        if (0 == wordsAvailable) {
            return Error::CLOSED;
        }
    }

    if (Policy::BLOCKING == m_policy) {
        lock.unlock();
    }
    if (nWords > wordsAvailable) {
        nWords = wordsAvailable;
    }

    // Don't read beyond our close index.
    if ((*m_readerCursor + nWords) > readerCloseIndex) {
        nWords = readerCloseIndex - *m_readerCursor;
    }

    // Split it across the wrap.
    size_t beforeWrap = m_bufferLayout->wordsUntilWrap(*m_readerCursor);
    if (beforeWrap > nWords) {
        beforeWrap = nWords;
    }
    size_t afterWrap = nWords - beforeWrap;

    // Copy the two segments.
    auto buf8 = static_cast<uint8_t*>(buf);
    memcpy(buf8, m_bufferLayout->getData(*m_readerCursor), beforeWrap * getWordSize());
    if (afterWrap > 0) {
        memcpy(
            buf8 + (beforeWrap * getWordSize()),
            m_bufferLayout->getData(*m_readerCursor + beforeWrap),
            afterWrap * getWordSize());
    }

    // Advance the read cursor.
    *m_readerCursor += nWords;

    // Final check for overrun (do this before the updateOldestUnconsumedCursor() call below for improved accuracy).
    bool overrun = ((header->writeEndCursor - *m_readerCursor) > m_bufferLayout->getDataSize());

    // Move the unconsumed cursor before returning.
    m_bufferLayout->updateOldestUnconsumedCursor();

    // Now we can safely error out if there was an overrun.
    if (overrun) {
        return Error::OVERRUN;
    }

    return nWords;
}

template <typename T>
bool SharedDataStream<T>::Reader::seek(Index offset, Reference reference) {
    auto header = m_bufferLayout->getHeader();
    auto writeStartCursor = &header->writeStartCursor;
    auto writeEndCursor = &header->writeEndCursor;
    Index absolute = std::numeric_limits<Index>::max();

    switch (reference) {
        case Reference::AFTER_READER:
            absolute = *m_readerCursor + offset;
            break;
        case Reference::BEFORE_READER:
            if (offset > *m_readerCursor) {
                logger::acsdkError(logger::LogEntry(TAG, "seekFailed")
                                       .d("reason", "seekBeforeStreamStart")
                                       .d("reference", "BEFORE_READER")
                                       .d("seekOffset", offset)
                                       .d("readerCursor", m_readerCursor->load()));
                return false;
            }
            absolute = *m_readerCursor - offset;
            break;
        case Reference::BEFORE_WRITER:
            if (offset > *writeStartCursor) {
                logger::acsdkError(logger::LogEntry(TAG, "seekFailed")
                                       .d("reason", "seekBeforeStreamStart")
                                       .d("reference", "BEFORE_WRITER")
                                       .d("seekOffset", offset)
                                       .d("writeStartCursor", writeStartCursor->load()));
                return false;
            }
            absolute = *writeStartCursor - offset;
            break;
        case Reference::ABSOLUTE:
            absolute = offset;
    }

    // Don't seek beyond the close index.
    if (absolute > *m_readerCloseIndex) {
        logger::acsdkError(logger::LogEntry(TAG, "seekFailed")
                               .d("reason", "seekBeyondCloseIndex")
                               .d("position", absolute)
                               .d("readerCloseIndex", m_readerCloseIndex->load()));
        return false;
    }

    // Per documentation of updateOldestUnconsumedCursor(), don't try to seek backwards while oldestConsumedCursor is
    // being updated.
    bool backward = absolute < *m_readerCursor;
    std::unique_lock<Mutex> lock(header->backwardSeekMutex, std::defer_lock);
    if (backward) {
        lock.lock();
    }

    // Don't seek to past data which has been (or soon will be) overwritten.
    // Note: If this is a backward seek, it is important that this check is performed while holding the
    // backwardSeekMutex to prevent a writer from starting to overwrite us between here and the m_readerCursor update
    // below.  If this is not a backward seek, then the mutex is not held.
    if (*writeEndCursor >= absolute && *writeEndCursor - absolute > m_bufferLayout->getDataSize()) {
        logger::acsdkError(logger::LogEntry(TAG, "seekFailed").d("reason", "seekOverwrittenData"));
        return false;
    }

    *m_readerCursor = absolute;

    if (backward) {
        m_bufferLayout->updateOldestUnconsumedCursorLocked();
        lock.unlock();
    } else {
        m_bufferLayout->updateOldestUnconsumedCursor();
    }

    return true;
}

template <typename T>
typename SharedDataStream<T>::Index SharedDataStream<T>::Reader::tell(Reference reference) const {
    auto writeStartCursor = &m_bufferLayout->getHeader()->writeStartCursor;
    switch (reference) {
        case Reference::AFTER_READER:
            return 0;
        case Reference::BEFORE_READER:
            return 0;
        case Reference::BEFORE_WRITER:
            return (*writeStartCursor >= *m_readerCursor) ? *writeStartCursor - *m_readerCursor : 0;
        case Reference::ABSOLUTE:
            return *m_readerCursor;
    }
    logger::acsdkError(logger::LogEntry(TAG, "tellFailed").d("reason", "invalidReference"));
    return std::numeric_limits<Index>::max();
}

template <typename T>
void SharedDataStream<T>::Reader::close(Index offset, Reference reference) {
    auto writeStartCursor = &m_bufferLayout->getHeader()->writeStartCursor;
    Index absolute = 0;
    bool validReference = false;
    switch (reference) {
        case Reference::AFTER_READER:
            absolute = *m_readerCursor + offset;
            validReference = true;
            break;
        case Reference::BEFORE_READER:
            absolute = *m_readerCursor;
            validReference = true;
            break;
        case Reference::BEFORE_WRITER:
            if (*writeStartCursor < offset) {
                logger::acsdkError(logger::LogEntry(TAG, "closeFailed")
                                       .d("reason", "invalidIndex")
                                       .d("reference", "BEFORE_WRITER")
                                       .d("offset", offset)
                                       .d("writeStartCursor", writeStartCursor->load()));
            } else {
                absolute = *writeStartCursor - offset;
            }
            validReference = true;
            break;
        case Reference::ABSOLUTE:
            absolute = offset;
            validReference = true;
            break;
    }
    if (!validReference) {
        logger::acsdkError(logger::LogEntry(TAG, "closeFailed").d("reason", "invalidReference"));
    }

    *m_readerCloseIndex = absolute;
}

template <typename T>
size_t SharedDataStream<T>::Reader::getId() const {
    return m_id;
}

template <typename T>
size_t SharedDataStream<T>::Reader::getWordSize() const {
    return m_bufferLayout->getHeader()->wordSize;
}

template <typename T>
std::string SharedDataStream<T>::Reader::errorToString(Error error) {
    switch (error) {
        case Error::CLOSED:
            return "CLOSED";
        case Error::OVERRUN:
            return "OVERRUN";
        case Error::WOULDBLOCK:
            return "WOULDBLOCK";
        case Error::TIMEDOUT:
            return "TIMEDOUT";
        case Error::INVALID:
            return "INVALID";
    }
    logger::acsdkError(logger::LogEntry(TAG, "errorToStringFailed").d("reason", "invalidError").d("error", error));
    return "(unknown error " + to_string(error) + ")";
}

}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_READER_H_
