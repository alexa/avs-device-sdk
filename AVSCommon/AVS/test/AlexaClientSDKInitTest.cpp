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

/// @file AlexaClientSDKInitTest.cpp

#include <sstream>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {
namespace test {

using namespace std;

/**
 * Tests @c initialize without any JSON configuration, expecting it to return @c true.
 *
 * @note This test also validates whether libcurl supports HTTP2.
 */
TEST(AlexaClientSDKInitTest, initializeNoJSONConfig) {
    ASSERT_TRUE(AlexaClientSDKInit::initialize({}));
    AlexaClientSDKInit::uninitialize();
}

/**
 * Tests @c initialize with an invalid JSON configuration, expecting it to return @c false.
 *
 * @note This test also validates whether libcurl supports HTTP2.
 */
TEST(AlexaClientSDKInitTest, initializeInvalidJSONConfig) {
    auto invalidJSON = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*invalidJSON) << "{";
    ASSERT_FALSE(AlexaClientSDKInit::initialize({invalidJSON}));
}

/**
 * Tests @c initialize with a valid JSON configuration, expecting it to return @c true.
 *
 * @note This test also validates whether libcurl supports HTTP2.
 */
TEST(AlexaClientSDKInitTest, initializeValidJSONConfig) {
    auto validJSON = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*validJSON) << R"({"key":"value"})";
    ASSERT_TRUE(AlexaClientSDKInit::initialize({validJSON}));
    AlexaClientSDKInit::uninitialize();
}

/**
 * Tests @c isInitialized when the SDK has not been initialized yet, expecting it to return @c false.
 */
TEST(AlexaClientSDKInitTest, uninitializedIsInitialized) {
    ASSERT_FALSE(AlexaClientSDKInit::isInitialized());
}

/**
 * Tests @c isInitialized when the SDK is initialized, expecting it to return @c true.
 */
TEST(AlexaClientSDKInitTest, isInitialized) {
    ASSERT_TRUE(AlexaClientSDKInit::initialize({}));
    // Expect used to ensure we uninitialize.
    EXPECT_TRUE(AlexaClientSDKInit::isInitialized());
    AlexaClientSDKInit::uninitialize();
}

/**
 * Tests @c uninitialize when the SDK has not been initialized yet, expecting no crashes or exceptions.
 */
TEST(AlexaClientSDKInitTest, uninitialize) {
    AlexaClientSDKInit::uninitialize();
}

}  // namespace test
}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
