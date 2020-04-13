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
#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Metrics/MetricEventBuilder.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/// String to identify log entries originating from this file.
static const std::string TAG("MetricEventBuilder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MetricEventBuilder::MetricEventBuilder() : m_priority{Priority::NORMAL} {
}

MetricEventBuilder& MetricEventBuilder::setActivityName(const std::string& activityName) {
    m_activityName = activityName;
    return *this;
}

MetricEventBuilder& MetricEventBuilder::setPriority(Priority priority) {
    m_priority = priority;
    return *this;
}

MetricEventBuilder& MetricEventBuilder::addDataPoint(const DataPoint& dataPoint) {
    if (!dataPoint.isValid()) {
        ACSDK_WARN(LX("addDataPointFailed").d("reason", "invalidDataPoint"));
        return *this;
    }

    auto key = generateKey(dataPoint.getName(), dataPoint.getDataType());
    if (m_dataPoints.find(key) != m_dataPoints.end()) {
        ACSDK_WARN(LX("addDataPointFailed").m("dataPointAlreadyExists"));
        m_dataPoints.erase(key);
    }

    m_dataPoints.insert(std::make_pair(key, dataPoint));
    return *this;
}

MetricEventBuilder& MetricEventBuilder::addDataPoints(const std::vector<DataPoint>& dataPoints) {
    for (const auto& dataPoint : dataPoints) {
        addDataPoint(dataPoint);
    }

    return *this;
}

MetricEventBuilder& MetricEventBuilder::removeDataPoint(const DataPoint& dataPoint) {
    return removeDataPoint(generateKey(dataPoint.getName(), dataPoint.getDataType()));
}

MetricEventBuilder& MetricEventBuilder::removeDataPoint(const std::string& name, DataType dataType) {
    return removeDataPoint(generateKey(name, dataType));
}

MetricEventBuilder& MetricEventBuilder::removeDataPoints() {
    m_dataPoints.clear();
    return *this;
}

void MetricEventBuilder::clear() {
    m_activityName = "";
    m_priority = Priority::NORMAL;
    removeDataPoints();
}

std::shared_ptr<MetricEvent> MetricEventBuilder::build() {
    if (m_activityName.empty()) {
        ACSDK_WARN(LX("buildFailed").d("reason", "componentNameEmpty"));
        return nullptr;
    }

    return std::make_shared<MetricEvent>(m_activityName, m_priority, m_dataPoints, std::chrono::steady_clock::now());
}

MetricEventBuilder& MetricEventBuilder::removeDataPoint(const std::string& key) {
    if (m_dataPoints.find(key) != m_dataPoints.end()) {
        m_dataPoints.erase(key);
    }

    return *this;
}

std::string MetricEventBuilder::generateKey(const std::string& name, DataType dataType) {
    std::stringstream ss;
    ss << name << "-" << dataType;
    return ss.str();
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK