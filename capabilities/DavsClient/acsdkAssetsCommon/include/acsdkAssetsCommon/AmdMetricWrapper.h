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

#ifndef ACSDKASSETSCOMMON_AMDMETRICWRAPPER_H_
#define ACSDKASSETSCOMMON_AMDMETRICWRAPPER_H_

#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#define METRIC_PREFIX_ERROR(str) ("ERROR." str)
#define METRIC_PREFIX_ERROR_CREATE(str) METRIC_PREFIX_ERROR("Create." str)
#define METRIC_PREFIX_ERROR_INIT(str) METRIC_PREFIX_ERROR("Init." str)

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/*
 * Wrapper around the MetricRecorder
 */
class AmdMetricsWrapper {
public:
    /**
     * Creates and starts a new metric given a program and source name
     */
    AmdMetricsWrapper(const std::string& sourceName);

    static std::function<AmdMetricsWrapper()> creator(const std::string& sourceName) {
        return [sourceName] { return AmdMetricsWrapper(sourceName); };
    }

    static void setStaticRecorder(
            std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> recorder);

    AmdMetricsWrapper(const AmdMetricsWrapper&) = delete;
    AmdMetricsWrapper& operator=(const AmdMetricsWrapper& other) = delete;
    AmdMetricsWrapper(AmdMetricsWrapper&& other) = default;
    AmdMetricsWrapper& operator=(AmdMetricsWrapper&& other) = delete;

    ~AmdMetricsWrapper();

    /**
     * Add a count data point
     */
    AmdMetricsWrapper& addCounter(const std::string& name, int count = 1);

    /**
     * Add a zero count data point
     */
    inline AmdMetricsWrapper& addZeroCounter(const std::string& name) {
        return addCounter(name, 0);
    }

    /**
     * Add a timer data point
     */
    AmdMetricsWrapper& addTimer(const std::string& name, std::chrono::milliseconds value);

    /**
     * Add a string data point
     */
    AmdMetricsWrapper& addString(const std::string& name, const std::string& str);

private:
    /**
     * Activity name of the metric
     */
    const std::string m_sourceName;
    /**
     * vector to store datapoints before submitting the metric
     */
    std::vector<alexaClientSDK::avsCommon::utils::metrics::DataPoint> m_dataPoints;

    static std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> s_recorder;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_AMDMETRICWRAPPER_H_
