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
#include <string>

#include <acsdk/CryptoInterfaces/test/MockCryptoFactory.h>
#include <acsdk/CryptoInterfaces/test/MockKeyStore.h>
#include <acsdk/Crypto/CryptoFactory.h>
#include <acsdk/Pkcs11/KeyStoreFactory.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <acsdk/PropertiesInterfaces/test/MockPropertiesFactory.h>
#include <acsdk/PropertiesInterfaces/test/StubPropertiesFactory.h>
#include <acsdk/Properties/private/EncryptedPropertiesFactory.h>

namespace alexaClientSDK {
namespace properties {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::propertiesInterfaces::test;

using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces;
using namespace ::alexaClientSDK::crypto;
using namespace ::alexaClientSDK::pkcs11;
using namespace ::alexaClientSDK::cryptoInterfaces::test;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;

/// @private
static const std::string JSON_TEST_CONFIG = R"(
{
    "pkcs11Module": {
        "libraryPath":")" PKCS11_LIBRARY R"(",
        "tokenName": ")" PKCS11_TOKEN_NAME R"(",
        "userPin": ")" PKCS11_PIN R"(",
        "defaultKeyName": ")" PKCS11_KEY_NAME R"("
    }
}
)";

/// @private
static const std::string CONFIG_URI{"component/config"};

/// @private
static void initConfig() {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>(JSON_TEST_CONFIG);
    EXPECT_TRUE(ConfigurationNode::initialize({ss}));
}

/// Test that the constructor with a nullptr doesn't segfault.
TEST(EncryptedPropertiesFactoryTest, test_createNonNull) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();
    auto mockKeyStore = std::make_shared<MockKeyStore>();
    auto mockPropertiesFactory = std::make_shared<MockPropertiesFactory>();

    auto factory = EncryptedPropertiesFactory::create(mockPropertiesFactory, mockCryptoFactory, mockKeyStore);

    ASSERT_NE(nullptr, factory);
}

TEST(EncryptedPropertiesFactoryTest, test_getPropertiesEncrypted) {
    initConfig();

    auto cryptoFactory = createCryptoFactory();
    auto keyStore = createKeyStore();
    auto innerPropertiesFactory = StubPropertiesFactory::create();

    auto factory = EncryptedPropertiesFactory::create(innerPropertiesFactory, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, factory);

    auto props = factory->getProperties(CONFIG_URI);
    ASSERT_NE(nullptr, props);

    auto innerProperties = innerPropertiesFactory->getProperties(CONFIG_URI);
    PropertiesInterface::Bytes value;
    ASSERT_TRUE(innerProperties->getBytes("$acsdkEncryption$", value));
}

TEST(EncryptedPropertiesFactoryTest, test_createNullInnerFactory) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();
    auto mockKeyStore = std::make_shared<MockKeyStore>();

    auto factory = EncryptedPropertiesFactory::create(nullptr, mockCryptoFactory, mockKeyStore);

    ASSERT_EQ(nullptr, factory);
}

TEST(EncryptedPropertiesFactoryTest, test_createNullCryptoFactory) {
    auto mockKeyStore = std::make_shared<MockKeyStore>();
    auto mockPropertiesFactory = std::make_shared<MockPropertiesFactory>();

    auto factory = EncryptedPropertiesFactory::create(mockPropertiesFactory, nullptr, mockKeyStore);

    ASSERT_EQ(nullptr, factory);
}

TEST(EncryptedPropertiesFactoryTest, test_createNullKeyStore) {
    auto mockCryptoFactory = std::make_shared<MockCryptoFactory>();
    auto mockPropertiesFactory = std::make_shared<MockPropertiesFactory>();

    auto factory = EncryptedPropertiesFactory::create(mockPropertiesFactory, mockCryptoFactory, nullptr);

    ASSERT_EQ(nullptr, factory);
}

}  // namespace test
}  // namespace properties
}  // namespace alexaClientSDK
