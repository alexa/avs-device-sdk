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

#include "AVSCommon/Utils/JSON/JSONUtils.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {
namespace jsonUtils {

/// String to identify log entries originating from this file.
#define TAG "JsonUtils"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Write a @c rapidjson::Type value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param type The type value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
static std::ostream& operator<<(std::ostream& stream, rapidjson::Type type) {
    switch (type) {
        case rapidjson::Type::kNullType:
            stream << "NULL";
            break;
        case rapidjson::Type::kFalseType:
            stream << "FALSE";
            break;
        case rapidjson::Type::kTrueType:
            stream << "TRUE";
            break;
        case rapidjson::Type::kObjectType:
            stream << "OBJECT";
            break;
        case rapidjson::Type::kArrayType:
            stream << "ARRAY";
            break;
        case rapidjson::Type::kStringType:
            stream << "STRING";
            break;
        case rapidjson::Type::kNumberType:
            stream << "NUMBER";
            break;
    }
    return stream;
}

/**
 * Invoke a rapidjson parse a on JSON string.
 *
 * @param jsonContent The JSON content to be parsed.
 * @param[out] document The output parameter rapidjson document.
 * @return @c true If the JSON content was valid, @c false otherwise.
 */
bool parseJSON(const std::string& jsonContent, rapidjson::Document* document) {
    if (!document) {
        ACSDK_ERROR(LX("parseJSONFailed").d("reason", "nullDocument"));
        return false;
    }

    document->Parse(jsonContent.c_str());

    if (document->HasParseError()) {
        ACSDK_ERROR(LX("parseJSONFailed")
                        .d("offset", document->GetErrorOffset())
                        .d("error", GetParseError_En(document->GetParseError())));
        return false;
    }
    return true;
}

/**
 * Serialize a rapidjson document node, which must be of Object type, into a string.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter where the converted string will be assigned.
 */
static void serializeJSONObjectToString(const rapidjson::Value& documentNode, std::string* value) {
    if (!documentNode.IsObject()) {
        ACSDK_ERROR(LX("serializeJSONObjectToStringFailed")
                        .d("reason", "invalidType")
                        .d("expectedType", rapidjson::Type::kObjectType)
                        .d("type", documentNode.GetType()));
        return;
    }

    rapidjson::StringBuffer stringBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);

    if (!documentNode.Accept(writer)) {
        ACSDK_ERROR(LX("serializeJSONObjectToStringFailed").d("reason", "acceptFailed").d("handler", "Writer"));
        return;
    }

    *value = stringBuffer.GetString();
}

bool findNode(
    const rapidjson::Value& jsonNode,
    const std::string& key,
    rapidjson::Value::ConstMemberIterator* iteratorPtr) {
    if (!iteratorPtr) {
        ACSDK_ERROR(LX("findNodeFailed").d("reason", "nullIteratorPtr"));
        return false;
    }

    auto iterator = jsonNode.FindMember(key);
    if (iterator == jsonNode.MemberEnd()) {
        ACSDK_DEBUG5(LX("findNode").d("reason", "missingDirectChild").d("child", key));
        return false;
    }

    *iteratorPtr = iterator;

    return true;
}

// Overloads of convertToValue

bool convertToValue(const rapidjson::Value& documentNode, std::string* value) {
    if (!value) {
        ACSDK_ERROR(LX("convertToStringValueFailed").d("reason", "nullValue"));
        return false;
    }

    if (!documentNode.IsString() && !documentNode.IsObject()) {
        std::ostringstream expectedTypes;
        expectedTypes << rapidjson::Type::kObjectType << "/" << rapidjson::Type::kStringType;

        ACSDK_ERROR(LX("convertToStringValueFailed")
                        .d("reason", "invalidType")
                        .d("expectedTypes", expectedTypes.str())
                        .d("type", documentNode.GetType()));
        return false;
    }

    if (documentNode.IsString()) {
        *value = documentNode.GetString();
    } else if (documentNode.IsObject()) {
        serializeJSONObjectToString(documentNode, value);
    }

    return true;
}

bool convertToValue(const rapidjson::Value& documentNode, uint64_t* value) {
    if (!value) {
        ACSDK_ERROR(LX("convertToUnsignedInt64ValueFailed").d("reason", "nullValue"));
        return false;
    }

    if (!documentNode.IsUint64()) {
        ACSDK_ERROR(LX("convertToUnsignedInt64ValueFailed")
                        .d("reason", "invalidValue")
                        .d("expectedValue", "Uint64")
                        .d("type", documentNode.GetType()));
        return false;
    }

    *value = documentNode.GetUint64();

    return true;
}

bool convertToValue(const rapidjson::Value& documentNode, int64_t* value) {
    if (!value) {
        ACSDK_ERROR(LX("convertToInt64ValueFailed").d("reason", "nullValue"));
        return false;
    }

    if (!documentNode.IsInt64()) {
        ACSDK_ERROR(LX("convertToInt64ValueFailed")
                        .d("reason", "invalidValue")
                        .d("expectedValue", "Int64")
                        .d("type", documentNode.GetType()));

        return false;
    }

    *value = documentNode.GetInt64();

    return true;
}

bool convertToValue(const rapidjson::Value& documentNode, bool* value) {
    if (!value) {
        ACSDK_ERROR(LX("convertToBoolValueFailed").d("reason", "nullValue"));
        return false;
    }

    if (!documentNode.IsBool()) {
        ACSDK_ERROR(LX("convertToBoolValueFailed")
                        .d("reason", "invalidValue")
                        .d("expectedValue", "Bool")
                        .d("type", documentNode.GetType()));
        return false;
    }

    *value = documentNode.GetBool();

    return true;
}

bool convertToValue(const rapidjson::Value& documentNode, double* value) {
    if (!value) {
        ACSDK_ERROR(LX("convertToDoubleValueFailed").d("reason", "nullValue"));
        return false;
    }

    if (!documentNode.IsDouble()) {
        ACSDK_ERROR(LX("convertToDoubleValueFailed")
                        .d("reason", "invalidValue")
                        .d("expectedValue", "Double")
                        .d("type", documentNode.GetType()));
        return false;
    }

    *value = documentNode.GetDouble();

    return true;
}

bool jsonArrayExists(const rapidjson::Value& parsedDocument, const std::string& key) {
    auto iter = parsedDocument.FindMember(key);
    if (parsedDocument.MemberEnd() == iter) {
        ACSDK_DEBUG0(LX(__func__).d("reason", "keyNotFound").d("key", key));
        return false;
    }

    if (!iter->value.IsArray()) {
        ACSDK_DEBUG0(LX(__func__).d("reason", "notArrayType").d("key", key).d("type", parsedDocument.GetType()));
        return false;
    }

    return true;
}

template <>
std::vector<std::string> retrieveStringArray<std::vector<std::string>>(
    const std::string& jsonString,
    const std::string& key) {
    rapidjson::Document document;
    document.Parse(jsonString);

    if (document.HasParseError()) {
        ACSDK_ERROR(LX("retrieveElementsFailed")
                        .d("offset", document.GetErrorOffset())
                        .d("error", GetParseError_En(document.GetParseError())));
        return std::vector<std::string>();
    }

    auto iter = document.FindMember(key);
    if (document.MemberEnd() == iter) {
        ACSDK_ERROR(LX("retrieveElementsFailed").d("reason", "keyNotFound").d("key", key));
        return std::vector<std::string>();
    }

    return retrieveStringArray<std::vector<std::string>>(iter->value);
}

template <>
std::vector<std::string> retrieveStringArray<std::vector<std::string>>(const std::string& jsonString) {
    rapidjson::Document document;
    document.Parse(jsonString);

    if (document.HasParseError()) {
        ACSDK_ERROR(LX("retrieveElementsFailed")
                        .d("offset", document.GetErrorOffset())
                        .d("error", GetParseError_En(document.GetParseError())));
        return std::vector<std::string>();
    }

    return retrieveStringArray<std::vector<std::string>>(document);
}

template <>
std::vector<std::string> retrieveStringArray<std::vector<std::string>>(const rapidjson::Value& value) {
    std::vector<std::string> elements;
    if (!value.IsArray()) {
        ACSDK_ERROR(LX("retrieveElementsFailed").d("reason", "nonArrayString"));
        return elements;
    }

    for (auto& item : value.GetArray()) {
        if (item.IsString()) {
            elements.push_back(item.GetString());
        } else {
            ACSDK_WARN(LX("retrieveStringArray").d("result", "ignoredEntry"));
        }
    }
    return elements;
}

template <>
std::string convertToJsonString<std::vector<std::string>>(const std::vector<std::string>& elements) {
    rapidjson::Document document{rapidjson::kArrayType};
    for (auto& item : elements) {
        document.PushBack(rapidjson::StringRef(item.c_str()), document.GetAllocator());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!document.Accept(writer)) {
        ACSDK_ERROR(LX("convertToJsonStringFailed")
                        .d("reason", "")
                        .d("offset", document.GetErrorOffset())
                        .d("error", GetParseError_En(document.GetParseError())));
        return std::string();
    }

    return buffer.GetString();
}

std::map<std::string, std::string> retrieveStringMap(const rapidjson::Value& value, const std::string& key) {
    std::map<std::string, std::string> elements;
    auto objectIt = value.FindMember(key);
    if (value.MemberEnd() == objectIt || !objectIt->value.IsObject()) {
        ACSDK_DEBUG0(LX("retrieveElementsFailed").d("reason", "couldNotFindObject").d("key", key));
        return elements;
    }

    for (auto it = objectIt->value.MemberBegin(); it != objectIt->value.MemberEnd(); ++it) {
        if (it->name.IsString() && it->value.IsString()) {
            elements[it->name.GetString()] = it->value.GetString();
        } else {
            ACSDK_WARN(LX(__func__).d("result", "ignoredEntry"));
        }
    }
    return elements;
}

void retrieveStringMapFromArray(
    const rapidjson::Value& value,
    const std::string& key,
    std::map<std::string, std::string>& elements) {
    rapidjson::Value::ConstMemberIterator objectIt = value.FindMember(key);
    if (objectIt == value.MemberEnd()) {
        ACSDK_DEBUG0(LX("retrieveStringMapFromArrayFailed").d("reason", "couldNotFindObject").d("key", key));
        return;
    }

    const rapidjson::Value& mapOfAttributes = objectIt->value;
    if (!mapOfAttributes.IsArray()) {
        ACSDK_DEBUG0(LX("retrieveStringMapFromArrayFailed").d("reason", "NotAnArray").d("key", key));
        return;
    }

    for (rapidjson::Value::ConstValueIterator itr = mapOfAttributes.Begin(); itr != mapOfAttributes.End(); ++itr) {
        const rapidjson::Value& attribute = *itr;
        // each attribute is an object
        if (!attribute.IsObject()) {
            ACSDK_DEBUG0(LX("retrieveStringMapFromArrayFailed").d("reason", "NotAnObject").d("key", key));
            return;
        }

        rapidjson::Value::ConstMemberIterator keyEntry = attribute.MemberBegin();
        if (keyEntry != attribute.MemberEnd()) {
            rapidjson::Value::ConstMemberIterator valueEntry = keyEntry + 1;
            if (valueEntry != attribute.MemberEnd()) {
                if (keyEntry->value.IsString() && valueEntry->value.IsString()) {
                    elements[keyEntry->value.GetString()] = valueEntry->value.GetString();
                } else {
                    ACSDK_DEBUG0(LX("retrieveStringMapFromArrayFailed").d("reason", "NotAString").d("key", key));
                    elements.clear();
                    return;
                }
            }
        }
    }
}

bool retrieveArrayOfStringMapFromArray(
    const rapidjson::Value& value,
    const std::string& key,
    std::vector<std::map<std::string, std::string>>& elements) {
    rapidjson::Value::ConstMemberIterator objectIt = value.FindMember(key);
    if (objectIt == value.MemberEnd()) {
        ACSDK_DEBUG0(LX(__func__).d("reason", "couldNotFindObject").d("key", key));
        return false;
    }

    const rapidjson::Value& mapOfAttributes = objectIt->value;
    if (!mapOfAttributes.IsArray()) {
        ACSDK_DEBUG0(LX(__func__).d("reason", "NotAnArray").d("key", key));
        return false;
    }

    for (rapidjson::Value::ConstValueIterator itr = mapOfAttributes.Begin(); itr != mapOfAttributes.End(); ++itr) {
        std::map<std::string, std::string> element;
        const rapidjson::Value& attribute = *itr;
        // each attribute is an object
        if (!attribute.IsObject()) {
            ACSDK_DEBUG0(LX(__func__).d("reason", "NotAnObject").d("key", key));
            elements.clear();
            return false;
        }

        for (auto keyEntry = attribute.MemberBegin(); keyEntry != attribute.MemberEnd(); keyEntry++) {
            if (keyEntry->name.IsString() && keyEntry->value.IsString()) {
                element[keyEntry->name.GetString()] = keyEntry->value.GetString();
            } else {
                ACSDK_DEBUG0(LX(__func__).d("reason", "ObjectValuesNotString").d("key", key));
                elements.clear();
                return false;
            }
        }

        elements.push_back(element);
    }

    return true;
}

}  // namespace jsonUtils
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
