/*
 * JSONParser.cpp
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

#include "AVSCommon/JSON/JSONUtils.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "AVSUtils/Logging/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace jsonUtils {

using namespace avsUtils;

/**
 * Invoke a rapidjson parse a on JSON string.
 *
 * @param jsonContent The JSON content to be parsed.
 * @param[out] document The output parameter rapidjson document.
 * @return @c true If the JSON content was valid, @c false otherwise.
 */
static bool parseJSON(const std::string& jsonContent, rapidjson::Document* document) {

    document->Parse(jsonContent.c_str());

    if (document->HasParseError()) {
        Logger::log("rapidjson detected a parsing error at offset:" + std::to_string(document->GetErrorOffset()) +
                            ", error message: " + GetParseError_En(document->GetParseError()));
        return false;
    }
    return true;
}

/**
 * Verify a given rapidjson document node contains a given key as a direct child node.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param key The key being looked up.
 * @return @c true If the document node contains the key as a direct child node, @c false otherwise.
 */
static bool documentNodeContainsKey(const rapidjson::Value& documentNode, const std::string& key) {
    if (!documentNode.IsObject()) {
        Logger::log("The JSON node is not of object type.  Only object types can contain key-value pair child nodes.");
        return false;
    }
    return documentNode.FindMember(key.c_str()) != documentNode.MemberEnd();
}

/**
 * Serialize a rapidjson document node, which must be of Object type, into a string.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter where the converted string will be assigned.
 */
static void serializeJSONObjectToString(const rapidjson::Value& documentNode, std::string* value) {

    if(!documentNode.IsObject()) {
        Logger::log("The given rapidjson node is not of Object type.");
        return;
    }

    rapidjson::StringBuffer stringBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);

    if (!documentNode.Accept(writer)) {
        Logger::log("The given rapidjson node could not accept a writer object.");
        return;
    }

    *value = stringBuffer.GetString();
}

/**
 * Converts a given rapidjson document node to a string.  The node must be either of Object or String type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the string value.
 * @return @c true If the node was converted to a string ok, @c false otherwise.
 */
static bool getValueAsString(const rapidjson::Value& documentNode, std::string* value) {

    if(!documentNode.IsString() && !documentNode.IsObject()) {
        Logger::log("rapidjson document node cannot be converted to a string.");
        return false;
    }

    if (documentNode.IsString()) {
        *value = documentNode.GetString();
    } else if (documentNode.IsObject()) {
        serializeJSONObjectToString(documentNode, value);
    }

    return true;
}

/**
 * Converts a given rapidjson value node to a 64-bit signed integer.  The node must be Int64 type.
 *
 * @param documentNode A logical node within a parsed JSON document which rapidjson understands.
 * @param[out] value The output parameter which will be assigned the int64_t value.
 * @return @c true If the node was converted to an int64 ok, @c false otherwise.
 */
static bool getValueAsInt64(const rapidjson::Value& valueNode, int64_t* value) {
    if(!valueNode.IsInt64()) {
        Logger::log("rapidjson document node cannot be converted to an int64.");
        return false;
    }

    *value = valueNode.GetInt64();

    return true;
}

bool lookupStringValue(const std::string& jsonContent, const std::string& key, std::string* value) {
    if (!value) {
        Logger::log("The output parameter value is nullptr.");
        return false;
    }

    rapidjson::Document document;
    if (!parseJSON(jsonContent, &document)) {
        Logger::log("The json content could not be parsed.");
        return false;
    }

    // To traverse the logical JSON tree, rapidjson works with value types, from which document derives.
    // Let's make a local reference of this type to clarify what's going on.
    const rapidjson::Value& documentRootNode = document;

    if (!documentNodeContainsKey(documentRootNode, key)) {
        Logger::log("The parsed JSON document does not contain a direct child node with the key:'" + key + "'.");
        return false;
    }

    if (!getValueAsString(document[key.c_str()], value)) {
        Logger::log("Could not convert the rapidjson document node to a string.");
        return false;
    }

    return true;
}

bool lookupInt64Value(const std::string& jsonContent, const std::string& key, int64_t* value) {
    if (!value) {
        Logger::log("The output parameter value is nullptr.");
        return false;
    }

    rapidjson::Document document;
    if (!parseJSON(jsonContent, &document)) {
        Logger::log("The json content could not be parsed.");
        return false;
    }

    // To traverse the logical JSON tree, rapidjson works with value types, from which document derives.
    // Let's make a local reference of this type to clarify what's going on.
    const rapidjson::Value& documentRootNode = document;

    if (!documentNodeContainsKey(documentRootNode, key)) {
        Logger::log("The parsed JSON document does not contain a direct child node with the key:'" + key + "'.");
        return false;
    }

    if (!getValueAsInt64(document[key.c_str()], value)) {
        Logger::log("Could not convert the rapidjson document node to an int64.");
        return false;
    }

    return true;
}

} // namespace json
} // namespace avsCommon
} // namespace alexaClientSDK
