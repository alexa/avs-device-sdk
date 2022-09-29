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

#include <fstream>
#include <iostream>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "Metrics/SampleMetricSink.h"

/// Root key for metricvs settings.
static std::string METRICS_KEY = "metrics";

/// Key under metrics for string value specifying name of the file to write metrics to.
static std::string METRICS_FILENAME_KEY = "fileName";

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace alexaClientSDK::avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
#define TAG "SampleMetricSink"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<MetricSinkInterface> SampleMetricSink::createMetricSinkInterface() {
    std::string fileName;
    if (!ConfigurationNode::getRoot()[METRICS_KEY].getString(METRICS_FILENAME_KEY, &fileName)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "NoFileNameInConfiguration"));
        return nullptr;
    }
    if (fileName.empty()) {
        ACSDK_ERROR(LX("createFiled").d("reason", "emptyFileName"));
        return nullptr;
    }
    return std::unique_ptr<MetricSinkInterface>(new SampleMetricSink(fileName));
}

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