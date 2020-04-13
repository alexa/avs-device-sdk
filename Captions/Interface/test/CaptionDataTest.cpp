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

/// @file CaptionDataTest.cpp

#include <gtest/gtest.h>

#include <Captions/CaptionData.h>

namespace alexaClientSDK {
namespace captions {
namespace test {

/**
 * Test that @c CaptionData is invalid when it contains text and has a format of UNKNOWN.
 */
TEST(CaptionDataTest, test_captionFormatUnknownIsInvalidWithNonBlankText) {
    CaptionData captionData = CaptionData(CaptionFormat::UNKNOWN, "Some unknown caption data");
    ASSERT_FALSE(captionData.isValid());
}

/**
 * Test that @c CaptionData is invalid when it contains no text and has a format of UNKNOWN.
 */
TEST(CaptionDataTest, test_captionFormatUnknownIsInvalidWithBlankText) {
    CaptionData captionData = CaptionData(CaptionFormat::UNKNOWN);
    ASSERT_FALSE(captionData.isValid());
}

/**
 * Test that @c CaptionData is valid when it contains text and has a format of WEBVTT.
 */
TEST(CaptionDataTest, test_captionFormatWebvttIsValidWithNonBlankText) {
    CaptionData captionData =
        CaptionData(CaptionFormat::WEBVTT, "WEBVTT\n\n1\n00:00:00.000 --> 00:00:01.500\nTest for WebVTT format.");
    ASSERT_TRUE(captionData.isValid());
}

/**
 * Test that @c CaptionData is invalid when it contains no text and has a format of WEBVTT.
 */
TEST(CaptionDataTest, test_captionFormatWebvttIsValidWithBlankText) {
    CaptionData captionData = CaptionData(CaptionFormat::WEBVTT);
    ASSERT_FALSE(captionData.isValid());
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK