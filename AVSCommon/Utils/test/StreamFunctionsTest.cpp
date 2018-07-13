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

#include <gtest/gtest.h>

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

/**
 * Utility function to check if a stream contents match a data block
 */
static bool streamAndDataAreEqual(const std::istream& stream, const unsigned char* data, size_t length) {
    std::ostringstream oss;
    oss << stream.rdbuf();

    return oss.str() == std::string(data, data + length);
}

static const unsigned char TEST_DATA[] = {'T', 'E', 'S', 'T', '_', 'D', 'A', 'T', 'A'};

class StreamFunctionsTest : public ::testing::Test {
public:
    StreamFunctionsTest() : m_stream(stream::streamFromData(TEST_DATA, sizeof(TEST_DATA))) {
    }

protected:
    std::unique_ptr<std::istream> m_stream;
};

/**
 * Verify that audio bytes passed in are returned exactly the same
 */
TEST_F(StreamFunctionsTest, StreamFromData) {
    ASSERT_TRUE(streamAndDataAreEqual(*m_stream, TEST_DATA, sizeof(TEST_DATA)));
}

/**
 * Verify that non-char data streams work correctly
 */
TEST_F(StreamFunctionsTest, DataContainsUnprintableChars) {
    const std::vector<std::vector<unsigned char>> testData = {
        {5, 0, 3, 6},         // NULLS in data
        {0, 0, 6, 6},         // NULLS at beginning
        {6, 6, 0, 0},         // NULLS at beginning
        {3, 255, 5, 255, 4},  // -1
        {255, 255, 5, 4},     // -1 at beginning
        {3, 5, 255, 255},     // -1 at end
        {0, 255},             // BOTH
        {255, 0}              // BOTH
    };

    for (const auto& data : testData) {
        auto stream = stream::streamFromData(data.data(), data.size());
        ASSERT_TRUE(streamAndDataAreEqual(*stream, data.data(), data.size()));
    }
}

/**
 * Verify that empty datasets work
 */
TEST_F(StreamFunctionsTest, EmptyVector) {
    const unsigned char empty[] = {};
    auto stream = stream::streamFromData(empty, sizeof(empty));
    ASSERT_TRUE(streamAndDataAreEqual(*stream, empty, sizeof(empty)));
}

/**
 * Verify that multiple streams created from the same source can be operated on independently
 */
TEST_F(StreamFunctionsTest, MultipleStreams) {
    // get two streams to the same data
    auto stream1 = stream::streamFromData(TEST_DATA, sizeof(TEST_DATA));
    auto stream2 = stream::streamFromData(TEST_DATA, sizeof(TEST_DATA));

    // get a single char from one stream
    char c;
    stream1->get(c);

    // check to make sure that the streams are not locked together
    ASSERT_EQ(TEST_DATA[0], c);
    ASSERT_TRUE(streamAndDataAreEqual(*stream1, TEST_DATA + 1, sizeof(TEST_DATA) - 1));
    ASSERT_TRUE(streamAndDataAreEqual(*stream2, TEST_DATA, sizeof(TEST_DATA)));
}

/**
 * Verify that seekg works going forward
 */
TEST_F(StreamFunctionsTest, seekgBasicForward) {
    const std::streampos step = 2;
    m_stream->seekg(step);
    ASSERT_TRUE(streamAndDataAreEqual(*m_stream, TEST_DATA + step, sizeof(TEST_DATA) - step));
}

/**
 * Verify that seekg can reset
 */
TEST_F(StreamFunctionsTest, seekgBasicReset) {
    // get 4 chars from char from one stream
    char c;
    m_stream->get(c);
    m_stream->get(c);
    m_stream->get(c);
    m_stream->get(c);

    // reset back to beginning
    m_stream->seekg(0);

    ASSERT_TRUE(streamAndDataAreEqual(*m_stream, TEST_DATA, sizeof(TEST_DATA)));
}

/**
 * Verify that tellg works on creation
 */
TEST_F(StreamFunctionsTest, tellgBasic) {
    ASSERT_EQ(0, m_stream->tellg());
}

/**
 * Verify that the stream will have a bad tellg result when seeking past end
 */
TEST_F(StreamFunctionsTest, tellgPastEnd) {
    m_stream->seekg(sizeof(TEST_DATA) + 1);
    ASSERT_EQ(-1, m_stream->tellg());
}

/**
 * Verify that the stream will have a bad tellg result when seeking before beginning
 */
TEST_F(StreamFunctionsTest, tellgBeforeBeginning) {
    m_stream->seekg(-1);
    ASSERT_EQ(-1, m_stream->tellg());
}

/**
 * Verify that tellg is set correctly after seeking
 */
TEST_F(StreamFunctionsTest, tellgAfterSeeking) {
    const std::streampos step = 2;
    m_stream->seekg(step);
    ASSERT_EQ(step, m_stream->tellg());
    ASSERT_TRUE(streamAndDataAreEqual(*m_stream, TEST_DATA + step, sizeof(TEST_DATA) - step));
}

/**
 * Verify that tellg is set correctly after reading from stream
 */
TEST_F(StreamFunctionsTest, tellgAfterReading) {
    // get 4 chars from char from one stream
    const int numberToRead = 4;

    for (int i = 0; i < numberToRead; ++i) {
        char c;
        m_stream->get(c);
    }

    ASSERT_EQ(numberToRead, m_stream->tellg());
}

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
