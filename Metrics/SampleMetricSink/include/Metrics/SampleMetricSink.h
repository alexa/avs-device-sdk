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
#ifndef ALEXA_CLIENT_SDK_METRICS_SAMPLEMETRICSINK_INCLUDE_METRICS_SAMPLEMETRICSINK_H_
#define ALEXA_CLIENT_SDK_METRICS_SAMPLEMETRICSINK_INCLUDE_METRICS_SAMPLEMETRICSINK_H_

#include <iostream>
#include <fstream>
#include <string>

#include <AVSCommon/Utils/Metrics/MetricSinkInterface.h>
#include <AVSCommon/Utils/Metrics/MetricEvent.h>

namespace alexaClientSDK {
namespace metrics {
namespace implementations {

/**
 * This class implements a sample metric sink that prints metric event fields to the console
 */
class SampleMetricSink : public avsCommon::utils::metrics::MetricSinkInterface {
public:
    /**
     * Create a @c SampleMetricSink.
     *
     * @return A @c SampleMetricSink, or nullptr if the operation fails.
     */
    static std::unique_ptr<avsCommon::utils::metrics::MetricSinkInterface> createMetricSinkInterface();

    /**
     * Destructor
     */
    ~SampleMetricSink();

    /**
     * Overloaded constructor with file name passed in
     *
     * @param fileName is the fileName we want consumeMetric to write into
     */
    explicit SampleMetricSink(const std::string& fileName);

    /// @name Overridden MetricSinkInterface method.
    /// @{
    void consumeMetric(std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricEvent> metricEvent) override;
    /// @}
private:
    // File descriptor
    std::ofstream m_file;
};

}  // namespace implementations
}  // namespace metrics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_METRICS_SAMPLEMETRICSINK_INCLUDE_METRICS_SAMPLEMETRICSINK_H_