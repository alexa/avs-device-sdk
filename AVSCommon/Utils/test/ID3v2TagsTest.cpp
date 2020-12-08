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

#include <AVSCommon/Utils/ID3Tags/ID3v2Tags.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

using namespace alexaClientSDK::avsCommon::utils::id3Tags;

TEST(ID3v2TagsTest, test_validID3TagsSuccess) {
    constexpr unsigned char validID3v3Tag[] = {'I', 'D', '3', 3, 0, 0, 0, 0, 0, 1};
    auto length = getID3v2TagSize(validID3v3Tag, sizeof(validID3v3Tag));
    ASSERT_EQ(11U, length);

    constexpr unsigned char validID3v4Tag[] = {'I', 'D', '3', 4, 0, 0, 0, 0, 0, 1};
    length = getID3v2TagSize(validID3v4Tag, sizeof(validID3v4Tag));
    ASSERT_EQ(11U, length);
}

TEST(ID3v2TagsTest, test_nullDataFailed) {
    auto length = getID3v2TagSize(nullptr, 0);
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_validID3TagsMaxTagSizeSuccess) {
    constexpr unsigned char validID3v3Tag[] = {'I', 'D', '3', 3, 0, 0, 0x7f, 0x7f, 0x7f, 0x7f};
    auto length = getID3v2TagSize(validID3v3Tag, sizeof(validID3v3Tag));
    unsigned int expectedResult = 0x0fffffff + ID3V2TAG_HEADER_SIZE;
    ASSERT_EQ(expectedResult, length);
}

TEST(ID3v2TagsTest, test_shortID3TagsFailed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 4, 0, 0, 0, 0, 0};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_invalidVersionID3TagsVersionFailed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 5, 0, 0, 0, 0, 0, 1};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_invalidID3TagsSize0Failed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 4, 0, 0, 0, 0, 0, 0};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_invalidID3TagsSize1Failed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 4, 0, 0, 0x80, 0, 0, 1};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_invalidID3TagsSize2Failed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 4, 0, 0, 0, 0x80, 0, 1};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_invalidID3TagsSize3Failed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 4, 0, 0, 0, 0, 0x80, 1};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

TEST(ID3v2TagsTest, test_invalidID3TagsSize4Failed) {
    constexpr unsigned char validID3Tag[] = {'I', 'D', '3', 4, 0, 0, 0, 0, 0, 0x80};
    auto length = getID3v2TagSize(validID3Tag, sizeof(validID3Tag));
    ASSERT_EQ(0U, length);
}

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
