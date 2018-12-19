/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <string>

#include <gtest/gtest.h>

#include "PlaylistParser/PlaylistUtils.h"

namespace alexaClientSDK {
namespace playlistParser {
namespace test {

using namespace ::testing;

static void verifyGetAbsoluteURLResult(std::string baseURL, std::string relativePath, const std::string& expected) {
    std::string actual;
    auto result = getAbsoluteURLFromRelativePathToURL(baseURL, relativePath, &actual);
    EXPECT_EQ(!expected.empty(), result);
    EXPECT_EQ(expected, actual);
}

TEST(PlaylistUtilsTest, testGetAbsoluteURLFromAbsolutePath) {
    std::string baseURL("http://a/b/c/d.m3u8");

    // Success cases.
    verifyGetAbsoluteURLResult(baseURL, "g.mp4", "http://a/b/c/g.mp4");
    verifyGetAbsoluteURLResult(baseURL, "/g.mp4", "http://a/g.mp4");
    verifyGetAbsoluteURLResult(baseURL, "", baseURL);
    verifyGetAbsoluteURLResult(baseURL, "e/f/g.mp4", "http://a/b/c/e/f/g.mp4");
    verifyGetAbsoluteURLResult(baseURL, "/e/f/g.mp4", "http://a/e/f/g.mp4");

    // Failure cases.
    verifyGetAbsoluteURLResult("", "g.mp4", "");
    verifyGetAbsoluteURLResult("http", "g.mp4", "");
    verifyGetAbsoluteURLResult("http://a", "g.mp4", "");
    verifyGetAbsoluteURLResult("http://a", "/g.mp4", "");

    EXPECT_FALSE(getAbsoluteURLFromRelativePathToURL(baseURL, "g.mp4", nullptr));
}

}  // namespace test
}  // namespace playlistParser
}  // namespace alexaClientSDK