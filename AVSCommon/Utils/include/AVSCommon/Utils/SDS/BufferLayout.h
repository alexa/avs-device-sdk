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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_BUFFERLAYOUT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_BUFFERLAYOUT_H_

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"
#include "SharedDataStream.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {

/**
 * This is a nested class inside @c SharedDatastream which defines the layout of a @c Buffer for use with a
 * @c SharedDataStream.  This layout begins with a fixed @c Header structure, followed by two arrays of
 * @c Reader @c Indexes, with the remainder allocated to data.  All four of these sections are aligned on a 64-bit
 * boundary.
 */
template <typename T>
class SharedDataStream<T>::BufferLayout {
public:
    /// Magic number used to identify a valid Header in memory.
    static const uint32_t MAGIC_NUMBER = 0x53445348;

    /// Version of this header layout.
    static const uint32_t VERSION = 2;

    /**
     * The constructor only initializes a shared pointer to the provided buffer.  Attaching and/or initializing is
     * performed by the @c init()/@c attach() functions.
     *
     * @param buffer The raw buffer which holds (or will hold) the header, arrays, and circular data buffer.
     */
    BufferLayout(std::shared_ptr<Buffer> buffer);

    /// The destructor ensures the BufferLayout is @c detach()es from the Buffer.
    ~BufferLayout();

    /**
     * This structure defines the header fields for the @c Buffer.  The header fields in the shared @c Buffer are the
     * mechanism by which SDS instances in different processes share state.  When initializing a new @c Buffer, this
     * struct must be placement-constructed at the base of the @c Buffer.  When accessing a pre-initialized @c Buffer,
     * this struct must be reinterpret_cast from the base of the @c Buffer.
     */
    struct Header {
        /**
         * This field contains the magic number which is used to identify a valid @c Header.  For a
         * @c SharedDataStream::buffer to be valid, this field must contain @c MAGIC_NUMBER.
         */
        uint32_t magic;

        /**
         * This field specifies the version of the header layout used for the subsequent fields.  For a
         * @c SharedDataStream to use a buffer, this field must match @c VERSION.
         */
        uint8_t version;

        /**
         * This field contains a stable hash of the @c T::traitsName string from the @c SharedDataStream::create()
         * call.  For a @c SharedDataStream to @c open() a buffer, this field must match the caller's @c T::traitsName
         * string hash.
         */
        uint32_t traitsNameHash;

        /**
         * This field specifies the word size (in bytes).  All SharedDataStream operations that work with data or
         * position in the stream are quantified in words.
         */
        uint16_t wordSize;

        /**
         * This field specifies the maximum number of @c Readers.
         *
         * @note This value determines the size of the three reader arrays that follow the @c Header in the @c Buffer.
         */
        uint8_t maxReaders;

        /// This field contains the condition variable used to notify @c Readers that data is available.
        ConditionVariable dataAvailableConditionVariable;

        /// This field contains the mutex used by @c dataAvailableConditionVariable.
        Mutex dataAvailableMutex;

        /**
         * This field contains the condition variable used to notify @c Writers that space is available.  Note that
         * this condition variable does not have a dedicated mutex; the condition is protected by backwardSeekMutex.
         */
        ConditionVariable spaceAvailableConditionVariable;

        /**
         * This field contains a mutex used to temporarily hold off @c Readers from seeking backwards in the buffer
         * while a @c Reader is updating @c oldestUnconsumedCursor.  The is necessary to prevent a race condition where
         * a @c Reader::seek() at the same time that a @c Reader is updating @c oldestUnconsumedCursor could result in
         * a @c Reader which is reading from an older @c Index than @c oldestUnconsumedCursor.
         */
        Mutex backwardSeekMutex;

        /// This field indicates whether there is an enabled (not closed) @c Writer.
        AtomicBool isWriterEnabled;

        /// This field indicates that a @c Writer had at one point been enabled and then closed.
        AtomicBool hasWriterBeenClosed;

        /**
         * This mutex is used to protect creation of the writer.  In particular, it is locked when attempting to add
         * the writer so that there are no races between overlapping calls to @c createWriter().
         */
        Mutex writerEnableMutex;

        /// This field contains the next location to write to.
        AtomicIndex writeStartCursor;

        /**
         * This field contains the end of the region currently being written to (when no write is in progress,
         * `(writEndCursor == writeStartCursor)`).
         */
        AtomicIndex writeEndCursor;

        /**
         * This field contains the location of oldest word in the buffer which has not been consumed (read by a
         * @c Reader).  This field is used as a barrier by @c Writers which have a policy not to overwrite readers.
         */
        AtomicIndex oldestUnconsumedCursor;

        /// This field tracks the number of BufferLayout instances currently attached to a Buffer.
        uint32_t referenceCount;

        /**
         * This mutex protects attaching/detatching BufferLayout instances to a Buffer.  Specifically, it must be held
         * when updating referenceCount, and while cleaning up after referenceCount reaches zero.
         */
        Mutex attachMutex;

        /**
         * This mutex is used to protect creation of readers.  In particular, it is locked when attempting to add a new
         * reader so that there are no races between overlapping calls to @c createReader().
         */
        Mutex readerEnableMutex;
    };

    /**
     * This function provides access to the Header structure stored at the start of the @c Buffer.
     *
     * @return A pointer to the @c Header structure inside the @c Buffer.
     */
    Header* getHeader() const;

    /**
     * This function provides access to the array of booleans which specify whether a particular reader is enabled.
     *
     * This array of enabled booleans comes next in @c m_buffer after the @c Header.
     *
     * @return A pointer to the array of @c maxReaders cursor @c Indexes.
     */
    AtomicBool* getReaderEnabledArray() const;

    /**
     * This function provides access to the array of indices which specify the location each @c Reader will read from.
     *
     * This array of cursor indices comes next in @c m_buffer after the @c getReaderEnabledArray() listed above.
     *
     * @return A pointer to the array of @c maxReaders cursor @c Indexes.
     */
    AtomicIndex* getReaderCursorArray() const;

    /**
     * This function provides access to the array of indices which specify the @c Index where each @c Reader
     * will stop reading.  When a @c Reader's close @c Index is set to zero, this indicates that the @c Reader is
     * disabled.  When a @c Reader's close @c Index is less than or equal to the @c Reader's cursor, this
     * indicates that the @c Reader has reached the end of its stream, and subsequent calls to @c Reader::read() will
     * return 0.  When a @c Reader's close @c Index is greater than the @c Reader's cursor, the @c Reader will continue
     * to return data as it becomes available in the stream up to the point when it reaches the close @c Index.
     *
     * @note An enabled @c Reader will initially have its close @c Index set to
     *     @c std::numeric_limits_max<AtomicIndex>, meaning it can effectively continue to read indefinitely, until the
     *     @c Writer closes.  If a call to @c Reader::close() occurs, it will move the @c Reader's close @c Index to
     *     the current write start cursor, which will cause the read stream to end when the @c Reader finishes
     *     consuming the data that was in the buffer at the time @c Reader::close() was called.
     *
     * This array of close @c Index indices comes next in @c m_buffer after the @c getReaderCursorArray() listed above.
     *
     * @return A pointer to the array of @c maxReaders close @c Indexes.
     */
    AtomicIndex* getReaderCloseIndexArray() const;

    /**
     * This function returns the size (in words) of the data (non-Header) portion of @c buffer.  The data comes next in
     * @c m_buffer after the @c getReaderCloseIndexarray() listed above.
     *
     * @return The maximum number of words the stream can store.
     */
    Index getDataSize() const;

    /**
     * This function provides access to the data (non-Header) portion of @c buffer.
     *
     * The data comes next in @c m_buffer after the @c getReaderCloseIndexArray() array listed above.
     *
     * @param at An optional word @c Index to get a data pointer for.  This function will calculate where @c at would
     *     fall in the circular buffer and return a pointer to it, but note that this function does not check whether
     *     the specified @c Index currently resides in the buffer.  This parameter defaults to 0, which will return a
     *     pointer to the base of the circular data buffer.
     * @return A pointer to the data which would hold the specified @c Index.
     */
    uint8_t* getData(Index at = 0) const;

    /**
     * This function initalizes the @c Header and arrays in the @c Buffer managed by this @c BufferLayout.
     * This function must not be called on a @c BufferLayout which is managing a @c Buffer which has already been
     * initialized.  This function must return before attempting to @c attach() another @c BufferLayout to the
     * @c Buffer being initialized here.  This function must not be called if the @c Buffer it would initialized has
     * other @c BufferLayouts attached to it.
     *
     * @param wordSize The size (in bytes) of words in the stream.  All @c SharedDataStream operations that work with
     *     data or position in the stream are quantified in words.
     * @param maxReaders The maximum number of readers the stream will support.
     * @return @c false if wordSize or maxReaders are too large to be stored, else @c true.
     */
    bool init(size_t wordSize, size_t maxReaders);

    /**
     * This function tries to attach this @c BufferLayout to a @c Buffer which was already initialized by another
     * @c BufferLayout.  This function will verify that the @c Buffer contains a valid header which is compatible with
     * this @c Buffer's traits.  This function can be safely called from multiple threads or processes after another
     * another @c BufferLayout which is managing the same @c Buffer has completed a call to @c init().
     *
     * @return @c true if successfully attached (@c Buffer contains a valid and compatible header), else @c false.
     */
    bool attach();

    /**
     * This function detaches from the @c Buffer which it is managing.  If this is the last @c BufferLayout attached to
     * the @c Buffer, it will also call the destructors on the objects in the @c Buffer.
     */
    void detach();

    /**
     * This function checks whether the specified reader is enabled.
     *
     * @note This function does not require the caller to hold Header::readerEnableMutex.  Reading the enabled flag is
     *     an atomic operation in and of itself.  It is up to the caller to determine whether there are subsequent
     *     operations which depend on the enabled state that might require holding the mutex to avoid a race condition.
     *
     * @param id The id of the reader to check the enabled status of.
     * @return @c true if the specified reader is enabled, else @c false.
     */
    bool isReaderEnabled(size_t id) const;

    /**
     * This function enables the specified reader.  The caller must be holding Header::readerEnableMutex when calling
     * this function.
     *
     * @param id The id of the reader to enable.
     */
    void enableReaderLocked(size_t id);

    /**
     * This function disables the specified reader.  The caller must be holding Header::readerEnableMutex when calling
     * this function.
     *
     * @param id The id of the reader to disable.
     */
    void disableReaderLocked(size_t id);

    /**
     * This function returns a count of the number of words after the specified @c Index before the circular data
     * will wrap.
     *
     * @c param after The @c Index to count from.
     * @c return The count of words after @c after until the circular data will wrap.
     */
    Index wordsUntilWrap(Index after) const;

    /**
     * This function calculates the offset (in bytes) from the start of a @c Buffer to the start of the circular data.
     *
     * @param wordSize The size (in bytes) of words in the stream.  All @c SharedDataStream operations that work with
     *     data or position in the stream are quantified in words.
     * @param maxReaders The maximum number of readers the stream will support.
     * @return The offset (in bytes) from the start of a @c Buffer to the start of the circular data.
     */
    static size_t calculateDataOffset(size_t wordSize, size_t maxReaders);

    /// This function calls @c updateOldestUnconsumedCursorLocked() while holding @c Header::backwardSeekMutex.
    void updateOldestUnconsumedCursor();

    /**
     * This function scans through the array of @c Reader cursors, finds the oldest enabled cursor, and records it in
     * @c oldestUnconsumedCursor.  This function should be called whenever a @c Reader moves its cursor.  This
     * function needs to guarantee that no @c Reader cursors are older than @c oldestUnconsumedCursor when it
     * completes, so it holds @c backwardSeekMutex to prevent race conditions.  This function must be called while
     * holding @c Header::backwardSeekMutex.
     *
     * @note Without mutexes, any @c Reader::read() or @c Reader::seek() call could move a read cursor while this
     *     function is iterating through the list of @c Readers.  The @c Reader::read() calls will always move their
     *     cursors to a newer location, so no mutex is needed (if this function looks at a particular @c Reader and
     *     updates @c oldestUnconsumedCursor, and the Reader later @c Reader::read()s before this function finishes,
     *     @c oldestUnconsumedCursor will still be older than that @c Reader, so there is no problem).  Similarly,
     *     @c Reader::seek() calls which move the cursor to a newer location are not a problem, but @c Reader::seek()
     *     calls which move a cursor backwards pose a risk of a race condition.  If this function looks at a particular
     *     @c Reader, and the @c Reader does a backwards @c Reader::seek() after it has been examined, this function
     *     could end up with @c oldestUnconsumedCursor at an @c Index which is ahead of the @c Reader that performed
     *     the backwards @c Reader::seek().  To prevent this race condition, this function takes the
     *     @c backwardSeekMutex, which prevents backwards @c Reader::seek()s while @c oldestUnconsumedCursor is being
     *     updated.
     *
     * @note As an optimization, we could skip this function if Writer policy is nonblockable (ACSDK-251).
     */
    void updateOldestUnconsumedCursorLocked();

private:
    /**
     * This function calculates a 32-bit stable hash of the provided string.  Note that this hash is just used for
     * basic verification, and does not need to meet stringent security requirements.
     *
     * @param string A null-terminatad ascii string to hash.
     * @return The hash code of @c string.
     */
    static uint32_t stableHash(const char* string);

    /**
     * This function rounds @c size up to a multiple of @c align.
     *
     * @param size The size to round up.
     * @param align The alignment to round to.
     * @return @c size rounded up to a multiple of @c align.
     */
    static size_t alignSizeTo(size_t size, size_t align);

    /**
     * This function calculates the offset (in bytes) from the start of a @c Buffer to the start of the @c Reader
     * enabled array.
     *
     * @return The offset (in bytes) from the start of a @c Buffer to the start of the @c Reader enabled array.
     */
    static size_t calculateReaderEnabledArrayOffset();

    /**
     * This function calculates the offset (in bytes) from the start of a @c Buffer to the start of the @c Reader
     * cursor array.
     *
     * @param maxReaders The maximum number of readers the stream will support.
     * @return The offset (in bytes) from the start of a @c Buffer to the start of the @c Reader cursor array.
     */
    static size_t calculateReaderCursorArrayOffset(size_t maxReaders);

    /**
     * This function calculates the offset (in bytes) from the start of a @c Buffer to the start of the @c Reader
     * close @c Index array.
     *
     * @param maxReaders The maximum number of readers the stream will support.
     * @return The offset (in bytes) from the start of a @c Buffer to the start of the @c Reader close @c Index array.
     */
    static size_t calculateReaderCloseIndexArrayOffset(size_t maxReaders);

    /**
     * This function calculates several frequently-accessed constants and caches them in member variables.
     *
     * @param wordSize The size (in bytes) of words in the stream.  All @c SharedDataStream operations that work with
     *     data or position in the stream are quantified in words.
     * @param maxReaders The maximum number of readers the stream will support.
     */
    void calculateAndCacheConstants(size_t wordSize, size_t maxReaders);

    /**
     * The tag associated with log entries from this class.
     */
    static const std::string TAG;

    /**
     * This function reports whether this SDS is attached to its @c Buffer.
     *
     * @return @c true if this SDS is attached to its @c Buffer, else @c false.
     */
    bool isAttached() const;

    /// The @c Buffer used to store the stream's header and data.
    std::shared_ptr<Buffer> m_buffer;

    /// Precalculated pointer to the @c Reader enabled array.
    AtomicBool* m_readerEnabledArray;

    /// Precalculated pointer to the @c Reader cursor array.
    AtomicIndex* m_readerCursorArray;

    /// Precalculated pointer to the @c Reader close @c Index array.
    AtomicIndex* m_readerCloseIndexArray;

    /// Precalculated size (in words) of the circular data.
    Index m_dataSize;

    /// Precalculated pointer to the circular data.
    uint8_t* m_data;
};

template <typename T>
const std::string SharedDataStream<T>::BufferLayout::TAG = "SdsBufferLayout";

template <typename T>
SharedDataStream<T>::BufferLayout::BufferLayout(std::shared_ptr<Buffer> buffer) :
        m_buffer{buffer},
        m_readerEnabledArray{nullptr},
        m_readerCursorArray{nullptr},
        m_readerCloseIndexArray{nullptr},
        m_dataSize{0},
        m_data{nullptr} {
}

template <typename T>
SharedDataStream<T>::BufferLayout::~BufferLayout() {
    detach();
}

template <typename T>
typename SharedDataStream<T>::BufferLayout::Header* SharedDataStream<T>::BufferLayout::getHeader() const {
    return reinterpret_cast<Header*>(m_buffer->data());
}

template <typename T>
typename SharedDataStream<T>::AtomicBool* SharedDataStream<T>::BufferLayout::getReaderEnabledArray() const {
    return m_readerEnabledArray;
}

template <typename T>
typename SharedDataStream<T>::AtomicIndex* SharedDataStream<T>::BufferLayout::getReaderCursorArray() const {
    return m_readerCursorArray;
}

template <typename T>
typename SharedDataStream<T>::AtomicIndex* SharedDataStream<T>::BufferLayout::getReaderCloseIndexArray() const {
    return m_readerCloseIndexArray;
}

template <typename T>
typename SharedDataStream<T>::Index SharedDataStream<T>::BufferLayout::getDataSize() const {
    return m_dataSize;
}

template <typename T>
uint8_t* SharedDataStream<T>::BufferLayout::getData(Index at) const {
    return m_data + (at % getDataSize()) * getHeader()->wordSize;
}

template <typename T>
bool SharedDataStream<T>::BufferLayout::init(size_t wordSize, size_t maxReaders) {
    // Make sure parameters are not too large to store.
    if (wordSize > std::numeric_limits<decltype(Header::wordSize)>::max()) {
        logger::acsdkError(logger::LogEntry(TAG, "initFailed")
                               .d("reason", "wordSizeTooLarge")
                               .d("wordSize", wordSize)
                               .d("wordSizeLimit", std::numeric_limits<decltype(Header::wordSize)>::max()));
        return false;
    }
    if (maxReaders > std::numeric_limits<decltype(Header::maxReaders)>::max()) {
        logger::acsdkError(logger::LogEntry(TAG, "initFailed")
                               .d("reason", "maxReadersTooLarge")
                               .d("maxReaders", maxReaders)
                               .d("maxReadersLimit", std::numeric_limits<decltype(Header::maxReaders)>::max()));
        return false;
    }

    // Pre-calculate some pointers and sizes that are frequently accessed.
    calculateAndCacheConstants(wordSize, maxReaders);

    // Default construction of the Header.
    auto header = new (getHeader()) Header;

    // Default construction of the reader arrays.
    size_t id;
    for (id = 0; id < maxReaders; ++id) {
        new (m_readerEnabledArray + id) AtomicBool;
        new (m_readerCursorArray + id) AtomicIndex;
        new (m_readerCloseIndexArray + id) AtomicIndex;
    }

    // Header field initialization.
    header->magic = MAGIC_NUMBER;
    header->version = VERSION;
    header->traitsNameHash = stableHash(T::traitsName);
    header->wordSize = wordSize;
    header->maxReaders = maxReaders;
    header->isWriterEnabled = false;
    header->hasWriterBeenClosed = false;
    header->writeStartCursor = 0;
    header->writeEndCursor = 0;
    header->oldestUnconsumedCursor = 0;
    header->referenceCount = 1;

    // Reader arrays initialization.
    for (id = 0; id < maxReaders; ++id) {
        m_readerEnabledArray[id] = false;
        m_readerCursorArray[id] = 0;
        m_readerCloseIndexArray[id] = 0;
    }

    return true;
}

template <typename T>
bool SharedDataStream<T>::BufferLayout::attach() {
    // Verify compatibility.
    auto header = getHeader();
    if (header->magic != MAGIC_NUMBER) {
        logger::acsdkError(logger::LogEntry(TAG, "attachFailed")
                               .d("reason", "magicNumberMismatch")
                               .d("magicNumber", header->magic)
                               .d("expectedMagicNumber", std::to_string(MAGIC_NUMBER)));
        return false;
    }
    if (header->version != VERSION) {
        logger::acsdkError(logger::LogEntry(TAG, "attachFailed")
                               .d("reason", "incompatibleVersion")
                               .d("version", header->version)
                               .d("expectedVersion", std::to_string(VERSION)));
        return false;
    }
    if (header->traitsNameHash != stableHash(T::traitsName)) {
        logger::acsdkError(logger::LogEntry(TAG, "attachFailed")
                               .d("reason", "traitsNameHashMismatch")
                               .d("hash", header->traitsNameHash)
                               .d("expectedHash", stableHash(T::traitsName)));
        return false;
    }

    // Attach.
    std::lock_guard<Mutex> lock(header->attachMutex);
    if (0 == header->referenceCount) {
        logger::acsdkError(logger::LogEntry(TAG, "attachFailed").d("reason", "zeroUsers"));
        return false;
    }
    if (std::numeric_limits<decltype(header->referenceCount)>::max() == header->referenceCount) {
        logger::acsdkError(logger::LogEntry(TAG, "attachFailed")
                               .d("reason", "bufferMaxUsersExceeded")
                               .d("numUsers", header->referenceCount)
                               .d("maxNumUsers", std::numeric_limits<decltype(header->referenceCount)>::max()));
        return false;
    }
    ++header->referenceCount;

    // Pre-calculate some pointers and sizes that are frequently accessed.
    calculateAndCacheConstants(header->wordSize, header->maxReaders);

    return true;
}

template <typename T>
void SharedDataStream<T>::BufferLayout::detach() {
    if (!isAttached()) {
        return;
    }

    auto header = getHeader();
    std::lock_guard<Mutex> lock(header->attachMutex);
    --header->referenceCount;
    if (header->referenceCount > 0) {
        return;
    }

    // Destruction of reader arrays.
    for (size_t id = 0; id < header->maxReaders; ++id) {
        m_readerCloseIndexArray[id].~AtomicIndex();
        m_readerCursorArray[id].~AtomicIndex();
        m_readerEnabledArray[id].~AtomicBool();
    }

    // Destruction of the Header.
    header->~Header();
}

template <typename T>
bool SharedDataStream<T>::BufferLayout::isReaderEnabled(size_t id) const {
    return m_readerEnabledArray[id];
}

template <typename T>
void SharedDataStream<T>::BufferLayout::enableReaderLocked(size_t id) {
    m_readerEnabledArray[id] = true;
}

template <typename T>
void SharedDataStream<T>::BufferLayout::disableReaderLocked(size_t id) {
    m_readerEnabledArray[id] = false;
}

template <typename T>
typename SharedDataStream<T>::Index SharedDataStream<T>::BufferLayout::wordsUntilWrap(Index after) const {
    return alignSizeTo(after, getDataSize()) - after;
}

template <typename T>
size_t SharedDataStream<T>::BufferLayout::calculateDataOffset(size_t wordSize, size_t maxReaders) {
    return alignSizeTo(calculateReaderCloseIndexArrayOffset(maxReaders) + (maxReaders * sizeof(AtomicIndex)), wordSize);
}

template <typename T>
void SharedDataStream<T>::BufferLayout::updateOldestUnconsumedCursor() {
    // Note: as an optimization, we could skip this function if Writer policy is nonblockable (ACSDK-251).
    std::lock_guard<Mutex> backwardSeekLock(getHeader()->backwardSeekMutex);
    updateOldestUnconsumedCursorLocked();
}

