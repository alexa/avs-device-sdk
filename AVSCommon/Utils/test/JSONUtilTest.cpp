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

/// @file JSONUtilTest.cpp

#include <memory>
#include <map>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include "AVSCommon/AVS/AVSDirective.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon;
using namespace rapidjson;
using namespace jsonUtils;

/// The header key in the JSON content of an AVS message.
static const std::string JSON_MESSAGE_HEADER_STRING = "header";
/// The namespace key in the JSON content of an AVS message.
static const std::string JSON_MESSAGE_NAMESPACE_STRING = "namespace";
/// The name key in the JSON content of an AVS message.
static const std::string JSON_MESSAGE_NAME_STRING = "name";
/// The messageId key in the JSON content of an AVS message.
static const std::string JSON_MESSAGE_ID_STRING = "messageId";
/// The dialogRequestId key in the JSON content of an AVS message.
static const std::string JSON_MESSAGE_DIALOG_REQUEST_ID_STRING = "dialogRequestId";
/// The payload key in the JSON content of an AVS message.
static const std::string JSON_MESSAGE_PAYLOAD_STRING = "payload";

/// The directive key in the JSON content of an AVS message
static const std::string DIRECTIVE_KEY = "directive";
/// The namespace in AVS message.
static const std::string NAMESPACE_TEST = "SpeechSynthesizer";
/// The name field in AVS message.
static const std::string NAME_TEST = "Speak";
/// The messageId in AVS message.
static const std::string MESSAGE_ID_TEST = "testMessageId";
/// The dialogRequestId in AVS message.
static const std::string DIALOG_REQUEST_ID_TEST = "dialogRequestIdTest";
/// The payload in AVS message.
static const std::string PAYLOAD_TEST = R"({"url":"cid:testCID","format":"testFormat","token":"testToken"})";

/// A random text to test the output value modification.
static const std::string OUTPUT_DEFAULT_TEXT_STRING = "defaultString";
/// A random integer to test the output value modification.
static const int OUTPUT_DEFAULT_INT_VALUE = 42;
/// Expected string value.
static const std::string EXPECTED_STRING_VALUE = "expectedValue";
/// Expected int value.
static const int EXPECTED_INT_VALUE = 123;
/// Expected uint64_t value.
static const uint64_t EXPECTED_UNSIGNED_INT64_VALUE = UINT64_MAX;
/// Expected int value converted to a string.
static const std::string EXPECTED_INT_VALUE_STRINGIFIED = "123";

/// An invalid AVS directive JSON.``
static const std::string EMPTY_DIRECTIVE = R"({
            "directive": {
                          }
        })";

/// A sample AVS speak directive with all valid JSON keys.
// clang-format off
static const std::string SPEAK_DIRECTIVE = R"({
    "directive": {
        "header": {
            "namespace":")" + NAMESPACE_TEST + R"(",
            "name": ")" + NAME_TEST + R"(",
            "messageId": ")" + MESSAGE_ID_TEST + R"(",
            "dialogRequestId": ")" + DIALOG_REQUEST_ID_TEST + R"("
        },
        "payload": )" + PAYLOAD_TEST + R"(
    }
})";
// clang-format on

/// The JSON key used in value reading tests.
static const std::string VALUE_KEY = "anInt64";

/// A JSON key *not* used in value reading tests.
static const std::string MISSING_KEY = "missingKey";

/// An invalid JSON string for testing.
static const std::string INVALID_JSON = "invalidTestJSON";

// clang-format off
/// A valid JSON string with scalar string value.
static const std::string VALID_JSON_STRING_VALUE = R"({")" + VALUE_KEY + R"(":")" + EXPECTED_STRING_VALUE + R"("})";

/// A valid JSON string with integer value.
static const std::string VALID_JSON_INTEGER_VALUE = R"({")"
    + VALUE_KEY + R"(":)" + EXPECTED_INT_VALUE_STRINGIFIED + R"(})";

// clang-format on

/// A double.
static const double A_DOUBLE = 1.0;

/// A bool used for construction of rapidjson::Value objects.
static const bool A_BOOL = false;

/// A valid string JSON Value.
static const std::string STRING_VALUE = "stringValue";

/// A valid empty string JSON Value.
static const std::string STRING_VALUE_EMPTY_JSON_OBJECT = "{}";

/// Define test fixture for testing AVSMessage.
class JSONUtilTest : public ::testing::Test {};

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = std::string for getting child object as a string.
 */
TEST_F(JSONUtilTest, test_validJsonChildObjectAsString) {
    std::string value;
    ASSERT_TRUE(jsonUtils::retrieveValue(EMPTY_DIRECTIVE, DIRECTIVE_KEY, &value));
    ASSERT_EQ(value, STRING_VALUE_EMPTY_JSON_OBJECT);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = std::string for getting value of a scalar string.
 */
TEST_F(JSONUtilTest, test_validJsonScalarString) {
    std::string value;
    ASSERT_TRUE(jsonUtils::retrieveValue(VALID_JSON_STRING_VALUE, VALUE_KEY, &value));
    ASSERT_EQ(value, EXPECTED_STRING_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 for getting an integer value.
 */
TEST_F(JSONUtilTest, test_validJsonInteger) {
    int64_t value = OUTPUT_DEFAULT_INT_VALUE;
    ASSERT_TRUE(jsonUtils::retrieveValue(VALID_JSON_INTEGER_VALUE, VALUE_KEY, &value));
    ASSERT_EQ(value, EXPECTED_INT_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 and an invalid JSON. Returns false.
 */
TEST_F(JSONUtilTest, test_retrieveValueStringBasedInt64FromInvalidJSON) {
    int64_t value = OUTPUT_DEFAULT_INT_VALUE;
    ASSERT_FALSE(retrieveValue(INVALID_JSON, VALUE_KEY, &value));
    ASSERT_EQ(value, OUTPUT_DEFAULT_INT_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = std::string and an invalid JSON. Returns false.
 */
TEST_F(JSONUtilTest, test_retrieveValueStringBasedStringFromInvalidJSON) {
    std::string value = OUTPUT_DEFAULT_TEXT_STRING;
    ASSERT_FALSE(retrieveValue(INVALID_JSON, VALUE_KEY, &value));
    ASSERT_EQ(value, OUTPUT_DEFAULT_TEXT_STRING);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 and an incorrect key. Returns false.
 */
TEST_F(JSONUtilTest, test_retrieveValueStringBasedWithIncorrectKey) {
    int64_t value = OUTPUT_DEFAULT_INT_VALUE;
    ASSERT_FALSE(retrieveValue(VALID_JSON_INTEGER_VALUE, MISSING_KEY, &value));
    ASSERT_EQ(value, OUTPUT_DEFAULT_INT_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 and a null output param. Returns false.
 */
TEST_F(JSONUtilTest, test_retrieveValueStringBasedWithNull) {
    int64_t* value = nullptr;
    ASSERT_FALSE(retrieveValue(VALID_JSON_INTEGER_VALUE, VALUE_KEY, value));
}

/**
 * Tests retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value)
 * with T = int64 and a value of invalid type. Returns false.
 */
TEST_F(JSONUtilTest, test_retrieveValueDocumentBasedWithInvalidValueType) {
    Document doc;
    doc.Parse(VALID_JSON_STRING_VALUE);
    int64_t value;
    ASSERT_FALSE(retrieveValue(doc, VALUE_KEY, &value));
}

/**
 * Tests retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value)
 * with T = int64 and a null output param. Returns false.
 */
TEST_F(JSONUtilTest, test_retrieveValueDocumentBasedWithNull) {
    Document doc;
    doc.Parse(VALID_JSON_INTEGER_VALUE);
    int64_t* value = nullptr;
    ASSERT_FALSE(retrieveValue(doc, VALUE_KEY, value));
}

/**
 * Tests retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value)
 * with T = int64 and a valid value. Returns true and obtains the correct value.
 */
TEST_F(JSONUtilTest, test_retrieveValueDocumentBasedWithValidInt64) {
    Document doc;
    doc.Parse(VALID_JSON_INTEGER_VALUE);
    int64_t value;
    ASSERT_TRUE(retrieveValue(doc, VALUE_KEY, &value));
    ASSERT_EQ(value, EXPECTED_INT_VALUE);
}

/**
 * Tests findNode with a Null output param. Returns false.
 */
TEST_F(JSONUtilTest, test_findNodeNull) {
    Document doc;
    doc.Parse(SPEAK_DIRECTIVE);
    ASSERT_FALSE(findNode(doc, JSON_MESSAGE_HEADER_STRING, nullptr));
}

/**
 * Tests findNode with a valid key. Returns true with iterator != MemberEnd().
 */
TEST_F(JSONUtilTest, test_findNodeKeyExists) {
    Document doc;
    doc.Parse(SPEAK_DIRECTIVE);
    Value::ConstMemberIterator iterator;
    ASSERT_TRUE(findNode(doc, DIRECTIVE_KEY, &iterator));
    ASSERT_NE(iterator, doc.MemberEnd());
}

/**
 * Tests findNode with a non-existent key. Returns false.
 */
TEST_F(JSONUtilTest, test_findNodeKeyMissing) {
    Document doc;
    doc.Parse(SPEAK_DIRECTIVE);
    Value::ConstMemberIterator iterator;
    ASSERT_FALSE(findNode(doc, MISSING_KEY, &iterator));
}

/**
 * Tests parseJSON with a null output param. Returns false.
 */
TEST_F(JSONUtilTest, test_parseJSONNullOutputParam) {
    ASSERT_FALSE(parseJSON(SPEAK_DIRECTIVE, nullptr));
}

/**
 * Tests parseJSON with a valid json. Returns true.
 */
TEST_F(JSONUtilTest, test_parseJSONValidJSON) {
    Document doc;
    ASSERT_TRUE(parseJSON(SPEAK_DIRECTIVE, &doc));
    ASSERT_FALSE(doc.HasParseError());
}

/**
 * Tests parseJSON with an invalid json. Returns false.
 */
TEST_F(JSONUtilTest, test_parseJSONInvalidJSON) {
    Document doc;
    ASSERT_FALSE(parseJSON(INVALID_JSON, &doc));
    ASSERT_TRUE(doc.HasParseError());
}

/**
 * Tests convertToValue<std::string> with Value of rapidjson::Type::kStringType. Returns
 * true and contains the correct value.
 */
TEST_F(JSONUtilTest, test_convertToStringValueWithString) {
    rapidjson::Value expected;
    expected.SetString(STRING_VALUE.c_str(), STRING_VALUE.length());
    std::string actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetString(), actual);
}

/**
 * Tests convertToValue<std::string> with Value of rapidjson::Type::kObjectType.
 * Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, test_convertToStringValueWithObject) {
    rapidjson::Value emptyObject(kObjectType);
    std::string actual;
    ASSERT_TRUE(convertToValue(emptyObject, &actual));
    ASSERT_EQ(STRING_VALUE_EMPTY_JSON_OBJECT, actual);
}

/**
 * Tests convertToValue<std::string> with and invalid Value of rapidjson::Type::kNullType.
 * Returns false.
 */
TEST_F(JSONUtilTest, test_convertToStringValueWithInvalidValue) {
    rapidjson::Value nullValue(kNullType);
    std::string value;
    ASSERT_FALSE(convertToValue(nullValue, &value));
}

/**
 * Tests convertToValue<std::string> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, test_convertToStringValueWithNullOutputParam) {
    rapidjson::Value node;
    node.SetString(STRING_VALUE.c_str(), STRING_VALUE.length());
    std::string* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<int64_t> with valid int64_6. Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, test_convertToInt64ValueWithInt64) {
    rapidjson::Value expected(EXPECTED_INT_VALUE);
    int64_t actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetInt64(), actual);
}

/**
 * Tests convertToValue<int64_t> with double. Returns false.
 */
TEST_F(JSONUtilTest, test_convertToInt64ValueWithDouble) {
    rapidjson::Value expected(A_DOUBLE);
    int64_t actual;
    ASSERT_FALSE(convertToValue(expected, &actual));
}

/**
 * Tests convertToValue<uint64_t> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, test_convertToUint64ValueWithNullOutputParam) {
    rapidjson::Value node(EXPECTED_UNSIGNED_INT64_VALUE);
    uint64_t* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<uint64_t> with valid uint64_t. Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, test_convertToUint64ValueWithUint64) {
    rapidjson::Value expected(EXPECTED_UNSIGNED_INT64_VALUE);
    uint64_t actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetUint64(), actual);
}

/**
 * Tests convertToValue<uint64_t> with double. Returns false.
 */
TEST_F(JSONUtilTest, test_convertToUint64ValueWithDouble) {
    rapidjson::Value expected(A_DOUBLE);
    uint64_t actual;
    ASSERT_FALSE(convertToValue(expected, &actual));
}

/**
 * Tests convertToValue<int64_t> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, test_convertToInt64ValueWithNullOutputParam) {
    rapidjson::Value node(EXPECTED_INT_VALUE);
    int64_t* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<bool> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, test_convertToBoolValueWithNullOutputParam) {
    rapidjson::Value node(A_BOOL);
    bool* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<bool> with a nonbool. Returns false.
 */
TEST_F(JSONUtilTest, test_convertToBoolValueWithNonBool) {
    rapidjson::Value expected(A_DOUBLE);
    bool actual;
    ASSERT_FALSE(convertToValue(expected, &actual));
}

/**
 * Tests convertToValue<bool> with valid bool. Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, test_convertToBoolValueWithBool) {
    rapidjson::Value expected(A_BOOL);
    bool actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetBool(), actual);
}

/**
 * Test convert to string set with valid entry.
 */
TEST_F(JSONUtilTest, test_retrieveStringArraySucceed) {
    auto entries = retrieveStringArray<std::set<std::string>>(R"(["ONE","TWO"])");
    ASSERT_EQ(entries.size(), 2u);
    ASSERT_NE(entries.find("ONE"), entries.end());
    ASSERT_NE(entries.find("TWO"), entries.end());
}

/**
 * Test convert to string set with empty entry.
 */
TEST_F(JSONUtilTest, test_retrieveStringArrayEmpty) {
    auto entries = retrieveStringArray<std::set<std::string>>(R"([])");
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string set with non-array.
 */
TEST_F(JSONUtilTest, test_retrieveStringArrayNonArray) {
    auto entries = retrieveStringArray<std::set<std::string>>(R"({"key":"value"})");
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string set with invalid json.
 */
TEST_F(JSONUtilTest, test_retrieveStringArrayNonJson) {
    auto entries = retrieveStringArray<std::set<std::string>>("Not json");
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string set with an array with elements that are not string.
 */
TEST_F(JSONUtilTest, test_retrieveStringArrayWithNonStringArrayFails) {
    auto entries = retrieveStringArray<std::set<std::string>>(R"([true,1])");
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string set with empty set.
 */
TEST_F(JSONUtilTest, test_convertToJsonStringFromEmptySet) {
    auto jsonString = convertToJsonString<std::set<std::string>>(std::set<std::string>());
    ASSERT_EQ(jsonString, std::string("[]"));
}

/**
 * Test convert to string set with non-empty set.
 */
TEST_F(JSONUtilTest, test_convertToJsonStringFromStringSet) {
    auto jsonString = convertToJsonString<std::set<std::string>>(std::set<std::string>({"ONE", "TWO"}));
    ASSERT_EQ(jsonString, std::string(R"(["ONE","TWO"])"));
}

/**
 * Test convert to json list with non-empty string vector.
 */
TEST_F(JSONUtilTest, test_convertToJsonFromStringVector) {
    auto jsonString = convertToJsonString<std::vector<std::string>>(std::vector<std::string>({"ONE", "TWO", "THREE"}));
    ASSERT_EQ(jsonString, std::string(R"(["ONE","TWO","THREE"])"));
}

/**
 * Test convert to string set with valid entry.
 */
TEST_F(JSONUtilTest, test_retrieveElementsWithKeySucceed) {
    std::string key{"key"};
    auto entries = retrieveStringArray<std::set<std::string>>(R"({"key":["ONE","TWO"]})", key);
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_NE(entries.find("ONE"), entries.end());
    EXPECT_NE(entries.find("TWO"), entries.end());
}

/**
 * Test convert to string set with mix of string and non-strings entries.
 */
TEST_F(JSONUtilTest, test_retrieveMixStringNonStringElementsWithKeySucceed) {
    std::string key{"key"};
    auto entries = retrieveStringArray<std::set<std::string>>(R"({"key":["ONE",2]})", key);
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_NE(entries.find("ONE"), entries.end());
}

/**
 * Test convert to string set with empty entry.
 */
TEST_F(JSONUtilTest, test_retrieveElementsWithKeyEmptyArray) {
    std::string key{"key"};
    auto entries = retrieveStringArray<std::set<std::string>>(R"({"key":[]})", key);
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string set with non-array.
 */
TEST_F(JSONUtilTest, test_retrieveElementsWithKeyNonArray) {
    std::string key{"key"};
    auto entries = retrieveStringArray<std::set<std::string>>(R"({"key":"value"})", key);
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string set with non-array.
 */
TEST_F(JSONUtilTest, test_retrieveElementsWithKeyMissing) {
    std::string key{"key"};
    auto entries = retrieveStringArray<std::set<std::string>>(R"({"anotherKey":"value"})", key);
    ASSERT_TRUE(entries.empty());
}

/**
 * Test convert to string array with valid entry.
 */
TEST_F(JSONUtilTest, test_retrieveArrayOfElementsWithKeySucceed) {
    std::string key{"key"};
    auto entries = retrieveStringArray<std::vector<std::string>>(R"({"key":["ONE","TWO","THREE"]})", key);
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0], std::string("ONE"));
    EXPECT_EQ(entries[1], std::string("TWO"));
    EXPECT_EQ(entries[2], std::string("THREE"));
}

/**
 * Test convert to map from string array with only key entry.
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArrayOnlyKeyEntry) {
    std::string key{"manifest"};
    std::map<std::string, std::string> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[{"name": "one"}]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    retrieveStringMapFromArray(headerIterator->value, key, elements);
    ASSERT_EQ(elements.size(), 0u);
}

/**
 * Test convert to map from string array with invalid entry.
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArrayInvalidEntry) {
    std::string key{"manifest"};
    std::map<std::string, std::string> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[{"name": "one", "value":"1"}, {"name" : "two", "value":"2"}, {"name":"three", "value":3}]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    retrieveStringMapFromArray(headerIterator->value, key, elements);
    ASSERT_EQ(elements.size(), 0u);
}

/**
 * Test convert to map from string array with valid entry.
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArray) {
    std::string key{"manifest"};
    std::map<std::string, std::string> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[{"name": "one", "value":"1"}, {"name" : "two", "value":"2"}, {"name":"three", "value":"3"}]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    retrieveStringMapFromArray(headerIterator->value, key, elements);
    ASSERT_EQ(elements.size(), 3u);
    EXPECT_EQ(elements["one"], std::string("1"));
    EXPECT_EQ(elements["two"], std::string("2"));
    EXPECT_EQ(elements["three"], std::string("3"));
}

/**
 * Test convert to map from string array with multiple valid entries.
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArrayMultiple) {
    std::string json = R"({
      "httpHeaders" : {
           "manifest": [
                {"name": "one", "value":"1"},
                {"name" : "two", "value":"2"},
                {"name":"three", "value":"3"}
           ],
           "all": [
                {"name":"tick", "value":"tock"}
           ]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    {
        std::string key{"manifest"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 3u);
        EXPECT_EQ(elements["one"], std::string("1"));
        EXPECT_EQ(elements["two"], std::string("2"));
        EXPECT_EQ(elements["three"], std::string("3"));
    }

    {
        std::string key{"all"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 1u);
        EXPECT_EQ(elements["tick"], std::string("tock"));
    }
}

/**
 * Test convert to map from string array with multiple valid entries.
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArrayAll) {
    std::string json = R"({
    "httpHeaders": {
        "key": [
                 {"name": "one", "value":"1"},
                 {"name" : "two", "value":"2"},
                 {"name":"three", "value":"3"}
               ],
        "manifest": [
                 {"name":"tick", "value":"tock"}
               ],
        "audioSegment": [
                 {"name":"hdr", "value":"password"}
               ],
        "all": [
                 {"name":"login", "value":"authToken"},
                 {"name":"bill", "value":"authTokenIsStillValid"}
               ]
        }})";

    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    {
        std::string key{"key"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 3u);
        EXPECT_EQ(elements["one"], std::string("1"));
        EXPECT_EQ(elements["two"], std::string("2"));
        EXPECT_EQ(elements["three"], std::string("3"));
    }

    {
        std::string key{"manifest"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 1u);
        EXPECT_EQ(elements["tick"], std::string("tock"));
    }

    {
        std::string key{"audioSegment"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 1u);
        EXPECT_EQ(elements["hdr"], std::string("password"));
    }

    {
        std::string key{"all"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 2u);
        EXPECT_EQ(elements["login"], std::string("authToken"));
        EXPECT_EQ(elements["bill"], std::string("authTokenIsStillValid"));
    }
}

/**
 * Test convert to map from string array with valid and empty entries.
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArrayNoneFew) {
    std::string json = R"({
      "httpHeaders" : {
           "manifest": [
           ],
           "all": [
                {"name":"tick", "value":"tock"},
                {"name":"tick1", "value":"rock"},
                {"name":"tick2", "value":"kick"},
                {"name":"tick3", "value":"thisiskey"},
                {"name":"tick4", "value":"thisismore"},
                {"name":"tick5", "value":"thisisless"},
                {"name":"tick6", "value":"thisisnot"},
                {"name":"tick7", "value":"thatiswhere"},
                {"name":"tick8", "value":"thoseisthis"}
           ]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    {
        std::string key{"manifest"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 0u);
    }

    {
        std::string key{"all"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 9u);
        EXPECT_EQ(elements["tick"], std::string("tock"));
        EXPECT_EQ(elements["tick1"], std::string("rock"));
        EXPECT_EQ(elements["tick2"], std::string("kick"));
        EXPECT_EQ(elements["tick3"], std::string("thisiskey"));
        EXPECT_EQ(elements["tick4"], std::string("thisismore"));
        EXPECT_EQ(elements["tick5"], std::string("thisisless"));
        EXPECT_EQ(elements["tick6"], std::string("thisisnot"));
        EXPECT_EQ(elements["tick7"], std::string("thatiswhere"));
        EXPECT_EQ(elements["tick8"], std::string("thoseisthis"));
    }
}

/**
 * Test check map is empty for empty or missing header entries in JSON
 */
TEST_F(JSONUtilTest, test_retrieveStringMapFromArrayNoneAll) {
    std::string json = R"({
      "httpHeaders" : {
           "manifest": [ ],
           "all": [ ]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    {
        std::string key{"manifest"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 0u);
    }

    {
        std::string key{"all"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 0u);
    }

    {
        std::string key{"audioSegment"};
        std::map<std::string, std::string> elements;
        retrieveStringMapFromArray(headerIterator->value, key, elements);
        ASSERT_EQ(elements.size(), 0u);
    }
}

/**
 * Test convert to vector of map from string array with valid entry.
 */
TEST_F(JSONUtilTest, test_retrieveArrayStringMapFromArray) {
    std::string key{"manifest"};
    std::vector<std::map<std::string, std::string>> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[
            {"name": "one", "value1":"1", "value2":"2", "value3":"3"},
            {"name": "two", "value1":"1", "value2":"2", "value3":"3"},
            {"name": "three", "value1":"1", "value2":"2", "value3":"3"}
        ]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    EXPECT_TRUE(retrieveArrayOfStringMapFromArray(headerIterator->value, key, elements));
    ASSERT_EQ(elements.size(), 3u);
    for (auto element : elements) {
        EXPECT_TRUE(element["name"] == "one" || element["name"] == "two" || element["name"] == "three");
        EXPECT_EQ(element["value1"], std::string("1"));
        EXPECT_EQ(element["value2"], std::string("2"));
        EXPECT_EQ(element["value3"], std::string("3"));
    }
}

/**
 * Test convert to vector of map from string array with invalid entry.
 */
TEST_F(JSONUtilTest, test_retrieveArrayStringMapFromArrayInvalidEntry) {
    std::string key{"manifest"};
    std::vector<std::map<std::string, std::string>> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[
            {"name": "one", "value1":"1", "value2":"2", "value3":"3"},
            {"name": "two", "value1":"1", "value2":"2", "value3":"3"},
            {"name": "three", "value1":"1", "value2":"2", "value3":3}
        ]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    EXPECT_FALSE(retrieveArrayOfStringMapFromArray(headerIterator->value, key, elements));
    ASSERT_EQ(elements.size(), 0u);
}

/**
 * Test convert to vector of map from string array with missing key.
 */
TEST_F(JSONUtilTest, test_retrieveArrayStringMapFromArrayInvalidKey) {
    std::string key{"notManifest"};
    std::vector<std::map<std::string, std::string>> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[
            {"name": "one", "value1":"1", "value2":"2", "value3":"3"},
            {"name": "two", "value1":"1", "value2":"2", "value3":"3"},
            {"name": "three", "value1":"1", "value2":"2", "value3":"3"}
        ]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    EXPECT_FALSE(retrieveArrayOfStringMapFromArray(headerIterator->value, key, elements));
    ASSERT_EQ(elements.size(), 0u);
}

/**
 * Test convert to vector of map from string array with empty Array.
 */
TEST_F(JSONUtilTest, test_retrieveArrayStringMapFromArrayEmptyArray) {
    std::string key{"manifest"};
    std::vector<std::map<std::string, std::string>> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[]} })";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    EXPECT_TRUE(retrieveArrayOfStringMapFromArray(headerIterator->value, key, elements));
    ASSERT_EQ(elements.size(), 0u);
}

/**
 * Test convert to vector of map from string array with empty Array.
 */
TEST_F(JSONUtilTest, test_retrieveArrayStringMapFromArrayEmptyMap) {
    std::string key{"manifest"};
    std::vector<std::map<std::string, std::string>> elements;
    std::string json =
        R"({"httpHeaders" : {"manifest":[ {}]}})";
    rapidjson::Document document;
    ASSERT_TRUE(jsonUtils::parseJSON(json, &document));
    rapidjson::Value::ConstMemberIterator headerIterator = document.FindMember("httpHeaders");
    ASSERT_TRUE(headerIterator != document.MemberEnd());

    EXPECT_TRUE(retrieveArrayOfStringMapFromArray(headerIterator->value, key, elements));
    ASSERT_EQ(elements.size(), 1u);

    auto& emptyElement = elements.front();
    ASSERT_EQ(emptyElement.size(), 0u);
}

}  // namespace test
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
