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

/// @file CaptionFormatAvsTest.cpp

#include <gtest/gtest.h>

#include <Captions/CaptionFormat.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace captions;

/**
 * Tests parsing of the avs-compliant text "WEBVTT" to CaptionFormat::WEBVTT.
 */
TEST(CaptionFormatAvsTest, test_parseWebvttCaptionTypeFromString) {
    ASSERT_EQ(CaptionFormat::WEBVTT, avsStringToCaptionFormat("WEBVTT"));
}

/**
 * Tests parsing of an unknown text "FOO" to CaptionFormat::UNKNOWN.
 */
TEST(CaptionFormatAvsTest, test_parseUnkownCaptionTypeFromString) {
    ASSERT_EQ(CaptionFormat::UNKNOWN, avsStringToCaptionFormat("FOO"));
}

/**
 * Tests parsing of the empty string to CaptionFormat::UNKNOWN.
 */
TEST(CaptionFormatAvsTest, test_parseUnkownCaptionTypeFromUnknownString) {
    ASSERT_EQ(CaptionFormat::UNKNOWN, avsStringToCaptionFormat(""));
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK