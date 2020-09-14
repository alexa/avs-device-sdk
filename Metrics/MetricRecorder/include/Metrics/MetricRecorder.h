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
#ifndef ALEXA_CLIENT_SDK_METRICS_METRICRECORDER_INCLUDE_METRICS_METRICRECORDER_H_
#define ALEXA_CLIENT_SDK_METRICS_METRICRECORDER_INCLUDE_METRICS_METRICRECORDER_H_

#include <unordered_set>

#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/MetricSinkInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/**
 * This class implements the interface for recording metrics to sinks.
 */
class MetricRecorder : public avsCommon::utils::metrics::MetricRecorderInterface {
public:
    /**
     * Create a MetricRecorder.
     *
     * @param sink A sink to send metrics to.
     * @return A new @c MetricRecorder instance.
     */
    static std::shared_ptr<MetricRecorderInterface> createMetricRecorderInterface(
        std::unique_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricSinkInterface> sink);

    /**
     * Destructor
     */
    virtual ~MetricRecorder() = default;

    /**
     * Function adds sinks to the metric recorder
     *
     * @param sink is the sink being added to metric recorder
     * @return true, if the sink is added successfully
     *         false, otherwise
     */
    bool addSink(std::unique_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricSinkInterface> sink);

    /// @name Overridden MetricRecorderInterface method.
    /// @{
    void recordMetric(std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent) override;
    /// @}

private:
    // Unordered set of sinks
    std::unordered_set<std::unique_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricSinkInterface>> m_sinks;

    // Executor to perform asynchronous operation for recording metric
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_METRICS_METRICRECORDER_INCLUDE_METRICS_METRICRECORDER_H_