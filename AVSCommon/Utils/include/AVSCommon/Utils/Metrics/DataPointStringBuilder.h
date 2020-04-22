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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTSTRINGBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTSTRINGBUILDER_H_

#include "AVSCommon/Utils/Metrics/DataPoint.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * DataPointStringBuilder is a builder class responsible for building immutable string DataPoint objects.
 */
class DataPointStringBuilder {
public:
    /**
     * Sets the name of the string dataPoint
     *
     * @param name The name of the string dataPoint.
     * @return This instance to facilitate setting more information to this dataPoint string builder.
     */
    DataPointStringBuilder& setName(const std::string& name);

    /**
     * Sets the value of the string dataPoint
     *
     * @param value The value of the string dataPoint.
     * @return This instance to facilitate setting more information to this dataPoint string builder.
     */
    DataPointStringBuilder& setValue(const std::string& value);

    /**
     * Builds a new immutable DataPoint object with the current state stored in dataPoint string builder.
     *
     * @return A new immutable DataPoint object
     */
    DataPoint build();

private:
    // The current name of the dataPoint string builder
    std::string m_name;

    // The current value of the dataPoint string builder
    std::string m_value;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_DATAPOINTSTRINGBUILDER_H_