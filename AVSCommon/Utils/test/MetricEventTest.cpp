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

// @file MetricEventTest.cpp

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Metrics/DataPoint.h"
#include "AVSCommon/Utils/Metrics/DataPointStringBuilder.h"
#include "AVSCommon/Utils/Metrics/MetricEvent.h"
#include "AVSCommon/Utils/Metrics/MetricEventBuilder.h"
#include "AVSCommon/Utils/Metrics/Priority.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon::utils::metrics;

/**
 * Class for testing the MetricEvent class
 */
class MetricEventTest : public ::testing::Test {};

/**
 * Tests default MetricEventBuilder build()
 */
TEST_F(MetricEventTest, test_buildDefault) {
    MetricEventBuilder builder = MetricEventBuilder{};
    std::shared_ptr<MetricEvent> metricEvent = builder.build();
    ASSERT_TRUE(metricEvent == nullptr);

    metricEvent = builder.setActivityName("activityName").build();
    ASSERT_TRUE(metricEvent != nullptr);
}

/**
 * Tests default MetricEventBuilder addDataPoint()
 */
TEST_F(MetricEventTest, test_addDataPoint) {
    MetricEventBuilder builder = MetricEventBuilder{};
    DataPointStringBuilder stringBuilder = DataPointStringBuilder{};

    DataPoint stringData1 = stringBuilder.setName("stringName").setValue("stringValue").build();
    DataPoint stringData2 = stringBuilder.setValue("changedValue").build();
    std::shared_ptr<MetricEvent> metricEvent = builder.setActivityName("activityName")
                                                   .setPriority(Priority::NORMAL)
                                                   .addDataPoint(stringData1)
                                                   .addDataPoint(stringData2)
                                                   .build();

    ASSERT_TRUE(metricEvent != nullptr);
    ASSERT_EQ(metricEvent->getActivityName(), "activityName");
    ASSERT_EQ(metricEvent->getPriority(), Priority::NORMAL);

    DataPoint retrievedDataPoint = metricEvent->getDataPoint("stringName", DataType::STRING).value();
    ASSERT_EQ(retrievedDataPoint.getName(), "stringName");
    ASSERT_EQ(retrievedDataPoint.getValue(), "changedValue");
    ASSERT_EQ(retrievedDataPoint.getDataType(), DataType::STRING);

    DataPoint invalidDataPoint = metricEvent->getDataPoint("invalid", DataType::STRING).value();
    ASSERT_TRUE(!invalidDataPoint.isValid());
    ASSERT_EQ(invalidDataPoint.getName(), "");
    ASSERT_EQ(invalidDataPoint.getValue(), "");

    unsigned int expectedSize = 1;
    ASSERT_EQ(metricEvent->getDataPoints().size(), expectedSize);
}

/**
 * Tests default MetricEventBuilder removeDataPoint()
 */
TEST_F(MetricEventTest, test_removeDataPointByObject) {
    MetricEventBuilder builder = MetricEventBuilder{};
    DataPointStringBuilder stringBuilder = DataPointStringBuilder{};

    DataPoint stringData1 = stringBuilder.setName("stringName").setValue("stringValue").build();

    std::shared_ptr<MetricEvent> metricEvent = builder.setActivityName("activityName")
                                                   .setPriority(Priority::NORMAL)
                                                   .addDataPoint(stringData1)
                                                   .removeDataPoint(stringData1)
                                                   .build();

    ASSERT_TRUE(metricEvent != nullptr);
    ASSERT_EQ(metricEvent->getActivityName(), "activityName");
    ASSERT_EQ(metricEvent->getPriority(), Priority::NORMAL);
    unsigned int expectedSize = 0;
    ASSERT_EQ(metricEvent->getDataPoints().size(), expectedSize);
}

/**
 * Tests default MetricEventBuilder removeDataPoint()
 */
TEST_F(MetricEventTest, test_removeDataPointByNameAndDataType) {
    MetricEventBuilder builder = MetricEventBuilder{};
    DataPointStringBuilder stringBuilder = DataPointStringBuilder{};

    DataPoint stringData1 = stringBuilder.setName("stringName").setValue("stringValue").build();

    std::shared_ptr<MetricEvent> metricEvent = builder.setActivityName("activityName")
                                                   .setPriority(Priority::NORMAL)
                                                   .addDataPoint(stringData1)
                                                   .removeDataPoint("stringName", DataType::STRING)
                                                   .build();

    ASSERT_TRUE(metricEvent != nullptr);
    ASSERT_EQ(metricEvent->getActivityName(), "activityName");
    ASSERT_EQ(metricEvent->getPriority(), Priority::NORMAL);
    unsigned int expectedSize = 0;
    ASSERT_EQ(metricEvent->getDataPoints().size(), expectedSize);
}

/**
 * Tests default MetricEventBuilder removeDataPoints()
 */
TEST_F(MetricEventTest, test_removeDataPoints) {
    MetricEventBuilder builder = MetricEventBuilder{};
    DataPointStringBuilder stringBuilder = DataPointStringBuilder{};

    DataPoint stringData1 = stringBuilder.setName("stringName").setValue("stringValue").build();

    DataPoint stringData2 = stringBuilder.setName("anotherString").setValue("anotherValue").build();

    std::shared_ptr<MetricEvent> metricEvent = builder.setActivityName("activityName")
                                                   .setPriority(Priority::NORMAL)
                                                   .addDataPoint(stringData1)
                                                   .addDataPoint(stringData2)
                                                   .removeDataPoints()
                                                   .build();

    ASSERT_TRUE(metricEvent != nullptr);
    ASSERT_EQ(metricEvent->getActivityName(), "activityName");
    ASSERT_EQ(metricEvent->getPriority(), Priority::NORMAL);
    unsigned int expectedSize = 0;
    ASSERT_EQ(metricEvent->getDataPoints().size(), expectedSize);
}

/**
 * Tests default MetricEventBuilder clear()
 */
TEST_F(MetricEventTest, test_clear) {
    MetricEventBuilder builder = MetricEventBuilder{};
    DataPointStringBuilder stringBuilder = DataPointStringBuilder{};

    DataPoint stringData1 = stringBuilder.setName("stringName").setValue("stringValue").build();

    DataPoint stringData2 = stringBuilder.setName("anotherString").setValue("anotherValue").build();

    builder.setActivityName("activityName")
        .setPriority(Priority::NORMAL)
        .addDataPoint(stringData1)
        .addDataPoint(stringData2)
        .clear();

    std::shared_ptr<MetricEvent> metricEvent = builder.build();
    ASSERT_TRUE(metricEvent == nullptr);
}

}  // namespace test
}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
