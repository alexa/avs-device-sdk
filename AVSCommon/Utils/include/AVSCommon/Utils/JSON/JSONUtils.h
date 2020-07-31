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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONUTILS_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

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
 * Converts a given rapidjson value node to a double. The node must be double type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the double value.
 * @return @c true If the node was successfully converted, @c false otherwise.
 */
bool convertToValue(const rapidjson::Value& documentNode, double* value);

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
 *
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

/**
 * Find and retrieve a string collection from the provided stringified JSON.
 *
 * Example:
 * @code
 * auto elements = retrieveStringArray(R"({"key":["element1", "element2"]})", "key");
 * @endcode
 *
 * @tparam CollectionT The collection type.
 * @param jsonString The input JSON string.
 * @param key The name of the array being looked for.
 * @return The output collection which will contain all extracted elements.  An empty collection if jsonString is
 * malformed, contains non-string elements, or if array elements with key is not found.
 *
 * @note  This function will only look at the first level to find the array with the key.
 */
template <class CollectionT>
CollectionT retrieveStringArray(const std::string& jsonString, const std::string& key);

/**
 * Convert a JSON array of strings into a string collection.
 *
 * @param jsonString The input JSON. This should represent the array string not the entire document. E.g.:
 * @code
 * auto elements = retrieveStringArray(R"(["element1", "element2"])");
 * @endcode
 * @return The output string collection which will contain all extracted elements.  An empty collection if jsonString is
 * malformed, or contains non-string elements.
 */
template <class CollectionT>
CollectionT retrieveStringArray(const std::string& jsonString);

/**
 * Retrieve string elements from a rapidjson value.
 *
 * @param value A @c Value that represents an array of strings.
 * @return The output string collection which will contain all extracted elements.  An empty collection if value
 * contains non-string elements.
 */
template <class CollectionT>
CollectionT retrieveStringArray(const rapidjson::Value& value);

/**
 * Convert a string collection into a JSON string array representation.
 *
 * @param elements A collection that will be converted to the JSON array representation.
 * @return The JSON string array representation if it succeeds; otherwise, return an empty string.
 */
template <class CollectionT>
std::string convertToJsonString(const CollectionT& elements);

/**
 * Retrieve a string map from a @c rapidjson value with the given key.
 *
 * @param value A @c Value that represents a string map.
 * @param key The name of the array being looked for.
 * @return The output collection which will contain all extracted elements.  An empty collection if value
 * contains non-string elements or if the key was not found.
 */
std::map<std::string, std::string> retrieveStringMap(const rapidjson::Value& value, const std::string& key);

/**
 * Retrieve string map from array of a @c rapidjson value with the given key.
 *
 * @param value A @c Value that represents a array of string map.
 * @param key The name of the array being looked for.
 * @param elements The output collection which will contain all extracted elements.  An empty collection if value
 * contains non-string elements or if the key was not found.
 */
void retrieveStringMapFromArray(
    const rapidjson::Value& value,
    const std::string& key,
    std::map<std::string, std::string>& elements);

/**
 * Retrieve array of string map from array of a @c rapidjson value with the given key.
 *
 * @param value A @c Value that represents a array of string map.
 * @param key The name of the array being looked for.
 * @param elements The output collection which will contain all extracted elements.
 *
 * @return A bool indicating success or failure retrieving elements
 */
bool retrieveArrayOfStringMapFromArray(
    const rapidjson::Value& value,
    const std::string& key,
    std::vector<std::map<std::string, std::string>>& elements);

//----- Templated functions implementation and specializations.

template <>
std::vector<std::string> retrieveStringArray<std::vector<std::string>>(
    const std::string& jsonString,
    const std::string& key);

template <>
std::vector<std::string> retrieveStringArray<std::vector<std::string>>(const std::string& jsonString);

template <>
std::vector<std::string> retrieveStringArray<std::vector<std::string>>(const rapidjson::Value& value);

template <>
std::string convertToJsonString<std::vector<std::string>>(const std::vector<std::string>& elements);

template <class CollectionT>
CollectionT retrieveStringArray(const std::string& jsonString, const std::string& key) {
    auto values = retrieveStringArray<std::vector<std::string>>(jsonString, key);
    return CollectionT{values.begin(), values.end()};
}

template <class CollectionT>
CollectionT retrieveStringArray(const std::string& jsonString) {
    auto values = retrieveStringArray<std::vector<std::string>>(jsonString);
    return CollectionT{values.begin(), values.end()};
}

template <class CollectionT>
CollectionT retrieveStringArray(const rapidjson::Value& value) {
    auto values = retrieveStringArray<std::vector<std::string>>(value);
    return CollectionT{values.begin(), values.end()};
}

template <class CollectionT>
std::string convertToJsonString(const CollectionT& elements) {
    return convertToJsonString(std::vector<std::string>{elements.begin(), elements.end()});
}

}  // namespace jsonUtils
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONUTILS_H_
