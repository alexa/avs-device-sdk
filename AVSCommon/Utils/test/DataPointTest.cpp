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

// @file DataPointTest.cpp

#include <gtest/gtest.h>
#include <chrono>
#include <limits>
#include <thread>

#include "AVSCommon/Utils/Metrics/DataPoint.h"
#include "AVSCommon/Utils/Metrics/DataPointCounterBuilder.h"
#include "AVSCommon/Utils/Metrics/DataPointDurationBuilder.h"
#include "AVSCommon/Utils/Metrics/DataPointStringBuilder.h"
#include "AVSCommon/Utils/Metrics/DataType.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::utils::metrics;

/**
 * Class for testing the DataPoint class
 */
class DataPointTest : public ::testing::Test {};

/**
 * Tests default build()
 */
TEST_F(DataPointTest, test_buildDefault) {
    DataPoint stringDataPoint = DataPointStringBuilder{}.build();
    DataPoint counterDataPoint = DataPointCounterBuilder{}.build();
    DataPoint timerDataPoint = DataPointDurationBuilder{}.build();

    ASSERT_TRUE(!stringDataPoint.isValid());
    ASSERT_TRUE(!counterDataPoint.isValid());
    ASSERT_TRUE(!timerDataPoint.isValid());

    ASSERT_EQ(stringDataPoint.getName(), "");
    ASSERT_EQ(stringDataPoint.getValue(), "");
    ASSERT_EQ(stringDataPoint.getDataType(), DataType::STRING);

    ASSERT_EQ(counterDataPoint.getName(), "");
    ASSERT_EQ(counterDataPoint.getValue(), "0");
    ASSERT_EQ(counterDataPoint.getDataType(), DataType::COUNTER);

    ASSERT_EQ(timerDataPoint.getName(), "");
    ASSERT_EQ(timerDataPoint.getValue(), "0");
    ASSERT_EQ(timerDataPoint.getDataType(), DataType::DURATION);
}

/**
 * Tests setName and setValue
 */
TEST_F(DataPointTest, test_builderMethods) {
    DataPoint stringDataPoint = DataPointStringBuilder{}.setName("stringName").setValue("stringValue").build();

    DataPoint counterDataPoint = DataPointCounterBuilder{}.setName("counterName").increment(100).build();

    std::chrono::milliseconds duration(39342);
    DataPoint timerDataPoint = DataPointDurationBuilder{duration}.setName("timerName").build();

    ASSERT_TRUE(stringDataPoint.isValid());
    ASSERT_TRUE(counterDataPoint.isValid());
    ASSERT_TRUE(timerDataPoint.isValid());

    ASSERT_EQ(stringDataPoint.getName(), "stringName");
    ASSERT_EQ(stringDataPoint.getValue(), "stringValue");
    ASSERT_EQ(stringDataPoint.getDataType(), DataType::STRING);

    ASSERT_EQ(counterDataPoint.getName(), "counterName");
    ASSERT_EQ(counterDataPoint.getValue(), "100");
    ASSERT_EQ(counterDataPoint.getDataType(), DataType::COUNTER);

    ASSERT_EQ(timerDataPoint.getName(), "timerName");
    ASSERT_EQ(timerDataPoint.getValue(), "39342");
    ASSERT_EQ(timerDataPoint.getDataType(), DataType::DURATION);
}

/**
 * Tests counter dataPoint
 */
TEST_F(DataPointTest, test_counterDataPoint) {
    DataPoint counterDataPoint = DataPointCounterBuilder{}
                                     .setName("counterName")
                                     .increment(std::numeric_limits<uint64_t>::max())
                                     .increment(1)
                                     .build();

    std::string expectedValue = std::to_string(std::numeric_limits<uint64_t>::max());
    ASSERT_EQ(counterDataPoint.getName(), "counterName");
    ASSERT_EQ(counterDataPoint.getValue(), expectedValue);
    ASSERT_EQ(counterDataPoint.getDataType(), DataType::COUNTER);
}

/**
 * Tests duration dataPoint
 */
TEST_F(DataPointTest, test_durationDataPoint) {
    DataPointDurationBuilder timerBuilder = DataPointDurationBuilder{}.setName("durationName");
    timerBuilder.startDurationTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    DataPoint timerDataPoint = timerBuilder.stopDurationTimer().build();
    ASSERT_TRUE(timerDataPoint.isValid());
    ASSERT_EQ(timerDataPoint.getName(), "durationName");
    ASSERT_TRUE(stoull(timerDataPoint.getValue()) > 0);
    ASSERT_EQ(timerDataPoint.getDataType(), DataType::DURATION);
}

/**
 * Tests negative duration dataPoint
 */
TEST_F(DataPointTest, test_negativeDurationDataPoint) {
    DataPointDurationBuilder timerBuilder =
        DataPointDurationBuilder{std::chrono::milliseconds(-100)}.setName("durationName");
    DataPoint timerDataPoint = timerBuilder.build();
    ASSERT_TRUE(timerDataPoint.isValid());
    ASSERT_EQ(timerDataPoint.getName(), "durationName");
    ASSERT_EQ(timerDataPoint.getValue(), "0");
    ASSERT_EQ(timerDataPoint.getDataType(), DataType::DURATION);
}

}  // namespace test
}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
