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

#include <memory>
#include <map>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVSDirective.h"
#include "AVSCommon/JSON/JSONUtils.h"

namespace alexaClientSDK {
namespace avsCommon {

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


} // namespace avsCommon
} // namespace alexaClientSDK