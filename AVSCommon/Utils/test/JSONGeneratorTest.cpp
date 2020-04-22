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

#include <set>
#include <string>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>

#include "AVSCommon/Utils/JSON/JSONGenerator.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {
namespace test {

using namespace ::testing;
using namespace rapidjson;

class JsonGeneratorTest : public Test {
protected:
    JsonGenerator m_generator;
};

/// Test json generator when no member is given.
TEST_F(JsonGeneratorTest, test_emptyJson) {
    EXPECT_EQ(m_generator.toString(), "{}");
}

/// Test json generator object creation.
TEST_F(JsonGeneratorTest, test_jsonObject) {
    EXPECT_TRUE(m_generator.startObject("key"));
    EXPECT_TRUE(m_generator.finishObject());

    auto expected = R"({"key":{}})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator int creation.
TEST_F(JsonGeneratorTest, test_jsonInt) {
    int value = std::numeric_limits<int>::max();
    EXPECT_TRUE(m_generator.addMember("member", value));

    auto expected = R"({"member":)" + std::to_string(value) + "}";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator long creation.
TEST_F(JsonGeneratorTest, test_jsonLong) {
    int64_t value = std::numeric_limits<int64_t>::max();
    EXPECT_TRUE(m_generator.addMember("member", value));

    auto expected = R"({"member":)" + std::to_string(value) + "}";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator long creation.
TEST_F(JsonGeneratorTest, test_jsonUInt) {
    unsigned int value = std::numeric_limits<unsigned int>::max();
    EXPECT_TRUE(m_generator.addMember("member", value));

    auto expected = R"({"member":)" + std::to_string(value) + "}";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator long creation.
TEST_F(JsonGeneratorTest, test_jsonULong) {
    uint64_t value = std::numeric_limits<uint64_t>::max();
    EXPECT_TRUE(m_generator.addMember("member", value));

    auto expected = R"({"member":)" + std::to_string(value) + "}";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator boolean creation.
TEST_F(JsonGeneratorTest, test_jsonBool) {
    bool value = true;
    EXPECT_TRUE(m_generator.addMember("member", value));

    auto expected = R"({"member":true})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator double creation.
TEST_F(JsonGeneratorTest, test_jsonDouble) {
    double value = 1234.456;
    EXPECT_TRUE(m_generator.addMember("member", value));

    std::stringstream expectedString;
    expectedString.precision(3);
    expectedString << std::fixed << value;

    auto expected = R"({"member":)" + expectedString.str() + R"(})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator char creation.
TEST_F(JsonGeneratorTest, test_jsonCString) {
    EXPECT_TRUE(m_generator.addMember("member", "value"));

    auto expected = R"({"member":"value"})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator char creation.
TEST_F(JsonGeneratorTest, test_jsonNullCString) {
    EXPECT_FALSE(m_generator.addMember("member", nullptr));

    auto expected = R"({})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator for string array creation.
TEST_F(JsonGeneratorTest, test_jsonStringArray) {
    std::vector<std::string> values({"value1", "value2"});
    EXPECT_TRUE(m_generator.addStringArray("member", values));

    auto expected = R"({"member":["value1","value2"]})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator for array creation.
TEST_F(JsonGeneratorTest, test_jsonArray) {
    std::vector<std::string> values({R"("value1")", "true"});
    EXPECT_TRUE(m_generator.addMembersArray("member", values));

    auto expected = R"({"member":["value1",true]})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator for empty array creation.
TEST_F(JsonGeneratorTest, test_jsonEmptyList) {
    std::set<std::string> values;
    EXPECT_TRUE(m_generator.addStringArray("member", values));

    auto expected = R"({"member":[]})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json raw creation.
TEST_F(JsonGeneratorTest, test_jsonRawJsonMember) {
    EXPECT_TRUE(m_generator.addRawJsonMember("member1", R"({"member11":"value11"})"));
    EXPECT_TRUE(m_generator.addMember("member2", "value2"));

    auto expected = R"({"member1":{"member11":"value11"},"member2":"value2"})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json raw validation.
TEST_F(JsonGeneratorTest, test_jsonRawJsonMemberFailed) {
    EXPECT_FALSE(m_generator.addRawJsonMember("member1", R"(invalid)"));
    EXPECT_TRUE(m_generator.addMember("member2", "value2"));

    auto expected = R"({"member2":"value2"})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test close when there is no open object.
TEST_F(JsonGeneratorTest, test_closeTooMany) {
    EXPECT_TRUE(m_generator.finishObject());
    EXPECT_FALSE(m_generator.finishObject());
}

/// Test to string with open objects.
TEST_F(JsonGeneratorTest, test_openObjects) {
    EXPECT_TRUE(m_generator.startObject("key"));

    auto expected = R"({"key":{)";
    EXPECT_EQ(m_generator.toString(false), expected);
}

/// Test finalize open objects.
TEST_F(JsonGeneratorTest, test_finalizeObjects) {
    EXPECT_TRUE(m_generator.startObject("key1"));
    EXPECT_TRUE(m_generator.startObject("key2"));

    auto expected = R"({"key1":{"key2":{}}})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test operations after finalize.
TEST_F(JsonGeneratorTest, test_addMemberAfterFinalize) {
    EXPECT_EQ(m_generator.toString(), "{}");
    EXPECT_EQ(m_generator.toString(), "{}");
    ASSERT_TRUE(m_generator.isFinalized());
    EXPECT_FALSE(m_generator.startObject("key2"));
    EXPECT_FALSE(m_generator.addMember("key1", "value"));
    EXPECT_FALSE(m_generator.addMember("key2", 10));
    EXPECT_FALSE(m_generator.addMember("key3", 10u));
    EXPECT_FALSE(m_generator.addMember("key4", static_cast<int64_t>(10L)));
    EXPECT_FALSE(m_generator.addMember("key5", static_cast<uint64_t>(10uL)));

    auto expected = "{}";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator for collection of string array creation.
TEST_F(JsonGeneratorTest, test_jsonCollectionOfStringArray) {
    std::vector<std::vector<std::string>> values{{"value1", "value2"}, {"value3"}};
    EXPECT_TRUE(m_generator.addCollectionOfStringArray("member", values));

    auto expected = R"({"member":[["value1","value2"],["value3"]]})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator for collection of empty collection.
TEST_F(JsonGeneratorTest, test_jsonCollectionOfEmptyCollection) {
    std::vector<std::set<std::string>> values{};
    EXPECT_TRUE(m_generator.addCollectionOfStringArray("member", values));

    auto expected = R"({"member":[]})";
    EXPECT_EQ(m_generator.toString(), expected);
}

/// Test json generator for collection of empty string array.
TEST_F(JsonGeneratorTest, test_jsonCollectionOfEmptyStringArray) {
    std::vector<std::vector<std::string>> values{{"value1"}, {}};
    EXPECT_TRUE(m_generator.addCollectionOfStringArray("member", values));

    auto expected = R"({"member":[["value1"],[]]})";
    EXPECT_EQ(m_generator.toString(), expected);
}

}  // namespace test
}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
