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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTDURATIONBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTDURATIONBUILDER_H_

#include <chrono>

#include "AVSCommon/Utils/Metrics/DataPoint.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * DataPointDurationBuilder is a builder class responsible for building immutable timer DataPoint objects.
 */
class DataPointDurationBuilder {
public:
    /**
     * Constructor
     */
    DataPointDurationBuilder();

    /**
     * Overloaded constructor
     *
     * @param duration The duration of the timer datapoint. Durations cannot be negative. If a negative duration is
     * passed in, the duration will be set to 0.
     */
    explicit DataPointDurationBuilder(std::chrono::milliseconds duration);

    /**
     * Sets the name of the timer dataPoint
     *
     * @param name The name of the timer dataPoint.
     * @return This instance to facilitate setting more information to this dataPoint timer builder.
     */
    DataPointDurationBuilder& setName(const std::string& name);

    /**
     * Starts a timer to help calculate duration dataPoints.
     * Each time startDurationTimer is called, it will update the running start timer
     *
     * @return This instance to facilitate setting more information to this dataPoint timer builder.
     */
    DataPointDurationBuilder& startDurationTimer();

    /**
     * If stopDurationTimer is called when startDurationTimer is not running, it would do nothing and return this
     * instance. If stopDurationTimer is called when startDurationTimer is running, it would calculate and set the
     * duration value before returning this instance.
     *
     * @return This instance to facilitate setting more information to this dataPoint timer builder.
     */
    DataPointDurationBuilder& stopDurationTimer();

    /**
     * Builds a new immutable DataPoint object with the current duration stored in dataPoint timer builder.
     * If build() is called when the timer is running, this will be considered valid and the current timer duration will
     * be used.
     *
     * @return A new immutable DataPoint object
     */
    DataPoint build();

private:
    // The current name of the dataPoint timer builder
    std::string m_name;

    // The current duration of the dataPoint timer builder
    std::chrono::milliseconds m_duration;

    // The start time value of the dataPoint timer builder
    std::chrono::steady_clock::time_point m_startTime;

    // The flag to indicate if startDurationTimer is running
    bool m_isStartDurationTimerRunning;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTDURATIONBUILDER_H_