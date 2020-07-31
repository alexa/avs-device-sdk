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
#include "AVSCommon/Utils/Metrics/DataPointCounterBuilder.h"

#include <limits>

#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/// String to identify log entries originating from this file.
static const std::string TAG("DataPointCounterBuilder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DataPointCounterBuilder::DataPointCounterBuilder() : m_value{0} {
}

DataPointCounterBuilder& DataPointCounterBuilder::setName(const std::string& name) {
    m_name = name;
    return *this;
}

DataPointCounterBuilder& DataPointCounterBuilder::increment(uint64_t toAdd) {
    if (m_value < (std::numeric_limits<uint64_t>::max() - toAdd)) {
        m_value += toAdd;
    } else {
        ACSDK_WARN(LX("incrementWarning").d("reason", "counterValueOverflow"));
        m_value = std::numeric_limits<uint64_t>::max();
    }

    return *this;
}

DataPoint DataPointCounterBuilder::build() {
    return DataPoint{m_name, std::to_string(m_value), DataType::COUNTER};
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK