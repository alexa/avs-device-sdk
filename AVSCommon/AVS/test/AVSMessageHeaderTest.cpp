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

#include <gtest/gtest.h>

#include "AVSCommon/AVS/AVSDirective.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace utils;

/**
 * Validate that the given string is a valid json.
 *
 * @param json The stringified json.
 * @return Whether the parameter is a valid json.
 */
static bool isValidJson(const std::string& json) {
    rapidjson::Document document;
    return json::jsonUtils::parseJSON(json, &document);
}

TEST(AVSMessageHeaderTest, test_toJsonWithoutOptionalFields) {
    AVSMessageHeader header{"Namespace", "Name", "Id"};
    auto json = header.toJson();

    // Mandatory fields should be included.
    EXPECT_TRUE(isValidJson(json));
    EXPECT_NE(json.find(R"("namespace":")" + header.getNamespace()), std::string::npos);
    EXPECT_NE(json.find(R"("name":")" + header.getName()), std::string::npos);
    EXPECT_NE(json.find(R"("messageId":")" + header.getMessageId()), std::string::npos);

    // Optional fields that are not present should be omitted.
    EXPECT_EQ(json.find(R"("dialogRequestId")"), std::string::npos);
    EXPECT_EQ(json.find(R"("correlationToken")"), std::string::npos);
    EXPECT_EQ(json.find(R"("eventCorrelationToken")"), std::string::npos);
    EXPECT_EQ(json.find(R"("payloadVersion")"), std::string::npos);
    EXPECT_EQ(json.find(R"("instance")"), std::string::npos);
}

TEST(AVSMessageHeaderTest, test_toJsonWithOptionalFields) {
    AVSMessageHeader header{"Namespace",
                            "Name",
                            "Id",
                            "DialogId",
                            "CorrelationToken",
                            "EventCorrelationToken",
                            "PayloadVersion",
                            "Instance"};

    auto json = header.toJson();

    // All fields should be included.
    EXPECT_TRUE(isValidJson(json));
    EXPECT_NE(json.find(R"("namespace":")" + header.getNamespace()), std::string::npos);
    EXPECT_NE(json.find(R"("name":")" + header.getName()), std::string::npos);
    EXPECT_NE(json.find(R"("messageId":")" + header.getMessageId()), std::string::npos);
    EXPECT_NE(json.find(R"("dialogRequestId":")" + header.getDialogRequestId()), std::string::npos);
    EXPECT_NE(json.find(R"("correlationToken":")" + header.getCorrelationToken()), std::string::npos);
    EXPECT_NE(json.find(R"("eventCorrelationToken":")" + header.getEventCorrelationToken()), std::string::npos);
    EXPECT_NE(json.find(R"("payloadVersion":")" + header.getPayloadVersion()), std::string::npos);
    EXPECT_NE(json.find(R"("instance":")" + header.getInstance()), std::string::npos);
}

TEST(AVSMessageHeaderTest, test_eventHeaderToJsonGenerateEventCorrelationToken) {
    auto header = AVSMessageHeader::createAVSEventHeader("Namespace", "Name", "Id");
    auto json = header.toJson();

    // Ensure that the header includes an event correlation token.
    EXPECT_TRUE(isValidJson(json));
    EXPECT_FALSE(header.getEventCorrelationToken().empty());
    EXPECT_NE(json.find(R"("eventCorrelationToken")"), std::string::npos);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
