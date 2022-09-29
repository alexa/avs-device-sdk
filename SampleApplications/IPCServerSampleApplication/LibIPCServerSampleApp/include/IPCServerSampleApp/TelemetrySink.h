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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_TELEMETRYSINK_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_TELEMETRYSINK_H_

#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <APLClient/Telemetry/AplMetricsSinkInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * Records telemetry reported by APL through a @c MetricRecorderInterface.
 */
class TelemetrySink : public APLClient::Telemetry::AplMetricsSinkInterface {
public:
    TelemetrySink(std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);
    ~TelemetrySink() override = default;

    /**
     * Outputs a timer value through this instance's metrics recorder.
     * @param metadata The metadata associated with this timer.
     * @param name The name of the timer to report
     * @param value The elapsed time recorded by the timer
     */
    void reportTimer(
        const std::map<std::string, std::string>& metadata,
        const std::string& name,
        const std::chrono::nanoseconds& value) override;

    /**
     * Outputs a counter value through this instance's metrics recorder.
     * @param metadata The metadata associated with this counter.
     * @param name The name of the counter to report
     * @param value The recorded value of the counter
     */
    void reportCounter(const std::map<std::string, std::string>& metadata, const std::string& name, uint64_t value)
        override;

private:
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_TELEMETRYSINK_H_
