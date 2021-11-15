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
#ifndef ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_UPLMETRICSINK_H_
#define ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_UPLMETRICSINK_H_

#include <memory>
#include <unordered_map>

#include <AVSCommon/Utils/Metrics/MetricEvent.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/MetricSinkInterface.h>
#include <AVSCommon/Utils/Metrics/UplCalculatorInterface.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/**
 * This class implements a metric sink that inspects each incoming metric with SampleUplCalculator.
 */
class UplMetricSink : public avsCommon::utils::metrics::MetricSinkInterface {
public:
    /**
     * Create a @c UplMetricSink.
     *
     * @param metricRecorder is the MetricRecorder object to publish UPL metrics to.
     * @return A @c UplMetricSink.
     */
    static std::unique_ptr<avsCommon::utils::metrics::MetricSinkInterface> createMetricSinkInterface(
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Destructor
     */
    ~UplMetricSink() = default;

    /// @name Overridden MetricSinkInterface method.
    /// @{
    void consumeMetric(std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent) override;
    /// @}
private:
    /**
     * Overloaded constructor with MetricRecorder pointer passed in.
     *
     * @param metricRecorder is the MetricRecorder object to publish UPL metrics to.
     */
    explicit UplMetricSink(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /// Unordered map of UplCalculators.
    std::unordered_map<std::string, std::unique_ptr<alexaClientSDK::avsCommon::utils::metrics::UplCalculatorInterface>>
        uplCalculators;

    /// MetricRecorder to publish UPL metrics.
    std::weak_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;
};

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_METRICS_UPLCALCULATOR_INCLUDE_METRICS_UPLMETRICSINK_H_
