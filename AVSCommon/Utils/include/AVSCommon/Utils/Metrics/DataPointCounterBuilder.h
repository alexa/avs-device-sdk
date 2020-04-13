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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTCOUNTERBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTCOUNTERBUILDER_H_

#include "AVSCommon/Utils/Metrics/DataPoint.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * DataPointCounterBuilder is a builder class responsible for building immutable counter DataPoint objects.
 *
 * The range of values supported by DataPointCounterBuilder ranges from 0 to 2^64. When DataPointCounterBuilder is first
 * initialized the default value is 0.
 */
class DataPointCounterBuilder {
public:
    /**
     * Constructor
     */
    DataPointCounterBuilder();

    /**
     * Sets the name of the counter dataPoint
     *
     * @param name The name of the counter dataPoint.
     * @return This instance to facilitate setting more information to this dataPoint counter builder.
     */
    DataPointCounterBuilder& setName(const std::string& name);

    /**
     * Increments the value of the counter dataPoint
     * If an overflow should occur, the value will be set to the maximum value of uint64_t, 2^64.
     *
     * @param toAdd The value to be added to the counter dataPoint.
     * @return This instance to facilitate setting more information to this dataPoint counter builder.
     */
    DataPointCounterBuilder& increment(uint64_t toAdd);

    /**
     * Builds a new immutable DataPoint object with the current state stored in dataPoint counter builder.
     *
     * @return A new immutable DataPoint object
     */
    DataPoint build();

private:
    // The current name of the dataPoint counter builder
    std::string m_name;

    // The current value of the dataPoint counter builder
    uint64_t m_value;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTCOUNTERBUILDER_H_