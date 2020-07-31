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

#include "AVSCommon/AVS/EventBuilder.h"
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

TEST(EventBuilderTest, test_buildEventWithoutOptionalFields) {
    auto header = AVSMessageHeader::createAVSEventHeader("Namespace", "Name", "Id");
    auto event = buildJsonEventString(header);

    // Mandatory fields should be included. Payload is an empty object.
    EXPECT_TRUE(isValidJson(event));
    EXPECT_NE(event.find(R"("header":)" + header.toJson()), std::string::npos);
    EXPECT_NE(event.find(R"("payload":{})"), std::string::npos);

    // No endpoint field expected.
    EXPECT_EQ(event.find(R"("endpoint")"), std::string::npos);
}

TEST(EventBuilderTest, test_buildEventWithEndpoint) {
    auto header = AVSMessageHeader::createAVSEventHeader("Namespace", "Name", "Id");
    Optional<AVSMessageEndpoint> endpoint{AVSMessageEndpoint("EndpointId")};
    std::string payload{R"({"key":"value"})"};
    auto event = buildJsonEventString(header, endpoint, payload);

    // All fields should be included. Payload is an empty object.
    EXPECT_TRUE(isValidJson(event));
    EXPECT_NE(event.find(R"("header":)" + header.toJson()), std::string::npos);
    EXPECT_NE(event.find(R"("payload":)" + payload), std::string::npos);
    EXPECT_NE(event.find(R"("endpoint":{"endpointId":"EndpointId")"), std::string::npos);
}

TEST(EventBuilderTest, test_buildEventWithContext) {
    auto header = AVSMessageHeader::createAVSEventHeader("Namespace", "Name", "Id");
    AVSContext context;
    context.addState(
        CapabilityTag("CapabilityNamespace", "CapabilityName", "CapabilityEndpoint"), CapabilityState("true"));
    std::string payload{"{}"};
    auto event = buildJsonEventString(header, Optional<AVSMessageEndpoint>(), payload, context);

    // Message should include context.
    EXPECT_NE(event.find(R"("context":)" + context.toJson()), std::string::npos);
}

/**
 * Test used to check that the event should have the following hierarchy.
 * {
 *    "event": {
 *        "header": { ... }
 *        "payload": { ... }
 *        "endpoint": { ... }
 *    },
 *    "context": {
 *    }
 * }
 */
TEST(EventBuilderTest, test_buildFullEventShouldHaveCorrectHierarchy) {
    auto header = AVSMessageHeader::createAVSEventHeader("Namespace", "Name", "Id");
    Optional<AVSMessageEndpoint> endpoint{AVSMessageEndpoint("EndpointId")};
    std::string payload{R"({"key":"value"})"};
    AVSContext context;
    context.addState(CapabilityTag("CapabilityNamespace", "CapabilityName", "EndpointId"), CapabilityState("true"));
    auto json = buildJsonEventString(header, endpoint, payload, context);

    // Check event hierarchy
    rapidjson::Document document;
    ASSERT_TRUE(json::jsonUtils::parseJSON(json, &document));

    // Context and event should be in the first level.
    rapidjson::Value::ConstMemberIterator contextIt;
    EXPECT_TRUE(json::jsonUtils::findNode(document, "context", &contextIt));

    rapidjson::Value::ConstMemberIterator eventIt, headerIt, payloadIt, endpointIt;
    ASSERT_TRUE(json::jsonUtils::findNode(document, "event", &eventIt));
    EXPECT_TRUE(json::jsonUtils::findNode(eventIt->value, "header", &headerIt));
    EXPECT_TRUE(json::jsonUtils::findNode(eventIt->value, "payload", &payloadIt));
    EXPECT_TRUE(json::jsonUtils::findNode(eventIt->value, "endpoint", &endpointIt));
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
