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

#include <AVSCommon/Utils/Stream/Streambuf.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

static const std::vector<unsigned char> testData = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

class StreambufTest : public ::testing::Test {
public:
    StreambufTest() : m_sb(testData.data(), testData.size()) {
    }

protected:
    stream::Streambuf m_sb;
};

/**
 * Verify that the Streambuf is created correctly
 */
TEST_F(StreambufTest, Creation) {
    ASSERT_EQ(testData[0], m_sb.sgetc());
}

/**
 * Verify that the seekoff can be called based from the beginning
 */
TEST_F(StreambufTest, seekoffBeginning) {
    const std::vector<std::streampos> positions = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (const auto& pos : positions) {
        ASSERT_EQ(pos, m_sb.seekoff(pos, std::ios_base::beg));
        ASSERT_EQ(testData[pos], m_sb.sgetc());
    }
}

/**
 * Verify that the seekoff can be called based from the current positon
 */
TEST_F(StreambufTest, seekoffCurrentForward) {
    const std::streampos pos = 3;
    ASSERT_EQ(pos, m_sb.seekoff(pos, std::ios_base::cur));
    ASSERT_EQ(testData[pos], m_sb.sgetc());

    ASSERT_EQ(2 * pos, m_sb.seekoff(pos, std::ios_base::cur));
    ASSERT_EQ(testData[2 * pos], m_sb.sgetc());
}

/**
 * Verify that you can seek all the way until the end correctly
 */
TEST_F(StreambufTest, seekoffFromBeginningUntilEnd) {
    const std::streampos step = 1;
    m_sb.seekoff(0, std::ios_base::beg);
    for (size_t i = 0; i < testData.size() - 1; ++i) {
        ASSERT_EQ(testData[i], m_sb.sgetc());
        ASSERT_NE(-1, m_sb.seekoff(step, std::ios_base::cur));
    }
    ASSERT_EQ(testData[testData.size() - 1], m_sb.sgetc());
    ASSERT_EQ(-1, m_sb.seekoff(step, std::ios_base::cur));
}

/**
 * Verify that you can seek all the way from the end to the beginning
 */
TEST_F(StreambufTest, seekoffFromEndUntilBeginning) {
    const std::streampos step = -1;
    m_sb.seekoff(-1, std::ios_base::end);
    for (size_t i = 0; i < testData.size() - 1; ++i) {
        ASSERT_EQ(testData[testData.size() - i - 1], m_sb.sgetc());
        ASSERT_NE(-1, m_sb.seekoff(step, std::ios_base::cur));
    }
    ASSERT_EQ(testData[0], m_sb.sgetc());
    ASSERT_EQ(-1, m_sb.seekoff(step, std::ios_base::cur));
}

/**
 * Verify that you can seek backward from the end
 */
TEST_F(StreambufTest, seekoffCurrentBackward) {
    auto end = m_sb.seekoff(-1, std::ios_base::end);

    const std::streampos pos = 3;
    ASSERT_EQ(end - pos, m_sb.seekoff(-pos, std::ios_base::cur));
    ASSERT_EQ(testData[end - pos], m_sb.sgetc());

    ASSERT_EQ(end - (2 * pos), m_sb.seekoff(-pos, std::ios_base::cur));
    ASSERT_EQ(testData[end - (2 * pos)], m_sb.sgetc());
}

/**
 * Verify that a seek to before the stream results in an error
 */
TEST_F(StreambufTest, seekoffBeforeStart) {
    ASSERT_EQ(-1, m_sb.seekoff(-1, std::ios_base::beg));
}

/**
 * Verify that a seek to after the stream results in an error
 */
TEST_F(StreambufTest, seekoffPastEnd) {
    ASSERT_EQ(-1, m_sb.seekoff(1, std::ios_base::end));
}

/**
 * Verify that a basic seekpos works
 */
TEST_F(StreambufTest, seekpos) {
    const std::streampos pos = 3;
    ASSERT_EQ(pos, m_sb.seekpos(pos));
    ASSERT_EQ(testData[pos], m_sb.sgetc());
}

/**
 * Verify that a basic seekpos before the beginning results in an error
 */
TEST_F(StreambufTest, seekposBeforeStart) {
    ASSERT_EQ(-1, m_sb.seekpos(-1));
}

/**
 * Verify that a basic seekpos after the end results in an error
 */
TEST_F(StreambufTest, seekposAfterEnd) {
    ASSERT_EQ(-1, m_sb.seekpos(sizeof(testData) + 1));
}

/**
 * Verify that a seekpos to the end is correct
 */
TEST_F(StreambufTest, seekposToEnd) {
    auto end = m_sb.seekoff(0, std::ios_base::end);
    ASSERT_EQ(end, m_sb.seekpos(end));
}

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
