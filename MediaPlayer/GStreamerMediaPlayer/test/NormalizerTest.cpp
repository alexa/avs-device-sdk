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

#include "MediaPlayer/Normalizer.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace test {

using namespace testing;

class NormalizerTest : public ::testing::Test {};

/**
 * Test normalize with a nullptr.
 */
TEST_F(NormalizerTest, testNormalizeNullResult) {
    ASSERT_FALSE(Normalizer::create(0, 1, 0, 1)->normalize(1, nullptr));
}

/**
 * Test create with a source min larger than source max.
 */
TEST_F(NormalizerTest, testCreateSourceMinGreaterThanMax) {
    ASSERT_EQ(Normalizer::create(100, 0, 0, 1), nullptr);
}

/**
 * Test create with a source min equal to source max.
 */
TEST_F(NormalizerTest, testCreateSourceMinEqualToMax) {
    ASSERT_EQ(Normalizer::create(0, 0, 0, 1), nullptr);
}

/**
 * Test create with a normalized min larger than normalized max.
 */
TEST_F(NormalizerTest, testCreateNormalizeMinGreaterThanMax) {
    ASSERT_EQ(Normalizer::create(0, 1, 100, 1), nullptr);
}

/**
 * Test normalize with a normalized min equal to normalized max.
 */
TEST_F(NormalizerTest, testNormalizeNormalizedMinEqualToMax) {
    double result;
    ASSERT_TRUE(Normalizer::create(0, 10, 1, 1)->normalize(2, &result));
    ASSERT_EQ(result, 1);
}

/**
 * Test normalize with an input outside the source bounds.
 */
TEST_F(NormalizerTest, testNormalizeInputOutsideSourceBounds) {
    double result;
    ASSERT_FALSE(Normalizer::create(0, 1, 0, 1)->normalize(2, &result));
}

/**
 * Test normalizing to the same range.
 */
TEST_F(NormalizerTest, testNormalizeSameScale) {
    double result;
    ASSERT_TRUE(Normalizer::create(0, 2, 0, 2)->normalize(1, &result));
    ASSERT_EQ(result, 1);
}

/**
 * Test normalizing to a smaller range.
 */
TEST_F(NormalizerTest, testNormalizeScaleDown) {
    double result;
    ASSERT_TRUE(Normalizer::create(0, 100, 0, 10)->normalize(50, &result));
    ASSERT_EQ(result, 5);
}

/**
 * Test normalizing to a larger range.
 */
TEST_F(NormalizerTest, testNormalizeScaleUp) {
    double result;
    ASSERT_TRUE(Normalizer::create(0, 10, 0, 100)->normalize(5, &result));
    ASSERT_EQ(result, 50);
}

/**
 * Test normalizing to a negative range.
 */
TEST_F(NormalizerTest, testNormalizeNegativeRange) {
    double result;
    ASSERT_TRUE(Normalizer::create(0, 10, -10, 0)->normalize(4, &result));
    ASSERT_EQ(result, -6);
}

/**
 * Test where source min != normalize min.
 */
TEST_F(NormalizerTest, testNormalizeDifferentMinimums) {
    double result;
    ASSERT_TRUE(Normalizer::create(1, 5, 0, 100)->normalize(2, &result));
    ASSERT_EQ(result, 25);
}

/**
 * Test where result is a non-integer.
 */
TEST_F(NormalizerTest, testNonInteger) {
    double result;
    ASSERT_TRUE(Normalizer::create(0, 2, 0, 3)->normalize(1, &result));
    ASSERT_EQ(result, 1.5);
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
