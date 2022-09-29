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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <Metrics/BaseUplCalculator.h>

#include "Metrics/TtsUplCalculator.h"

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/// String to identify log entries originating from this file.
#define TAG "TtsUplCalculator"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Names of the monitored metrics for UPL
static const std::string FIRST_BYTES_AUDIO = "FIRST_BYTES_AUDIO";
static const std::string TTS_STARTED = "TTS_STARTED";

/// Names of the new recordered timepoints for UPL
static const std::string TTS_DIRECTIVE_PARSED = "TTS_DIRECTIVE_PARSED";
static const std::string TTS_DIRECTIVE_DISPATCHED = "TTS_DIRECTIVE_DISPATCHED";

/// UPL activity and datapoints names
static const std::string UPL_ACTIVITY_NAME = "UPL-TTS";
static const std::string TTS_LATENCY = "TTS_LATENCY";
static const std::string SERVER_PROCESSING = "SERVER_PROCESSING";
static const std::string UTTERANCE_END_TO_STOP_CAPTURE = "UTTERANCE_END_TO_STOP_CAPTURE";
static const std::string STOP_CAPTURE_TO_PARSE_COMPLETE = "STOP_CAPTURE_TO_PARSE_COMPLETE";
static const std::string DEVICE_PROCESSING = "DEVICE_PROCESSING";
static const std::string PARSE_COMPLETE_TO_DISPATCH = "PARSE_COMPLETE_TO_DISPATCH";
static const std::string DISPATCH_TO_FIRST_BYTE_AUDIO = "DISPATCH_TO_FIRST_BYTE_AUDIO";
static const std::string FIRST_BYTE_AUDIO_TO_TTS_STARTED = "FIRST_BYTE_AUDIO_TO_TTS_STARTED";

std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface> TtsUplCalculator::createTtsUplCalculator(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    if (!metricRecorder) {
        ACSDK_ERROR(LX("createTtsUplCalculatorFailed").d("reason", "nullMetricRecorder"));
        return nullptr;
    }
    return std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface>(new TtsUplCalculator(metricRecorder));
}

TtsUplCalculator::TtsUplCalculator(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_metricRecorder(metricRecorder),
        m_uplInhibited(false) {
}

void TtsUplCalculator::inspectMetric(
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent>& metricEvent) {
    if (!metricEvent) {
        ACSDK_ERROR(LX("inspectMetricFailed").d("reason", "nullMetricEvent"));
        return;
    }

    if (m_uplInhibited) {
        return;
    }

    if (!m_uplData) {
        return;
    }

    std::stringstream ss(metricEvent->getActivityName());
    std::string source;
    std::getline(ss, source, '-');
    std::string metricName;
    std::getline(ss, metricName);

    if (metricName == FIRST_BYTES_AUDIO) {
        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());
    } else if (metricName == TTS_STARTED) {
        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());

        std::string directiveId;
        if (BaseUplCalculator::getMetricTag(metricName, metricEvent, DIRECTIVE_MESSAGE_ID_TAG, directiveId)) {
            m_uplData->addTimepoint(TTS_DIRECTIVE_PARSED, m_uplData->getParseCompleteTimepoint(directiveId));
            m_uplData->addTimepoint(TTS_DIRECTIVE_DISPATCHED, m_uplData->getDirectiveDispatchedTimepoint(directiveId));
        }

        calculateTtsUpl();
    }  // else, metricName doesn't match any monitored metric names, do nothing
}

void TtsUplCalculator::setUplData(const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData>& uplData) {
    m_uplData = uplData;
}

std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData> TtsUplCalculator::getUplData() const {
    return m_uplData;
}

void TtsUplCalculator::addDuration(
    alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder& metricEventBuilder,
    const std::string& name,
    const std::string& startTimepointName,
    const std::string& endTimepointName) {
    UplTimePoint startTimepoint = m_uplData->getTimepoint(startTimepointName);
    UplTimePoint endTimepoint = m_uplData->getTimepoint(endTimepointName);

    if (startTimepoint.time_since_epoch().count() == 0) {
        ACSDK_ERROR(LX("addDurationFailed")
                        .d("reason", "invalid startTimepoint")
                        .d("name", name)
                        .d("startTime", startTimepoint.time_since_epoch().count()));
        return;
    }

    if (endTimepoint.time_since_epoch().count() == 0) {
        ACSDK_ERROR(LX("addDurationFailed")
                        .d("reason", "invalid endTimepoint")
                        .d("name", name)
                        .d("endTime", endTimepoint.time_since_epoch().count()));
        return;
    }

    std::chrono::milliseconds duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTimepoint - startTimepoint);

    if (duration < std::chrono::milliseconds::zero()) {
        ACSDK_ERROR(
            LX("addDurationFailed").d("reason", "invalid duration").d("name", name).d("duration", duration.count()));
        return;
    }

    metricEventBuilder.addDataPoint(
        alexaClientSDK::avsCommon::utils::metrics::DataPointDurationBuilder{duration}.setName(name).build());
}

void TtsUplCalculator::calculateTtsUpl() {
    if (m_uplInhibited) {
        return;
    }

    auto metricEventBuilder =
        alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder{}.setActivityName(UPL_ACTIVITY_NAME);

    addDuration(metricEventBuilder, TTS_LATENCY, END_OF_UTTERANCE, TTS_STARTED);
    addDuration(metricEventBuilder, SERVER_PROCESSING, END_OF_UTTERANCE, TTS_DIRECTIVE_PARSED);
    addDuration(metricEventBuilder, DEVICE_PROCESSING, TTS_DIRECTIVE_PARSED, TTS_STARTED);

    addDuration(metricEventBuilder, UTTERANCE_END_TO_STOP_CAPTURE, END_OF_UTTERANCE, STOP_CAPTURE);
    addDuration(metricEventBuilder, STOP_CAPTURE_TO_PARSE_COMPLETE, STOP_CAPTURE, TTS_DIRECTIVE_PARSED);
    addDuration(metricEventBuilder, PARSE_COMPLETE_TO_DISPATCH, TTS_DIRECTIVE_PARSED, TTS_DIRECTIVE_DISPATCHED);
    addDuration(metricEventBuilder, DISPATCH_TO_FIRST_BYTE_AUDIO, TTS_DIRECTIVE_DISPATCHED, FIRST_BYTES_AUDIO);
    addDuration(metricEventBuilder, FIRST_BYTE_AUDIO_TO_TTS_STARTED, FIRST_BYTES_AUDIO, TTS_STARTED);

    metricEventBuilder.addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointStringBuilder{}
                                        .setName(DIALOG_REQUEST_ID_TAG)
                                        .setValue(m_uplData->getStringData(DIALOG_REQUEST_ID_TAG))
                                        .build());
    auto metricsRecorder = m_metricRecorder.lock();
    if (metricsRecorder) {
        metricsRecorder->recordMetric(metricEventBuilder.build());
    } else {
        ACSDK_ERROR(LX("calculateTtsUplFailed").d("reason", "nullMetricRecorder"));
    }
    inhibitSubmission();
}

void TtsUplCalculator::inhibitSubmission() {
    m_uplInhibited = true;
}

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK
