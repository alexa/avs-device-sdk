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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_SHAREDDATASTREAM_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_SHAREDDATASTREAM_H_

#include <cstdint>
#include <cstddef>
#include <memory>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {

/**
 * Class for streaming data from a single producer (@c Writer) to multiple consumers (@c Reader).  This class
 * implements streaming in a generic manner, and utilizes template traits to decouple from platform specifics related
 * to how memory and state are shared/sychronized between the readers and writers.  Care must be taken in choosing
 * appropriate types for the template parameters which will work reliably for the execution environment where the
 * @c SharedDataStream will be used.
 *
 * @tparam T::AtomicIndex An atomic version of @c Index (see below) which implements the following methods:
 *     @li @c DefaultConstructible `(std::is_default_constructible<AtomicIndex> == true)`.
 *     @li Basic arithmetic, conversion and assignment operations with @c Index.
 *     @li @c load() performs an atomic read of the @c Index.
 *     @li If the stream will be shared between processes, the @c AtomicIndex type *must* be a PODType:
 *         `(std::is_pod<AtomicIndex> == true)`.
 *
 *     This should be an equivalent type to @c Index, but which ensures atomic reads and writes between readers and
 *     writers in the execution environment where the @c SharedDataStream will be used.  No methods are called on this
 *     type; it must simply be readable and writable with values of type @c Index or @c AtomicIndex.
 *
 * @tparam T::AtomicBool An atomic boolean type which implements the following methods:
 *     @li @c DefaultConstructible `(std::is_default_constructible<AtomicIndex> == true)`.
 *     @li Basic conversion and assignment operations with @c bool.
 *     @li If the stream will be shared between processes, the @c AtomicBool type *must* be a PODType:
 *         `(std::is_pod<AtomicIndex> == true)`.
 *
 *     This should be an equivalent type to @c bool, but which ensures atomic reads and writes between readers and
 *     writers in the execution environment where the @c SharedDataStream will be used.  No methods are called on this
 *     type; it must simply be readable and writable with values of type @c bool or @c AtomicBool.
 *
 * @tparam T::Buffer A @c Container type which stores a contiguous block of data and implements the following methods:
 *     @li @c data() provides access to the underlying storage array.
 *     @li @c size() returns the size (in bytes) of the buffer.
 *
 *     This type must be capable of sharing data between readers and writers in the execution environment where the
 *     @c SharedDataStream will be used.
 *
 * @tparam T::Mutex A @c Mutex type which implements the following methods:
 *     @li @c DefaultConstructible `(std::is_default_constructible<Mutex> == true)`.
 *     @li @c lock() Waits indefinitely for the mutex to unlock and then locks the mutex.
 *     @li @c unlock Unlocks the mutex.
 *     @li If the stream will be shared between processes, the @c Mutex type *must* be a @c PODType:
 *         `(std::is_pod<Mutex> == true)`.
 *
 *     This type must be capable of locking a mutex for readers and writers in the execution environment where the
 *     @c SharedDataStream will be used.
 *
 * @tparam T::ConditionVariable A condition variable type which works with @c Mutex and implements the following
 *     methods:
 *     @li @c DefaultConstructible `(std::is_default_constructible<ConditionVariable> == true)`.
 *     @li @c notify_all() unblocks any threads waiting for this condition variable.
 *     @li @c wait(lock) waits indefinitely for @c lock.
 *     @li @c wait(lock, predicate) waits indefinitely using @c lock for the @c predicate to be satisfied.
 *     @li @c wait_for(lock, timeout, predicate) waits using @c lock up to the specified @c timeout for the @c
 *         predicate to be satisfied.
 *     @li If the stream will be shared between processes, the @c ConditionVariable type *must* be a PODType:
 *         `(std::is_pod<ConditionVariable> == true)`.
 *
 *     This type must be capable of synchronizing between readers and writers in the execution environment where the
 *     @c SharedDataStream will be used.
 *
 * @tparam T::traitsName A unique string value which describes the collection of traits specified by T.  This string
 *     is used to ensure that a SharedDataStream attempting to open() a buffer is using the same set of traits that
 *     were originally used to create() the buffer.
 */
template <typename T>
class SharedDataStream {
private:
    // Forward declare the nested @c BufferLayout structure (full declaration is in @c BufferLayout.h).
    class BufferLayout;

public:
    /**
     * An unsigned, integral type used to represent indexes in the stream.  @c SharedDataStream does not check for
     * @c Index wrapping, so this type must be large enough to guarantee that no wrapping will occur over the practical
     * lifespan of a @c SharedDataStream instance.
     */
    using Index = uint64_t;

    /// An atomic version of @c Index.
    using AtomicIndex = typename T::AtomicIndex;

    /// An atomic version of @c bool.
    using AtomicBool = typename T::AtomicBool;

    /// A @c Container type which stores a contiguous block of data.
    using Buffer = typename T::Buffer;

    /// A @c Mutex type.
    using Mutex = typename T::Mutex;

    /// A condition variable type which works with @c Mutex.
    using ConditionVariable = typename T::ConditionVariable;

    // Forward declare the nested @c Reader class (full declaration is in @c Reader.h).
    class Reader;

    // Forward declare the nested @c Writer class (full declaraion is in @c Writer.h).
    class Writer;

    /**
     * This function calculates the buffer size needed to support a @c SharedDataStream with the specified parameters.
     * This function can be safely called from multiple threads or processes.
     *
     * @param nWords The number of data words the stream will be able to hold.
     * @param wordSize The size (in bytes) of words in the stream.  All @c SharedDataStream operations that work with
     *     data or position in the stream are quantified in words.  The stream's data storage capacity in bytes is
     *     `nWords * wordSize`.  This parameter defaults to 1.
     * @param maxReaders The maximum number of readers the stream will support.  This parameter defaults to 1.
     * @return The buffer size (in bytes) required to support the specified parameters, or zero if parameters are
     *     invalid.
     */
    static size_t calculateBufferSize(size_t nWords, size_t wordSize = 1, size_t maxReaders = 1);

    /**
     * This function creates a new @c SharedDataStream.  It will first verify that the @c Buffer is large enough to
     * hold the stream's header, and will then proceed to initialize the header fields.  This function must
     * not be called more than once on @c buffer.  This function must return before attempting to @c open() @c buffer
     * with another @c SharedDataStream.
     *
     * @param buffer The @c Buffer which this stream will use to store its header and stream data.  Existing contents
     *     of @c buffer will be overwritten by this function.  Note that this function will fail if @c buffer is not
     *     large enough to hold the header or is not a multiple of @c wordSize (see @c calculateBufferSize()).
     * @param wordSize The size (in bytes) of words in the stream.  All @c SharedDataStream operations that work with
     *     data or position in the stream are quantified in words.  This parameter defaults to 1.
     * @param maxReaders The maximum number of readers the stream will support.  This parameter defaults to 1.
     * @return The new stream if @c buffer was successfully initialized, else @c nullptr.
     */
    static std::unique_ptr<SharedDataStream> create(
        std::shared_ptr<Buffer> buffer,
        size_t wordSize = 1,
        size_t maxReaders = 1);

    /**
     * This function creates a new @c SharedDataStream using a preinitialized @c Buffer.  This allows a stream to
     * attach to a @c Buffer shared with another pre-existing stream.  This function will verify that @c buffer
     * contains a valid header which is compatible with this stream's traits.  This function can be safely called from
     * multiple threads or processes after a call to @c create() on @c buffer has completed.
     *
     * @param buffer The @c Buffer which this stream will use to store its header and stream data.  Existing contents
     *     of @c buffer will not be overwritten by this function.
     * @return The new stream if @c buffer contains a valid and compatible header, else @c nullptr.
     */
    static std::unique_ptr<SharedDataStream> open(std::shared_ptr<Buffer> buffer);

    /**
     * This function reports the maximum number of readers supported by this @c SharedDataStream.  This function can be
     * safely called from multiple threads or processes.
     *
     * @return The maximum number of @c Readers supported.
     */
    size_t getMaxReaders() const;

    /**
     * This function returns the number of data words the stream is able to hold.  This function can be safely called
     * from multiple threads or processes.
     *
     * @return the number of data words the stream is able to hold.
     */
    Index getDataSize() const;

    /**
     * This function returns the word size (in bytes).  All @c SharedDataStream operations that work with data or
     * position in the stream are quantified in words.
     *
     * @return The size (in bytes) of words for this @c SharedDataStream.
     */
    size_t getWordSize() const;

    /**
     * This function creates a @c Writer to the stream.  Only one @c Writer is allowed at a time.  This function must
     * not be called from multiple threads or processes.
     *
     * @param policy The policy to use for writing to the stream.
     * @param forceReplacement If set to @c true, this parameter forcefully deallocates the @c Writer before allocating
     *     a new one. This parameter can be used when a Writer is known to be unused (possibly due to a process crash),
     *     but was not cleanly destroyed.  This parameter is set to @c false by default.
     * @return @c nullptr if a @c Writer is already allocated, else the new @c Writer.
     *
     * @warning Calling this function with `forceReplacement = true` will allow the call to @c createWriter() to
     *     succeed, but will not prevent an previously existing @c Writer from writing to the stream.  The
     *     `forceReplacement = true` option should only be used when higher level software can guarantee that the
     *     previous @c Writer will no longer be used to attempt to write to the stream.
     */
    std::unique_ptr<Writer> createWriter(typename Writer::Policy policy, bool forceReplacement = false);

    /**
     * This function adds a @c Reader to the stream.  Up to @c getMaxReaders() can be added to the stream.  This
     * function can be safely called from multiple threads or processes.
     *
     * @param policy The policy to use for reading from the stream.
     * @param startWithNewData Flag indicating that this @c Reader should start reading data which is written to the
     *     buffer after this @c Reader is created.  If this parameter is set to false, the @c Reader will start with
     *     the oldest valid data in the buffer.  This parameter defaults to false.
     * @return @c nullptr if all @c Reader ids are already allocated, else the new @c Reader.
     */
    std::unique_ptr<Reader> createReader(typename Reader::Policy policy, bool startWithNewData = false);

    /**
     * This function adds a @c Reader to the stream using a specific id.  This function can be safely called from
     * multiple threads or processes.
     *
     * A @c SharedDataStream maintains a fixed-size array of @c getMaxReaders() Readers.  If @c addReader() is called
     * without specifying an id, it will return the first unused @c Reader in the array and assign the corresponding
     * id. If an application wants to directly manage @c Reader ids, it can instead call @c addReader() with a specific
     * id (any value in the range `[0, getMaxReaders())`), and this function will add the @c Reader with that id if it
     * is not already in use.
     *
     * @param id The id to use for this Reader.
     * @param policy The policy to use for reading from the stream.
     * @param startWithNewData Flag indicating that this @c Reader should start reading data which is written to the
     *     buffer after this @c Reader is created.  If this parameter is set to false, the @c Reader will start with
     *     the oldest valid data in the buffer.  This parameter defaults to false.
     * @param forceReplacement If set to @c true, this parameter forcefully deallocates the @c Reader before allocating
     *     a new one. This parameter can be used when a Reader is known to be unused (possibly due to a process crash),
     *     but was not cleanly destroyed.  This parameter is set to @c false by default.
     * @return @c nullptr if the requested Reader id is already allocated, else the new Reader.
     *
     * @warning Calling this function with `forceReplacement = true` will allow the call to @c createReader() to
     *     succeed, but will not prevent an previously existing @c Reader from reading from the stream.  The
     *     `forceReplacement = true` option should only be used when higher level software can guarantee that the
     *     previous @c Reader will no longer be used to attempt to read from the stream.
     */
    std::unique_ptr<Reader> createReader(
        size_t id,
        typename Reader::Policy policy,
        bool startWithNewData = false,
        bool forceReplacement = false);

private:
    /**
     * Constructs a new @c SharedDataStream using the provided @c Buffer.  The constructor does not attempt to
     * initialize or verify the contents of the @c Buffer; that functionality is implemented by @c create() and
     * @c open().
     */
    SharedDataStream(std::shared_ptr<typename T::Buffer> buffer);

    /**
     * This function adds a @c Reader to the stream using a specific id.
     * @cm_bufferLayout->getHeader()->readerEnableMutex must be held while calling this function.
     *
     * A @c SharedDataStream maintains a fixed-size array of @c getMaxReaders() Readers.  If @c addReader() is called
     * without specifying an id, it will return the first unused @c Reader in the array and assign the corresponding
     * id. If an application wants to directly manage @c Reader ids, it can instead call @c addReader() with a specific
     * id (any value in the range `[0, getMaxReaders())`), and this function will add the @c Reader with that id if it
     * is not already in use.
     *
     * @param id The id to use for this Reader.
     * @param policy The policy to use for reading from the stream.
     * @param startWithNewData Flag indicating that this @c Reader should start reading data which is written to the
     *     buffer after this @c Reader is created.  If this parameter is set to false, the @c Reader will start with
     *     the oldest valid data in the buffer.  This parameter defaults to false.
     * @param forceReplacement If set to @c true, this parameter forcefully deallocates the @c Reader before allocating
     *     a new one. This parameter can be used when a Reader is known to be unused (possibly due to a process crash),
     *     but was not cleanly destroyed.
     * @return @c nullptr if the requested Reader id is already allocated, else the new Reader.
     *
     * @warning Calling this function with `forceReplacement = true` will allow the call to @c createReader() to
     *     succeed, but will not prevent an previously existing @c Reader from reading from the stream.  The
     *     `forceReplacement = true` option should only be used when higher level software can guarantee that the
     *     previous @c Reader will no longer be used to attempt to read from the stream.
     */
    std::unique_ptr<Reader> createReaderLocked(
        size_t id,
        typename Reader::Policy policy,
        bool startWithNewData,
        bool forceReplacement,
        std::unique_lock<Mutex>* lock);

    /**
     * The tag associated with log entries from this class.
     */
    static const std::string TAG;

    /// The @c BufferLayout of the shared buffer.
    std::shared_ptr<BufferLayout> m_bufferLayout;
};

template <typename T>
const std::string SharedDataStream<T>::TAG = "SharedDataStream";

template <typename T>
size_t SharedDataStream<T>::calculateBufferSize(size_t nWords, size_t wordSize, size_t maxReaders) {
    if (0 == nWords) {
        logger::acsdkError(logger::LogEntry(TAG, "calculateBufferSizeFailed").d("reason", "numWordsZero"));
        return 0;
    } else if (0 == wordSize) {
        logger::acsdkError(logger::LogEntry(TAG, "calculateBufferSizeFailed").d("reason", "wordSizeZero"));
        return 0;
    }
    size_t overhead = BufferLayout::calculateDataOffset(wordSize, maxReaders);
    size_t dataSize = nWords * wordSize;
    return overhead + dataSize;
}

template <typename T>
std::unique_ptr<SharedDataStream<T>> SharedDataStream<T>::create(
    std::shared_ptr<Buffer> buffer,
    size_t wordSize,
    size_t maxReaders) {
    size_t expectedSize = calculateBufferSize(1, wordSize, maxReaders);
    if (0 == expectedSize) {
        // Logged in calcutlateBuffersize().
        return nullptr;
    } else if (nullptr == buffer) {
        logger::acsdkError(logger::LogEntry(TAG, "createFailed").d("reason", "nullBuffer"));
        return nullptr;
    } else if (expectedSize > buffer->size()) {
        logger::acsdkError(logger::LogEntry(TAG, "createFailed")
                               .d("reason", "bufferSizeTooSmall")
                               .d("bufferSize", buffer->size())
                               .d("expectedSize", expectedSize));
        return nullptr;
    }

    std::unique_ptr<SharedDataStream<T>> sds(new SharedDataStream<T>(buffer));
    if (!sds->m_bufferLayout->init(wordSize, maxReaders)) {
        // Logged in init().
        return nullptr;
    }
    return sds;
}

template <typename T>
std::unique_ptr<SharedDataStream<T>> SharedDataStream<T>::open(std::shared_ptr<Buffer> buffer) {
    std::unique_ptr<SharedDataStream<T>> sds(new SharedDataStream<T>(buffer));
    if (!sds->m_bufferLayout->attach()) {
        // Logged in attach().
        return nullptr;
    } else {
        return sds;
    }
}

