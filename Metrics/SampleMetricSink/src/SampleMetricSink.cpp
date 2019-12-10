/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "Metrics/SampleMetricSink.h"

#include <fstream>
#include <iostream>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

using namespace alexaClientSDK::avsCommon::utils::metrics;

SampleMetricSink::~SampleMetricSink() {
    m_file.close();
}

SampleMetricSink::SampleMetricSink(const std::string& fileName) {
    m_file.open(fileName);
}

void SampleMetricSink::consumeMetric(std::shared_ptr<MetricEvent> metricEvent) {
    const std::string activityName = metricEvent->getActivityName();
    auto priority = metricEvent->getPriority();

    m_file << "MetricEvent" << std::endl;
    m_file << activityName << "," << priority << std::endl;
    m_file << std::endl;

    m_file << "DataPoints" << std::endl;
    m_file << "Name,Value,DataType" << std::endl;

    auto datapoints = metricEvent->getDataPoints();
    for (const auto& datapoint : datapoints) {
        std::string dpName = datapoint.getName();
        std::string dpValue = datapoint.getValue();
        auto dpType = datapoint.getDataType();
        m_file << dpName << "," << dpValue << "," << dpType << std::endl;
    }
}

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK