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

#include <memory>

#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

#ifndef ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_NULLMETRICRECORDER_H_
#define ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_NULLMETRICRECORDER_H_

namespace alexaClientSDK {
namespace acsdkDefaultSampleApplicationOptions {

/**
 * An implementation of MetricRecorderInterface that does not record anything.
 */
class NullMetricRecorder : public avsCommon::utils::metrics::MetricRecorderInterface {
public:
    /**
     * Create a NullMetricRecorder instance.
     *
     * @return A new instance of NullMetricRecorder.
     */
    static std::shared_ptr<MetricRecorderInterface> createMetricRecorderInterface();

    /// @name MetricRecorderInterface methods
    /// @{
    void recordMetric(std::shared_ptr<avsCommon::utils::metrics::MetricEvent> metricEvent) override;
    /// @}
};

inline std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> NullMetricRecorder::
    createMetricRecorderInterface() {
    return std::make_shared<NullMetricRecorder>();
}

inline void NullMetricRecorder::recordMetric(std::shared_ptr<avsCommon::utils::metrics::MetricEvent> metricEvent) {
}

}  // namespace acsdkDefaultSampleApplicationOptions
}  // namespace alexaClientSDK

#endif  // ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_NULLMETRICRECORDER_H_
