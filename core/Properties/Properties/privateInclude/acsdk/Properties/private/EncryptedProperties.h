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

#ifndef ACSDK_PROPERTIES_PRIVATE_ENCRYPTEDPROPERTIES_H_
#define ACSDK_PROPERTIES_PRIVATE_ENCRYPTEDPROPERTIES_H_

#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>
#include <acsdk/Properties/ErrorCallbackInterface.h>
#include <acsdk/Properties/private/RetryExecutor.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief Properties adapter with field encryption.
 *
 * This class wraps underlying PropertiesInterface with encryption support. All property values are encrypted on save
 * and decrypted on load. When this adapter initializes for the first time, it automatically encrypts all fields. To
 * manage encryption key, additional data is stored with '$acsdkEncryption$' property name. This property contains
 * algorithms to use and encrypted data key. The data key itself is encrypted using HSM key store.
 *
 * This class is thread safe and can be shared between multiple consumers.
 *
 * @ingroup Lib_acsdkProperties
 */
class EncryptedProperties : public alexaClientSDK::propertiesInterfaces::PropertiesInterface {
public:
    static std::shared_ptr<PropertiesInterface> create(
        const std::string& configUri,
        const std::shared_ptr<PropertiesInterface>& innerProperties,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface>& keyStore) noexcept;

    /// @name PropertiesInterface methods.
    /// @{
    bool getString(const std::string& key, std::string& value) noexcept override;
    bool putString(const std::string& key, const std::string& value) noexcept override;
    bool getBytes(const std::string& key, Bytes& value) noexcept override;
    bool putBytes(const std::string& key, const Bytes& value) noexcept override;
    bool remove(const std::string& key) noexcept override;
    bool getKeys(std::unordered_set<std::string>& valueContainer) noexcept override;
    bool clear() noexcept override;
    /// @}

protected:
    typedef cryptoInterfaces::KeyStoreInterface::IV IV;
    typedef cryptoInterfaces::KeyStoreInterface::DataBlock DataBlock;
    typedef cryptoInterfaces::KeyStoreInterface::KeyChecksum KeyChecksum;
    typedef cryptoInterfaces::CryptoCodecInterface::Key Key;
    typedef cryptoInterfaces::CryptoCodecInterface::Tag Tag;

    // Constructor.
    EncryptedProperties(
        const std::string& configUri,
        const std::shared_ptr<PropertiesInterface>& innerProperties,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface>& keyStore) noexcept;

    // Container initialization.
    bool init() noexcept;
    // Encryption upgrade.
    StatusCode upgradeEncryption(RetryExecutor& executor, const std::unordered_set<std::string>& keys) noexcept;
    // Prepare data key
    StatusCode loadAndDecryptDataKey(RetryExecutor& executor) noexcept;

    // Data property operations.
    bool encryptAndEncodePropertyValue(
        const std::string& key,
        const Bytes& plaintext,
        Bytes& encodedCiphertext) noexcept;
    bool decodeAndDecryptPropertyValue(
        const std::string& key,
        const Bytes& encodedCiphertext,
        Bytes& plaintext) noexcept;
    bool encryptAndPutInternal(const std::string& key, const Bytes& plaintext) noexcept;
    bool getAndDecryptInternal(const std::string& key, Bytes& plaintext) noexcept;

    // Encryption property operations.
    StatusCode generateAndStoreDataKeyWithRetries(RetryExecutor& executor) noexcept;

    // Inner properties operations
    bool loadKeysWithRetries(RetryExecutor& executor, std::unordered_set<std::string>& keys) noexcept;
    bool storeValueWithRetries(
        RetryExecutor& executor,
        const std::string& key,
        const Bytes& data,
        bool canDrop) noexcept;
    bool loadValueWithRetries(RetryExecutor& executor, const std::string& key, Bytes& data) noexcept;
    bool deleteValueWithRetries(RetryExecutor& executor, const std::string& key) noexcept;
    bool clearAllValuesWithRetries(RetryExecutor& executor) noexcept;
    bool executeKeyOperationWithRetries(
        RetryExecutor& executor,
        const std::string& operationName,
        const std::string& key,
        const std::function<bool()>& operation) noexcept;

    /**
     * @brief Generate new data key.
     *
     * Method generates new data key and stores it in this instance. If there is an error, the method attempts to
     * do retries.
     *
     * @param[in] helper Executor for perform operation with retries.
     * @return True if operation succeeds, false otherwise.
     */
    bool generateDataKeyWithRetries(RetryExecutor& executor) noexcept;
    bool encryptAndEncodeDataKeyWithRetries(RetryExecutor& executor, Bytes& encoded) noexcept;
    StatusCode decodeAndDecryptDataKey(const Bytes& encoded) noexcept;
    bool encryptDataKey(
        std::string& mainKeyAlias,
        alexaClientSDK::cryptoInterfaces::AlgorithmType& algorithmType,
        KeyChecksum& mainKeyChecksum,
        IV& dataKeyIV,
        DataBlock& dataKeyCiphertext,
        Tag& dataKeyTag) noexcept;
    bool decryptDataKey(
        const std::string& mainKeyAlias,
        alexaClientSDK::cryptoInterfaces::AlgorithmType dataKeyAlgorithm,
        const KeyChecksum& mainKeyChecksum,
        const IV& dataKeyIV,
        const DataBlock& keyCiphertext,
        const Tag& dataKeyTag) noexcept;

    bool doClear(RetryExecutor& helper) noexcept;

    /// Configuration namespace (for error callbacks).
    const std::string m_configUri;

    /// Underlying storage interface.
    const std::shared_ptr<PropertiesInterface> m_innerProperties;

    /// Cryptography service factory.
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface> m_cryptoFactory;

    /// HSM keystore interface.
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface> m_keyStore;

    /// Actual algorithm type in use.
    alexaClientSDK::cryptoInterfaces::AlgorithmType m_dataAlgorithmType;

    /// Data key in use
    Key m_dataKey;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_ENCRYPTEDPROPERTIES_H_
