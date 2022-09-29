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
#include "Metrics/MetricRecorder.h"

#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/// String to identify log entries originating from this file.
#define TAG "MetricRecorder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> MetricRecorder::createMetricRecorderInterface(
    std::unique_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricSinkInterface> sink) {
    if (!sink) {
        return nullptr;
    }
    auto recorder = std::make_shared<MetricRecorder>();
    recorder->addSink(std::move(sink));
    return recorder;
}

bool MetricRecorder::addSink(std::unique_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricSinkInterface> sink) {
    if (!sink) {
        ACSDK_WARN(LX("addSinkFailed").d("reason", "nullSink"));
        return false;
    }

    m_sinks.insert(std::move(sink));
    return true;
}

void MetricRecorder::recordMetric(std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent) {
    if (!metricEvent) {
        ACSDK_ERROR(LX("recordMetricFailed").d("reason", "nullMetricEvent"));
        return;
    }

    if (m_sinks.empty()) {
        ACSDK_WARN(LX("emptySinks"));
        return;
    }

    m_executor.execute([this, metricEvent]() {
        for (const auto& sink : m_sinks) {
            sink->consumeMetric(metricEvent);
        }
    });
}

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK