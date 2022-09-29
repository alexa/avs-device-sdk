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
#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <Metrics/BaseUplCalculator.h>

#include "Metrics/MediaUplCalculator.h"

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/// String to identify log entries originating from this file.
#define TAG "MediaUplCalculator"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// Names of the monitored metrics for UPL
static const std::string TTS_STARTED = "TTS_STARTED";
static const std::string TTS_FINISHED = "TTS_FINISHED";
static const std::string PLAY_DIRECTIVE_RECEIVED = "PLAY_DIRECTIVE_RECEIVED";
static const std::string STOP_DIRECTIVE_RECEIVED = "STOP_DIRECTIVE_RECEIVED";
static const std::string PLAYBACK_STARTED = "PLAYBACK_STARTED";
static const std::string PLAYBACK_STOPPED = "PLAYBACK_STOPPED";

// Names of the new recordered timepoints for UPL
static const std::string MEDIA_DIRECTIVE_PARSED = "MEDIA_DIRECTIVE_PARSED";
static const std::string MEDIA_DIRECTIVE_DISPATCHED = "MEDIA_DIRECTIVE_DISPATCHED";

// Metric tag names
static const std::string REQUESTER_TYPE = "REQUESTER_TYPE";

// UPL activity and datapoints names
static const std::string UPL_MEDIA_PREFIX = "UPL-MEDIA_";
static const std::string UPL_MEDIA_STOP = "STOP";
static const std::string UPL_MEDIA_PLAY = "PLAY";
static const std::string UPL_MEDIA_PLAY_AFTER_TTS = "PLAY_AFTER_TTS";
static const std::string MEDIA_LATENCY = "MEDIA_LATENCY";
static const std::string SERVER_PROCESSING = "SERVER_PROCESSING";
static const std::string UTTERANCE_END_TO_STOP_CAPTURE = "UTTERANCE_END_TO_STOP_CAPTURE";
static const std::string STOP_CAPTURE_TO_PARSE_COMPLETE = "STOP_CAPTURE_TO_PARSE_COMPLETE";
static const std::string DEVICE_PROCESSING = "DEVICE_PROCESSING";
static const std::string PARSE_COMPLETE_TO_DISPATCH = "PARSE_COMPLETE_TO_DISPATCH";
static const std::string DISPATCH_TO_DIRECTIVE_RECEIVED = "DISPATCH_TO_DIRECTIVE_RECEIVED";
static const std::string DIRECTIVE_RECEIVED_TO_MEDIA_UPDATE = "DIRECTIVE_RECEIVED_TO_MEDIA_UPDATE";

std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface> MediaUplCalculator::createMediaUplCalculator(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    if (!metricRecorder) {
        ACSDK_ERROR(LX("createMediaUplCalculatorFailed").d("reason", "nullMetricRecorder"));
        return nullptr;
    }
    return std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface>(new MediaUplCalculator(metricRecorder));
}

MediaUplCalculator::MediaUplCalculator(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_metricRecorder(metricRecorder),
        m_ttsStarted(),
        m_ttsFinished(),
        m_uplInhibited(false) {
}

void MediaUplCalculator::inspectMetric(
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent>& metricEvent) {
    if (!metricEvent) {
        ACSDK_ERROR(LX("inpectMetricFailed").d("reason", "nullMetricEvent"));
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

    if (TTS_STARTED == metricName || TTS_FINISHED == metricName) {
        // Save the timestamp of the TTS_STARTED and TTS_FINISHED metrics to account for variable TTS playing duration
        // by subtracting from the media player metrics after PARSE_COMPLETE
        std::string dialogRequestId;
        if (BaseUplCalculator::getMetricTag(metricName, metricEvent, DIALOG_REQUEST_ID_TAG, dialogRequestId)) {
            if (dialogRequestId == m_uplData->getStringData(DIALOG_REQUEST_ID_TAG)) {
                if (TTS_STARTED == metricName) {
                    m_ttsStarted = metricEvent->getSteadyTimestamp();
                } else {  // TTS_FINISHED == metricName
                    m_ttsFinished = metricEvent->getSteadyTimestamp();
                }
            }
        }
    } else if (STOP_DIRECTIVE_RECEIVED == metricName || PLAY_DIRECTIVE_RECEIVED == metricName) {
        std::string requesterType;
        if (BaseUplCalculator::getMetricTag(metricName, metricEvent, REQUESTER_TYPE, requesterType)) {
            // We abandon submitting a Media UPL metric for music alarms; since there is no utterance, there is no
            // point to start calculating from, and the current calculator will start from the last utterance
            // which is not related to the music alarm.
            if (requesterType == "ALERT") {
                inhibitSubmission();
                return;
            }
        }

        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());
        std::string directiveId;
        if (BaseUplCalculator::getMetricTag(metricName, metricEvent, DIRECTIVE_MESSAGE_ID_TAG, directiveId)) {
            m_uplData->addTimepoint(MEDIA_DIRECTIVE_PARSED, m_uplData->getParseCompleteTimepoint(directiveId));
            m_uplData->addTimepoint(
                MEDIA_DIRECTIVE_DISPATCHED, m_uplData->getDirectiveDispatchedTimepoint(directiveId));
        }
    } else if (PLAYBACK_STOPPED == metricName) {
        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());
        calculateMediaUpl(MediaUplType::STOP);
    } else if (PLAYBACK_STARTED == metricName) {
        m_uplData->addTimepoint(metricName, metricEvent->getSteadyTimestamp());

        if (m_ttsStarted.time_since_epoch() == std::chrono::steady_clock::duration::zero() ||
            m_ttsFinished.time_since_epoch() == std::chrono::steady_clock::duration::zero()) {
            calculateMediaUpl(MediaUplType::PLAY);
        } else {
            calculateMediaUpl(MediaUplType::PLAY_AFTER_TTS);
        }
    }  // else,  doesn't match any monitored metric names, do nothing
}

void MediaUplCalculator::setUplData(
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData>& uplData) {
    m_uplData = uplData;
}

std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData> MediaUplCalculator::getUplData() const {
    return m_uplData;
}

void MediaUplCalculator::addDuration(
    alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder& metricEventBuilder,
    const std::string& name,
    const std::string& startTimepointName,
    const std::string& endTimepointName) {
    UplTimePoint startTimepoint = m_uplData->getTimepoint(startTimepointName);
    UplTimePoint endTimepoint = m_uplData->getTimepoint(endTimepointName);

    addDuration(metricEventBuilder, name, startTimepoint, endTimepoint);
}

void MediaUplCalculator::addDuration(
    alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder& metricEventBuilder,
    const std::string& name,
    const UplTimePoint& startTimepoint,
    const UplTimePoint& endTimepoint) {
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

MediaUplCalculator::UplTimePoint MediaUplCalculator::getTimePointAdjustedForTtsDuration(
    const UplTimePoint& timepoint) const {
    UplTimePoint parseCompleteTimePoint = m_uplData->getTimepoint(MEDIA_DIRECTIVE_PARSED);
    UplTimePoint startAdjustmentTimePoint =
        m_ttsStarted > parseCompleteTimePoint ? m_ttsStarted : parseCompleteTimePoint;

    // Sanity checks
    if (timepoint > m_ttsFinished && m_ttsFinished > startAdjustmentTimePoint) {
        auto adjustDuration = m_ttsFinished - startAdjustmentTimePoint;
        UplTimePoint adjustedTimePoint = timepoint - adjustDuration;
        return adjustedTimePoint;
    }

    ACSDK_DEBUG5(LX(__func__)
                     .m("Not adjusting for TTS duration")
                     .d("originalTimePoint", timepoint.time_since_epoch().count())
                     .d("startAdjustmentTimePoint", startAdjustmentTimePoint.time_since_epoch().count())
                     .d("ttsFinishedTimePoint", m_ttsFinished.time_since_epoch().count()));

    return timepoint;
}

void MediaUplCalculator::calculateMediaUpl(MediaUplType type) {
    if (m_uplInhibited) {
        return;
    }

    auto metricEventBuilder = alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder{};

    // Common media UPL metrics
    addDuration(metricEventBuilder, SERVER_PROCESSING, END_OF_UTTERANCE, MEDIA_DIRECTIVE_PARSED);
    addDuration(metricEventBuilder, UTTERANCE_END_TO_STOP_CAPTURE, END_OF_UTTERANCE, STOP_CAPTURE);
    addDuration(metricEventBuilder, STOP_CAPTURE_TO_PARSE_COMPLETE, STOP_CAPTURE, MEDIA_DIRECTIVE_PARSED);

    if (MediaUplType::STOP == type) {
        metricEventBuilder.setActivityName(UPL_MEDIA_PREFIX + UPL_MEDIA_STOP);

        addDuration(metricEventBuilder, MEDIA_LATENCY, END_OF_UTTERANCE, PLAYBACK_STOPPED);
        addDuration(metricEventBuilder, DEVICE_PROCESSING, MEDIA_DIRECTIVE_PARSED, PLAYBACK_STOPPED);

        addDuration(metricEventBuilder, PARSE_COMPLETE_TO_DISPATCH, MEDIA_DIRECTIVE_PARSED, MEDIA_DIRECTIVE_DISPATCHED);
        addDuration(
            metricEventBuilder, DISPATCH_TO_DIRECTIVE_RECEIVED, MEDIA_DIRECTIVE_DISPATCHED, STOP_DIRECTIVE_RECEIVED);
        addDuration(metricEventBuilder, DIRECTIVE_RECEIVED_TO_MEDIA_UPDATE, STOP_DIRECTIVE_RECEIVED, PLAYBACK_STOPPED);
    } else if (MediaUplType::PLAY == type) {
        metricEventBuilder.setActivityName(UPL_MEDIA_PREFIX + UPL_MEDIA_PLAY);

        addDuration(metricEventBuilder, MEDIA_LATENCY, END_OF_UTTERANCE, PLAYBACK_STARTED);
        addDuration(metricEventBuilder, DEVICE_PROCESSING, MEDIA_DIRECTIVE_PARSED, PLAYBACK_STARTED);

        addDuration(metricEventBuilder, PARSE_COMPLETE_TO_DISPATCH, MEDIA_DIRECTIVE_PARSED, MEDIA_DIRECTIVE_DISPATCHED);
        addDuration(
            metricEventBuilder, DISPATCH_TO_DIRECTIVE_RECEIVED, MEDIA_DIRECTIVE_DISPATCHED, PLAY_DIRECTIVE_RECEIVED);
        addDuration(metricEventBuilder, DIRECTIVE_RECEIVED_TO_MEDIA_UPDATE, PLAY_DIRECTIVE_RECEIVED, PLAYBACK_STARTED);
    } else if (MediaUplType::PLAY_AFTER_TTS == type) {
        metricEventBuilder.setActivityName(UPL_MEDIA_PREFIX + UPL_MEDIA_PLAY_AFTER_TTS);

        UplTimePoint adjustedDirectiveDispatched =
            getTimePointAdjustedForTtsDuration(m_uplData->getTimepoint(MEDIA_DIRECTIVE_DISPATCHED));
        UplTimePoint adjustedDirectiveReceived =
            getTimePointAdjustedForTtsDuration(m_uplData->getTimepoint(PLAY_DIRECTIVE_RECEIVED));
        UplTimePoint adjustedPlaybackStarted =
            getTimePointAdjustedForTtsDuration(m_uplData->getTimepoint(PLAYBACK_STARTED));

        addDuration(
            metricEventBuilder, MEDIA_LATENCY, m_uplData->getTimepoint(END_OF_UTTERANCE), adjustedPlaybackStarted);
        addDuration(
            metricEventBuilder,
            DEVICE_PROCESSING,
            m_uplData->getTimepoint(MEDIA_DIRECTIVE_PARSED),
            adjustedPlaybackStarted);

        addDuration(
            metricEventBuilder,
            PARSE_COMPLETE_TO_DISPATCH,
            m_uplData->getTimepoint(MEDIA_DIRECTIVE_PARSED),
            adjustedDirectiveDispatched);
        addDuration(
            metricEventBuilder, DISPATCH_TO_DIRECTIVE_RECEIVED, adjustedDirectiveDispatched, adjustedDirectiveReceived);
        addDuration(
            metricEventBuilder, DIRECTIVE_RECEIVED_TO_MEDIA_UPDATE, adjustedDirectiveReceived, adjustedPlaybackStarted);
    } else {
        ACSDK_ERROR(LX("calculateMediaUplFailed").d("reason", "unexpected MediaUplType").d("type", type));
    }

    metricEventBuilder.addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointStringBuilder{}
                                        .setName(DIALOG_REQUEST_ID_TAG)
                                        .setValue(m_uplData->getStringData(DIALOG_REQUEST_ID_TAG))
                                        .build());
    auto metricRecorder = m_metricRecorder.lock();
    if (metricRecorder) {
        metricRecorder->recordMetric(metricEventBuilder.build());
    } else {
        ACSDK_ERROR(LX("calculateMediaUplFailed").d("reason", "nullMetricRecorder"));
    }
    inhibitSubmission();
}

void MediaUplCalculator::inhibitSubmission() {
    m_uplInhibited = true;
}

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK
