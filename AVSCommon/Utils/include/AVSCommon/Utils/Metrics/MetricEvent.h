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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICEVENT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICEVENT_H_

#include <unordered_map>
#include <vector>

#include "AVSCommon/Utils/Metrics/DataPoint.h"
#include "AVSCommon/Utils/Metrics/Priority.h"
#include "AVSCommon/Utils/Optional.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * This class represents the immutable MetricEvent object.
 */
class MetricEvent {
public:
    /**
     * Constructor
     *
     * @param activityName is the activity name of the metric event.
     * @param priority is the priority of the metric event
     * @param dataPoints is the collection of key value pairs from dataPoint id to dataPoint objects
     * @param timestamp is the timestamp at which this metric event was created.
     */
    MetricEvent(
        const std::string& activityName,
        Priority priority,
        const std::unordered_map<std::string, DataPoint>& dataPoints,
        std::chrono::steady_clock::time_point timestamp);

    /**
     * Getter method for the activity name of the metric event
     *
     * @return the activity name of the metric event
     */
    std::string getActivityName() const;

    /**
     * Getter method for the priority of the metric event
     *
     * @return the priority of the metric event
     */
    Priority getPriority() const;

    /**
     * Gets dataPoint object from the current metric event
     *
     * @param name The name of the dataPoint to be retrieved
     * @param dataType The dataType of the dataPoint to be retrieved
     * @return an optional dataPoint object
     */
    Optional<DataPoint> getDataPoint(const std::string& name, DataType dataType) const;

    /**
     * Getter method for the dataPoints of the metric event
     *
     * @return the dataPoints of the metric event
     */
    std::vector<DataPoint> getDataPoints() const;

    /**
     * Getter method for the timestamp of when the metric event was created as a system clock time point.
     *
     * @return the timestamp of the metric event
     */
    std::chrono::system_clock::time_point getTimestamp() const;

    /**
     * Getter method for the timestamp of when the metric event was created as a steady clock time point.
     * @return the timestamp of the metric event
     */
    std::chrono::steady_clock::time_point getSteadyTimestamp() const;

private:
    // The activity name of the metric event
    const std::string m_activityName;

    // The priority of the metric event
    const Priority m_priority;

    // The dataPoints of the metric event
    const std::unordered_map<std::string, DataPoint> m_dataPoints;

    // The timestamp for when the metric event was created
    const std::chrono::steady_clock::time_point m_timestamp;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICEVENT_H_
