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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_UPLCALCULATORINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_UPLCALCULATORINTERFACE_H_

#include <unordered_map>

#include <AVSCommon/Utils/Metrics/MetricEvent.h>
#include <AVSCommon/Utils/Metrics/UplData.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * Interface class to inspect metrics and perform UPL analysis.
 */
class UplCalculatorInterface {
public:
    /// Alias for convenience
    using UplTimePoint = std::chrono::steady_clock::time_point;

    /**
     * Destructor.
     */
    virtual ~UplCalculatorInterface() = default;

    /**
     * Inspect the given metric. If needed, record the metric and perform calculations.
     *
     * @param metricEvent is the @c MetricEvent to inspect.
     */
    virtual void inspectMetric(const std::shared_ptr<avsCommon::utils::metrics::MetricEvent>& metricEvent) = 0;

    /**
     * Sets the uplData to the given pointer.
     *
     * @param uplData Object to record in.
     */
    virtual void setUplData(const std::shared_ptr<UplData>& uplData) = 0;

protected:
    std::shared_ptr<UplData> m_uplData;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_UPLCALCULATORINTERFACE_H_