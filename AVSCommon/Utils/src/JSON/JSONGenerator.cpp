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
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "AVSCommon/Utils/JSON/JSONGenerator.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"

/// String to identify log entries originating from this file.
#define TAG "JSONGenerator"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {

JsonGenerator::JsonGenerator() : m_buffer{}, m_writer{m_buffer} {
    m_writer.StartObject();
}

bool JsonGenerator::startObject(const std::string& key) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.StartObject();
}

bool JsonGenerator::finishObject() {
    return checkWriter() && m_writer.EndObject();
}

bool JsonGenerator::addMember(const std::string& key, const std::string& value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.String(value);
}

bool JsonGenerator::addMember(const std::string& key, uint64_t value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.Uint64(value);
}

bool JsonGenerator::addMember(const std::string& key, unsigned int value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.Uint(value);
}

bool JsonGenerator::addMember(const std::string& key, int64_t value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.Int64(value);
}

bool JsonGenerator::addMember(const std::string& key, int value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.Int(value);
}

bool JsonGenerator::addMember(const std::string& key, bool value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.Bool(value);
}

bool JsonGenerator::addMember(const std::string& key, const char* value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return value && checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.String(value);
}

bool JsonGenerator::addMember(const std::string& key, double value) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.Double(value);
}

bool JsonGenerator::addRawJsonMember(const std::string& key, const std::string& json, bool validate) {
    // Validate the provided json.
    if (validate) {
        rapidjson::Document document;
        if (!jsonUtils::parseJSON(json, &document)) {
            ACSDK_ERROR(LX("addRawJsonMemberFailed").d("reason", "invalidJson").sensitive("rawJson", json));
            return false;
        }
    }
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) &&
           m_writer.RawValue(json.c_str(), json.length(), rapidjson::kStringType);
}

bool JsonGenerator::startArray(const std::string& key) {
    auto keyLength = static_cast<rapidjson::SizeType>(key.length());
    return checkWriter() && m_writer.Key(key.c_str(), keyLength) && m_writer.StartArray();
}

bool JsonGenerator::finishArray() {
    return checkWriter() && m_writer.EndArray();
}

bool JsonGenerator::startArrayElement() {
    return checkWriter() && m_writer.StartObject();
}
bool JsonGenerator::finishArrayElement() {
    return finishObject();
}

bool JsonGenerator::finalize() {
    while (!m_writer.IsComplete()) {
        if (!m_writer.EndObject()) {
            ACSDK_ERROR(LX("finishFailed").d("reason", "failToEndObject"));
            return false;
        }
    }
    return true;
}

std::string JsonGenerator::toString(bool finalizeJson) {
    if (finalizeJson) {
        finalize();
    }
    return m_buffer.GetString();
}

bool JsonGenerator::checkWriter() {
    if (m_writer.IsComplete()) {
        ACSDK_ERROR(LX("addMemberFailed").d("reason", "finalizedGenerator"));
        return false;
    }
    return true;
}

bool JsonGenerator::isFinalized() {
    return !checkWriter();
}

}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
