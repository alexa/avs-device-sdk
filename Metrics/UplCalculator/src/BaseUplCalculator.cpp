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
#include <AVSCommon/Utils/Metrics/DataType.h>

#include "Metrics/BaseUplCalculator.h"

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/// String to identify log entries originating from this file.
#define TAG "BaseUplCalculator"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// How long to wait to no longer record incoming metrics (mostly for PARSE_COMPLETE and DIRECTIVE_DISPATCHED)
static const std::chrono::seconds METRIC_RECORD_TIMEOUT = std::chrono::seconds(10);

std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface> BaseUplCalculator::createBaseUplCalculator() {
    return std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface>(new BaseUplCalculator());
}

BaseUplCalculator::BaseUplCalculator() : m_startTime(), m_uplInhibited(false) {
}

void BaseUplCalculator::inspectMetric(
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

    if (m_startTime.time_since_epoch() == std::chrono::steady_clock::duration::zero()) {
        m_startTime = std::chrono::steady_clock::now();
    } else if (std::chrono::steady_clock::now() - m_startTime > METRIC_RECORD_TIMEOUT) {
        inhibitSubmission();
        return;
    }

    std::stringstream ss(metricEvent->getActivityName());
    std::string source;
    std::getline(ss, source, '-');
    std::string metricName;
    std::getline(ss, metricName);

    if (START_OF_UTTERANCE == metricName) {
        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());
        std::string dialogID;
        if (getMetricTag(metricName, metricEvent, DIALOG_REQUEST_ID_TAG, dialogID)) {
            m_uplData->addStringData(DIALOG_REQUEST_ID_TAG, dialogID);
        }
    } else if (WW_DURATION == metricName) {
        std::chrono::milliseconds startOfStreamTimestamp;
        std::chrono::milliseconds wakeWordDuration;
        if (getDuration(START_OF_STREAM_TIMESTAMP, metricEvent, startOfStreamTimestamp)) {
            // Sends a warning since it overwrites previous START_OF_UTTERANCE
            m_uplData->addTimepoint(START_OF_UTTERANCE, UplTimePoint(startOfStreamTimestamp));
            if (getDuration(WW_DURATION, metricEvent, wakeWordDuration)) {
                m_uplData->addTimepoint(END_OF_WW, UplTimePoint(startOfStreamTimestamp + wakeWordDuration));
            }
        } else {
            ACSDK_ERROR(
                LX("inspectMetricFailed").d("reason", "missing START_OF_STREAM_TIMESTAMP").d("metricName", metricName));
        }
    } else if (RECOGNIZE_EVENT_IS_BUILT == metricName) {
        m_uplData->addTimepoint(RECOGNIZE_EVENT_IS_BUILT, metricEvent->getSteadyTimestamp());
    } else if (STOP_CAPTURE == metricName) {
        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());
    } else if (END_OF_SPEECH_OFFSET == metricName) {
        std::chrono::milliseconds duration;
        if (getDuration(metricName, metricEvent, duration)) {
            UplTimePoint startOfUtterance = m_uplData->getTimepoint(START_OF_UTTERANCE);
            UplTimePoint endOfUtterance = startOfUtterance + duration;
            m_uplData->addTimepoint(END_OF_UTTERANCE, endOfUtterance);
        }
    } else if (PARSE_COMPLETE == metricName) {
        std::string directiveId;
        if (getMetricTag(metricName, metricEvent, DIRECTIVE_MESSAGE_ID_TAG, directiveId)) {
            m_uplData->addParseCompleteTimepoint(directiveId, metricEvent->getSteadyTimestamp());
        }
    } else if (DIRECTIVE_DISPATCHED_HANDLE == metricName || DIRECTIVE_DISPATCHED_IMMEDIATE == metricName) {
        std::string directiveId;
        if (getMetricTag(metricName, metricEvent, DIRECTIVE_MESSAGE_ID_TAG, directiveId)) {
            m_uplData->addDirectiveDispatchedTimepoint(directiveId, metricEvent->getSteadyTimestamp());
        }
    }  // else, metricName doesn't match any monitored metric names, do nothing
}

bool BaseUplCalculator::getMetricTag(
    const std::string& metricName,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent,
    const std::string& tagName,
    std::string& value) {
    if (!metricEvent) {
        ACSDK_ERROR(LX("getMetricTagFailed").d("reason", "nullMetricEvent"));
        return false;
    }

    auto tag = metricEvent->getDataPoint(tagName, alexaClientSDK::avsCommon::utils::metrics::DataType::STRING);
    if (!tag.hasValue()) {
        ACSDK_ERROR(LX("getMetricTagFailed")
                        .d("reason", "getDataPointFailed")
                        .d("metricName", metricName)
                        .d("tagName", tagName));
        return false;
    }

    auto tagValue = tag.value().getValue();
    if (tagValue.empty()) {
        ACSDK_ERROR(
            LX("getMetricTagFailed").d("reason", "empty tagValue").d("metricName", metricName).d("tagName", tagName));
        return false;
    }

    value = tagValue;
    return true;
}

bool BaseUplCalculator::getDuration(
    const std::string& metricName,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent,
    std::chrono::milliseconds& duration) {
    if (!metricEvent) {
        ACSDK_ERROR(LX("getDurationFailed").d("reason", "nullMetricEvent"));
        return false;
    }

    auto durationResult =
        metricEvent->getDataPoint(metricName, alexaClientSDK::avsCommon::utils::metrics::DataType::DURATION);
    if (!durationResult.hasValue()) {
        ACSDK_ERROR(LX("getDurationFailed").d("reason", "getDataPointFailed").d("metricName", metricName));
        return false;
    }

    duration = std::chrono::milliseconds(std::strtoull(durationResult.value().getValue().c_str(), nullptr, 10));
    return true;
}

void BaseUplCalculator::setUplData(const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData>& uplData) {
    m_uplData = uplData;
}

std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData> BaseUplCalculator::getUplData() const {
    return m_uplData;
}

void BaseUplCalculator::inhibitSubmission() {
    m_uplInhibited = true;
}

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK