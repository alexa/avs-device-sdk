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

#include <acsdk/CodecUtils/Hex.h>
#include <acsdk/Crypto/CryptoFactory.h>
#include <acsdk/CryptoInterfaces/test/MockCryptoFactory.h>
#include <acsdk/CryptoInterfaces/test/MockKeyStore.h>
#include <acsdk/Pkcs11/KeyStoreFactory.h>
#include <acsdk/PropertiesInterfaces/test/MockProperties.h>
#include <acsdk/PropertiesInterfaces/test/StubPropertiesFactory.h>
#include <acsdk/Properties/private/EncryptedProperties.h>
#include <acsdk/Properties/private/MiscStorageProperties.h>
#include <acsdk/Properties/private/Logging.h>
#include <AVSCommon/SDKInterfaces/Storage/StubMiscStorage.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace properties {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::codecUtils;
using namespace ::alexaClientSDK::crypto;
using namespace ::alexaClientSDK::cryptoInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces::test;
using namespace ::alexaClientSDK::pkcs11;
using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::propertiesInterfaces::test;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage::test;

/// String to identify log entries originating from this file.
/// @private
#define TAG "EncryptedPropertiesTest"

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
static const std::string COMPONENT_NAME{"component"};
/// @private
static const std::string CONFIG_NAMESPACE{"config"};
/// @private
static const std::string CONFIG_URI{"component/config"};
/// @private
static const std::string KEY_PROPERTY_NAME = "$acsdkEncryption$";

/// @private
static void initConfig() {
    ConfigurationNode::uninitialize();
    std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>(JSON_TEST_CONFIG);
    EXPECT_TRUE(ConfigurationNode::initialize({ss}));
}

TEST(EncryptedPropertiesTest, test_create) {
    initConfig();

    auto cryptoFactory = createCryptoFactory();
    auto keyStore = createKeyStore();
    auto innerStorage = StubMiscStorage::create();
    auto innerProperties = MiscStorageProperties::create(innerStorage, CONFIG_URI, COMPONENT_NAME, CONFIG_NAMESPACE);
    ASSERT_NE(nullptr, innerProperties);
    auto properties = EncryptedProperties::create(CONFIG_URI, innerProperties, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, properties);

    PropertiesInterface::Bytes value;
    ASSERT_TRUE(innerProperties->getBytes(KEY_PROPERTY_NAME, value));
    ASSERT_FALSE(value.empty());
}

TEST(EncryptedPropertiesTest, test_createUpgradeEncryptionString) {
    initConfig();

    auto cryptoFactory = createCryptoFactory();
    auto keyStore = createKeyStore();
    auto innerStorage = StubMiscStorage::create();
    auto innerProperties = MiscStorageProperties::create(innerStorage, CONFIG_URI, COMPONENT_NAME, CONFIG_NAMESPACE);
    ASSERT_NE(nullptr, innerProperties);

    std::string plaintextString = R"({"json":"text"})";
    EncryptedProperties::Bytes ciphertext;

    ASSERT_TRUE(innerProperties->putString("StringKey", plaintextString));

    std::string decryptedString;
    ASSERT_TRUE(innerProperties->getString("StringKey", decryptedString));

    ACSDK_DEBUG0(LX("UpgradingEncryption"));

    auto properties = EncryptedProperties::create(CONFIG_URI, innerProperties, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, properties);

    ACSDK_DEBUG0(LX("UpgradedEncryption"));
    ASSERT_TRUE(innerProperties->getBytes(KEY_PROPERTY_NAME, ciphertext));
    ACSDK_DEBUG0(LX("keyProperty").d("data", ciphertext));

    ciphertext.clear();
    ACSDK_DEBUG0(LX("loading encrypted key value"));

    ASSERT_TRUE(innerProperties->getBytes("StringKey", ciphertext));
    ACSDK_DEBUG0(LX("stringKeyEncrypted").d("data", ciphertext));

    ACSDK_DEBUG0(LX("loading decrypted key value"));

    ASSERT_TRUE(properties->getString("StringKey", decryptedString));
    ACSDK_DEBUG0(LX("stringKeyPlaintext").d("data", decryptedString));
    EXPECT_EQ(plaintextString, decryptedString);

    PropertiesInterface::Bytes encryptedString;

    ASSERT_TRUE(innerProperties->getBytes("StringKey", encryptedString));

    EXPECT_NE(plaintextString, (std::string{encryptedString.data(), encryptedString.data() + encryptedString.size()}));
}

TEST(EncryptedPropertiesTest, test_createUpgradeEncryptionBytes) {
    initConfig();

    auto cryptoFactory = createCryptoFactory();
    auto keyStore = createKeyStore();
    auto innerPropertiesFactory = StubPropertiesFactory::create();
    auto innerProperties = innerPropertiesFactory->getProperties(CONFIG_URI);
    ASSERT_NE(nullptr, innerProperties);

    PropertiesInterface::Bytes plaintextBytes{0, 1, 2};

    ASSERT_TRUE(innerProperties->putBytes("BytesKey", plaintextBytes));

    auto properties = EncryptedProperties::create(CONFIG_URI, innerProperties, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, properties);

    PropertiesInterface::Bytes decryptedBytes;
    ASSERT_TRUE(properties->getBytes("BytesKey", decryptedBytes));
    EXPECT_EQ(plaintextBytes, decryptedBytes);

    PropertiesInterface::Bytes encryptedBytes;

    ASSERT_TRUE(innerProperties->getBytes("BytesKey", encryptedBytes));

    EXPECT_NE(plaintextBytes, encryptedBytes);
}

TEST(EncryptedPropertiesTest, test_createNullInnerProperties) {
    auto mockCryptoFacotry = std::make_shared<MockCryptoFactory>();
    auto mockKeyStore = std::make_shared<MockKeyStore>();

    auto properties = EncryptedProperties::create(CONFIG_URI, nullptr, mockCryptoFacotry, mockKeyStore);
    ASSERT_EQ(nullptr, properties);
}

TEST(EncryptedPropertiesTest, test_createNullCryptoFactory) {
    auto mockKeyStore = std::make_shared<MockKeyStore>();
    auto mockProperties = std::make_shared<MockProperties>();

    auto properties = EncryptedProperties::create(CONFIG_URI, mockProperties, nullptr, mockKeyStore);
    ASSERT_EQ(nullptr, properties);
}

TEST(EncryptedPropertiesTest, test_createNullKeyStore) {
    auto mockCryptoFacotry = std::make_shared<MockCryptoFactory>();
    auto mockProperties = std::make_shared<MockProperties>();

    auto properties = EncryptedProperties::create(CONFIG_URI, mockProperties, mockCryptoFacotry, nullptr);
    ASSERT_EQ(nullptr, properties);
}

TEST(EncryptedPropertiesTest, test_encryptPut) {
    initConfig();

    auto cryptoFactory = createCryptoFactory();
    ASSERT_NE(nullptr, cryptoFactory);
    auto keyStore = createKeyStore();
    ASSERT_NE(nullptr, keyStore);

    auto stubPropsFactory = StubPropertiesFactory::create();
    auto innerProps = stubPropsFactory->getProperties("test/test");

    auto properties = EncryptedProperties::create(CONFIG_URI, innerProps, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, properties);

    PropertiesInterface::Bytes tmp;
    ASSERT_TRUE(innerProps->getBytes("$acsdkEncryption$", tmp));

    ASSERT_FALSE(innerProps->getBytes("property1", tmp));
    ASSERT_TRUE(properties->putString("property1", "some plaintext value"));
    ASSERT_TRUE(innerProps->getBytes("property1", tmp));
}

TEST(EncryptedPropertiesTest, test_reopenEncryptedProperties) {
    initConfig();

    auto cryptoFactory = createCryptoFactory();
    ASSERT_NE(nullptr, cryptoFactory);
    auto keyStore = createKeyStore();
    ASSERT_NE(nullptr, keyStore);

    auto stubPropsFactory = StubPropertiesFactory::create();
    auto innerProps = stubPropsFactory->getProperties("test/test");

    auto properties = EncryptedProperties::create(CONFIG_URI, innerProps, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, properties);
    ASSERT_TRUE(properties->putString("property1", "some plaintext value"));
    properties.reset();

    properties = EncryptedProperties::create(CONFIG_URI, innerProps, cryptoFactory, keyStore);
    ASSERT_NE(nullptr, properties);
    std::string value;
    ASSERT_TRUE(properties->getString("property1", value));
    ASSERT_EQ("some plaintext value", value);
}

}  // namespace test
}  // namespace properties
}  // namespace alexaClientSDK
