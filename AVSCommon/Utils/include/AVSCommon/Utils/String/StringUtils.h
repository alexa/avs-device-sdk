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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STRING_STRINGUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STRING_STRINGUTILS_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace string {

/**
 * A utility function to convert a std::string to an integer.
 * If the string is successfully parsed, then the out parameter will be updated.
 *
 * @param str The string input.
 * @param[out] result The resulting integer, if successfully parsed from the string.
 * @return @c true If the string was parsed as an integer, otherwise @c false.
 */
bool stringToInt(const std::string& str, int* result);

/**
 * A utility function to convert a c-string to an integer.
 * If the string is successfully parsed, then the out parameter will be updated.
 *
 * @param str The C-string input.
 * @param[out] result The resulting integer, if successfully parsed from the string.
 * @return @c true If the string was parsed as an integer, otherwise @c false.
 */
bool stringToInt(const char* str, int* result);

/**
 * A utility function to convert a vector of bytes to a printable string.  For example, the vector {1, 2, 3} will return
 * the string "0x01 0x02 0x03"
 *
 * @param byteVector a vector of bytes
 * @return a string of the hex values of each byte printed.
 */
std::string byteVectorToString(const std::vector<uint8_t>& byteVector);

/**
 * A utility function to convert a string into lower case.
 *
 * @param input The input string to be converted.
 * @return The converted string in lower case.
 */
std::string stringToLowerCase(const std::string& input);

}  // namespace string
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_STRING_STRINGUTILS_H_
