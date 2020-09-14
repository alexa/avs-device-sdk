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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICSINKINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICSINKINTERFACE_H_

#include "AVSCommon/Utils/Metrics/MetricEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * This class provides an interface for metric sinks to consume metrics.
 *
 * @note implementations of MetricSinkInterface may not be thread-safe.
 */
class MetricSinkInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MetricSinkInterface() = default;

    /**
     * This function allows consumption of a MetricEvent.
     *
     * @param metricEvent is the metricEvent object to be consumed
     * @return true if metric sink consumes metric event successfully
     *         false otherwise
     */
    virtual void consumeMetric(std::shared_ptr<MetricEvent> metricEvent) = 0;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICSINKINTERFACE_H_