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

#include <acsdk/Pkcs11/private/PKCS11Config.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include <gtest/gtest.h>
#include <sstream>

namespace alexaClientSDK {
namespace pkcs11 {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;

/// @private
// clang-format off
static const std::string JSON_CONFIG =
    "{"
    "\"pkcs11Module\":{"
    "\"libraryPath\":\"library.so\","
    "\"tokenName\":\"ACSDK\","
    "\"userPin\":\"9999\","
    "\"defaultKeyName\":\"mainKey\""
    "}"
    "}";
// clang-format on

TEST(PKCS11ConfigTest, test_defaultConfig) {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>(JSON_CONFIG);
    ConfigurationNode::initialize({ss});

    auto config = PKCS11Config::create();
    ASSERT_NE(nullptr, config);
    ASSERT_EQ("mainKey", config->getDefaultKeyName());
    ASSERT_EQ("library.so", config->getLibraryPath());
    ASSERT_EQ("ACSDK", config->getTokenName());
    ASSERT_EQ("9999", config->getUserPin());
}

}  // namespace test
}  // namespace pkcs11
}  // namespace alexaClientSDK
