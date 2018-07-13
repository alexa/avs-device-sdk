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

/// @file SharedDataStreamTest.cpp

#include <vector>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include <unordered_map>

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Timing/Timer.h"
#include "AVSCommon/Utils/SDS/InProcessSDS.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace sds {
namespace test {

/**
 * Custom traits type suitable for use with SDS which uses custom types for all traits.  This set of traits is
 * used to verify that SDS does not try to use types or functions which were not listed in the template parameter
 * documentation.
 */
struct MinimalTraits {
    /// Forward declare an @c AtomicIndex type.
    class AtomicIndex;
    /// Forward declare an @c AtomicBool type.
    class AtomicBool;
    /// Forward declare a @c Buffer type.
    class Buffer;
    /// Forward declare a @c Mutex type.
    class Mutex;
    /// Forward declare a @c ConditionVariable type.
    class ConditionVariable;
    /// Unique string describing this set of traits.
    static constexpr const char* traitsName = "alexaClientSDK::avsCommon::utils::sds::test::MinimalTraits";
};

/**
 * A second custom traits type which is functionally compatible with @c MinimalTraits, but has a different name.  This
 * set of traits is used to verify that SDS can detect a mismatch in traitsName when opening a buffer initialized by
 * a different SDS.
 */
struct MinimalTraits2 {
    /// Same @c AtomicIndex type as @c MinimalTraits.
    using AtomicIndex = MinimalTraits::AtomicIndex;
    /// Same @c AtomicBool type as @c MinimalTraits.
    using AtomicBool = MinimalTraits::AtomicBool;
    /// Same @c Buffer type as @c MinimalTraits.
    using Buffer = MinimalTraits::Buffer;
    /// Same @c Mutex type as @c MinimalTraits.
    using Mutex = MinimalTraits::Mutex;
    /// Same @c ConditionVariable type as @c MinimalTraits.
    using ConditionVariable = MinimalTraits::ConditionVariable;
    /// Unique string descring this set of traits.  Note that this differes from @c MinimalTraits::traitsName.
    static constexpr const char* traitsName = "alexaClientSDK::avsCommon::utils::sds::test::MinimalTraits2";
};

/// An @c AtomicIndex type with the minimum functionality required by SDS.
class MinimalTraits::AtomicIndex : private InProcessSDS::AtomicIndex {
public:
    /// Conversion to @c Index.
    operator InProcessSDS::Index() {
        return InProcessSDS::AtomicIndex::load();
    }
    /// Read the atomic value.
    InProcessSDS::Index load() {
        return InProcessSDS::AtomicIndex::load();
    }
    /// Assign the atomic value.
    AtomicIndex& operator=(const InProcessSDS::Index& rhs) {
        InProcessSDS::AtomicIndex::operator=(rhs);
        return *this;
    }
    /// Add and assign the atomic value.
    AtomicIndex& operator+=(const InProcessSDS::Index& rhs) {
        InProcessSDS::AtomicIndex::operator+=(rhs);
        return *this;
    }
};

/// An @c AtomicBool type with the minimum functionality required by SDS.
class MinimalTraits::AtomicBool : private InProcessSDS::AtomicBool {
public:
    /// Conversion to bool.
    operator bool() {
        return InProcessSDS::AtomicBool::load();
    }
    // Assign the atomic value.
    AtomicBool& operator=(const bool& rhs) {
        InProcessSDS::AtomicBool::operator=(rhs);
        return *this;
    }
};

/// A @c Buffer type with the minimum functionality required by SDS.
class MinimalTraits::Buffer : private InProcessSDS::Buffer {
public:
    /// Construct a pre-sized buffer.  Note that this funciton is not required by SDS, but is required by the tests.
    Buffer(InProcessSDS::Buffer::size_type size) : InProcessSDS::Buffer(size) {
    }
    /// Get the buffer size.
    InProcessSDS::Buffer::size_type size() const {
        return InProcessSDS::Buffer::size();
    }
    /// Get a pointer to the raw data buffer.
    InProcessSDS::Buffer::value_type* data() {
        return InProcessSDS::Buffer::data();
    }
};

/// A @c Mutex type with the minimum functionality required by SDS.
class MinimalTraits::Mutex : private InProcessSDS::Mutex {
public:
    /// Lock the mutex.
    void lock() {
        InProcessSDS::Mutex::lock();
    }
    /// Unlock the mutex.
    void unlock() {
        InProcessSDS::Mutex::unlock();
    }
};

/// A @c ConditionVariable type with the minimum functionality required by SDS.
class MinimalTraits::ConditionVariable : private InProcessSDS::ConditionVariable {
public:
    /// Notify all waiters.
    void notify_all() {
        InProcessSDS::ConditionVariable::notify_all();
    }
    /// Wait forever.
    void wait(std::unique_lock<Mutex>& lock) {
        auto lockPointer = reinterpret_cast<std::unique_lock<InProcessSDS::Mutex>*>(&lock);
        InProcessSDS::ConditionVariable::wait(*lockPointer);
    }
    /// Wait forever for @c pred to be true.
    template <class Predicate>
    void wait(std::unique_lock<Mutex>& lock, Predicate pred) {
        auto lockPointer = reinterpret_cast<std::unique_lock<InProcessSDS::Mutex>*>(&lock);
        InProcessSDS::ConditionVariable::wait(*lockPointer, pred);
    }
    /// Wait until timeout for @c pred to be true.
    template <class Rep, class Period, class Predicate>
    bool wait_for(std::unique_lock<Mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred) {
        auto lockPointer = reinterpret_cast<std::unique_lock<InProcessSDS::Mutex>*>(&lock);
        return InProcessSDS::ConditionVariable::wait_for(*lockPointer, rel_time, pred);
    }
};

/// For brevity in the tests below, alias an SDS type which uses @c MinimalTraits.
using Sds = SharedDataStream<MinimalTraits>;

/// A data source class which can generate an aribrary amount of data at a specified rate and block size.
class Source {
public:
    /**
     * This function generates a test pattern and writes @c frequencyHz words per second to @c writer.  It will group
     * these writes into @c blockSizeWords blocks and write them as blocks.  The optional @c maxWords parameter can be
     * used to limit the amount of data sent; when @c maxWords have been sent, the @c Source automatically closes the
     * @c writer.  This function will assert if there is a problem writing data to the SDS.
     *
     * @param writer The SDS Writer to write to.
     * @param frequencyHz The number of words-per-second to send.
     * @param blockSizeWords The block size to group writes into.
     * @param maxWords The maximum number of words to send.  This parmeter defaults to 0, which indicates that there is
     *     no maximum and it should continue to write until deleted.
     * @return A @c future for the total number of words written.
     */
    std::future<size_t> run(
        std::shared_ptr<Sds::Writer> writer,
        size_t frequencyHz,
        size_t blockSizeWords,
        size_t maxWords = 0);

private:
    /// The @c Timer to use for sending data.
    timing::Timer m_timer;

    /// A counter which tracks the number of words sent.
    size_t m_counter;

    /// A promise to use for returning the number of words sent.
    std::promise<size_t> m_promise;
};

std::future<size_t> Source::run(
    std::shared_ptr<Sds::Writer> writer,
    size_t frequencyHz,
    size_t blockSizeWords,
    size_t maxWords) {
    m_counter = 0;
    size_t wordSize = writer->getWordSize();
    std::chrono::nanoseconds period(frequencyHz ? ((1000000000 / frequencyHz) * blockSizeWords) : 0);
    bool started = m_timer.start(
        period,
        timing::Timer::PeriodType::RELATIVE,
        timing::Timer::FOREVER,
        [this, writer, blockSizeWords, maxWords, wordSize] {
            std::vector<uint8_t> block(blockSizeWords * writer->getWordSize());
            size_t wordsToWrite = 0;
            for (size_t word = 0; word < blockSizeWords; ++word) {
                for (size_t byte = 0; byte < wordSize; ++byte) {
                    size_t byteIndex = word * wordSize + byte;
                    uint8_t byteValue = m_counter >> (byte % wordSize);
                    block[byteIndex] = byteValue;
                }
                ++m_counter;
                ++wordsToWrite;
                if (maxWords > 0) {
                    if (m_counter == maxWords) {
                        break;
                    }
                }
            }
            ssize_t nWords;
            do {
                nWords = writer->write(block.data(), wordsToWrite);
            } while (nWords == Sds::Writer::Error::WOULDBLOCK);
            bool unexpectedWriteReturn = nWords != static_cast<ssize_t>(block.size() / wordSize);
            EXPECT_FALSE(unexpectedWriteReturn) << "write returned " << nWords;
            if (unexpectedWriteReturn || (maxWords > 0 && m_counter == maxWords)) {
                m_timer.stop();
                writer->close();
                m_promise.set_value(m_counter);
            }
        });
    if (!started) {
        return std::future<size_t>();
    }
    return m_promise.get_future();
}

/// A data sink class which can read and verify an aribrary amount of data at a specified rate and block size.
class Sink {
public:
    /**
     * This function reads @c frequencyHz words per second from @c reader and verifies that they match an expected test
     * pattern.  It will group these reads into @c blockSizeWords blocks and read them as blocks.  The optional
     * @c maxWords parameter can be used to limit the amount of data read; when @c maxWords have been read, the
     * @c Source automatically closes the @c reader.  This function will assert if there is a problem reading data from
     * the SDS or a deviation from the expected test pattern values.
     *
     * @param reader The SDS Reader to read from.
     * @param frequencyHz The number of words-per-second to receive.
     * @param blockSizeWords The block size to group reads into.
     * @param maxWords The maximum number of words to read.  This parmeter defaults to 0, which indicates that there is
     *     no maximum and it should continue to read until the reader closes or it is deleted.
     * @return A @c future for the total number of words read.
     */
    std::future<size_t> run(
        std::shared_ptr<Sds::Reader> reader,
        size_t frequencyHz,
        size_t blockSizeWords,
        size_t maxWords = 0);

private:
    /// The @c Timer to use for receiving data.
    timing::Timer m_timer;

    /// A counter which tracks the number of words received.
    size_t m_counter;

    /// A promise to use for returning the number of words received.
    std::promise<size_t> m_promise;
};

std::future<size_t> Sink::run(
    std::shared_ptr<Sds::Reader> reader,
    size_t frequencyHz,
    size_t blockSizeWords,
    size_t maxWords) {
    m_counter = 0;
    size_t wordSize = reader->getWordSize();
    std::chrono::nanoseconds period(frequencyHz ? ((1000000000 / frequencyHz) * blockSizeWords) : 0);
    bool started = m_timer.start(
        period,
        timing::Timer::PeriodType::RELATIVE,
        timing::Timer::FOREVER,
        [this, reader, blockSizeWords, maxWords, wordSize] {
            std::vector<uint8_t> block(blockSizeWords * wordSize);
            ssize_t nWords = reader->read(block.data(), block.size() / wordSize);
            if (nWords == Sds::Reader::Error::WOULDBLOCK) {
                return;
            } else if (nWords == Sds::Reader::Error::CLOSED) {
                m_timer.stop();
                m_promise.set_value(m_counter);
                return;
            }
            bool unexpectedReadReturn = nWords <= 0 || nWords > static_cast<ssize_t>(block.size() / wordSize);
            EXPECT_FALSE(unexpectedReadReturn) << "read returned " << nWords;
            if (unexpectedReadReturn) {
                m_timer.stop();
                m_promise.set_value(m_counter);
            }
            for (ssize_t word = 0; word < nWords; ++word) {
                for (size_t byte = 0; byte < wordSize; ++byte) {
                    size_t byteIndex = word * wordSize + byte;
                    uint8_t byteValue = m_counter >> (byte % wordSize);
                    ASSERT_EQ(block[byteIndex], byteValue);
                }
                ++m_counter;
                if (maxWords > 0 && m_counter == maxWords) {
                    m_timer.stop();
                    m_promise.set_value(m_counter);
                }
            }
        });
    if (!started) {
        return std::future<size_t>();
    }
    return m_promise.get_future();
}

/// The test harness for the tests below.
class SharedDataStreamTest : public ::testing::Test {
public:
private:
};

/// This tests @c SharedDataStream::calculateCreateSize() and @c SharedDataStream::create().
TEST_F(SharedDataStreamTest, sdsCalculateCreateSize) {
    static const size_t SDK_MAXREADERS_REQUIRED = 2;
    static const size_t SDK_WORDSIZE_REQUIRED = sizeof(uint16_t);
    size_t maxReaders, wordCount, wordSize;
    static const size_t MULTIPLE_WORDS = 2;
    for (maxReaders = 0; maxReaders <= SDK_MAXREADERS_REQUIRED; ++maxReaders) {
        for (wordSize = 0; wordSize <= SDK_WORDSIZE_REQUIRED; ++wordSize) {
            for (wordCount = 0; wordCount <= MULTIPLE_WORDS; ++wordCount) {
                // Basic check that calculated size is larger than requested ring buffer size.
                size_t bufferSize = Sds::calculateBufferSize(wordCount, wordSize, maxReaders);
                if (wordCount == 0 || wordSize == 0) {
                    // wordSize=0 and wordCount=0 are invalid and should result in bufferSize==0
                    ASSERT_EQ(bufferSize, 0U);
                    continue;
                }
                ASSERT_GT(bufferSize, wordCount * wordSize);

                // Should fail to create an SDS with an empty buffer.
                auto buffer = std::make_shared<Sds::Buffer>(0);
                auto sds = Sds::create(buffer, wordSize, maxReaders);
                ASSERT_EQ(sds, nullptr);

                // Should fail to create an SDS which can't hold any words.
                buffer = std::make_shared<Sds::Buffer>(bufferSize - wordCount * wordSize);
                sds = Sds::create(buffer, wordSize, maxReaders);
                ASSERT_EQ(sds, nullptr);

                // Should be able to create an SDS which can only hold one word.
                buffer = std::make_shared<Sds::Buffer>(bufferSize - (wordCount - 1) * wordSize);
                sds = Sds::create(buffer, wordSize, maxReaders);
                ASSERT_NE(sds, nullptr);
                ASSERT_EQ(sds->getDataSize(), 1U);
                ASSERT_EQ(sds->getWordSize(), wordSize);
                ASSERT_EQ(sds->getMaxReaders(), maxReaders);

                // Should be able to create an SDS which can hold the requested number of words
                buffer = std::make_shared<Sds::Buffer>(bufferSize);
                sds = Sds::create(buffer, wordSize, maxReaders);
                ASSERT_NE(sds, nullptr);
                ASSERT_EQ(sds->getDataSize(), wordCount);
                ASSERT_EQ(sds->getWordSize(), wordSize);
                ASSERT_EQ(sds->getMaxReaders(), maxReaders);
            }
        }
    }

    // Verify create() detects the upper limit on maxReaders, and that the limit meets/exceeds SDK requirements.
    for (maxReaders = 1; maxReaders < std::numeric_limits<size_t>::max(); maxReaders <<= 1) {
        static const size_t WORDSIZE = 1;
        static const size_t WORDCOUNT = 1;
        size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, maxReaders);
        auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
        auto sds = Sds::create(buffer, WORDSIZE, maxReaders);
        if (sds == nullptr) {
            break;
        }
        ASSERT_EQ(maxReaders, sds->getMaxReaders());
    }
    ASSERT_GE(maxReaders, SDK_MAXREADERS_REQUIRED);

    // Verify create() detects the upper limit on wordSize, and that the limit meets/exceeds SDK requirements.
    for (wordSize = 1; wordSize < std::numeric_limits<size_t>::max(); wordSize <<= 1) {
        static const size_t WORDCOUNT = 1;
        static const size_t MAXREADERS = 1;
        size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, wordSize, MAXREADERS);
        auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
        auto sds = Sds::create(buffer, wordSize, MAXREADERS);
        if (sds == nullptr) {
            break;
        }
        ASSERT_EQ(wordSize, sds->getWordSize());
    }
    ASSERT_GT(wordSize, SDK_WORDSIZE_REQUIRED);
}

/// This tests @c SharedDataStream::open().
TEST_F(SharedDataStreamTest, sdsOpen) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 10;
    static const size_t MAXREADERS = 2;

    // Initialize a buffer with sds1.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds1 = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds1, nullptr);
    ASSERT_EQ(sds1->getDataSize(), WORDCOUNT);

    // Verify a compatible sds can open it and the parameters are consistent.
    auto sds2 = Sds::open(buffer);
    ASSERT_NE(sds2, nullptr);
    ASSERT_EQ(sds2->getDataSize(), WORDCOUNT);
    ASSERT_EQ(sds2->getWordSize(), WORDSIZE);
    ASSERT_EQ(sds2->getMaxReaders(), MAXREADERS);

    // Verify an sds with different traits fails to open it.
    auto sds3 = SharedDataStream<MinimalTraits2>::open(buffer);
    ASSERT_EQ(sds3, nullptr);

    // Verify that open fails if magic number is wrong.
    uint32_t* buffer32 = reinterpret_cast<uint32_t*>(buffer->data());
    buffer32[0] = ~buffer32[0];
    sds2 = Sds::open(buffer);
    ASSERT_EQ(sds2, nullptr);
    buffer32[0] = ~buffer32[0];
    sds2 = Sds::open(buffer);
    ASSERT_NE(sds2, nullptr);

    // Verify that open fails if version is incompatible.
    buffer32[1] = ~buffer32[1];
    sds2 = Sds::open(buffer);
    ASSERT_EQ(sds2, nullptr);
    buffer32[1] = ~buffer32[1];
    sds2 = Sds::open(buffer);
    ASSERT_NE(sds2, nullptr);
}

/// This tests @c SharedDataStream::createWriter().
TEST_F(SharedDataStreamTest, createWriter) {
    static const size_t WORDSIZE = 1;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 1;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a writer without forcing.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Verify that we can't create another writer while the first one is still open.
    auto writer2 = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_EQ(writer2, nullptr);

    // Verify that can create another writer after the first one is closed.
    writer->close();
    writer2 = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer2, nullptr);

    // Verify that we can create another writer after deleting.
    writer.reset();
    writer2.reset();
    writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Verify that we can delete a closed writer after creating another, without affecting the other (open) writer.
    writer->close();
    writer2 = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer2, nullptr);
    writer.reset();
    writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_EQ(writer, nullptr);

    // Verify that we can force-create a writer when another is still open.
    writer.reset();
    writer2.reset();
    writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);
    writer2 = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_EQ(writer2, nullptr);
    writer2 = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE, true);
    ASSERT_NE(writer2, nullptr);
    writer.reset();
    writer2.reset();
}

/// This tests @c SharedDataStream::createReader().
TEST_F(SharedDataStreamTest, createReader) {
    static const size_t WORDSIZE = 1;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 2;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a reader without forcing.
    auto reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);

    // Verify that we can create a second reader while the first one is still open.
    auto reader2 = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader2, nullptr);

    // Verify that we can't create a third reader while the first two are still open.
    auto reader3 = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_EQ(reader3, nullptr);

    // Verify that we can't create a third reader after the first one is closed.
    reader->close();
    reader3 = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_EQ(reader3, nullptr);

    // Verify that we can create another reader after deleting the one that is closed.
    reader.reset();
    reader3 = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader3, nullptr);

    // Verify that we can create a readers with a specific ID.
    static const size_t FIXED_ID = 0;
    reader2.reset();
    reader3.reset();
    reader = sds->createReader(FIXED_ID, Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);

    // Verify that we can't create a reader with an ID that is already in use.
    reader2 = sds->createReader(FIXED_ID, Sds::Reader::Policy::NONBLOCKING);
    ASSERT_EQ(reader2, nullptr);
    reader.reset();
    reader2 = sds->createReader(FIXED_ID, Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader2, nullptr);

    // Verify that we can force-create a reader with an ID that is already in use.
    reader = sds->createReader(FIXED_ID, Sds::Reader::Policy::NONBLOCKING, false, true);
    ASSERT_NE(reader, nullptr);

    // Verify that onlyReadNewData=false puts the reader at the newest data in the buffer.
    uint8_t buf[WORDSIZE * WORDCOUNT];
    auto writer = sds->createWriter(Sds::Writer::Policy::ALL_OR_NOTHING);
    ASSERT_NE(writer, nullptr);
    ASSERT_EQ(writer->write(buf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_WRITER), WORDCOUNT);
    reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING, true);
    ASSERT_NE(reader, nullptr);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_WRITER), 0U);
}

/// This tests @c SharedDataStream::Reader::read().
TEST_F(SharedDataStreamTest, readerRead) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 2;
    static const size_t MAXREADERS = 2;
    static const std::chrono::milliseconds TIMEOUT{10};
    static const uint8_t WRITEFILL = 1;
    static const uint8_t READFILL = 0;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create blocking and nonblocking readers.
    std::shared_ptr<Sds::Reader> blocking = sds->createReader(Sds::Reader::Policy::BLOCKING);
    ASSERT_NE(blocking, nullptr);
    auto nonblocking = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(nonblocking, nullptr);

    // Verify bad parameter handling.
    uint8_t readBuf[WORDSIZE * WORDCOUNT * 2];
    ASSERT_EQ(blocking->read(nullptr, WORDCOUNT), Sds::Reader::Error::INVALID);
    ASSERT_EQ(blocking->read(readBuf, 0), Sds::Reader::Error::INVALID);

    // Verify both read types detect unopened stream (no writer).
    ASSERT_EQ(blocking->read(readBuf, WORDCOUNT, TIMEOUT), Sds::Reader::Error::TIMEDOUT);
    ASSERT_EQ(nonblocking->read(readBuf, WORDCOUNT), Sds::Reader::Error::WOULDBLOCK);

    // Attach a writer.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Verify both read types detect empty stream.
    ASSERT_EQ(blocking->read(readBuf, WORDCOUNT, TIMEOUT), Sds::Reader::Error::TIMEDOUT);
    ASSERT_EQ(nonblocking->read(readBuf, WORDCOUNT), Sds::Reader::Error::WOULDBLOCK);

    // Verify correct number of bytes are read.
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    std::fill(std::begin(writeBuf), std::end(writeBuf), WRITEFILL);
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    std::fill(std::begin(readBuf), std::end(readBuf), READFILL);
    ASSERT_EQ(nonblocking->read(readBuf, WORDCOUNT / 2), static_cast<ssize_t>(WORDCOUNT) / 2);
    auto mismatch = std::mismatch(std::begin(writeBuf), std::end(writeBuf), std::begin(readBuf));
    ASSERT_EQ(mismatch.second - readBuf, static_cast<ssize_t>(WORDSIZE * WORDCOUNT) / 2);

    // Read more data than the buffer contains.
    ASSERT_TRUE(blocking->seek(0, Sds::Reader::Reference::BEFORE_WRITER));
    ASSERT_TRUE(nonblocking->seek(0, Sds::Reader::Reference::BEFORE_WRITER));
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(blocking->read(readBuf, WORDCOUNT * 2, TIMEOUT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(nonblocking->read(readBuf, WORDCOUNT * 2), static_cast<ssize_t>(WORDCOUNT));

    // Verify both readers detect overflows.
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(blocking->read(readBuf, WORDCOUNT * 2, TIMEOUT), Sds::Reader::Error::OVERRUN);
    ASSERT_EQ(nonblocking->read(readBuf, WORDCOUNT * 2), Sds::Reader::Error::OVERRUN);

    // Verify blocked reader unblocks.
    ASSERT_TRUE(blocking->seek(0, Sds::Reader::Reference::BEFORE_WRITER));
    auto numRead = std::async([blocking, readBuf]() mutable { return blocking->read(readBuf, WORDCOUNT, TIMEOUT); });
    ASSERT_NE(numRead.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(numRead.get(), static_cast<ssize_t>(WORDCOUNT));

    // Verify blocked reader which is seeked to a future index unblocks.
    size_t indexesToSkip = 1;
    ASSERT_TRUE(blocking->seek(indexesToSkip, Sds::Reader::Reference::AFTER_READER));
    numRead = std::async([blocking, readBuf]() mutable { return blocking->read(readBuf, WORDCOUNT, TIMEOUT); });
    ASSERT_NE(numRead.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(numRead.get(), static_cast<ssize_t>(WORDCOUNT - indexesToSkip));
}

/// This tests @c SharedDataStream::Reader::seek().
TEST_F(SharedDataStreamTest, readerSeek) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 10;
    static const size_t MAXREADERS = 2;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a reader.
    auto reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);
    Sds::Index readerPos = 0;

    // Attach a writer and fill half of the buffer with a pattern.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);
    Sds::Index writerPos = 0;
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    for (size_t i = 0; i < sizeof(writeBuf); ++i) {
        writeBuf[i] = i;
    }
    size_t writeWords = WORDCOUNT / 2;
    ASSERT_EQ(writer->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));
    writerPos += writeWords;

    //--- Sds::Reader::Reference::AFTER_READER ---

    // Verify we can seek forward from the current read position to the middle of the written data.
    size_t seekWords = 1;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::AFTER_READER));
    readerPos += seekWords;
    uint8_t readBuf[WORDSIZE * WORDCOUNT];
    ssize_t readWords = 1;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify we can seek forward from the current read position to the end of the written data.
    seekWords = writeWords - readerPos;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::AFTER_READER));
    readerPos += seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::WOULDBLOCK);

    // Verify we can seek forward from the current read position beyond the end of the written data.
    seekWords = 1;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::AFTER_READER));
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::WOULDBLOCK);
    readerPos += seekWords;

    //--- Sds::Reader::Reference::BEFORE_READER ---

    // Verify we can seek backward from the current read position to the middle of the written data.
    seekWords = 2;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_READER));
    readerPos -= seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify we can seek backward from the current read position to the beginning of the written data.
    seekWords = writeWords;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_READER));
    readerPos -= seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify we can't seek backward from the current read position before the beginning of the written data.
    seekWords = readerPos + 1;
    ASSERT_FALSE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_READER));
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    //--- Sds::Reader::Reference::BEFORE_WRITER ---

    // Verify we can seek backward from the current write position to the end of the written data.
    seekWords = 0;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_WRITER));
    readerPos = writerPos - seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::WOULDBLOCK);

    // Verify we can seek backward from the current write position to the middle of the written data.
    seekWords = 1;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_WRITER));
    readerPos = writerPos - seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify we can seek backward from the current write position to the beginning of the written data.
    seekWords = writeWords;
    ASSERT_TRUE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_WRITER));
    readerPos = writerPos - seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify we can't seek backward from the current write position before the beginning of the written data.
    seekWords = writeWords + 1;
    ASSERT_FALSE(reader->seek(seekWords, Sds::Reader::Reference::BEFORE_WRITER));
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    //--- Sds::Reader::Reference::ABSOLUTE ---

    // Verify we can seek directly to the end of the written data.
    seekWords = writerPos;
    ASSERT_TRUE(reader->seek(seekWords));
    readerPos = seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::WOULDBLOCK);

    // Verify we can seek directly to a position beyond the end of the written data.
    seekWords = writerPos + 1;
    ASSERT_TRUE(reader->seek(seekWords));
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::WOULDBLOCK);
    readerPos = seekWords;

    // Verify we can seek directly to the middle of the written data.
    seekWords = writerPos - 2;
    ASSERT_TRUE(reader->seek(seekWords));
    readerPos = seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify we can seek directly to the beginning of the written data.
    seekWords = 0;
    ASSERT_TRUE(reader->seek(seekWords));
    readerPos = seekWords;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    ASSERT_EQ(readBuf[0], writeBuf[readerPos * WORDSIZE]);
    readerPos += readWords;

    // Verify that we can't seek to a position that has been overwritten.
    writeWords = WORDCOUNT;
    ASSERT_EQ(writer->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));
    writerPos += writeWords;
    seekWords = 0;
    ASSERT_FALSE(reader->seek(seekWords));
}

/// This tests @c SharedDataStream::Reader::tell().
TEST_F(SharedDataStreamTest, readerTell) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 10;
    static const size_t MAXREADERS = 2;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a reader.
    auto reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);
    Sds::Index readerPos = 0;

    // Check initial position.
    ASSERT_EQ(reader->tell(), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::AFTER_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_READER), 0U);

    // Attach a writer and fill the buffer.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Check initial reader position relative to writer.
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_WRITER), 0U);

    // Fill half the buffer.
    Sds::Index writerPos = 0;
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    size_t writeWords = WORDCOUNT / 2;
    ASSERT_EQ(writer->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));
    writerPos += writeWords;

    // Verify position relative to writer has changed, but others are unchanged.
    ASSERT_EQ(reader->tell(), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::AFTER_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_WRITER), static_cast<Sds::Index>(writerPos));

    // Read a word, then verify that position relative to writer and absolute have changed, but others are unchanged.
    uint8_t readBuf[WORDSIZE * WORDCOUNT];
    size_t readWords = 1;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    readerPos += readWords;
    ASSERT_EQ(reader->tell(), static_cast<Sds::Index>(readerPos));
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::AFTER_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_WRITER), static_cast<Sds::Index>(writerPos - readerPos));

    // Read remaining words, then verify that position relative to writer is zero, aboslute has changed, others are
    // unchanged.
    readWords = writerPos - readerPos;
    ASSERT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(readWords));
    readerPos += readWords;
    ASSERT_EQ(reader->tell(), readerPos);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::AFTER_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_READER), 0U);
    ASSERT_EQ(reader->tell(Sds::Reader::Reference::BEFORE_WRITER), 0U);
}

/// This tests @c SharedDataStream::Reader::close().
TEST_F(SharedDataStreamTest, readerClose) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 10;
    static const size_t MAXREADERS = 2;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a reader.
    auto reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader, nullptr);

    // Attach a writer and fill the buffer.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    size_t writeWords = WORDCOUNT;
    ASSERT_EQ(writer->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));

    // Request reader to close immediately and verify that it does so.
    uint8_t readBuf[WORDSIZE * WORDCOUNT];
    size_t readWords = 2;
    reader->close();
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::CLOSED);

    // Request the reader to close later and verify that does so.
    size_t closeWords = 2;
    readWords = writeWords;
    reader->close(closeWords, Sds::Reader::Reference::BEFORE_WRITER);
    EXPECT_EQ(reader->read(readBuf, readWords), static_cast<ssize_t>(writeWords - closeWords));
    ASSERT_EQ(reader->read(readBuf, readWords), Sds::Reader::Error::CLOSED);
}

/// This tests @c SharedDataStream::Reader::getId().
TEST_F(SharedDataStreamTest, readerGetId) {
    static const size_t WORDSIZE = 1;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 10;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create all readers and veriy that their IDs are unique and less than Sds::getMaxReaders().
    std::unordered_map<size_t, std::shared_ptr<Sds::Reader>> readers;
    std::shared_ptr<Sds::Reader> reader;
    while ((reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING)) != nullptr) {
        ASSERT_LT(reader->getId(), sds->getMaxReaders());
        ASSERT_TRUE(readers.insert(std::make_pair(reader->getId(), reader)).second);
    }
    ASSERT_EQ(readers.size(), sds->getMaxReaders());
    readers.clear();
    reader.reset();

    // Create all readers with manually-assigned IDs and make sure they read back correctly.
    for (size_t i = 0; i < MAXREADERS; ++i) {
        readers[i] = sds->createReader(i, Sds::Reader::Policy::NONBLOCKING);
        ASSERT_NE(readers[i], nullptr);
        ASSERT_EQ(readers[i]->getId(), i);
    }
}

/// This tests @c SharedDataStream::Reader::getWordSize().
TEST_F(SharedDataStreamTest, readerGetWordSize) {
    static const size_t MINWORDSIZE = 1;
    static const size_t MAXWORDSIZE = 8;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 1;

    for (size_t wordSize = MINWORDSIZE; wordSize <= MAXWORDSIZE; ++wordSize) {
        size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, wordSize, MAXREADERS);
        auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
        auto sds = Sds::create(buffer, wordSize, MAXREADERS);
        ASSERT_NE(sds, nullptr);
        auto reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
        ASSERT_NE(reader, nullptr);
        ASSERT_EQ(reader->getWordSize(), wordSize);
    }
}

/// This tests @c SharedDataStream::Writer::write().
TEST_F(SharedDataStreamTest, writerWrite) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 2;
    static const size_t MAXREADERS = 1;
    static const std::chrono::milliseconds TIMEOUT{100};

    // Initialize three sdses.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer1 = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds1 = Sds::create(buffer1, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds1, nullptr);
    auto buffer2 = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds2 = Sds::create(buffer2, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds2, nullptr);
    auto buffer3 = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds3 = Sds::create(buffer3, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds3, nullptr);

    // Create nonblockable, all-or-nothing and blocking writers.
    auto nonblockable = sds1->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(nonblockable, nullptr);
    auto allOrNothing = sds2->createWriter(Sds::Writer::Policy::ALL_OR_NOTHING);
    ASSERT_NE(allOrNothing, nullptr);
    std::shared_ptr<Sds::Writer> blocking = sds3->createWriter(Sds::Writer::Policy::BLOCKING);
    ASSERT_NE(blocking, nullptr);

    // Verify bad parameter handling.
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    ASSERT_EQ(nonblockable->write(nullptr, WORDCOUNT), Sds::Writer::Error::INVALID);
    ASSERT_EQ(nonblockable->write(writeBuf, 0), Sds::Writer::Error::INVALID);

    // Verify all writers can write data to their buffers.
    size_t writeWords = WORDCOUNT / 2;
    ASSERT_EQ(nonblockable->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));
    ASSERT_EQ(allOrNothing->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));
    ASSERT_EQ(blocking->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));

    // Verify nonblockable writer can overflow the buffer without blocking.
    writeWords = WORDCOUNT;
    ASSERT_EQ(nonblockable->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));

    // Verify all-or-nothing writer can't overflow the buffer.
    ASSERT_EQ(allOrNothing->write(writeBuf, writeWords), Sds::Writer::Error::WOULDBLOCK);

    // Verify blocking writer can fill the buffer.
    ASSERT_EQ(blocking->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT / 2));

    // Verify blocking writer can't write to a full buffer.
    ASSERT_EQ(blocking->write(writeBuf, writeWords, TIMEOUT), Sds::Writer::Error::TIMEDOUT);

    // Verify blocked writer unblocks.
    writeWords = 1;
    auto reader3 = sds3->createReader(Sds::Reader::Policy::NONBLOCKING);
    auto result = std::async([blocking, writeBuf]() mutable { return blocking->write(writeBuf, WORDCOUNT, TIMEOUT); });
    ASSERT_NE(result.wait_for(std::chrono::milliseconds::zero()), std::future_status::ready);
    ASSERT_TRUE(reader3->seek(writeWords, Sds::Reader::Reference::AFTER_READER));
    ASSERT_EQ(result.get(), static_cast<ssize_t>(writeWords));

    // Verify all-or-nothing writer can't overrun a reader who is in the future.
    auto reader2 = sds2->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_NE(reader2, nullptr);
    ASSERT_TRUE(reader2->seek(WORDCOUNT, Sds::Reader::Reference::AFTER_READER));
    writeWords = WORDCOUNT * 2;
    ASSERT_EQ(allOrNothing->write(writeBuf, writeWords), Sds::Writer::Error::WOULDBLOCK);

    // Verify all-or-nothing writer can discard data that will not be read by a reader who is waiting in the future.
    writeWords = WORDCOUNT + WORDCOUNT / 2;
    ASSERT_EQ(allOrNothing->write(writeBuf, writeWords), static_cast<ssize_t>(writeWords));
}

/// This tests @c SharedDataStream::Writer::tell().
TEST_F(SharedDataStreamTest, writerTell) {
    static const size_t WORDSIZE = 1;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 1;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a writer.
    auto writer = sds->createWriter(Sds::Writer::Policy::ALL_OR_NOTHING);
    ASSERT_NE(writer, nullptr);

    // Verify initial position.
    ASSERT_EQ(writer->tell(), 0U);

    // Verify position changes after a successful write.
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    ASSERT_EQ(writer->tell(), static_cast<Sds::Index>(WORDCOUNT));

    // Verify position doesn't change after an unsuccessful write.
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), Sds::Writer::Error::WOULDBLOCK);
    ASSERT_EQ(writer->tell(), static_cast<Sds::Index>(WORDCOUNT));
}

/// This tests @c SharedDataStream::Writer::close().
TEST_F(SharedDataStreamTest, writerClose) {
    static const size_t WORDSIZE = 1;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 1;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    // Create a writer.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Verify can write before close, but not after.
    uint8_t writeBuf[WORDSIZE * WORDCOUNT];
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), static_cast<ssize_t>(WORDCOUNT));
    writer->close();
    ASSERT_EQ(writer->write(writeBuf, WORDCOUNT), Sds::Writer::Error::CLOSED);
}

/// This tests @c SharedDataStream::Writer::getWordSize().
TEST_F(SharedDataStreamTest, writerGetWordSize) {
    static const size_t MINWORDSIZE = 1;
    static const size_t MAXWORDSIZE = 8;
    static const size_t WORDCOUNT = 1;
    static const size_t MAXREADERS = 1;

    for (size_t wordSize = MINWORDSIZE; wordSize <= MAXWORDSIZE; ++wordSize) {
        size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, wordSize, MAXREADERS);
        auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
        auto sds = Sds::create(buffer, wordSize, MAXREADERS);
        ASSERT_NE(sds, nullptr);
        auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
        ASSERT_NE(writer, nullptr);
        ASSERT_EQ(writer->getWordSize(), wordSize);
    }
}

/// This tests a nonblockable, slow @c Writer streaming concurrently to two fast @c Readers (one of each type).
TEST_F(SharedDataStreamTest, concurrencyNonblockableWriterDualReader) {
    static const size_t WORDSIZE = 2;
    static const size_t WRITE_FREQUENCY_HZ = 1000;
    static const size_t READ_FREQUENCY_HZ = 0;
    static const size_t BUFFER_MILLISECONDS = 100;
    static const size_t WORDCOUNT = WRITE_FREQUENCY_HZ * BUFFER_MILLISECONDS / 1000;
    static const size_t MAXREADERS = 2;
    static const size_t TEST_SIZE_WORDS = WORDCOUNT * 3;
    static const size_t WRITE_BLOCK_SIZE_WORDS = WRITE_FREQUENCY_HZ / 10;
    static const size_t READ_BLOCK_SIZE_WORDS = 1;

    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_TRUE(sds);

    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_TRUE(writer);
    auto blockingReader = sds->createReader(Sds::Reader::Policy::BLOCKING);
    ASSERT_TRUE(blockingReader);
    std::shared_ptr<Sds::Reader> nonblockingReader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_TRUE(nonblockingReader);

    Source source;
    Sink blockingSink, nonblockingSink;
    source.run(std::move(writer), WRITE_FREQUENCY_HZ, WRITE_BLOCK_SIZE_WORDS);
    auto blockingWords =
        blockingSink.run(std::move(blockingReader), READ_FREQUENCY_HZ, READ_BLOCK_SIZE_WORDS, TEST_SIZE_WORDS);
    auto nonblockingWords = nonblockingSink.run(nonblockingReader, READ_FREQUENCY_HZ, READ_BLOCK_SIZE_WORDS);
    ASSERT_EQ(blockingWords.get(), TEST_SIZE_WORDS);
    nonblockingReader->close(0, Sds::Reader::Reference::BEFORE_WRITER);
    ASSERT_GE(nonblockingWords.get(), TEST_SIZE_WORDS);
}

/// This tests an all-or-nothing, fast @c Writer streaming concurrently to a slow non-blocking @c Reader.
TEST_F(SharedDataStreamTest, concurrencyAllOrNothingWriterNonblockingReader) {
    static const size_t WORDSIZE = 1;
    static const size_t WRITE_FREQUENCY_HZ = 320000;
    static const size_t READ_FREQUENCY_HZ = 160000;
    static const size_t BUFFER_MILLISECONDS = 100;
    static const size_t WORDCOUNT = WRITE_FREQUENCY_HZ * BUFFER_MILLISECONDS / 1000;
    static const size_t MAXREADERS = 1;
    static const size_t TEST_SIZE_WORDS = WORDCOUNT * 3;
    static const size_t WRITE_BLOCK_SIZE_WORDS = WRITE_FREQUENCY_HZ / 10;
    static const size_t READ_BLOCK_SIZE_WORDS = READ_FREQUENCY_HZ / 10;

    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_TRUE(sds);

    std::shared_ptr<Sds::Writer> writer = sds->createWriter(Sds::Writer::Policy::ALL_OR_NOTHING);
    ASSERT_TRUE(writer);
    auto reader = sds->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_TRUE(reader);

    Source source;
    Sink sink;
    source.run(writer, WRITE_FREQUENCY_HZ, WRITE_BLOCK_SIZE_WORDS, TEST_SIZE_WORDS);
    auto caWords = sink.run(std::move(reader), READ_FREQUENCY_HZ, READ_BLOCK_SIZE_WORDS);
    ASSERT_EQ(caWords.get(), TEST_SIZE_WORDS);
}

/// This tests a @c Writer from one SDS streaming to a @c Reader from a different SDS, usig a shared @c Buffer.
TEST_F(SharedDataStreamTest, concurrencyMultipleSds) {
    static const size_t WORDSIZE = 1;
    static const size_t WRITE_FREQUENCY_HZ = 320000;
    static const size_t READ_FREQUENCY_HZ = 160000;
    static const size_t BUFFER_MILLISECONDS = 100;
    static const size_t WORDCOUNT = WRITE_FREQUENCY_HZ * BUFFER_MILLISECONDS / 1000;
    static const size_t MAXREADERS = 1;
    static const size_t TEST_SIZE_WORDS = WORDCOUNT * 3;
    static const size_t WRITE_BLOCK_SIZE_WORDS = WRITE_FREQUENCY_HZ / 10;
    static const size_t READ_BLOCK_SIZE_WORDS = READ_FREQUENCY_HZ / 10;

    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);

    auto sds1 = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_TRUE(sds1);
    std::shared_ptr<Sds::Writer> writer = sds1->createWriter(Sds::Writer::Policy::ALL_OR_NOTHING);
    ASSERT_TRUE(writer);

    auto sds2 = Sds::open(buffer);
    ASSERT_TRUE(sds2);
    auto reader = sds2->createReader(Sds::Reader::Policy::NONBLOCKING);
    ASSERT_TRUE(reader);

    Source source;
    Sink sink;
    source.run(writer, WRITE_FREQUENCY_HZ, WRITE_BLOCK_SIZE_WORDS, TEST_SIZE_WORDS);
    auto caWords = sink.run(std::move(reader), READ_FREQUENCY_HZ, READ_BLOCK_SIZE_WORDS);
    ASSERT_EQ(caWords.get(), TEST_SIZE_WORDS);
}

/// This tests that a @c Reader closes if a @c Writer is attached and closed before writing anything
TEST_F(SharedDataStreamTest, writerClosedBeforeWriting) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 2;
    static const size_t MAXREADERS = 2;
    static const std::chrono::milliseconds READ_TIMEOUT{100};
    // explicitly set the time for closing the writer to be less than the READ_TIMEOUT
    static const std::chrono::milliseconds CLOSE_WRITER_AFTER_READ{50};

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    uint8_t readBuf[WORDSIZE * WORDCOUNT * 2];

    // Create blocking reader.
    std::shared_ptr<Sds::Reader> blocking = sds->createReader(Sds::Reader::Policy::BLOCKING);
    ASSERT_NE(blocking, nullptr);

    // Attach a writer.
    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Close the writer before reader times out
    auto writerCloseThread = std::async(std::launch::async, [&writer]() {
        std::this_thread::sleep_for(CLOSE_WRITER_AFTER_READ);
        writer->close();
        return true;
    });

    auto error = blocking->read(readBuf, WORDCOUNT, READ_TIMEOUT);

    // Ensure that the reader did not timeout
    ASSERT_EQ(error, Sds::Reader::Error::CLOSED);
}

/// This tests that a @c Reader closes if a @c Writer is attached and closed before the @c Reader is first attached
TEST_F(SharedDataStreamTest, writerClosedBeforeAttachingReader) {
    static const size_t WORDSIZE = 2;
    static const size_t WORDCOUNT = 2;
    static const size_t MAXREADERS = 2;

    // Initialize an sds.
    size_t bufferSize = Sds::calculateBufferSize(WORDCOUNT, WORDSIZE, MAXREADERS);
    auto buffer = std::make_shared<Sds::Buffer>(bufferSize);
    auto sds = Sds::create(buffer, WORDSIZE, MAXREADERS);
    ASSERT_NE(sds, nullptr);

    uint8_t readBuf[WORDSIZE * WORDCOUNT * 2];

    auto writer = sds->createWriter(Sds::Writer::Policy::NONBLOCKABLE);
    ASSERT_NE(writer, nullptr);

    // Close the writer before creating reader
    writer->close();

    // Create blocking reader.
    auto blocking = sds->createReader(Sds::Reader::Policy::BLOCKING);
    ASSERT_NE(blocking, nullptr);

    auto error = blocking->read(readBuf, WORDCOUNT);

    ASSERT_EQ(error, Sds::Reader::Error::CLOSED);
}

}  // namespace test
}  // namespace sds
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
