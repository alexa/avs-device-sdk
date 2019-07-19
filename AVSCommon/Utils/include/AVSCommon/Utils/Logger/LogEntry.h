/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRY_H_

#include <sstream>
#include <string>

#include "AVSCommon/Utils/Logger/LogEntryStream.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// LogEntry is used to compile the log entry text to log via Logger.
class LogEntry {
public:
    /**
     * Constructor.
     *
     * @param source The name of the source of this log entry.
     * @param event The name of the event that this log entry describes.
     */
    LogEntry(const std::string& source, const char* event);

    /**
     * Constructor.
     *
     * @param source The name of the source of this log entry.
     * @param event The name of the event that this log entry describes.
     */
    LogEntry(const std::string& source, const std::string& event);

    /**
     * Add a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const std::string& key, const char* value);

    /**
     * Add a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, char* value);

    /**
     * Add a @c key, @c value pair to the metadata of this log entry.
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, const char* value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const std::string& key, const std::string& value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, const std::string& value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The boolean value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const std::string& key, bool value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The boolean value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, bool value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    template <typename ValueType>
    inline LogEntry& d(const char* key, const ValueType& value);

    /**
     * Add sensitive data in the form of a @c key, @c value pair to the metadata of this log entry.
     * Because the data is 'sensitive' it will only be emitted in DEBUG builds.
     *
     * @param key The key identifying the value to add to this LogEntry.
     * @param value The value to add to this LogEntry.
     * @return This instance to facilitate adding more information to this log entry.
     */
    template <typename ValueType>
    inline LogEntry& sensitive(const char* key, const ValueType& value);

    /**
     * Add an arbitrary message to the end of the text of this LogEntry.  Once this has been called no other
     * additions should be made to this LogEntry.
     *
     * @param message The message to add to the end of the text of this LogEntry.
     * @return This instance to facilitate passing this instance on.
     */
    LogEntry& m(const char* message);

    /**
     * Add an arbitrary message to the end of the text of this LogEntry.  Once this has been called no other
     * additions should be made to this LogEntry.
     *
     * @param message The message to add to the end of the text of this LogEntry.
     * @return This instance to facilitate passing this instance on.
     */
    LogEntry& m(const std::string& message);

    /**
     * Get the rendered text of this LogEntry.
     *
     * @return The rendered text of this LogEntry.  The returned buffer is only guaranteed to be valid for
     * the lifetime of this LogEntry, and only as long as no further modifications are made to it.
     */
    const char* c_str() const;

private:
    /// Add the appropriate prefix for a key,value pair that is about to be appended to the text of this LogEntry.
    void prefixKeyValuePair();

    /// Add the appropriate prefix for an arbitrary message that is about to be appended to the text of this LogEntry.
    void prefixMessage();

    /**
     * Append an escaped string to m_stream.
     * Our metadata and subsequent optional message is of the form:
     *     <key>=<value>[,<key>=<value>]:[<message>]
     * ...so we need to reserve ',', '=' and ':'.  We escape those vales with '\' so we escape '\' as well.
     *
     * @param in The string to escape and append.
     */
    void appendEscapedString(const char* in);

    /// Character used to separate @c key from @c value text in metadata.
    static const char KEY_VALUE_SEPARATOR = '=';

    /// Flag indicating (if true) that some metadata has already been appended to this LogEntry.
    bool m_hasMetadata;

    /// A stream with which to accumulate the text for this LogEntry.
    LogEntryStream m_stream;
};

template <typename ValueType>
LogEntry& LogEntry::d(const char* key, const ValueType& value) {
    prefixKeyValuePair();
    m_stream << key << KEY_VALUE_SEPARATOR << value;
    return *this;
}

// Define ACSDK_EMIT_SENSITIVE_LOGS if you want to include sensitive data in log output.
#ifdef ACSDK_EMIT_SENSITIVE_LOGS

template <typename ValueType>
LogEntry& LogEntry::sensitive(const char* key, const ValueType& value) {
    return d(key, value);
}
#else
template <typename ValueType>
LogEntry& LogEntry::sensitive(const char* key, const ValueType& value) {
    return *this;
}
#endif

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRY_H_
