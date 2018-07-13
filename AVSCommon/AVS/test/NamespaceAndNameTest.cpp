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

#include <string>
#include <unordered_map>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "AVSCommon/AVS/NamespaceAndName.h"

using namespace ::testing;

namespace alexaClientSDK {

namespace avsCommon {
namespace avs {
namespace test {

/// @c nameSpace value for testing.
static const std::string NAMESPACE_SPEECH_RECOGNIZER("SpeechRecognizer");

/// @c name value for testing.
static const std::string NAME_RECOGNIZE("Recognize");

/// @c nameSpace value for testing.
static const std::string NAMESPACE_SPEAKER("Speaker");

/// @c name value for testing.
static const std::string NAME_SET_VOLUME("SetVolume");

/// NamespaceAndNameTest
class NamespaceAndNameTest : public ::testing::Test {};

/**
 * Invoke default constructor.  Expect @c nameSpace and @c name properties are both empty strings.
 */
TEST_F(NamespaceAndNameTest, testDefaultConstructor) {
    NamespaceAndName instance;
    ASSERT_TRUE(instance.nameSpace.empty());
    ASSERT_TRUE(instance.name.empty());
}

/**
 * Invoke constructor with member values.  Expect properties match those provided to the constructor.
 */
TEST_F(NamespaceAndNameTest, testConstructorWithValues) {
    NamespaceAndName instance(NAMESPACE_SPEECH_RECOGNIZER, NAME_RECOGNIZE);
    ASSERT_EQ(instance.nameSpace, NAMESPACE_SPEECH_RECOGNIZER);
    ASSERT_EQ(instance.name, NAME_RECOGNIZE);
}

/**
 * Create an @c std::unordered_map using NamespaceAndName values as keys. Expect that the keys map to distinct values.
 */
TEST_F(NamespaceAndNameTest, testInUnorderedMap) {
    std::unordered_map<NamespaceAndName, int> testMap;
    NamespaceAndName key1(NAMESPACE_SPEECH_RECOGNIZER, NAME_RECOGNIZE);
    NamespaceAndName key2(NAMESPACE_SPEAKER, NAME_SET_VOLUME);
    testMap[key1] = 1;
    testMap[key2] = 2;
    ASSERT_EQ(testMap[key1], 1);
    ASSERT_EQ(testMap[key2], 2);
    ASSERT_NE(testMap[key1], testMap[key2]);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
