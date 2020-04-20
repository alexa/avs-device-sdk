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

// @file ConfigurationNodeTest.cpp

#include <sstream>

#include <gtest/gtest.h>

#include "AVSCommon/Utils/Configuration/ConfigurationNode.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace configuration {
namespace test {

using namespace ::testing;

/// Name of non-existent object for exercising failure to find a @c ConfigurationNode.
static const std::string NON_OBJECT = "non-existent-object";

/// Name of first root level object.
static const std::string OBJECT1 = "object1";

/// Name of first bool value in first root level object.
static const std::string BOOL1_1 = "bool1.1";

/// Value of first bool value in first root level object.
static const bool BOOL_VALUE1_1 = true;

/// Name of first object inside first root level object.
static const std::string OBJECT1_1 = "object1.1";

/// Name of first string value in first object inside first root level object.
static const std::string STRING1_1_1 = "string1.1.1";

/// Value of first string value in first object inside first root level object.
static const std::string STRING_VALUE1_1_1 = "stringValue1.1.1";

/// Name of second root level object.
static const std::string OBJECT2 = "object2";

/// Name of first string in second root level object.
static const std::string STRING2_1 = "string2.1";

/// Replaced value of first string in second root level object.
static const std::string NEW_STRING_VALUE2_1 = "new-stringValue2.1";

/// Name for  non-existent int value in second root level object.
static const std::string NON_EXISTENT_INT2_1 = "non-existent-int2.1";

/// Default value for non-existent int value in second root level object.
static const int NON_EXISTENT_INT_VALUE2_1 = 123;

/// Name of first int value in second root level object.
static const std::string INT2_1 = "int2.1";

/// Name of first object inside second root level object.
static const std::string OBJECT2_1 = "object2.1";

/// Name of first string inside first object inside second root level object.
static const std::string STRING2_1_1 = "string2.1.1";

/// Replaced value of first string inside first object inside second root level object.
static const std::string NEW_STRING_VALUE2_1_1 = "new-stringValue2.1.1";

/// Bad JSON string to verify handling the failure to parse JSON
static const std::string BAD_JSON = "{ bad json }";

/// Name of array root level object.
static const std::string ARRAY_OBJECT = "arrayObject";

/// First JSON string to parse, serving as default for configuration values.
// clang-format off
static const std::string FIRST_JSON = R"(
    {
        "object1" : {
            "bool1.1" : true
        },
        "object2" : {
            "int2.1" : 21,
            "string2.1" : "stringValue2.1",
            "object2.1" : {
                "string2.1.1" : "stringValue2.1.1"
            }
        }
    })";
// clang-format on

/// Second JSON string to parse, overlaying configuration values from FIRST_JSON.
// clang-format off
static const std::string SECOND_JSON = R"(
    {
        "object1" : {
            "object1.1" : {
                "string1.1.1" : "stringValue1.1.1"
            },
            "int1.1" : 11
        }
    })";
// clang-format on

/// Third JSON string to parse, overlaying configuration values from FIRST_JSON and SECOND_JSON.
// clang-format off
static const std::string THIRD_JSON = R"(
    {
        "object2" : {
            "int2.1" : 21,
            "string2.1" : "new-stringValue2.1",
            "object2.1" : {
                "string2.1.1" : "new-stringValue2.1.1"
            }
        }
    })";
// clang-format on

/// A JSON string to test array.
// clang-format off
static const std::string ARRAY_JSON = R"(
    {
        "arrayObject" : [
            {
                "object2.1" : "new-stringValue2.1"
            },
            {
                "object2.1" : "new-stringValue2.1.1"
            }
        ]
    })";
// clang-format on

/**
 * Class for testing the ConfigurationNode class
 */
class ConfigurationNodeTest : public ::testing::Test {
protected:
    void TearDown() override;
};

void ConfigurationNodeTest::TearDown() {
    ConfigurationNode::uninitialize();
}

/**
 * Initializes the root configuration with the given JSON string.
 *
 * @param jsonConfiguration The configuration string.
 * @return Whether it succeeded or not.
 */
bool initializeConfiguration(const std::string& jsonConfiguration) {
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(std::make_shared<std::stringstream>(jsonConfiguration));
    return ConfigurationNode::initialize(jsonStream);
}

/**
 * Verify initialization a configuration. Verify both the implementation of accessor methods and the results
 * of merging JSON streams.
 */
TEST_F(ConfigurationNodeTest, test_initializationAndAccess) {
    // Verify a null configuration results in failure
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(nullptr);
    ASSERT_FALSE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Verify invalid JSON results in failure
    auto badStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*badStream) << BAD_JSON;
    jsonStream.push_back(badStream);
    ASSERT_FALSE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Combine valid JSON streams with overlapping values. Verify reported success.
    auto firstStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*firstStream) << FIRST_JSON;
    auto secondStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*secondStream) << SECOND_JSON;
    auto thirdStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*thirdStream) << THIRD_JSON;
    auto arrayStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*arrayStream) << ARRAY_JSON;
    jsonStream.push_back(firstStream);
    jsonStream.push_back(secondStream);
    jsonStream.push_back(thirdStream);
    jsonStream.push_back(arrayStream);
    ASSERT_TRUE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Verify failure reported for subsequent initializations.
    auto firstStream1 = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*firstStream1) << FIRST_JSON;
    jsonStream.push_back(firstStream1);
    ASSERT_FALSE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Verify non-found name results in a ConfigurationNode that evaluates to false.
    ASSERT_FALSE(ConfigurationNode::getRoot()[NON_OBJECT]);

    // Verify found name results in a ConfigurationNode that evaluates to true.
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT1]);

    // Verify extraction of bool value.
    bool bool11 = true;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT1].getBool(BOOL1_1, &bool11));
    ASSERT_EQ(bool11, BOOL_VALUE1_1);

    // Verify traversal of multiple levels of ConfigurationNode and extraction of a string value.
    std::string string111;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT1][OBJECT1_1].getString(STRING1_1_1, &string111));
    ASSERT_EQ(string111, STRING_VALUE1_1_1);

    // Verify retrieval of default value when name does not match any value.
    int nonExistentInt21 = 0;
    ASSERT_NE(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);
    ASSERT_FALSE(ConfigurationNode::getRoot()[OBJECT2].getInt(
        NON_EXISTENT_INT2_1, &nonExistentInt21, NON_EXISTENT_INT_VALUE2_1));
    ASSERT_EQ(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);

    // Verify extraction if an integer value.
    int int21;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT2].getInt(INT2_1, &int21));
    ASSERT_EQ(int21, 21);

    // Verify overwrite of string value by subsequent JSON.
    std::string newString21;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT2].getString(STRING2_1, &newString21));
    ASSERT_EQ(newString21, NEW_STRING_VALUE2_1);

    // Verify retrieval of default value when type does not match an existing value.
    nonExistentInt21 = 0;
    ASSERT_NE(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);
    ASSERT_FALSE(ConfigurationNode::getRoot()[OBJECT2].getInt(STRING2_1, &nonExistentInt21, NON_EXISTENT_INT_VALUE2_1));
    ASSERT_EQ(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);

    // Verify overwrite of string value in nested Configuration node.
    std::string string211;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT2][OBJECT2_1].getString(STRING2_1_1, &string211));
    ASSERT_EQ(string211, NEW_STRING_VALUE2_1_1);

    // Verify getting a non-array object with getArray will return an empty Configuration node.
    ASSERT_FALSE(ConfigurationNode::getRoot().getArray(OBJECT1));

    // Verify getting the array size of a non-array object will return zero.
    ASSERT_TRUE(0 == ConfigurationNode::getRoot()[OBJECT1].getArraySize());

    // Verify getting the array from a non-array object will return an empty Configuration node.
    ASSERT_FALSE(ConfigurationNode::getRoot()[OBJECT1][1]);

    // Verify getting a array object with getArray will return an valid Configuration node.
    auto array = ConfigurationNode::getRoot().getArray(ARRAY_OBJECT);
    ASSERT_TRUE(array);

    // Make sure that the array size is 2
    auto arraySize = array.getArraySize();
    ASSERT_TRUE(2U == arraySize);

    // Make sure accessing an array outside range will return an empty Configuration Node.
    ASSERT_FALSE(array[arraySize]);

    // Check if we can get the string from the first and second array item
    std::string arrayString;
    ASSERT_TRUE(array[0].getString(OBJECT2_1, &arrayString));
    ASSERT_EQ(arrayString, NEW_STRING_VALUE2_1);
    ASSERT_TRUE(array[1].getString(OBJECT2_1, &arrayString));
    ASSERT_EQ(arrayString, NEW_STRING_VALUE2_1_1);
}

TEST_F(ConfigurationNodeTest, test_arrayElement) {
    // clang-format off
    initializeConfiguration(R"(
    {
        "array1" : ["value1","value2"],
        "NonStringArray" : [1, 2]
    })");
    // clang-format on

    const std::string key{"array1"};
    const std::set<std::string> expectedValue = {"value1", "value2"};

    std::set<std::string> configValue;
    EXPECT_TRUE(ConfigurationNode::getRoot().getStringValues(key, &configValue));
    EXPECT_EQ(configValue, expectedValue);

    const std::string missingKey{"missingKey"};
    const std::set<std::string> expectedValueForMissingKey = {};
    configValue.clear();
    EXPECT_FALSE(ConfigurationNode::getRoot().getStringValues(missingKey, &configValue));
    EXPECT_EQ(configValue, expectedValueForMissingKey);

    const std::string nonStringArrayKey{"NonStringArray"};
    const std::set<std::string> expectedValueForNonStringArray = {};
    EXPECT_TRUE(ConfigurationNode::getRoot().getStringValues(nonStringArrayKey, &configValue));
    configValue.clear();
    EXPECT_EQ(configValue, expectedValueForNonStringArray);
}

TEST_F(ConfigurationNodeTest, test_emptyArrayElement) {
    // clang-format off
    initializeConfiguration(R"(
    {
        "array1" : []
    })");
    // clang-format on

    const std::string key{"array1"};

    std::set<std::string> configValue;
    EXPECT_TRUE(ConfigurationNode::getRoot().getStringValues(key, &configValue));
    EXPECT_TRUE(ConfigurationNode::getRoot().getStringValues(key, nullptr));
    EXPECT_TRUE(configValue.empty());
}

}  // namespace test
}  // namespace configuration
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
