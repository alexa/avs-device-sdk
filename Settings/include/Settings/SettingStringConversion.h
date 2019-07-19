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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSTRINGCONVERSION_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSTRINGCONVERSION_H_

#include <sstream>
#include <string>
#include <utility>

namespace alexaClientSDK {
namespace settings {

/// Quote used for json string values.
constexpr char QUOTE = '"';

//----------------- Setting conversion functions for int8_t and uint8_t -----------------//

/**
 * Test whether the type is int8_t or uint8_t.
 *
 * @tparam ValueT The value type to be tested.
 * @return @c true if the given type is (u)int8_t; false otherwise.
 */
template <typename ValueT>
static constexpr bool isIntegralByteType() {
    return std::is_same<ValueT, int8_t>::value || std::is_same<ValueT, uint8_t>::value;
}

/**
 * Define a valid type for int8_t and uint8_t.
 *
 * @tparam ValueT The value type. This will only be a valid type for int8_t or uint8_t.
 */
template <typename ValueT>
using IntegralByteType = typename std::enable_if<isIntegralByteType<ValueT>(), ValueT>::type;

/**
 * Convert a setting that is either a int8_t or uint8_t to a string (json format) representation.
 *
 * @tparam ValueT The value type (either int8_t or uint8_t).
 * @param value The value to be converted.
 * @return A pair where first represents whether the conversion succeeded or not, and the second is the converted
 * string.
 */
template <typename ValueT>
std::pair<bool, std::string> toSettingString(const IntegralByteType<ValueT>& value) {
    std::stringstream stream;
    int valueInt = value;
    stream << valueInt;
    return {!stream.fail(), stream.str()};
}

/**
 * Convert a string (json format) to (int8_t or uint8_t) setting representation.
 *
 * @tparam ValueT The value type (either int8_t or uint8_t).
 * @param str The json string that represents the object.
 * @param defaultValue In case the string conversion fails, we'll return this default value.
 * @return A pair where first represents whether the conversion succeeded or not, and the second is the converted
 * value.
 */
template <typename ValueT>
std::pair<bool, ValueT> fromSettingString(const std::string& str, const IntegralByteType<ValueT>& defaultValue) {
    int16_t value = defaultValue;
    std::stringstream stream;
    stream << str;
    stream >> value;
    return {!stream.fail(), stream.fail() ? defaultValue : value};
}

//----------------- Setting conversion functions for std::string and enums -----------------//

/**
 * Test whether the type is either an enum type or string.
 *
 * @tparam ValueT The value type to be tested.
 * @return @c true if the given type is (u)int8_t; false otherwise.
 */
template <typename ValueT>
static constexpr bool isEnumOrString() {
    return std::is_enum<ValueT>::value || std::is_same<ValueT, std::string>::value;
}

/**
 * Define a valid type for enums and string types.
 *
 * @tparam ValueT The value type. This will only be a valid type for enums and string.
 */
template <typename ValueT>
using EnumOrString = typename std::enable_if<isEnumOrString<ValueT>(), ValueT>::type;

/**
 * Convert a setting that is either a enum or string to a json format string representation.
 *
 * @tparam ValueT The value type.
 * @param value The value to be converted.
 * @return A pair where first represents whether the conversion succeeded or not, and the second is the converted
 * string.
 * @note For enums, we assume the operator<< is defined and the failbit should be set in case of failure to convert to
 * string.
 */
template <typename ValueT>
std::pair<bool, std::string> toSettingString(const EnumOrString<ValueT>& value) {
    std::stringstream stream;
    stream << QUOTE << value << QUOTE;
    return {!stream.fail(), stream.str()};
}

/**
 * Convert a string (json format) to setting representation for strings and enums.
 *
 * @tparam ValueT The value type.
 * @param str The json string that represents the object.
 * @param defaultValue In case the string conversion fails, we'll return this default value.
 * @return A pair where first represents whether the conversion succeeded or not, and the second is the converted
 * value.
 * @note For enums, we assume the operator>> is defined and the failbit should be set in case of failure to convert
 * from string.
 */
template <typename ValueT>
std::pair<bool, ValueT> fromSettingString(const std::string& str, const EnumOrString<ValueT>& defaultValue) {
    if (str.length() < 2 || str[0] != QUOTE || str[str.length() - 1] != QUOTE) {
        return {false, defaultValue};
    }

    ValueT value = defaultValue;
    std::stringstream stream;
    stream << str.substr(1, str.length() - 2);  // Remove quotes from the string.
    stream >> value;
    return {!stream.fail(), value};
}

//----------------- Setting conversion functions for arithmetic types and classes -----------------//

/**
 * Define a valid type for arithmetic types (except (u)int8_t), and classes.
 *
 * @tparam ValueT The value type. This will only be a valid type for arithmetic types (except (u)int8_t), and classes.
 */
template <typename ValueT>
using OtherTypes = typename std::enable_if<!isEnumOrString<ValueT>() && !isIntegralByteType<ValueT>(), ValueT>::type;

/**
 * Convert a setting that is either arithmetic types (except (u)int8_t), and classes to a json format representation.
 *
 * @tparam ValueT The value type.
 * @param value The value to be converted.
 * @return A pair where first represents whether the conversion succeeded or not, and the second is the converted
 * string.
 * @note For classes, we assume the operator<< is defined and the failbit should be set in case of failure to convert to
 * string.
 */
template <typename ValueT>
std::pair<bool, std::string> toSettingString(const OtherTypes<ValueT>& value) {
    std::stringstream stream;
    stream << std::boolalpha << value;
    return {!stream.fail(), stream.str()};
}

/**
 * Convert a string (json format) to setting representation for arithmetic types (except (u)int8_t), and classes.
 *
 * @tparam ValueT The value type.
 * @param str The json string that represents the object.
 * @param defaultValue In case the string conversion fails, we'll return this default value.
 * @return A pair where first represents whether the conversion succeeded or not, and the second is the converted
 * value.
 * @note For classes and structs, we assume the operator>> is defined and the failbit should be set in case of failure
 * to convert from string.
 */
template <typename ValueT>
std::pair<bool, ValueT> fromSettingString(const std::string& str, const OtherTypes<ValueT>& defaultValue) {
    ValueT value = defaultValue;
    std::stringstream stream;
    stream << str;
    stream >> std::boolalpha >> value;
    return {!stream.fail(), stream.fail() ? defaultValue : value};
}

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGSTRINGCONVERSION_H_