template <typename T>
void SharedDataStream<T>::BufferLayout::updateOldestUnconsumedCursorLocked() {
    auto header = getHeader();

    // Note: as an optimization, we could skip this function if Writer policy is nonblockable (ACSDK-251).

    // The only barrier to a blocking writer overrunning a reader is oldestUnconsumedCursor, so we have to be careful
    // not to ever move it ahead of any readers.  The loop below searches through the readers to find the oldest point,
    // without moving oldestUnconsumedCursor.  Note that readers can continue to read while we are looping; it means
    // oldest may not be completely accurate, but it will always be older than the readers because they are reading
    // away from it.  Also note that backwards seeks (which would break the invariant) are prevented with a mutex which
    // is held while this function is called.  Also note that all read cursors may be in the future, so we start with
    // an unlimited barrier and work back from there.
    Index oldest = std::numeric_limits<Index>::max();
    for (size_t id = 0; id < header->maxReaders; ++id) {
        // Note that this code is calling isReaderEnabled() without holding readerEnableMutex.  On the surface, this
        // appears to be a race condition because a reader may be disabled and/or re-enabled before the subsequent code
        // reads the cursor, but it turns out to be safe because:
        // - if a reader is enabled, its cursor is valid
        // - if a reader becomes disabled, its cursor moves to writeCursor (which will never be the oldest)
        // - if a reader becomes re-enabled, its cursor defaults to writeCursor (which will never be the oldest)
        // - if a reader is created that wants to be at an older index, it gets there by doing a backward seek (which
        //   is locked when this function is called)
        if (isReaderEnabled(id) && getReaderCursorArray()[id] < oldest) {
            oldest = getReaderCursorArray()[id];
        }
    }

    // If no barrier was found, block at the write cursor so that we retain data until a reader comes along to read it.
    if (std::numeric_limits<Index>::max() == oldest) {
        oldest = header->writeStartCursor;
    }

    // Now that we've measured the oldest cursor, we can safely update oldestUnconsumedCursor with no risk of an
    // overrun of any readers.

    // To clarify the logic here, the code above reviewed all of the enabled readers to see where the oldest cursor is
    // at.  This value is captured in the 'oldest' variable.  Now we want to move up our writer barrier
    // ('oldestUnconsumedCursor') if it is older than it needs to be.
    if (oldest > header->oldestUnconsumedCursor) {
        header->oldestUnconsumedCursor = oldest;

        // Notify the writer(s).
        // Note: as an optimization, we could skip this if there are no blocking writers (ACSDK-251).
        header->spaceAvailableConditionVariable.notify_all();
    }
}

template <typename T>
uint32_t SharedDataStream<T>::BufferLayout::stableHash(const char* string) {
    // Simple, stable hash which XORs all bytes of string into the hash value.
    uint32_t hashed = 0;
    size_t pos = 0;
    while (*string) {
        hashed ^= *string << ((pos % sizeof(uint32_t)) * 8);
        ++string;
        ++pos;
    }
    return hashed;
}

template <typename T>
size_t SharedDataStream<T>::BufferLayout::alignSizeTo(size_t size, size_t align) {
    if (size) {
        return (((size - 1) / align) + 1) * align;
    } else {
        return 0;
    }
}

template <typename T>
size_t SharedDataStream<T>::BufferLayout::calculateReaderEnabledArrayOffset() {
    return alignSizeTo(sizeof(Header), alignof(AtomicBool));
}

template <typename T>
size_t SharedDataStream<T>::BufferLayout::calculateReaderCursorArrayOffset(size_t maxReaders) {
    return alignSizeTo(calculateReaderEnabledArrayOffset() + (maxReaders * sizeof(AtomicBool)), alignof(AtomicIndex));
}

template <typename T>
size_t SharedDataStream<T>::BufferLayout::calculateReaderCloseIndexArrayOffset(size_t maxReaders) {
    return calculateReaderCursorArrayOffset(maxReaders) + (maxReaders * sizeof(AtomicIndex));
}

template <typename T>
void SharedDataStream<T>::BufferLayout::calculateAndCacheConstants(size_t wordSize, size_t maxReaders) {
    auto buffer = reinterpret_cast<uint8_t*>(m_buffer->data());
    m_readerEnabledArray = reinterpret_cast<AtomicBool*>(buffer + calculateReaderEnabledArrayOffset());
    m_readerCursorArray = reinterpret_cast<AtomicIndex*>(buffer + calculateReaderCursorArrayOffset(maxReaders));
    m_readerCloseIndexArray = reinterpret_cast<AtomicIndex*>(buffer + calculateReaderCloseIndexArrayOffset(maxReaders));
    m_dataSize = (m_buffer->size() - calculateDataOffset(wordSize, maxReaders)) / wordSize;
    m_data = buffer + calculateDataOffset(wordSize, maxReaders);
}

template <typename T>
bool SharedDataStream<T>::BufferLayout::isAttached() const {
    return m_data != nullptr;
}

}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDS_BUFFERLAYOUT_H_
