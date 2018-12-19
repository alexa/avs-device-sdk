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
TEST_F(JSONUtilTest, validJsonChildObjectAsString) {
    std::string value;
    ASSERT_TRUE(jsonUtils::retrieveValue(EMPTY_DIRECTIVE, DIRECTIVE_KEY, &value));
    ASSERT_EQ(value, STRING_VALUE_EMPTY_JSON_OBJECT);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = std::string for getting value of a scalar string.
 */
TEST_F(JSONUtilTest, validJsonScalarString) {
    std::string value;
    ASSERT_TRUE(jsonUtils::retrieveValue(VALID_JSON_STRING_VALUE, VALUE_KEY, &value));
    ASSERT_EQ(value, EXPECTED_STRING_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 for getting an integer value.
 */
TEST_F(JSONUtilTest, validJsonInteger) {
    int64_t value = OUTPUT_DEFAULT_INT_VALUE;
    ASSERT_TRUE(jsonUtils::retrieveValue(VALID_JSON_INTEGER_VALUE, VALUE_KEY, &value));
    ASSERT_EQ(value, EXPECTED_INT_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 and an invalid JSON. Returns false.
 */
TEST_F(JSONUtilTest, retrieveValueStringBasedInt64FromInvalidJSON) {
    int64_t value = OUTPUT_DEFAULT_INT_VALUE;
    ASSERT_FALSE(retrieveValue(INVALID_JSON, VALUE_KEY, &value));
    ASSERT_EQ(value, OUTPUT_DEFAULT_INT_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = std::string and an invalid JSON. Returns false.
 */
TEST_F(JSONUtilTest, retrieveValueStringBasedStringFromInvalidJSON) {
    std::string value = OUTPUT_DEFAULT_TEXT_STRING;
    ASSERT_FALSE(retrieveValue(INVALID_JSON, VALUE_KEY, &value));
    ASSERT_EQ(value, OUTPUT_DEFAULT_TEXT_STRING);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 and an incorrect key. Returns false.
 */
TEST_F(JSONUtilTest, retrieveValueStringBasedWithIncorrectKey) {
    int64_t value = OUTPUT_DEFAULT_INT_VALUE;
    ASSERT_FALSE(retrieveValue(VALID_JSON_INTEGER_VALUE, MISSING_KEY, &value));
    ASSERT_EQ(value, OUTPUT_DEFAULT_INT_VALUE);
}

/**
 * Tests retrieveValue(const std::string jsonString, const std::string& key, T* value)
 * with T = int64 and a null output param. Returns false.
 */
TEST_F(JSONUtilTest, retrieveValueStringBasedWithNull) {
    int64_t* value = nullptr;
    ASSERT_FALSE(retrieveValue(VALID_JSON_INTEGER_VALUE, VALUE_KEY, value));
}

/**
 * Tests retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value)
 * with T = int64 and a value of invalid type. Returns false.
 */
TEST_F(JSONUtilTest, retrieveValueDocumentBasedWithInvalidValueType) {
    Document doc;
    doc.Parse(VALID_JSON_STRING_VALUE);
    int64_t value;
    ASSERT_FALSE(retrieveValue(doc, VALUE_KEY, &value));
}

/**
 * Tests retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value)
 * with T = int64 and a null output param. Returns false.
 */
TEST_F(JSONUtilTest, retrieveValueDocumentBasedWithNull) {
    Document doc;
    doc.Parse(VALID_JSON_INTEGER_VALUE);
    int64_t* value = nullptr;
    ASSERT_FALSE(retrieveValue(doc, VALUE_KEY, value));
}

/**
 * Tests retrieveValue(const rapidjson::Value& jsonNode, const std::string& key, T* value)
 * with T = int64 and a valid value. Returns true and obtains the correct value.
 */
TEST_F(JSONUtilTest, retrieveValueDocumentBasedWithValidInt64) {
    Document doc;
    doc.Parse(VALID_JSON_INTEGER_VALUE);
    int64_t value;
    ASSERT_TRUE(retrieveValue(doc, VALUE_KEY, &value));
    ASSERT_EQ(value, EXPECTED_INT_VALUE);
}

/**
 * Tests findNode with a Null output param. Returns false.
 */
TEST_F(JSONUtilTest, findNodeNull) {
    Document doc;
    doc.Parse(SPEAK_DIRECTIVE);
    ASSERT_FALSE(findNode(doc, JSON_MESSAGE_HEADER_STRING, nullptr));
}

/**
 * Tests findNode with a valid key. Returns true with iterator != MemberEnd().
 */
TEST_F(JSONUtilTest, findNodeKeyExists) {
    Document doc;
    doc.Parse(SPEAK_DIRECTIVE);
    Value::ConstMemberIterator iterator;
    ASSERT_TRUE(findNode(doc, DIRECTIVE_KEY, &iterator));
    ASSERT_NE(iterator, doc.MemberEnd());
}

/**
 * Tests findNode with a non-existent key. Returns false.
 */
TEST_F(JSONUtilTest, findNodeKeyMissing) {
    Document doc;
    doc.Parse(SPEAK_DIRECTIVE);
    Value::ConstMemberIterator iterator;
    ASSERT_FALSE(findNode(doc, MISSING_KEY, &iterator));
}

/**
 * Tests parseJSON with a null output param. Returns false.
 */
TEST_F(JSONUtilTest, parseJSONNullOutputParam) {
    ASSERT_FALSE(parseJSON(SPEAK_DIRECTIVE, nullptr));
}

/**
 * Tests parseJSON with a valid json. Returns true.
 */
TEST_F(JSONUtilTest, parseJSONValidJSON) {
    Document doc;
    ASSERT_TRUE(parseJSON(SPEAK_DIRECTIVE, &doc));
    ASSERT_FALSE(doc.HasParseError());
}

/**
 * Tests parseJSON with an invalid json. Returns false.
 */
TEST_F(JSONUtilTest, parseJSONInvalidJSON) {
    Document doc;
    ASSERT_FALSE(parseJSON(INVALID_JSON, &doc));
    ASSERT_TRUE(doc.HasParseError());
}

/**
 * Tests convertToValue<std::string> with Value of rapidjson::Type::kStringType. Returns
 * true and contains the correct value.
 */
TEST_F(JSONUtilTest, convertToStringValueWithString) {
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
TEST_F(JSONUtilTest, convertToStringValueWithObject) {
    rapidjson::Value emptyObject(kObjectType);
    std::string actual;
    ASSERT_TRUE(convertToValue(emptyObject, &actual));
    ASSERT_EQ(STRING_VALUE_EMPTY_JSON_OBJECT, actual);
}

/**
 * Tests convertToValue<std::string> with and invalid Value of rapidjson::Type::kNullType.
 * Returns false.
 */
TEST_F(JSONUtilTest, convertToStringValueWithInvalidValue) {
    rapidjson::Value nullValue(kNullType);
    std::string value;
    ASSERT_FALSE(convertToValue(nullValue, &value));
}

/**
 * Tests convertToValue<std::string> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, convertToStringValueWithNullOutputParam) {
    rapidjson::Value node;
    node.SetString(STRING_VALUE.c_str(), STRING_VALUE.length());
    std::string* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<int64_t> with valid int64_6. Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, convertToInt64ValueWithInt64) {
    rapidjson::Value expected(EXPECTED_INT_VALUE);
    int64_t actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetInt64(), actual);
}

/**
 * Tests convertToValue<int64_t> with double. Returns false.
 */
TEST_F(JSONUtilTest, convertToInt64ValueWithDouble) {
    rapidjson::Value expected(A_DOUBLE);
    int64_t actual;
    ASSERT_FALSE(convertToValue(expected, &actual));
}

/**
 * Tests convertToValue<uint64_t> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, convertToUint64ValueWithNullOutputParam) {
    rapidjson::Value node(EXPECTED_UNSIGNED_INT64_VALUE);
    uint64_t* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<uint64_t> with valid uint64_t. Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, convertToUint64ValueWithUint64) {
    rapidjson::Value expected(EXPECTED_UNSIGNED_INT64_VALUE);
    uint64_t actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetUint64(), actual);
}

/**
 * Tests convertToValue<uint64_t> with double. Returns false.
 */
TEST_F(JSONUtilTest, convertToUint64ValueWithDouble) {
    rapidjson::Value expected(A_DOUBLE);
    uint64_t actual;
    ASSERT_FALSE(convertToValue(expected, &actual));
}

/**
 * Tests convertToValue<int64_t> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, convertToInt64ValueWithNullOutputParam) {
    rapidjson::Value node(EXPECTED_INT_VALUE);
    int64_t* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<bool> with null output param.
 * Returns false.
 */
TEST_F(JSONUtilTest, convertToBoolValueWithNullOutputParam) {
    rapidjson::Value node(A_BOOL);
    bool* value = nullptr;
    ASSERT_FALSE(convertToValue(node, value));
}

/**
 * Tests convertToValue<bool> with a nonbool. Returns false.
 */
TEST_F(JSONUtilTest, convertToBoolValueWithNonBool) {
    rapidjson::Value expected(A_DOUBLE);
    bool actual;
    ASSERT_FALSE(convertToValue(expected, &actual));
}

/**
 * Tests convertToValue<bool> with valid bool. Returns true and contains the correct value.
 */
TEST_F(JSONUtilTest, convertToBoolValueWithBool) {
    rapidjson::Value expected(A_BOOL);
    bool actual;
    ASSERT_TRUE(convertToValue(expected, &actual));
    ASSERT_EQ(expected.GetBool(), actual);
}

}  // namespace test
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
