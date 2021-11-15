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
#ifndef ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_TTSUPLCALCULATOR_H_
#define ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_TTSUPLCALCULATOR_H_

#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/UplCalculatorInterface.h>
#include <AVSCommon/Utils/Metrics/UplData.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/**
 * This class implements a sample UPL calculator that calculates the time to a TTS response from the device.
 * TtsUplCalculator needs BaseUplCalculator to be also running for it to calculate UPL correctly.
 */
class TtsUplCalculator : public avsCommon::utils::metrics::UplCalculatorInterface {
public:
    /**
     * Create a @c TtsUplCalculator.
     *
     * @param metricRecorder is the MetricRecorder object to publish UPL metrics to.
     * @return A @c TtsUplCalculator.
     */
    static std::unique_ptr<avsCommon::utils::metrics::UplCalculatorInterface> createTtsUplCalculator(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Destructor
     */
    virtual ~TtsUplCalculator() = default;

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
     * Overloaded constructor with MetricRecorder pointer passed in.
     *
     * @param metricRecorder is the MetricRecorder object to publish UPL metrics to.
     */
    explicit TtsUplCalculator(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Add a time-span metric from two timepoints in the UplData time points map
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
     * Calculate the TTS UPL from the recorded time points
     */
    void calculateTtsUpl();

    /**
     * Stops the UPL Calculator from further recording or submitting metrics
     */
    void inhibitSubmission();

    /// MetricRecorder to publish UPL metrics.
    std::weak_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// Stop UPL calculations for unwanted cases
    bool m_uplInhibited;
};

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_TTSUPLCALCULATOR_H_
