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

#include "AVSCommon/Utils/Logger/LogEntry.h"

#include <cstring>
#include <iomanip>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// List of characters we need to escape.
static constexpr auto RESERVED_METADATA_CHARS = R"(\,=:)";

/// Escape sequence for '%'.
static constexpr auto ESCAPED_METADATA_ESCAPE = R"(\\)";

/// Escape sequence for ','.
static constexpr auto ESCAPED_PAIR_SEPARATOR = R"(\,)";

/// Escape sequence for ':'.
static constexpr auto ESCAPED_SECTION_SEPARATOR = R"(\:)";

/// Escape sequence for '='.
static constexpr auto ESCAPED_KEY_VALUE_SEPARATOR = R"(\=)";

/// Reserved in metadata sequences for escaping other reserved values.
static constexpr char METADATA_ESCAPE = '\\';

/// Reserved in metadata sequences to separate key,value pairs.
static constexpr char PAIR_SEPARATOR = ',';

/// Reserved in metadata sequences to separate them from a preceding event and an optional terminal message.
static constexpr char SECTION_SEPARATOR = ':';

/// String for boolean TRUE
static constexpr auto BOOL_TRUE = "true";

/// String for boolean FALSE
static constexpr auto BOOL_FALSE = "false";

/// Array of strings for obfuscation.
/// @see LogEntry::obfuscatePrivateData()
static constexpr const char* const DENYLIST[] = {"ssid"};

LogEntry::LogEntry(const char* source, const char* event) : m_hasMetadata(false) {
    if (source) {
        m_stream << source;
    }
    m_stream << SECTION_SEPARATOR;
    if (event) {
        m_stream << event;
    }
}

LogEntry::LogEntry(const std::string& source, const char* event) : LogEntry(source.c_str(), event) {
}

LogEntry::LogEntry(const std::string& source, const std::string& event) : LogEntry(source.c_str(), event.c_str()) {
}

LogEntry& LogEntry::d(const char* key, const char* value) {
    prefixKeyValuePair();
    if (!key) {
        key = "";
    }
    m_stream << key << KEY_VALUE_SEPARATOR;
    appendEscapedString(value);
    return *this;
}

LogEntry& LogEntry::d(const char* key, const std::string& value) {
    return d(key, value.c_str());
}

LogEntry& LogEntry::d(const char* key, bool value) {
    return d(key, value ? BOOL_TRUE : BOOL_FALSE);
}

LogEntry& LogEntry::m(const char* message) {
    prefixMessage();
    if (message) {
        m_stream << message;
    }
    return *this;
}

LogEntry& LogEntry::m(const std::string& message) {
    prefixMessage();
    m_stream << message;
    return *this;
}

LogEntry& LogEntry::p(const char* key, const void* ptr) {
    return d(key, ptr);
}

const char* LogEntry::c_str() const {
    return m_stream.c_str();
}

void LogEntry::prefixKeyValuePair() {
    if (m_hasMetadata) {
        m_stream << PAIR_SEPARATOR;
    } else {
        m_stream << SECTION_SEPARATOR;
        m_hasMetadata = true;
    }
}

void LogEntry::prefixMessage() {
    if (!m_hasMetadata) {
        m_stream << SECTION_SEPARATOR;
    }
    m_stream << SECTION_SEPARATOR;
}

void LogEntry::appendEscapedString(const char* in) {
    if (!in) {
        return;
    }
    auto pos = in;
    // A little insurance against an infinite loop.
    auto maxCount = strlen(in);
    while (maxCount-- > 0 && *pos != 0) {
        auto next = strpbrk(pos, RESERVED_METADATA_CHARS);
        if (next) {
            m_stream.write(pos, next - pos);
            switch (*next) {
                case METADATA_ESCAPE:
                    m_stream << ESCAPED_METADATA_ESCAPE;
                    break;
                case PAIR_SEPARATOR:
                    m_stream << ESCAPED_PAIR_SEPARATOR;
                    break;
                case SECTION_SEPARATOR:
                    m_stream << ESCAPED_SECTION_SEPARATOR;
                    break;
                case KEY_VALUE_SEPARATOR:
                    m_stream << ESCAPED_KEY_VALUE_SEPARATOR;
                    break;
            }
            pos = next + 1;
        } else {
            m_stream << pos;
            return;
        }
    }
}

LogEntry& LogEntry::obfuscatePrivateData(const char* key, const std::string& value) {
    // if value contains any  private label, obfuscate the section after the label
    // since it can (but shouldn't) contain multiple,  obfuscate from the earliest one found onward
    auto firstPosition = value.length();

    const size_t count = sizeof(DENYLIST) / sizeof(DENYLIST[0]);
    for (size_t index = 0; index < count; ++index) {
        const auto privateLabel = DENYLIST[index];
        const auto privateLabelLen = std::strlen(privateLabel);
        auto it = std::search(
            value.begin(),
            value.end(),
            privateLabel,
            privateLabel + privateLabelLen,
            [](char valueChar, char denyListChar) { return std::tolower(valueChar) == std::tolower(denyListChar); });
        if (it != value.end()) {
            // capture the least value
            auto thisPosition = std::distance(value.begin(), it) + privateLabelLen;
            if (thisPosition < firstPosition) {
                firstPosition = thisPosition;
            }
        }
    }

    if (firstPosition <= value.length()) {
        // hash everything after the label itself
        auto labelPart = value.substr(0, firstPosition);
        auto obfuscatedPart = std::to_string(std::hash<std::string>{}(value.substr(firstPosition)));
        return d(key, labelPart + obfuscatedPart);
    }
    return d(key, value);
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
