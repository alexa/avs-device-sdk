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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONUTILS_H_

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {
namespace jsonUtils {

/**
 * Returns the tag for logging purposes.
 */
inline static std::string getTag() {
    return "JsonUtils";
}

/**
 * Give a @c rapidjson::Value object, this function will find a direct child node that matches the
 * @c key. Copy constructors are hidden for @c rapidjson::Value objects, so iterators are used to
 * retrieve the value of the node.
 *
 * @param jsonNode The JSON node object.
 * @param key The key of the underlying JSON content we wish to acquire the value of.
 * @param[out] iteratorPtr A pointer to a @c ConstMemberIterator, which will contain the @c Value.
 * @return @c true if the lookup is successful, @c false otherwise.
 */
bool findNode(
    const rapidjson::Value& jsonNode,
    const std::string& key,
    rapidjson::Value::ConstMemberIterator* iteratorPtr);

/**
 * Invoke a rapidjson parse on a JSON string.
 *
 * @param jsonContent The JSON content to be parsed.
 * @param[out] document The output parameter rapidjson document.
 * @return @c true If the JSON content was valid, @c false otherwise.
 */
bool parseJSON(const std::string& jsonContent, rapidjson::Document* document);

/**
 * Converts a given rapidjson document node to a string. The node must be either of Object or String type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the string value.
 * @return @c true If the node was converted to a string ok, @c false otherwise.
 */
bool convertToValue(const rapidjson::Value& documentNode, std::string* value);

/**
 * Converts a given rapidjson value node to a 64-bit signed integer. The node must be Int64 type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the int64_t value.
 * @return @c true If the node was successfully converted, @c false otherwise.
 */
bool convertToValue(const rapidjson::Value& documentNode, int64_t* value);

/**
 * Converts a given rapidjson value node to a 64-bit unsigned integer. The node must
 * be unsigned int type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the unsigned int value.
 * @return @c true If the node was successfully converted, @c false otherwise.
 */
bool convertToValue(const rapidjson::Value& documentNode, uint64_t* value);

/**
 * Converts a given rapidjson value node to a bool. The node must be Bool type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the bool value.
 * @return @c true If the node was successfully converted, @c false otherwise.
 */
bool convertToValue(const rapidjson::Value& documentNode, bool* value);

/**
 * A template function to find and retrieve a value of type T from a direct child of the
 * provided @c rapidjson::Value object. The type T must have an overload of the function
 * @c convertToValue.
 *
 * @param jsonNode A logical node within a parsed JSON document which rapidjson understands.
 * @param key The key in which to look for the value.
 * @param[out] value The output parameter which will be assigned the value of type T if the function
 * succeeds. No modification is done in case of failure.
 * @return @c true If the value was successfully retrieved, @c false otherwise.
 */
template <typename T>
bool retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value) {
    if (!value) {
        logger::acsdkError(logger::LogEntry(getTag(), "retrieveValueFailed").d("reason", "nullValue"));
        return false;
    }

    rapidjson::Value::ConstMemberIterator iterator;
    if (!findNode(jsonNode, key, &iterator)) {
        return false;
    }

    return convertToValue(iterator->value, value);
}

/**
 * A template function to find and retrieve a value of type T from the provided JSON string.
 * The provided string will first be parsed into a JSON document, after which the associated
 * value T will be retrieved. The type T must have an overload of the function @c convertToValue.
 *
 * @param jsonString A JSON string.
 * @param key The key in which to look for the value.
 * @param[out] value The output parameter which will be assigned the value of type T if the function
 * succeeds. No modification is done in case of failure.
 * @return @c true If the value was successfully retrieved, @c false otherwise.
 */
template <typename T>
bool retrieveValue(const std::string jsonString, const std::string& key, T* value) {
    if (!value) {
        logger::acsdkError(logger::LogEntry(getTag(), "retrieveValueFailed").d("reason", "nullValue"));
        return false;
    }

    rapidjson::Document document;
    if (!parseJSON(jsonString, &document)) {
        logger::acsdkError(logger::LogEntry(getTag(), "retrieveValueFailed").d("reason", "parsingError"));
        return false;
    }

    return retrieveValue(document, key, value);
}

/**
 * Queries whether an array object exists as a child of a pre-parsed rapidjson::Value element.
 *
 * @param parsedDocument The Value within which the array should be looked for.
 * @param key The name of the array being looked for.
 * @return Whether a child element of array type was found.
 */
bool jsonArrayExists(const rapidjson::Value& parsedDocument, const std::string& key);

}  // namespace jsonUtils
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONUTILS_H_
