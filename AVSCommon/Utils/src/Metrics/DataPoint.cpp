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

#include <sstream>

#include "AVSCommon/Utils/Metrics/DataPoint.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

DataPoint::DataPoint() : m_dataType{DataType::STRING} {
}

DataPoint::DataPoint(const std::string& name, const std::string& value, DataType dataType) :
        m_name{name},
        m_value{value},
        m_dataType{dataType} {
}

std::string DataPoint::getName() const {
    return m_name;
}

std::string DataPoint::getValue() const {
    return m_value;
}

DataType DataPoint::getDataType() const {
    return m_dataType;
}

bool DataPoint::isValid() const {
    return !m_name.empty() && !m_value.empty();
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK