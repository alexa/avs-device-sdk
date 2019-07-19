/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/LibcurlUtils/CallbackData.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("CallbackData");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

CallbackData::CallbackData(const char* data) {
    appendData(data);
}

size_t CallbackData::appendData(const char* data, size_t dataSize) {
    if (!data) {
        ACSDK_ERROR(LX("appendDataFailed").d("reason", "nullData"));
        return 0;
    }
    m_memory.insert(m_memory.end(), data, data + dataSize);
    return dataSize;
}

size_t CallbackData::appendData(const char* data) {
    if (!data) {
        ACSDK_ERROR(LX("appendDataFailed").d("reason", "nullData"));
        return 0;
    }
    return appendData(data, strlen(data));
}

void CallbackData::clearData() {
    m_memory.clear();
}

size_t CallbackData::getData(char* dataContainer, size_t dataSize) {
    if (!dataContainer) {
        ACSDK_ERROR(LX("getDataFailed").d("reason", "nullDataContainer"));
        return 0;
    }
    if ((m_memory.empty()) || (dataSize == 0)) {
        ACSDK_INFO(LX("getDataFailed").d("reason", "noDataToGet"));
        return 0;
    }

    size_t getSize = std::min(dataSize, m_memory.size());
    std::memcpy(dataContainer, m_memory.data(), getSize);

    return getSize;
}

size_t CallbackData::getSize() {
    return m_memory.size();
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
