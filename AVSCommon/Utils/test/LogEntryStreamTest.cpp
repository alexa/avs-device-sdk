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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "AVSCommon/Utils/Logger/LogEntryStream.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {
namespace test {

using namespace ::testing;

/**
 * Class for testing the LogEntryStream class
 */
class LogEntryStreamTest : public ::testing::Test {
protected:
    /// The LogEntryStream to test.
    LogEntryStream m_stream;
};

/// Test that a new LogEntryStream instance's c_str() returns an empty string.
TEST_F(LogEntryStreamTest, emptyStream) {
    ASSERT_NE(m_stream.c_str(), nullptr);
    ASSERT_EQ(strlen(m_stream.c_str()), 0u);
}

/// Send a character to an empty LogEntryStream.  Expect that c_str() returns a string with just that character.
TEST_F(LogEntryStreamTest, shortString) {
    const char SOME_CHAR = 'x';
    m_stream << SOME_CHAR;
    ASSERT_EQ(SOME_CHAR, m_stream.c_str()[0]);
    ASSERT_EQ(strlen(m_stream.c_str()), 1u);
}

/// Send a medium sized string test to an empty LogEntryStream.  Expect that c_str() returns a matching string.
TEST_F(LogEntryStreamTest, mediumString) {
    const std::string MEDIUM_STRING = "Hello World!";
    m_stream << MEDIUM_STRING;
    ASSERT_EQ(MEDIUM_STRING, m_stream.c_str());
    ASSERT_EQ(strlen(m_stream.c_str()), MEDIUM_STRING.length());
}

/// Send a long string test to an empty LogEntryStream.  Expect that c_str() returns a matching string.
TEST_F(LogEntryStreamTest, longString) {
    std::string longString;
    for (int ix = 0; ix < 100; ix++) {
        longString += "The quick brown fox jumped over the lazy dog.";
    }
    m_stream << longString;
    ASSERT_EQ(longString, m_stream.c_str());
    ASSERT_EQ(strlen(m_stream.c_str()), longString.length());
}

/// Send a few short strings.  Expect that c_str() returns the concatenation of those strings.
TEST_F(LogEntryStreamTest, aFewStrings) {
    const std::string SHORT_STRING_1 = "abc";
    m_stream << SHORT_STRING_1;
    const std::string SHORT_STRING_2 = "xyz";
    m_stream << SHORT_STRING_2;
    const std::string SHORT_STRING_3 = "123";
    m_stream << SHORT_STRING_3;
    auto expectedString = SHORT_STRING_1 + SHORT_STRING_2 + SHORT_STRING_3;
    ASSERT_EQ(expectedString, m_stream.c_str());
    ASSERT_EQ(strlen(m_stream.c_str()), expectedString.length());
}

/// Send a bunch of ints and strings.  Expect that c_str() matches the result of sending the same to ostringstream.
TEST_F(LogEntryStreamTest, aLotOfStrings) {
    std::ostringstream expected;
    const std::string MEDIUM_STRING = "Half a bee, philosophically\nMust, ipso facto, half not be.";
    for (int ix = 0; ix < 100; ix++) {
        m_stream << ix << MEDIUM_STRING;
        expected << ix << MEDIUM_STRING;
    }
    ASSERT_EQ(expected.str(), m_stream.c_str());
    ASSERT_EQ(strlen(m_stream.c_str()), expected.str().length());
}

}  // namespace test
}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
