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

#include <chrono>

#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

#include "IPCServerSampleApp/TelemetrySink.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

TelemetrySink::TelemetrySink(std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_metricRecorder{std::move(metricRecorder)} {
}

void TelemetrySink::reportTimer(
    const std::map<std::string, std::string>& metadata,
    const std::string& name,
    const std::chrono::nanoseconds& value) {
    auto durationInMillis = std::chrono::duration_cast<std::chrono::milliseconds>(value);
    avsCommon::utils::metrics::MetricEventBuilder builder;
    builder.setActivityName(name);
    builder.setPriority(avsCommon::utils::metrics::Priority::HIGH);
    builder.addDataPoint(avsCommon::utils::metrics::DataPointDurationBuilder(durationInMillis).setName(name).build());
    for (const auto& entry : metadata) {
        builder.addDataPoint(
            avsCommon::utils::metrics::DataPointStringBuilder{}.setName(entry.first).setValue(entry.second).build());
    }

    auto event = builder.build();
    m_metricRecorder->recordMetric(event);
}

void TelemetrySink::reportCounter(
    const std::map<std::string, std::string>& metadata,
    const std::string& name,
    uint64_t value) {
    avsCommon::utils::metrics::MetricEventBuilder builder;
    builder.setActivityName(name);
    builder.setPriority(avsCommon::utils::metrics::Priority::HIGH);
    builder.addDataPoint(avsCommon::utils::metrics::DataPointCounterBuilder().setName(name).increment(value).build());
    for (const auto& entry : metadata) {
        builder.addDataPoint(
            avsCommon::utils::metrics::DataPointStringBuilder{}.setName(entry.first).setValue(entry.second).build());
    }

    auto event = builder.build();
    m_metricRecorder->recordMetric(event);
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK