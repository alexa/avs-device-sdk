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
#include "AVSCommon/Utils/Metrics/DataPointDurationBuilder.h"

#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/// String to identify log entries originating from this file.
#define TAG "DataPointDurationBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DataPointDurationBuilder::DataPointDurationBuilder() : DataPointDurationBuilder(std::chrono::milliseconds(0)) {
}

DataPointDurationBuilder::DataPointDurationBuilder(std::chrono::milliseconds duration) :
        m_startTime{std::chrono::steady_clock::now()},
        m_isStartDurationTimerRunning{false} {
    std::chrono::milliseconds zeroMilliseconds(0);
    if (duration < zeroMilliseconds) {
        ACSDK_WARN(LX("dataPointDurationBuilderConstructorWarning").d("reason", "negativeDurationSetToZero"));
        duration = zeroMilliseconds;
    }
    m_duration = duration;
}

DataPointDurationBuilder& DataPointDurationBuilder::setName(const std::string& name) {
    m_name = name;
    return *this;
}

DataPointDurationBuilder& DataPointDurationBuilder::startDurationTimer() {
    m_startTime = std::chrono::steady_clock::now();
    m_isStartDurationTimerRunning = true;
    return *this;
}

DataPointDurationBuilder& DataPointDurationBuilder::stopDurationTimer() {
    if (!m_isStartDurationTimerRunning) {
        ACSDK_WARN(LX("stopDurationTimerFailed").d("reason", "startDurationTimerNotRunning"));
        return *this;
    }

    std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
    m_duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_startTime);
    m_isStartDurationTimerRunning = false;
    return *this;
}

DataPoint DataPointDurationBuilder::build() {
    return DataPoint{m_name, std::to_string(m_duration.count()), DataType::DURATION};
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK