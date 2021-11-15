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

#include "acsdkAssetsCommon/AmdMetricWrapper.h"

#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include <utility>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace chrono;
using namespace alexaClientSDK::avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
static const std::string TAG("MetricWrapper");
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<MetricRecorderInterface> AmdMetricsWrapper::s_recorder;

void AmdMetricsWrapper::setStaticRecorder(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> recorder) {
    s_recorder = move(recorder);
}

AmdMetricsWrapper::AmdMetricsWrapper(const string& sourceName) : m_sourceName{sourceName} {
}
AmdMetricsWrapper::~AmdMetricsWrapper() {
    if (s_recorder == nullptr) {
        ACSDK_WARN(LX("~AmdMetricsWrapper").m("Metrics Recorder is not initialized"));
        return;
    }
    if (m_dataPoints.empty()) {
        ACSDK_ERROR(LX("~AmdMetricsWrapper").m("No datapoints to record"));
        return;
    }
    auto metricEvent = MetricEventBuilder{}.setActivityName(m_sourceName).addDataPoints(m_dataPoints).build();
    if (!metricEvent) {
        ACSDK_ERROR(LX("~AmdMetricsWrapper").m("Error creating metric."));
    }
    s_recorder->recordMetric(metricEvent);
}

AmdMetricsWrapper& AmdMetricsWrapper::addCounter(const string& name, int count) {
    ACSDK_DEBUG(LX("addCounter").m("addCounterDataPoint").d("name", name).d("count", count));
    m_dataPoints.push_back(DataPointCounterBuilder{}.setName(name).increment(count).build());

    return *this;
}

AmdMetricsWrapper& AmdMetricsWrapper::addTimer(const string& name, milliseconds value) {
    ACSDK_DEBUG(LX("addTimer").m("addTimerDataPoint").d("name", name).d("duration", value.count()));
    m_dataPoints.push_back(DataPointDurationBuilder{value}.setName(name).build());

    return *this;
}

AmdMetricsWrapper& AmdMetricsWrapper::addString(const string& name, const string& str) {
    ACSDK_DEBUG(LX("addString").m("addStringDataPoint").d("name", name).d("value ", str));
    m_dataPoints.push_back(DataPointStringBuilder{}.setName(name).setValue(str).build());

    return *this;
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
