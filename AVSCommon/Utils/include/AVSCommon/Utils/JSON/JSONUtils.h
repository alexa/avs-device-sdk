/*
 * JSONUtils.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_JSON_JSONUTILS_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_JSON_JSONUTILS_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {
namespace jsonUtils {

/**
 * Given a JSON string, this function will look up a particular string value of a direct child
 * node of the logical JSON document.  If the node being looked up is a logical JSON object,
 * then that object will be serialized and placed in the output string parameter.
 *
 * In this initial version of the api, this function will return false if the given key refers to a
 * boolean, number or array.
 *
 * TODO: [ACSDK-177] This function needs to be reconsidered in the future because it is of limited utility.
 *
 * @param jsonContent The JSON string content.
 * @param key The key of the underlying JSON content we wish to acquire the value of.
 * @param[out] value The output parameter which will be assigned the string value.
 * @return @c true if the lookup is successful, @c false otherwise.
 */
bool lookupStringValue(const std::string& jsonContent, const std::string& key, std::string* value);

/**
 * This function is similar to lookupStringValue(), but converts the value to an int64_t
 *
 * TODO: [ACSDK-177] This function needs to be reconsidered in the future because it is of limited utility.
 *
 * @param jsonContent The JSON string content.
 * @param key The key of the underlying JSON content we wish to acquire the value of.
 * @param[out] value The output parameter which will be assigned the int64_t value.
 * @return @c true if the lookup is successful, @c false otherwise.
 */
bool lookupInt64Value(const std::string& jsonContent, const std::string& key, int64_t* value);

} // namespace jsonUtils
} // namespace json
} // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_AVS_COMMON_UTILS_INCLUDE_AVS_COMMON_UTILS_JSON_JSONUTILS_H_
