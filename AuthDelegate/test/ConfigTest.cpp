/*
 * ConfigTest.cpp
 *
 * Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AuthDelegate/Config.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace authDelegate {

class ConfigTest : public ::testing::Test {
protected:
    Config configuration;
};

TEST_F(ConfigTest, lwa_url_is_secure) {
    std::string url = configuration.getLwaUrl();

    EXPECT_THAT(url, testing::StartsWith("https"));
}

} // namespace authDelegate
} // namespace alexaClientSDK
