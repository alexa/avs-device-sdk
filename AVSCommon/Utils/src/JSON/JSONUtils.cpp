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
static const std::string TAG("JsonUtils");

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
        ACSDK_ERROR(LX("findNodeFailed").d("reason", "missingDirectChild").d("child", key));
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

bool jsonArrayExists(const rapidjson::Value& parsedDocument, const std::string& key) {
    auto iter = parsedDocument.FindMember(key);
    if (parsedDocument.MemberEnd() == iter) {
        ACSDK_ERROR(LX("lookupArrayExistsFailed").d("reason", "keyNotFound").d("key", key));
        return false;
    }

    if (!iter->value.IsArray()) {
        ACSDK_ERROR(LX("lookupArrayExistsFailed").d("reason", "notArrayType"));
        return false;
    }

    return true;
}

}  // namespace jsonUtils
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
