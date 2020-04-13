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
#include <gmock/gmock.h>

#include "AVSCommon/AVS/AVSDirective.h"
#include "AVSCommon/Utils/Optional.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace utils;

TEST(AVSDirectiveTest, test_parseWithoutOptionalAttributes) {
    // clang-format off
    std::string directiveJson = R"({
    "directive": {
        "header": {
            "namespace": "Namespace",
            "name": "Name",
            "messageId": "Id"
        },
        "payload": {
            "key":"value"
        }
    }})";
    // clang-format on
    auto parseResult = AVSDirective::create(directiveJson, nullptr, "");
    EXPECT_EQ(parseResult.second, AVSDirective::ParseStatus::SUCCESS);
    ASSERT_THAT(parseResult.first, NotNull());

    auto& directive = *parseResult.first;
    EXPECT_FALSE(directive.getEndpoint().hasValue());
    EXPECT_EQ(directive.getInstance(), "");
    EXPECT_EQ(directive.getCorrelationToken(), "");
    EXPECT_EQ(directive.getEventCorrelationToken(), "");
    EXPECT_EQ(directive.getPayloadVersion(), "");
}

TEST(AVSDirectiveTest, test_parseWithEndpointAndInstance) {
    // clang-format off
    std::string directiveJson = R"({
    "directive": {
        "header": {
            "namespace": "Namespace",
            "name": "Name",
            "instance": "Instance",
            "messageId": "Id"
        },
        "endpoint": {
            "endpointId": "EndpointId"
        },
        "payload": {
            "key":"value"
        }
    }})";
    // clang-format on
    auto parseResult = AVSDirective::create(directiveJson, nullptr, "");
    EXPECT_EQ(parseResult.second, AVSDirective::ParseStatus::SUCCESS);
    ASSERT_THAT(parseResult.first, NotNull());

    auto& directive = *parseResult.first;
    EXPECT_EQ(directive.getNamespace(), "Namespace");
    EXPECT_EQ(directive.getName(), "Name");
    EXPECT_EQ(directive.getInstance(), "Instance");
    ASSERT_TRUE(directive.getEndpoint().hasValue());

    auto endpoint = directive.getEndpoint().value();
    EXPECT_EQ(endpoint.endpointId, "EndpointId");
    EXPECT_TRUE(endpoint.cookies.empty());
}

TEST(AVSDirectiveTest, test_parseWithCorrelationTokens) {
    // clang-format off
    std::string directiveJson = R"({
    "directive": {
        "header": {
            "namespace": "Namespace",
            "name": "Name",
            "messageId": "Id",
            "correlationToken": "Token123",
            "eventCorrelationToken": "Event123"
        },
        "payload": {
            "key":"value"
        }
    }})";
    // clang-format on
    auto parseResult = AVSDirective::create(directiveJson, nullptr, "");
    EXPECT_EQ(parseResult.second, AVSDirective::ParseStatus::SUCCESS);
    ASSERT_THAT(parseResult.first, NotNull());

    auto& directive = *parseResult.first;
    EXPECT_EQ(directive.getCorrelationToken(), "Token123");
    EXPECT_EQ(directive.getEventCorrelationToken(), "Event123");
}

TEST(AVSDirectiveTest, test_parseWithPayloadVersion) {
    // clang-format off
    std::string directiveJson = R"({
    "directive": {
        "header": {
            "namespace": "Namespace",
            "name": "Name",
            "messageId": "Id",
            "payloadVersion": "3"
        },
        "payload": {
            "key":"value"
        }
    }})";
    // clang-format on
    auto parseResult = AVSDirective::create(directiveJson, nullptr, "");
    EXPECT_EQ(parseResult.second, AVSDirective::ParseStatus::SUCCESS);
    ASSERT_THAT(parseResult.first, NotNull());

    auto& directive = *parseResult.first;
    EXPECT_EQ(directive.getPayloadVersion(), "3");
}

TEST(AVSDirectiveTest, test_parseWithEndpointCookie) {
    // clang-format off
    std::string directiveJson = R"({
    "directive": {
        "header": {
            "namespace": "Namespace",
            "name": "Name",
            "instance": "Instance",
            "messageId": "Id"
        },
        "endpoint": {
            "endpointId": "EndpointId",
            "cookie": {
                "key":"value"
            }
        },
        "payload": {
            "key":"value"
        }
    }})";
    // clang-format on
    auto parseResult = AVSDirective::create(directiveJson, nullptr, "");
    EXPECT_EQ(parseResult.second, AVSDirective::ParseStatus::SUCCESS);
    ASSERT_THAT(parseResult.first, NotNull());

    auto& directive = *parseResult.first;
    ASSERT_TRUE(directive.getEndpoint().hasValue());

    auto endpoint = directive.getEndpoint().value();
    EXPECT_EQ(endpoint.cookies.size(), 1u);
    EXPECT_EQ(endpoint.cookies["key"], "value");
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
