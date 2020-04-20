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
#include <rapidjson/rapidjson.h>

#include "AVSCommon/AVS/AVSContext.h"
#include "AVSCommon/AVS/CapabilityState.h"
#include "AVSCommon/AVS/CapabilityTag.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"
#include "AVSCommon/Utils/Optional.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace utils;

/// A capability tag.
static const CapabilityTag CAPABILITY_TAG{"Namespace", "Name", "EndpointId"};

/// A capability state.
static const CapabilityState CAPABILITY_STATE{R"("Value")"};

TEST(AVSContextTest, test_setterAndGetters) {
    AVSContext context;
    context.addState(CAPABILITY_TAG, CAPABILITY_STATE);

    EXPECT_TRUE(context.getState(CAPABILITY_TAG).hasValue());
    EXPECT_EQ(context.getState(CAPABILITY_TAG).value().valuePayload, CAPABILITY_STATE.valuePayload);

    EXPECT_EQ(context.getStates().size(), 1u);
    EXPECT_EQ(context.getStates()[CAPABILITY_TAG].valuePayload, CAPABILITY_STATE.valuePayload);
}

TEST(AVSContextTest, test_toJsonWithEmptyContext) {
    AVSContext context;
    EXPECT_EQ(context.toJson(), R"({"properties":[]})");
}

/// Test that AVSContext will include all fields including instance.
TEST(AVSContextTest, test_toJsonWithPropertyInstance) {
    AVSContext context;
    CapabilityTag tag{"Namespace", "Name", "EndpointId", Optional<std::string>("Instance")};
    context.addState(tag, CAPABILITY_STATE);

    auto json = context.toJson();
    rapidjson::Document document;
    EXPECT_TRUE(json::jsonUtils::parseJSON(json, &document));
    EXPECT_NE(json.find(R"("namespace":")" + tag.nameSpace), std::string::npos);
    EXPECT_NE(json.find(R"("name":")" + tag.name), std::string::npos);
    EXPECT_NE(json.find(R"("instance":")" + tag.instance.value()), std::string::npos);
    EXPECT_NE(json.find(R"("value":)" + CAPABILITY_STATE.valuePayload), std::string::npos);
    EXPECT_NE(json.find(R"("timeOfSample":")" + CAPABILITY_STATE.timeOfSample.getTime_ISO_8601()), std::string::npos);
    EXPECT_NE(
        json.find(R"("uncertaintyInMilliseconds":)" + std::to_string(CAPABILITY_STATE.uncertaintyInMilliseconds)),
        std::string::npos);
}

/// Test that AVSContext will not include an instance field if the state does not have an instance identifier.
TEST(AVSContextTest, test_toJsonWithoutPropertyInstance) {
    AVSContext context;
    CapabilityTag tag{"Namespace", "Name", "EndpointId", Optional<std::string>()};
    context.addState(tag, CAPABILITY_STATE);

    auto json = context.toJson();
    rapidjson::Document document;
    EXPECT_TRUE(json::jsonUtils::parseJSON(json, &document));
    EXPECT_NE(json.find(R"("namespace":")" + tag.nameSpace), std::string::npos);
    EXPECT_NE(json.find(R"("name":")" + tag.name), std::string::npos);
    EXPECT_EQ(json.find(R"("instance":)"), std::string::npos);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
