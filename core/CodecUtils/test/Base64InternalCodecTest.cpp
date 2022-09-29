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

#include <gtest/gtest.h>

#include <Base64Internal.cpp>

namespace alexaClientSDK {
namespace codecUtils {
namespace test {

using namespace ::testing;

// Test string.
static const std::string TEST_STR{"A quick brown fox jumps over the lazy dog."};

// Test string encoded in Base64.
static const std::string TEST_STR_B64{"QSBxdWljayBicm93biBmb3gganVtcHMgb3ZlciB0aGUgbGF6eSBkb2cu"};

// Test empty encoding works and resets buffer
TEST(Base64InternalCodecTest, test_base64EncodeEmpty) {
    std::string encoded;
    ASSERT_TRUE(encodeBase64(Bytes{}, encoded));
    ASSERT_TRUE(encoded.empty());
}

// Test encoding works and appends data
TEST(Base64InternalCodecTest, test_base64EncodeAppend) {
    std::string encoded{"prefix:"};
    ASSERT_TRUE(encodeBase64(Bytes{0, 1, 2}, encoded));
    ASSERT_EQ("prefix:AAEC", encoded);
}

// Test encoding works
TEST(Base64InternalCodecTest, test_base64EncodeTestStr) {
    std::string encoded;
    Bytes source{TEST_STR.data(), TEST_STR.data() + TEST_STR.size()};
    ASSERT_TRUE(encodeBase64(source, encoded));
    ASSERT_EQ(TEST_STR_B64, encoded);
}

// Test empty decoding works
TEST(Base64InternalCodecTest, test_base64DecodeEmpty) {
    Bytes decoded;
    ASSERT_TRUE(decodeBase64("", decoded));
    ASSERT_TRUE(decoded.empty());
}

// Test decoding works and appends buffer
TEST(Base64InternalCodecTest, test_base64DecodeAppend) {
    Bytes decoded{1};
    ASSERT_TRUE(decodeBase64("AAEC", decoded));
    ASSERT_EQ((Bytes{1, 0, 1, 2}), decoded);
}

// Test decoding works
TEST(Base64InternalCodecTest, test_base64DecodeTestStr) {
    Bytes decoded;
    ASSERT_TRUE(decodeBase64(TEST_STR_B64, decoded));
    std::string decodedStr{decoded.data(), decoded.data() + decoded.size()};
    ASSERT_EQ(TEST_STR, decodedStr);
}

// Test decoding works and appends buffer
TEST(Base64InternalCodecTest, test_base64DecodeAppendWhitespace) {
    Bytes decoded{1};
    ASSERT_TRUE(decodeBase64(" \t\n\rA A\t\n\r E C\r\n\t ", decoded));
    ASSERT_EQ((Bytes{1, 0, 1, 2}), decoded);
}

// Test decoding fails on error
TEST(Base64InternalCodecTest, test_base64DecodeError) {
    Bytes decoded;
    ASSERT_FALSE(decodeBase64("....", decoded));
}

// Test decoding fails on error
TEST(Base64InternalCodecTest, test_base64DecodeErrorBadTail) {
    Bytes decoded;
    ASSERT_FALSE(decodeBase64("AA=C", decoded));
}

// Test decoding fails on error
TEST(Base64InternalCodecTest, test_base64DecodeErrorDataAfterEnd) {
    Bytes decoded;
    ASSERT_FALSE(decodeBase64("AA==AAEC", decoded));
}

// Test decoding fails on error
TEST(Base64InternalCodecTest, test_base64DecodeErrorEarlyEnd) {
    Bytes decoded;
    ASSERT_FALSE(decodeBase64("A===", decoded));
}

}  // namespace test
}  // namespace codecUtils
}  // namespace alexaClientSDK
