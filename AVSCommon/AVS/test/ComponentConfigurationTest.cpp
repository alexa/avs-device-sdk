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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/AVS/ComponentConfiguration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace avs;
using namespace ::testing;

/**
 * Test creating a ComponentConfiguration
 */
TEST(ComponentConfigurationTest, test_createComponentConfiguration) {
    EXPECT_THAT(ComponentConfiguration::createComponentConfiguration("component", "1.0"), NotNull());
}

/**
 * Test creating a configuration with invalid version is not created.
 */
TEST(ComponentConfigurationTest, test_addBadVersion) {
    // Spaces not allowed in version
    EXPECT_THAT(ComponentConfiguration::createComponentConfiguration("component", "1.0 123"), IsNull());
    // version must follow dot notation
    EXPECT_THAT(ComponentConfiguration::createComponentConfiguration("component", "1..0"), IsNull());
}

/**
 * Test creating an empty configuration is not created.
 */
TEST(ComponentConfigurationTest, test_addEmptyConfig) {
    EXPECT_THAT(ComponentConfiguration::createComponentConfiguration("component", ""), IsNull());
    EXPECT_THAT(ComponentConfiguration::createComponentConfiguration("", "1.0"), IsNull());
    EXPECT_THAT(ComponentConfiguration::createComponentConfiguration("", ""), IsNull());
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
