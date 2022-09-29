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
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Diagnostics/DeviceProtocolTracer.h"

namespace alexaClientSDK {
namespace diagnostics {

using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// String to identify log entries originating from this file.
#define TAG "DeviceProtocolTracer"

/// Maximum number of trace messages stored in the device protocol tracer.
static const unsigned int DEFAULT_MAX_MESSAGES = 1;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<DeviceProtocolTracer> DeviceProtocolTracer::create() {
    return std::shared_ptr<DeviceProtocolTracer>(new DeviceProtocolTracer(DEFAULT_MAX_MESSAGES));
}

DeviceProtocolTracer::DeviceProtocolTracer(unsigned int maxMessages) :
        m_isProtocolTraceEnabled{false},
        m_maxMessages(maxMessages) {
}

unsigned int DeviceProtocolTracer::getMaxMessages() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxMessages;
}

bool DeviceProtocolTracer::setMaxMessages(unsigned int limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG5(LX(__func__).d("current", m_maxMessages).d("new", limit));

    if (limit < m_tracedMessages.size()) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "storedMessagesExceedLimit")
                        .d("storedMessages", m_tracedMessages.size())
                        .d("limt", limit));
        return false;
    }

    m_maxMessages = limit;

    return true;
}

void DeviceProtocolTracer::setProtocolTraceFlag(bool enabled) {
    ACSDK_DEBUG5(LX(__func__).d("enabled", enabled));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isProtocolTraceEnabled = enabled;
}

void DeviceProtocolTracer::clearTracedMessages() {
    std::lock_guard<std::mutex> lock{m_mutex};
    clearTracedMessagesLocked();
}

void DeviceProtocolTracer::clearTracedMessagesLocked() {
    m_tracedMessages.clear();
}

void DeviceProtocolTracer::receive(const std::string& contextId, const std::string& message) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    traceMessageLocked(message);
}

void DeviceProtocolTracer::traceEvent(const std::string& message) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    traceMessageLocked(message);
}

void DeviceProtocolTracer::traceMessageLocked(const std::string& messageContent) {
    if (m_isProtocolTraceEnabled) {
        if (m_tracedMessages.size() < m_maxMessages) {
            m_tracedMessages.push_back(messageContent);
        } else {
            ACSDK_WARN(LX(__func__).d("maxMessages", m_maxMessages).m("reached max trace message limit."));
        }
    } else {
        ACSDK_DEBUG5(LX(__func__).m("protocol trace disabled"));
    }
}

std::string DeviceProtocolTracer::getProtocolTrace() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartArray();
    for (const auto& message : m_tracedMessages) {
        writer.RawValue(message.c_str(), message.length(), rapidjson::kStringType);
    }
    writer.EndArray();

    return buffer.GetString();
}

}  // namespace diagnostics
}  // namespace alexaClientSDK
