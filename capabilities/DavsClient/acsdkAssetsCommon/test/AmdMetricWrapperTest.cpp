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

#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <gtest/gtest.h>

#include <future>

#include "TestUtil.h"
#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace test {

using namespace std;
using namespace chrono;
using namespace ::testing;
using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::avsCommon::utils::metrics;

class MetricRecorderTest : public MetricRecorderInterface {
public:
    ~MetricRecorderTest() override = default;
    void recordMetric(std::shared_ptr<MetricEvent> metricEvent) override {
        m_metricEvents.push_back(metricEvent);
    }

    vector<shared_ptr<MetricEvent>> m_metricEvents;
};

class AmdMetricWrapperTest : public Test {
public:
    void SetUp() override {
        metricRecorder = std::make_shared<MetricRecorderTest>();
        AmdMetricsWrapper::setStaticRecorder(metricRecorder);
    }

    void TearDown() override {
        AmdMetricsWrapper::setStaticRecorder(nullptr);
    }

    shared_ptr<MetricRecorderTest> metricRecorder;
    string source = "source";
    string count1 = "count1";
    string count2 = "count2";
    string timerS = "timerS";
    string timerMS = "timerMS";
    string testString = "string1";
};

TEST_F(AmdMetricWrapperTest, SubmittingMetricWithAllPossibleFormats) {  // NOLINT
    AmdMetricsWrapper(source)
            .addCounter(count1)
            .addTimer(timerS, seconds(3))
            .addString(testString, "Test 1")
            .addCounter(count2, 2)
            .addTimer(timerMS, seconds(5));

    ASSERT_TRUE(waitUntil([this] { return metricRecorder->m_metricEvents.size() == 1; }));
    auto metric = metricRecorder->m_metricEvents[0];
    ASSERT_EQ(metric->getActivityName(), source);
    ASSERT_EQ(metric->getDataPoint(count1, DataType::COUNTER).value().getValue(), "1");
    ASSERT_EQ(metric->getDataPoint(timerS, DataType::DURATION).value().getValue(), "3000");
    ASSERT_EQ(metric->getDataPoint(testString, DataType::STRING).value().getValue(), "Test 1");
    ASSERT_EQ(metric->getDataPoint(count2, DataType::COUNTER).value().getValue(), "2");
    ASSERT_EQ(metric->getDataPoint(timerMS, DataType::DURATION).value().getValue(), "5000");
}

}  // namespace test
