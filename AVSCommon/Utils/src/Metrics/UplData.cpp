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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "AVSCommon/Utils/Metrics/UplData.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/// String to identify log entries originating from this file.
#define TAG "UplData"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

void UplData::addTimepoint(const std::string& name, const UplTimePoint& timepoint) {
    auto key = m_uplTimePointsMap.find(name);
    if (key != m_uplTimePointsMap.end()) {
        ACSDK_WARN(LX(__func__).m("New timepoint already exists in UplTimePointsMap").d("name", name));
    }
    m_uplTimePointsMap[name] = timepoint;
}

UplData::UplTimePoint UplData::getTimepoint(const std::string& name) const {
    auto key = m_uplTimePointsMap.find(name);
    if (key == m_uplTimePointsMap.end()) {
        return UplTimePoint();
    }
    return key->second;
}

void UplData::addParseCompleteTimepoint(const std::string& directiveId, const UplTimePoint& timepoint) {
    auto key = m_messageIdToParseCompleteMap.find(directiveId);
    if (key != m_messageIdToParseCompleteMap.end()) {
        ACSDK_WARN(LX(__func__)
                       .m("New timepoint already exists in messageIdToParseCompleteMap")
                       .d("directiveId", directiveId));
    }
    m_messageIdToParseCompleteMap[directiveId] = timepoint;
}

UplData::UplTimePoint UplData::getParseCompleteTimepoint(const std::string& directiveId) const {
    auto key = m_messageIdToParseCompleteMap.find(directiveId);
    if (key == m_messageIdToParseCompleteMap.end()) {
        return UplTimePoint();
    }
    return key->second;
}

void UplData::addDirectiveDispatchedTimepoint(const std::string& directiveId, const UplTimePoint& timepoint) {
    auto key = m_messageIdToDirectiveDispatchedMap.find(directiveId);
    if (key != m_messageIdToDirectiveDispatchedMap.end()) {
        ACSDK_WARN(LX(__func__)
                       .m("New timepoint already exists in messageIdToDirectiveDispatchedMap")
                       .d("directiveId", directiveId));
    }
    m_messageIdToDirectiveDispatchedMap[directiveId] = timepoint;
}

UplData::UplTimePoint UplData::getDirectiveDispatchedTimepoint(const std::string& directiveId) const {
    auto key = m_messageIdToDirectiveDispatchedMap.find(directiveId);
    if (key == m_messageIdToDirectiveDispatchedMap.end()) {
        return UplTimePoint();
    }
    return key->second;
}

void UplData::addStringData(const std::string& name, const std::string& data) {
    auto key = m_otherData.find(name);
    if (key != m_otherData.end()) {
        ACSDK_WARN(LX(__func__).m("New string data already exists in otherData").d("name", name));
    }
    m_otherData[name] = data;
}

std::string UplData::getStringData(const std::string& name) const {
    auto key = m_otherData.find(name);
    if (key == m_otherData.end()) {
        return std::string();
    }
    return key->second;
}

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK