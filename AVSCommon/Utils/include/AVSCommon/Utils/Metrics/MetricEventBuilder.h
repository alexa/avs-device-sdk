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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICEVENTBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICEVENTBUILDER_H_

#include <unordered_map>

#include "AVSCommon/Utils/Metrics/DataPoint.h"
#include "AVSCommon/Utils/Metrics/MetricEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * MetricEventBuilder is a builder class responsible for building immutable MetricEvent objects.
 */
class MetricEventBuilder {
public:
    /**
     * Constructor
     */
    MetricEventBuilder();

    /**
     * Sets the activity name for the current metric event
     *
     * @param activityName The activity name of the current metric event
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& setActivityName(const std::string& activityName);

    /**
     * Sets the priority for the current metric event
     *
     * @param priority The priority of current metric event
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& setPriority(Priority priority);

    /**
     * Adds a dataPoint object for the current metric event
     *
     * @param dataPoint The dataPoint object to be added to the current metric event
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& addDataPoint(const DataPoint& dataPoint);

    /**
     * Adds dataPoint objects for the current metric event
     *
     * @param dataPoints The shared ptrs to dataPoints to be added to the current metric event
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& addDataPoints(const std::vector<DataPoint>& dataPoints);

    /**
     * Removes a dataPoint object from the current metric event
     *
     * @param dataPoint The dataPoint object to be removed from the current metric event
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& removeDataPoint(const DataPoint& dataPoint);

    /**
     * Removes a dataPoint object from the current metric event
     *
     * @param name The name of the dataPoint to be removed from the current metric event
     * @param dataType The datatype of the dataPoint to be removed from the current metric event
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& removeDataPoint(const std::string& name, DataType dataType);

    /**
     * Removes all dataPoint objects from the current metric event
     *
     * @return This instance to facilitate adding more information to this metric event.
     */
    MetricEventBuilder& removeDataPoints();

    /**
     * Clears all member variables to the default state
     */
    void clear();

    /**
     * Creates a MetricEvent object
     *
     * @return A shared pointer to a MetricEvent object
     */
    std::shared_ptr<MetricEvent> build();

    /**
     * Public static method to generate dataPoint key
     * This is needed to provide a consistent way of generating DataPoint objects' keys.
     *
     * @return the key to the dataPoint
     */
    static std::string generateKey(const std::string& name, DataType dataType);

private:
    /**
     * Private helper method to remove dataPoint by its key
     *
     * @param key is the key of the dataPoint that we want to remove
     * @return This instance to facilitate adding more information to this metric event
     */
    MetricEventBuilder& removeDataPoint(const std::string& key);

    // The activity name of the current metric event
    std::string m_activityName;

    // The priority of the current metric event
    Priority m_priority;

    // The dataPoints of the current metric event
    std::unordered_map<std::string, DataPoint> m_dataPoints;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_METRICEVENTBUILDER_H_