template <typename T>
size_t SharedDataStream<T>::getMaxReaders() const {
    return m_bufferLayout->getHeader()->maxReaders;
}

template <typename T>
typename SharedDataStream<T>::Index SharedDataStream<T>::getDataSize() const {
    return m_bufferLayout->getDataSize();
}

template <typename T>
size_t SharedDataStream<T>::getWordSize() const {
    return m_bufferLayout->getHeader()->wordSize;
}

template <typename T>
std::unique_ptr<typename SharedDataStream<T>::Writer> SharedDataStream<T>::createWriter(
    typename Writer::Policy policy,
    bool forceReplacement) {
    auto header = m_bufferLayout->getHeader();
    std::lock_guard<Mutex> lock(header->writerEnableMutex);
    if (header->isWriterEnabled && !forceReplacement) {
        logger::acsdkError(logger::LogEntry(TAG, "createWriterFailed")
                               .d("reason", "existingWriterAttached")
                               .d("forceReplacement", "false"));
        return nullptr;
    } else {
        return std::unique_ptr<Writer>(new Writer(policy, m_bufferLayout));
    }
}

template <typename T>
std::unique_ptr<typename SharedDataStream<T>::Reader> SharedDataStream<T>::createReader(
    typename Reader::Policy policy,
    bool startWithNewData) {
    std::unique_lock<Mutex> lock(m_bufferLayout->getHeader()->readerEnableMutex);
    for (size_t id = 0; id < m_bufferLayout->getHeader()->maxReaders; ++id) {
        if (!m_bufferLayout->isReaderEnabled(id)) {
            return createReaderLocked(id, policy, startWithNewData, false, &lock);
        }
    }
    logger::acsdkError(logger::LogEntry(TAG, "createWriterFailed").d("reason", "noAvailableReaders"));
    return nullptr;
}

template <typename T>
std::unique_ptr<typename SharedDataStream<T>::Reader> SharedDataStream<T>::createReader(
    size_t id,
    typename Reader::Policy policy,
    bool startWithNewData,
    bool forceReplacement) {
    std::unique_lock<Mutex> lock(m_bufferLayout->getHeader()->readerEnableMutex);
    return createReaderLocked(id, policy, startWithNewData, forceReplacement, &lock);
}

template <typename T>
SharedDataStream<T>::SharedDataStream(std::shared_ptr<Buffer> buffer) :
        m_bufferLayout{std::make_shared<BufferLayout>(buffer)} {
}

template <typename T>
std::unique_ptr<typename SharedDataStream<T>::Reader> SharedDataStream<T>::createReaderLocked(
    size_t id,
    typename Reader::Policy policy,
    bool startWithNewData,
    bool forceReplacement,
    std::unique_lock<Mutex>* lock) {
    if (m_bufferLayout->isReaderEnabled(id) && !forceReplacement) {
        logger::acsdkError(logger::LogEntry(TAG, "createReaderLockedFailed")
                               .d("reason", "readerAlreadyAttached")
                               .d("readerId", id)
                               .d("forceReplacement", "false"));
        return nullptr;
    } else {
        // Note: Reader constructor does not call updateUnconsumedCursor() automatically, because we may be seeking to
        // a blocked writer's cursor below (if !startWithNewData), and we don't want the writer to start moving before
        // we seek.
        auto reader = std::unique_ptr<Reader>(new Reader(policy, m_bufferLayout, id));
        lock->unlock();

        if (startWithNewData) {
            // We're not moving the cursor again, so call updateUnconsumedCursor() now.
            m_bufferLayout->updateOldestUnconsumedCursor();
        } else {
            Index offset = m_bufferLayout->getDataSize();
            if (m_bufferLayout->getHeader()->writeStartCursor < offset) {
                offset = m_bufferLayout->getHeader()->writeStartCursor;
            }
            // Note: seek() will call updateUnconsumedCursor().
            if (!reader->seek(offset, Reader::Reference::BEFORE_WRITER)) {
                // Logged in seek().
                return nullptr;
            }
        }
        return reader;
    }
}

}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#include "BufferLayout.h"
#include "Reader.h"
#include "Writer.h"

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_SHAREDDATASTREAM_H_
