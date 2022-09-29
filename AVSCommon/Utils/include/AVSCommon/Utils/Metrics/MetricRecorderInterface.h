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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICRECORDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICRECORDERINTERFACE_H_

#include "AVSCommon/Utils/Metrics/MetricEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * This class provides an interface for the SDK to record metrics to send to sinks.
 */
class MetricRecorderInterface {
public:
    /**
     * Destructor
     */
    virtual ~MetricRecorderInterface() = default;

    /**
     * This function is responsible for making sure that metrics is sent to a sink implementation for their
     * consumption of MetricEvent.
     *
     * @note implementations of this function should be non-blocking
     * @param metricEvent is the metric event to be recorded
     */
    virtual void recordMetric(std::shared_ptr<MetricEvent> metricEvent) = 0;
};

/**
 * Inline record metric function to handle if ACSDK_ENABLE_METRICS_RECORDING flag is defined or not.
 *
 * @param recorder Optional pointer to MetricRecorderInterface. If this parameter is nullptr, metric is not sent.
 * @param metricEvent Pointer to the MetricEvent.
 */
inline void recordMetric(
    const std::shared_ptr<MetricRecorderInterface>& recorder,
    std::shared_ptr<MetricEvent> metricEvent) {
#ifdef ACSDK_ENABLE_METRICS_RECORDING
    if (recorder) {
        recorder->recordMetric(std::move(metricEvent));
    }
#else
    (void)recorder;
    (void)metricEvent;
#endif
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICRECORDERINTERFACE_H_