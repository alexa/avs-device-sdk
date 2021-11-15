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
#ifndef ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_MEDIAUPLCALCULATOR_H_
#define ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_MEDIAUPLCALCULATOR_H_

#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/UplCalculatorInterface.h>
#include <AVSCommon/Utils/Metrics/UplData.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/**
 * This class implements a sample UPL calculator that calculates the time to a media related response, for example
 * starting or stopping music.
 * MediaUplCalculator needs BaseUplCalculator to be also running for it to calculate UPL correctly.
 *
 * MediaUplCalculator separates Media UPL in three categories: PLAY, STOP, and PLAY_AFTER_TTS.
 * Upon receiving the PLAYBACK_STARTED or PLAYBACK_STOPPED metric, if a TTS response played it substracts the TTS
 * response's duration from the UPL duration then calculates UPL.
 */
class MediaUplCalculator : public avsCommon::utils::metrics::UplCalculatorInterface {
public:
    /**
     * Create a @c MediaUplCalculator.
     *
     * @param metricRecorder is the MetricRecorder object to publish UPL metrics to.
     * @return A @c MediaUplCalculator.
     */
    static std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface> createMediaUplCalculator(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Destructor
     */
    virtual ~MediaUplCalculator() = default;

    /**
     * Returns the pointer to the collected UplData
     *
     * @returns @c UplData
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData> getUplData() const;

    /// @name Overridden UplCaclulatorInterface method.
    /// @{
    void inspectMetric(
        const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent>& metricEvent) override;
    void setUplData(const std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::UplData>& uplData) override;
    /// @}
private:
    /// Types of Media UPL
    enum MediaUplType {
        /// Media UPL when receiving PLAYBACK_STOPPED
        STOP,
        /// Media UPL when receiving PLAYBACK_STARTED
        PLAY,
        /// Media UPL when receiving PLAYBACK_STARTED and a TTS message plays before
        PLAY_AFTER_TTS
    };

    /**
     * Overloaded constructor with MetricRecorder pointer passed in.
     *
     * @param metricRecorder is the MetricRecorder object to publish UPL metrics to.
     */
    explicit MediaUplCalculator(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Add a time-span metric from two time points in the UplData time points map
     *
     * @param metricEventBuilder the builder to add the time-span to.
     * @param name of the upl metric.
     * @param startTimepointName the name of the start timepoint
     * @param endTimepointName the name of the end timepoint
     */
    void addDuration(
        alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder& metricEventBuilder,
        const std::string& name,
        const std::string& startTimepointName,
        const std::string& endTimepointName);

    /**
     * Add a time-span metric from two given time points
     *
     * @param metricEventBuilder the builder to add the time-span to.
     * @param name of the upl metric.
     * @param startTimepoint the start timepoint
     * @param endTimepoint the end timepoint
     */
    void addDuration(
        alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder& metricEventBuilder,
        const std::string& name,
        const UplTimePoint& startTimepoint,
        const UplTimePoint& endTimepoint);

    /**
     * Adjust the given timepoint to remove the TTS duration from UPL
     *
     * @param timepoint the time point to adjust
     * @return the adjusted time point
     */
    UplTimePoint getTimePointAdjustedForTtsDuration(const UplTimePoint& timepoint) const;

    /**
     * Calculate the Media player UPL from the recorded time points
     *
     * @param type Type of Media UPL to calculate
     */
    void calculateMediaUpl(MediaUplType type);

    /**
     * Stops the UPL Calculator from further recording or submitting metrics
     */
    void inhibitSubmission();

    /// MetricRecorder to publish UPL metrics.
    std::weak_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// Time point for the start of the TTS when media plays after TTS
    UplTimePoint m_ttsStarted;

    /// Time point for the end of the TTS when media plays after TTS
    UplTimePoint m_ttsFinished;

    /// Stop UPL calculations for unwanted cases
    bool m_uplInhibited;
};

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_MEDIAUPLCALCULATOR_H_
