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

#include <acsdk/CodecUtils/Hex.h>

namespace alexaClientSDK {
namespace codecUtils {
namespace test {

using namespace ::testing;

const std::string HEX_STR = "0123456789";
const Bytes HEX_STR_BINARY{0x01, 0x23, 0x45, 0x67, 0x89};
const Bytes HEX_STR_BINARY2{0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89};

// Test string.
static const std::string TEST_STR{"A quick brown fox jumps over the lazy dog."};

// Test string encoded in hex (uppercase).
static const std::string TEST_STR_HEX_U{
    "4120717569636B2062726F776E20666F78206A756D7073206F76657220746865206C617A7920646F672E"};

// Test string encoded in hex (lowercase).
static const std::string TEST_STR_HEX_L{
    "4120717569636b2062726f776e20666f78206a756d7073206f76657220746865206c617a7920646f672e"};

// Verify hex decoding works and resets the output buffer.
TEST(HexCodecTest, test_hexDecode) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex(HEX_STR, decoded));
    ASSERT_EQ(HEX_STR_BINARY, decoded);
}

// Verify hex decoding works up for lowercase letter values
TEST(HexCodecTest, test_hexDecodeAFLowerCase) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex("ab", decoded));
    ASSERT_TRUE(decodeHex("cd", decoded));
    ASSERT_TRUE(decodeHex("ef", decoded));
    ASSERT_EQ((Bytes{0xAB, 0xCD, 0xEF}), decoded);
}

// Verify hex decoding works up for uppercase letter values
TEST(HexCodecTest, test_hexDecodeAFUpperCase) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex("AB", decoded));
    ASSERT_TRUE(decodeHex("CD", decoded));
    ASSERT_TRUE(decodeHex("EF", decoded));
    ASSERT_EQ((Bytes{0xAB, 0xCD, 0xEF}), decoded);
}

// Verify hex decoding works up for mixed case letter values
TEST(HexCodecTest, test_hexDecodeAFMixedCase) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex("Ab", decoded));
    ASSERT_TRUE(decodeHex("cD", decoded));
    ASSERT_TRUE(decodeHex("eF", decoded));
    ASSERT_EQ((Bytes{0xAB, 0xCD, 0xEF}), decoded);
}

// Verify hex decoding works with larger input
TEST(HexCodecTest, test_hexDecodeTestStringUpperCase) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex(TEST_STR_HEX_U, decoded));
    std::string decodedStr{decoded.data(), decoded.data() + decoded.size()};
    ASSERT_EQ(TEST_STR, decodedStr);
}

// Verify hex decoding works with larger input
TEST(HexCodecTest, test_hexDecodeTestStringLowerCase) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex(TEST_STR_HEX_L, decoded));
    std::string decodedStr{decoded.data(), decoded.data() + decoded.size()};
    ASSERT_EQ(TEST_STR, decodedStr);
}

// Verify hex decoding can append data to buffer
TEST(HexCodecTest, test_hexDecodeAppend) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex(HEX_STR, decoded));
    ASSERT_TRUE(decodeHex(HEX_STR, decoded));
    ASSERT_EQ(HEX_STR_BINARY2, decoded);
}

// Verify hex decoding fails on bad size
TEST(HexCodecTest, test_hexDecodeBadSize) {
    Bytes decoded;
    ASSERT_FALSE(decodeHex("012", decoded));
}

// Verify hex decoding fails on bad size
TEST(HexCodecTest, test_hexDecodeBadChar) {
    Bytes decoded;
    ASSERT_FALSE(decodeHex("AZ", decoded));
}

// Verify hex encoding works and resets the buffer
TEST(HexCodecTest, test_hexEncode) {
    std::string encoded{};
    ASSERT_TRUE(encodeHex(HEX_STR_BINARY, encoded));
    ASSERT_EQ(HEX_STR, encoded);
}

// Verify hex encoding works and can append to buffer
TEST(HexCodecTest, test_hexEncodeAppend) {
    std::string encoded;
    ASSERT_TRUE(encodeHex(HEX_STR_BINARY, encoded));
    ASSERT_TRUE(encodeHex(HEX_STR_BINARY, encoded));
    ASSERT_EQ(HEX_STR + HEX_STR, encoded);
}

// Verify hex encode works for A-F
TEST(HexCodecTest, test_hexEncodeAF) {
    std::string encoded;
    ASSERT_TRUE(encodeHex(Bytes{0xab}, encoded));
    ASSERT_TRUE(encodeHex(Bytes{0xcd}, encoded));
    ASSERT_TRUE(encodeHex(Bytes{0xef}, encoded));
    ASSERT_EQ("abcdef", encoded);
}

// Verify hex encode works with test string
TEST(HexCodecTest, test_hexEncodeTestString) {
    std::string encoded;
    ASSERT_TRUE(encodeHex(Bytes{TEST_STR.data(), TEST_STR.data() + TEST_STR.size()}, encoded));
    ASSERT_EQ(TEST_STR_HEX_L, encoded);
}

// Verify hex decoding works up for whitespace values
TEST(HexCodecTest, test_hexDecodeWithWhitespace) {
    Bytes decoded;
    ASSERT_TRUE(decodeHex("\rA B\tC\nD\n", decoded));
    ASSERT_EQ((Bytes{0xAB, 0xCD}), decoded);
}

}  // namespace test
}  // namespace codecUtils
}  // namespace alexaClientSDK
