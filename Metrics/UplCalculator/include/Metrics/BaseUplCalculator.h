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
#ifndef ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_BASEUPLCALCULATOR_H_
#define ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_BASEUPLCALCULATOR_H_

#include <memory>

#include <AVSCommon/Utils/Metrics/MetricEvent.h>
#include <AVSCommon/Utils/Metrics/UplCalculatorInterface.h>
#include <AVSCommon/Utils/Metrics/UplData.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/// Names of the common monitored metrics for SDK UPL
static constexpr const char* START_OF_UTTERANCE = "START_OF_UTTERANCE";
static constexpr const char* WW_DURATION = "WW_DURATION";
static constexpr const char* STOP_CAPTURE = "STOP_CAPTURE";
static constexpr const char* END_OF_SPEECH_OFFSET = "END_OF_SPEECH_OFFSET";
static constexpr const char* PARSE_COMPLETE = "PARSE_COMPLETE";
static constexpr const char* DIRECTIVE_DISPATCHED_HANDLE = "DIRECTIVE_DISPATCHED_HANDLE";
static constexpr const char* DIRECTIVE_DISPATCHED_IMMEDIATE = "DIRECTIVE_DISPATCHED_IMMEDIATE";

/// Names of the new recordered timepoints for UPL
static constexpr const char* END_OF_UTTERANCE = "END_OF_UTTERANCE";
static constexpr const char* END_OF_WW = "END_OF_WW";
static constexpr const char* RECOGNIZE_EVENT_IS_BUILT = "RECOGNIZE_EVENT_IS_BUILT";

/// Datapoint name for the start of utterance with wakeword detection
static constexpr const char* START_OF_STREAM_TIMESTAMP = "START_OF_STREAM_TIMESTAMP";

/// Metric tag names
static constexpr const char* DIALOG_REQUEST_ID_TAG = "DIALOG_REQUEST_ID";
static constexpr const char* DIRECTIVE_MESSAGE_ID_TAG = "DIRECTIVE_MESSAGE_ID";

/**
 * This class implements a UplCalculatorInterface that records the common metrics for calculating User Perceived Latency
 * from a user utterance.
 *
 * It records metrics from the start of the user's utterance until the server sends the STOP_CAPTURE
 * directive, which are common to every UPL calculator starting from a user utterance. It also records all the
 * PARSE_COMPLETE and DIRECTIVE_DISPATCHED directives, which are then used by the UPL calculators to trace back the
 * path of the final directive (a TTS or a Playback directive).
 */
class BaseUplCalculator : public avsCommon::utils::metrics::UplCalculatorInterface {
public:
    /**
     * Create a @c BaseUplCalculator.
     *
     * @return A @c BaseUplCalculator.
     */
    static std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface> createBaseUplCalculator();

    /**
     * Destructor
     */
    virtual ~BaseUplCalculator() = default;

    /**
     * Get the value of a Metric tag.
     *
     * @param metricName The metric name
     * @param metric The Metric to get a tag from
     * @param tag The Tag name to get
     * @param value The return location for the tag value
     * @return false if there is an error
     */
    static bool getMetricTag(
        const std::string& metricName,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent,
        const std::string& tagName,
        std::string& value);

    /**
     * Returns duration value of Metric.
     *
     * @param metricName The metric name
     * @param metric The Metric to get the value of
     * @param duration Returned duration value
     * @return false if there is an error
     */
    static bool getDuration(
        const std::string& metricName,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent,
        std::chrono::milliseconds& duration);

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
    /**
     * Private constructor
     */
    BaseUplCalculator();

    /**
     * Stops the UPL Calculator from further recording metrics
     */
    void inhibitSubmission();

    /// Initial time point to check for UPL timeout
    std::chrono::steady_clock::time_point m_startTime;

    /// Stop UPL calculations for unwanted cases
    bool m_uplInhibited;
};

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_BASEUPLCALCULATOR_H_