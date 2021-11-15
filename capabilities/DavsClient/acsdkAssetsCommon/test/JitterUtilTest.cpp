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

#include "acsdkAssetsCommon/JitterUtil.h"

using namespace std;
using namespace std::chrono;
using namespace ::testing;
using namespace alexaClientSDK::acsdkAssets::common::jitterUtil;

static const int NUMBER_OF_TRIES = 1000000;
static constexpr milliseconds DELAY_MS(1000);
static const float FACTOR = 0.2;

class JitterUtilTest : public Test {};

TEST_F(JitterUtilTest, jitterNeverFallsTest) {
    const milliseconds MIN_RANGE(800);
    const milliseconds MAX_RANGE(1200);
    milliseconds minValue = MAX_RANGE;
    milliseconds maxValue = MIN_RANGE;
    auto value = DELAY_MS;
    for (int i = 0; i < NUMBER_OF_TRIES; i++) {
        auto newValue = jitter(value, FACTOR);
        ASSERT_GE(newValue, MIN_RANGE);
        ASSERT_LE(newValue, MAX_RANGE);
        minValue = (minValue < newValue) ? minValue : newValue;
        maxValue = (maxValue > newValue) ? maxValue : newValue;
    }

    // Test if it's truly random
    ASSERT_GT(maxValue.count(), 1190);
    ASSERT_LT(minValue.count(), 810);
}

TEST_F(JitterUtilTest, jitterNeverZeroTest) {
    auto value = milliseconds(DELAY_MS);
    for (int i = 0; i < NUMBER_OF_TRIES; i++) {
        value = jitter(value, FACTOR);
        ASSERT_NE(value.count(), 0);
    }
}

TEST_F(JitterUtilTest, expJitterAlwaysGreaterTest) {
    auto value = milliseconds(DELAY_MS);
    for (int i = 0; i < NUMBER_OF_TRIES; i++) {
        ASSERT_GT(expJitter(value, FACTOR).count(), value.count());
    }
}
