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
#include "AVSCommon/Utils/Metrics/MetricEvent.h"

#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Metrics/MetricEventBuilder.h"
#include "AVSCommon/Utils/Optional.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/// String to identify log entries originating from this file.
#define TAG "MetricEvent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MetricEvent::MetricEvent(
    const std::string& activityName,
    Priority priority,
    const std::unordered_map<std::string, DataPoint>& dataPoints,
    std::chrono::steady_clock::time_point timestamp) :
        m_activityName{activityName},
        m_priority{priority},
        m_dataPoints{dataPoints},
        m_timestamp{timestamp} {
}

std::string MetricEvent::getActivityName() const {
    return m_activityName;
}

Priority MetricEvent::getPriority() const {
    return m_priority;
}

Optional<DataPoint> MetricEvent::getDataPoint(const std::string& name, DataType dataType) const {
    std::string key = MetricEventBuilder::generateKey(name, dataType);
    if (m_dataPoints.find(key) == m_dataPoints.end()) {
        ACSDK_DEBUG9(LX("getDataPointWarning").d("reason", "dataPointDoesntExist"));
        return Optional<DataPoint>{};
    }

    return Optional<DataPoint>{m_dataPoints.find(key)->second};
}

std::vector<DataPoint> MetricEvent::getDataPoints() const {
    std::vector<DataPoint> dataPoints;
    for (const auto& keyValuePair : m_dataPoints) {
        dataPoints.push_back(keyValuePair.second);
    }

    return dataPoints;
}

std::chrono::system_clock::time_point MetricEvent::getTimestamp() const {
    return std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::system_clock::duration>(
                                                  std::chrono::steady_clock::now() - m_timestamp);
}

std::chrono::steady_clock::time_point MetricEvent::getSteadyTimestamp() const {
    return m_timestamp;
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK