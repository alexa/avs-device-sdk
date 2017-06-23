/*
 * AVSMessageTest.cpp
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

/// @file AVSMessageTest.cpp

#include <memory>
#include <map>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/AVSDirective.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {
namespace test {

using namespace ::testing;
using namespace alexaClientSDK::avsCommon;

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
static const std::string DIRECTIVE_TEST = "directive";
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

/// An invalid JSON string for testing.
static const std::string INVALID_JSON = "invalidTestJSON";
/// An invalid AVS directive JSON.
static const std::string INVALID_DIRECTIVE = R"({
            "directive": {
                          }
        })";

/// A sample AVS speak directive with all valid JSON keys.
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

/// The JSON key used in @c int64Json().
static const std::string INT64_KEY = "anInt64";

/// A JSON key *not* used in @c int64Json().
static const std::string INT64_MISSING_KEY = "missingKey";

// A JSON value which does not convert to an int64.
static const std::string NOT_AN_INT64 = R"("not a number")";

/// A function to generate JSON containing the key @c INT64_KEY with the provided @c value.
static const std::string int64Json(const std::string& value) {
  return R"({")" + INT64_KEY + R"(": )" + value + R"(})";
}

/// Define test fixture for testing AVSMessage.
class JSONUtilTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
    }
};

/**
 * Test with invalid JSON format, it should not crash the AVSMessage.
 */
TEST_F(JSONUtilTest, invalidJson) {
    std::string value;
    ASSERT_FALSE(jsonUtils::lookupStringValue(INVALID_JSON, DIRECTIVE_TEST, &value));
    ASSERT_TRUE(value.empty());
}

/**
 * Test with invalid directive, it should not crash the AVSMessage and return the empty.
 */
TEST_F(JSONUtilTest, invalidDirective) {
    std::string value;
    ASSERT_TRUE(jsonUtils::lookupStringValue(INVALID_DIRECTIVE, DIRECTIVE_TEST, &value));
    ASSERT_EQ(value, "{}");

    ASSERT_FALSE(jsonUtils::lookupStringValue(value, NAMESPACE_TEST, &value));
    ASSERT_EQ(value, "{}");
}

/**
 * Test extracting header with valid AVS directive.
 */
TEST_F(JSONUtilTest, extractHeaderFromValidDirective) {
    std::string jsonContent;
    std::string value;

    ASSERT_TRUE(jsonUtils::lookupStringValue(SPEAK_DIRECTIVE, DIRECTIVE_TEST, &jsonContent));

    ASSERT_TRUE(jsonUtils::lookupStringValue(jsonContent, JSON_MESSAGE_HEADER_STRING, &jsonContent));

    jsonUtils::lookupStringValue(jsonContent, JSON_MESSAGE_NAMESPACE_STRING, &value);
    EXPECT_EQ(value, NAMESPACE_TEST);

    jsonUtils::lookupStringValue(jsonContent, JSON_MESSAGE_NAME_STRING, &value);
    EXPECT_EQ(value, NAME_TEST);

    jsonUtils::lookupStringValue(jsonContent, JSON_MESSAGE_ID_STRING, &value);
    EXPECT_EQ(value, MESSAGE_ID_TEST);

    jsonUtils::lookupStringValue(jsonContent, JSON_MESSAGE_DIALOG_REQUEST_ID_STRING, &value);
    EXPECT_EQ(value, DIALOG_REQUEST_ID_TEST);
}

/**
 * Test extracting payload with valid AVS directive.
 */
TEST_F(JSONUtilTest, extractPayloadFromValidDirective) {
    std::string jsonContent;
    std::string value;
    std::string payload;

    ASSERT_TRUE(jsonUtils::lookupStringValue(SPEAK_DIRECTIVE, DIRECTIVE_TEST, &jsonContent));

    ASSERT_TRUE(jsonUtils::lookupStringValue(jsonContent, JSON_MESSAGE_PAYLOAD_STRING, &payload));

    ASSERT_EQ(payload, PAYLOAD_TEST);
}


/**
 * Test failure to extract an int64.
 */
TEST_F(JSONUtilTest, extractValidInt64Failures) {
    int64_t expected = 0;
    int64_t value;

    // Verify lookup fails with a null value pointer.
    EXPECT_FALSE(jsonUtils::lookupInt64Value(int64Json(std::to_string(expected)), INT64_KEY, nullptr));

    // Verify lookup fails with invalid json.
    EXPECT_FALSE(jsonUtils::lookupInt64Value(INVALID_JSON, INT64_KEY, &value));

    // Verify lookup fails with incorrect key.
    EXPECT_FALSE(jsonUtils::lookupInt64Value(int64Json(std::to_string(expected)), INT64_MISSING_KEY, &value));

    // Verify lookup fails with non-numeric value.
    EXPECT_FALSE(jsonUtils::lookupInt64Value(int64Json(NOT_AN_INT64), INT64_KEY, &value));
}

/**
 * Test extracting min int64.
 */
TEST_F(JSONUtilTest, extractMinInt64) {
    int64_t expected = std::numeric_limits<int64_t>::min();
    int64_t value;
    ASSERT_TRUE(jsonUtils::lookupInt64Value(int64Json(std::to_string(expected)), INT64_KEY, &value));
    ASSERT_EQ(value, expected);
}

/**
 * Test extracting max int64.
 */
TEST_F(JSONUtilTest, extractMaxInt64) {
    int64_t expected = std::numeric_limits<int64_t>::max();
    int64_t value;
    ASSERT_TRUE(jsonUtils::lookupInt64Value(int64Json(std::to_string(expected)), INT64_KEY, &value));
    ASSERT_EQ(value, expected);
}

} // namespace test
} // namespace json
} // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK
