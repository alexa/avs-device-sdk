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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRY_H_

#include <algorithm>
#include <cctype>
#include <functional>
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
     * @param[in] source The name of the source of this log entry. If @a source is nullptr, it is treated as an empty
     *                   string.
     * @param[in] event  The name of the event that this log entry describes. If @a event is nullptr, it is treated as
     *                   an empty string.
     */
    LogEntry(const char* source, const char* event);

    /**
     * Constructor.
     *
     * @param[in] source The name of the source of this log entry.
     * @param[in] event  The name of the event that this log entry describes. If @a event is nullptr, it is treated as
     *                   an empty string.
     */
    LogEntry(const std::string& source, const char* event);

    /**
     * Constructor.
     *
     * @param[in] source The name of the source of this log entry.
     * @param[in] event  The name of the event that this log entry describes.
     */
    LogEntry(const std::string& source, const std::string& event);

    /**
     * Add a @c key, @c value pair to the metadata of this log entry.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    template <typename ValueType>
    LogEntry& d(const std::string& key, const ValueType& value);

    /**
     * Add a @c key, @c value pair to the metadata of this log entry.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry. If @a value is nullptr, it is treated as an empty string.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, const char* value);

    /**
     * Add a @c key, @c value pair to the metadata of this log entry.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry. If @a value is nullptr, it is treated as an empty string.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, char* value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, const std::string& value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The boolean value to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& d(const char* key, bool value);

    /**
     * Add data (hence the name 'd') in the form of a @c key, @c value pair to the metadata of this log entry.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    template <typename ValueType>
    LogEntry& d(const char* key, const ValueType& value);

    /**
     * Add sensitive data in the form of a @c key, @c value pair to the metadata of this log entry.
     * Because the data is 'sensitive' it will only be emitted in DEBUG builds.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    template <typename ValueType>
    LogEntry& sensitive(const char* key, const ValueType& value);

    /**
     * Add data in the form of a @c key, @c value pair to the metadata of this log entry.
     * If the value includes a privacy denylist entry, the portion after that will be obfuscated.
     * This is done in a distinct method (instead of m or d)  to avoid the cost of always checking
     * against the denylist.
     *
     * @param[in] key   The key identifying the value to add to this LogEntry.
     * @param[in] value The value to add to this LogEntry, obfuscated if needed.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& obfuscatePrivateData(const char* key, const std::string& value);

    /**
     * Add an arbitrary message to the end of the text of this LogEntry.  Once this has been called no other
     * additions should be made to this LogEntry.
     *
     * @param[in] message The message to add to the end of the text of this LogEntry. If @a message is a nullptr, it is
     *                    treated as an empty string.
     *
     * @return This instance to facilitate passing this instance on.
     */
    LogEntry& m(const char* message);

    /**
     * Add an arbitrary message to the end of the text of this LogEntry.  Once this has been called no other
     * additions should be made to this LogEntry.
     *
     * @param[in] message The message to add to the end of the text of this LogEntry.
     *
     * @return This instance to facilitate passing this instance on.
     */
    LogEntry& m(const std::string& message);

    /**
     * Add pointer (hence the name 'p') in the form of a @c key, address of the object pointed to by the shared_ptr @c
     * ptr to the metadata of this log entry.
     *
     * @param[in] key The key identifying the value to add to this LogEntry. If @a key is a nullptr, it is treated as an
     *                empty string.
     * @param[in] ptr The shared_ptr of the object to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    template <typename PtrType>
    LogEntry& p(const char* key, const std::shared_ptr<PtrType>& ptr);

    /**
     * Add pointer (hence the name 'p') in the form of a @c key, address of the raw @c ptr to the metadata of this
     * log entry.
     *
     * @param[in] key The key identifying the value to add to this LogEntry. If @a key is a nullptr, it is treated as an
     *                empty string.
     * @param[in] ptr The raw pointer of the object to add to this LogEntry.
     *
     * @return This instance to facilitate adding more information to this log entry.
     */
    LogEntry& p(const char* key, const void* ptr);

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
inline LogEntry& LogEntry::d(const std::string& key, const ValueType& value) {
    return d(key.c_str(), value);
}

template <typename ValueType>
inline LogEntry& LogEntry::d(const char* key, const ValueType& value) {
    prefixKeyValuePair();
    m_stream << key << KEY_VALUE_SEPARATOR << value;
    return *this;
}

inline LogEntry& LogEntry::d(const char* key, char* value) {
    return d(key, const_cast<const char*>(value));
}

template <typename PtrType>
inline LogEntry& LogEntry::p(const char* key, const std::shared_ptr<PtrType>& ptr) {
    return d(key, ptr.get());
}

// Define ACSDK_EMIT_SENSITIVE_LOGS if you want to include sensitive data in log output.
#ifdef ACSDK_EMIT_SENSITIVE_LOGS

template <typename ValueType>
inline LogEntry& LogEntry::sensitive(const char* key, const ValueType& value) {
    return d(key, value);
}
#else
template <typename ValueType>
inline LogEntry& LogEntry::sensitive(const char*, const ValueType&) {
    return *this;
}
#endif

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_LOGENTRY_H_
