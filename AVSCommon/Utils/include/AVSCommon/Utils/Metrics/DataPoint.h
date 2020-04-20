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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINT_H_

#include <string>

#include "AVSCommon/Utils/Metrics/DataType.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * This class represents the immutable Datapoint objects.
 */
class DataPoint {
public:
    /**
     * Default constructor. Creates a invalid metric point with empty values.
     */
    DataPoint();

    /**
     * Constructor
     *
     * @param name is the name of the dataPoint
     * @param value is the value of the dataPoint
     * @param dataType is the datatype of the dataPoint
     */
    DataPoint(const std::string& name, const std::string& value, DataType dataType);

    /**
     * Getter method for the name of the dataPoint
     *
     * @return the name of the dataPoint
     */
    std::string getName() const;

    /**
     * Getter method for the value of the dataPoint
     *
     * @return the value of the dataPoint
     */
    std::string getValue() const;

    /**
     * Getter method for the data type of the dataPoint
     *
     * @return the data type of the dataPoint
     */
    DataType getDataType() const;

    /**
     * Checks to see if dataPoint is valid
     *
     * @return true, if dataPoint is valid
     *         false, otherwise
     */
    bool isValid() const;

private:
    // The name given to the DataPoint
    const std::string m_name;
    // The value given to the DataPoint
    const std::string m_value;

    // The datatype of the DataPoint
    const DataType m_dataType;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINT_H_