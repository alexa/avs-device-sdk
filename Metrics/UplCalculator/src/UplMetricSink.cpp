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
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/UplData.h>
#include <Metrics/BaseUplCalculator.h>
#include <Metrics/MediaUplCalculator.h>
#include <Metrics/TtsUplCalculator.h>

#include "Metrics/UplMetricSink.h"

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/// String to identify log entries originating from this file.
#define TAG "UplMetricSink"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// UPL calculators name
static const std::string BASE_UPL_NAME = "base";
static const std::string TTS_UPL_NAME = "tts";
static const std::string MEDIA_UPL_NAME = "media";

std::unique_ptr<avsCommon::utils::metrics::MetricSinkInterface> UplMetricSink::createMetricSinkInterface(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    if (!metricRecorder) {
        ACSDK_ERROR(LX("createMetricSinkInterfaceFailed").d("reason", "nullMetricRecorder"));
        return nullptr;
    }
    return std::unique_ptr<avsCommon::utils::metrics::MetricSinkInterface>(new UplMetricSink(metricRecorder));
}

UplMetricSink::UplMetricSink(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_metricRecorder(metricRecorder) {
}

void UplMetricSink::consumeMetric(std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent) {
    if (!metricEvent) {
        ACSDK_ERROR(LX("consumeMetricFailed").d("reason", "nullMetricEvent"));
        return;
    }

    std::stringstream ss(metricEvent->getActivityName());
    std::string source;
    std::getline(ss, source, '-');
    std::string metricName;
    std::getline(ss, metricName);

    // Reset UPL data on the start of a new utterance
    if (metricName == START_OF_UTTERANCE) {
        uplCalculators[BASE_UPL_NAME] = BaseUplCalculator::createBaseUplCalculator();
        auto metricRecorder = m_metricRecorder.lock();
        if (metricRecorder) {
            uplCalculators[TTS_UPL_NAME] = TtsUplCalculator::createTtsUplCalculator(metricRecorder);
            uplCalculators[MEDIA_UPL_NAME] = MediaUplCalculator::createMediaUplCalculator(metricRecorder);
            std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData> uplData =
                std::make_shared<alexaClientSDK::avsCommon::utils::metrics::UplData>();
            for (auto& kv : uplCalculators) {
                kv.second->setUplData(uplData);
            }
        }
    }

    if (!uplCalculators.empty()) {
        for (auto& kv : uplCalculators) {
            kv.second->inspectMetric(metricEvent);
        }
    }
}

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK
