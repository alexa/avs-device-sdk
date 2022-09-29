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

#include <unordered_map>

#include <acsdk/Properties/private/DataPropertyCodec.h>
#include <acsdk/Properties/private/EncryptedProperties.h>
#include <acsdk/Properties/private/EncryptionKeyPropertyCodec.h>
#include <acsdk/Properties/private/RetryExecutor.h>
#include <acsdk/Properties/private/Logging.h>
#include <acsdk/CodecUtils/Hex.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "EncryptedProperties"
/// @private
static const std::string KEY_PROPERTY_NAME = "$acsdkEncryption$";
/// @private
static const AlgorithmType DEFAULT_ALGORITHM_FOR_PROPERTIES = AlgorithmType::AES_256_GCM;
/// @private
static const AlgorithmType DEFAULT_ALGORITHM_FOR_KEYS = AlgorithmType::AES_256_GCM;

std::shared_ptr<PropertiesInterface> EncryptedProperties::create(
    const std::string& configUri,
    const std::shared_ptr<PropertiesInterface>& innerProperties,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept {
    auto res = std::shared_ptr<EncryptedProperties>(
        new EncryptedProperties(configUri, innerProperties, cryptoFactory, keyStore));

    if (res->init()) {
        ACSDK_DEBUG0(LX_CFG("createSuccess", configUri));
    } else {
        ACSDK_ERROR(LX_CFG("createFailed", configUri));
        res.reset();
    }
    return res;
}

EncryptedProperties::EncryptedProperties(
    const std::string& configUri,
    const std::shared_ptr<PropertiesInterface>& innerProperties,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept :
        m_configUri(configUri),
        m_innerProperties(innerProperties),
        m_cryptoFactory(cryptoFactory),
        m_keyStore(keyStore) {
}

bool EncryptedProperties::getString(const std::string& key, std::string& value) noexcept {
    Bytes byteValue;
    if (!getAndDecryptInternal(key, byteValue)) {
        ACSDK_ERROR(LX_CFG_KEY("getStringFailed", m_configUri, key));
        return false;
    }

    value.assign(
        reinterpret_cast<std::string::const_pointer>(byteValue.data()),
        reinterpret_cast<std::string::const_pointer>(byteValue.data()) + byteValue.size());

    ACSDK_DEBUG9(LX_CFG_KEY("getStringSuccess", m_configUri, key));
    return true;
}

bool EncryptedProperties::putString(const std::string& key, const std::string& value) noexcept {
    Bytes byteValue{reinterpret_cast<Bytes::const_pointer>(value.c_str()),
                    reinterpret_cast<Bytes::const_pointer>(value.c_str()) + value.size()};

    if (encryptAndPutInternal(key, byteValue)) {
        ACSDK_DEBUG0(LX_CFG_KEY("putStringSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("petStringFailed", m_configUri, key));
        return false;
    }
}

bool EncryptedProperties::getBytes(const std::string& key, Bytes& value) noexcept {
    if (KEY_PROPERTY_NAME == key) {
        ACSDK_ERROR(LX_CFG("getBytesFailed", m_configUri).m("propertyKeyForbidden"));
        return false;
    }

    if (getAndDecryptInternal(key, value)) {
        ACSDK_DEBUG0(LX_CFG_KEY("getBytesSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("getBytesFailed", m_configUri, key));
        return true;
    }
}

bool EncryptedProperties::getAndDecryptInternal(const std::string& key, Bytes& plaintext) noexcept {
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Get, m_configUri};

    auto result = executor.execute(
        "decodeAndDecryptPropertyValue",
        [this, &executor, &key, &plaintext]() -> StatusCodeWithRetry {
            Bytes encodedCiphertext;
            if (!loadValueWithRetries(executor, key, encodedCiphertext)) {
                ACSDK_ERROR(LX_CFG_KEY("getAndDecryptInternalFailed", m_configUri, key));
                return RetryExecutor::NON_RETRYABLE_INNER_PROPERTIES_ERROR;
            }

            ACSDK_DEBUG0(LX_CFG_KEY("getAndDecryptInternal", m_configUri, key).d("loaded", encodedCiphertext));

            plaintext.clear();
            if (decodeAndDecryptPropertyValue(key, encodedCiphertext, plaintext)) {
                return RetryExecutor::SUCCESS;
            } else {
                return RetryExecutor::RETRYABLE_CRYPTO_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        ACSDK_DEBUG0(LX_CFG_KEY("getAndDecryptInternalFailed", m_configUri, key));
        return false;
    } else {
        ACSDK_DEBUG0(LX_CFG_KEY("getAndDecryptInternalSuccess", m_configUri, key));
        return true;
    }
}

bool EncryptedProperties::putBytes(const std::string& key, const Bytes& value) noexcept {
    if (key == KEY_PROPERTY_NAME) {
        ACSDK_ERROR(LX_CFG("putBytesFailed", m_configUri).m("propertyKeyForbidden"));
        return false;
    }
    if (encryptAndPutInternal(key, value)) {
        ACSDK_DEBUG0(LX_CFG_KEY("putBytesSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("putBytesFailed", m_configUri, key));
        return true;
    }
}

bool EncryptedProperties::encryptAndPutInternal(const std::string& key, const Bytes& plaintext) noexcept {
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Put, m_configUri};

    Bytes encodedCiphertext;
    auto res = executor.execute(
        "encryptAndPutInternal",
        [this, &key, &plaintext, &encodedCiphertext]() -> StatusCodeWithRetry {
            if (!encryptAndEncodePropertyValue(key, plaintext, encodedCiphertext)) {
                // If local cryptography API fails, it is a non-retryable error.
                // We still notify error callback.
                ACSDK_DEBUG0(LX_CFG_KEY("encryptPropertyFailed", m_configUri, key));
                return RetryExecutor::RETRYABLE_CRYPTO_ERROR;
            } else {
                ACSDK_DEBUG0(LX_CFG_KEY("encryptPropertySuccess", m_configUri, key));
                return RetryExecutor::SUCCESS;
            }
        },
        Action::FAIL);
    switch (res) {
        case RetryableOperationResult::Cleanup:
            if (deleteValueWithRetries(executor, key)) {
                ACSDK_DEBUG0(LX_CFG_KEY("encryptAndPutInternalCleanupSuccess", m_configUri, key));
                return true;
            } else {
                ACSDK_DEBUG0(LX_CFG_KEY("encryptAndPutInternalCleanupSuccessFailure", m_configUri, key));
                return false;
            }
        case RetryableOperationResult::Success:
            break;
        case RetryableOperationResult::Failure:  // fall through
        default:
            return false;
    }

    if (storeValueWithRetries(executor, key, encodedCiphertext, true)) {
        ACSDK_DEBUG0(LX_CFG_KEY("encryptAndPutInternalSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("encryptAndPutInternalFailure", m_configUri, key));
        return false;
    }
}

bool EncryptedProperties::storeValueWithRetries(
    RetryExecutor& executor,
    const std::string& key,
    const Bytes& value,
    bool canDrop) noexcept {
    auto result = executor.execute(
        "storeKeyValue",
        [this, &key, &value]() -> StatusCodeWithRetry {
            // Compute property with encrypted key and algorithm types.
            if (m_innerProperties->putBytes(key, value)) {
                ACSDK_DEBUG9(LX_CFG_KEY("putBytesSuccess", m_configUri, key));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG_KEY("putBytesSRetryableFailure", m_configUri, key));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::RETRY);

    if (RetryableOperationResult::Success == result) {
        ACSDK_DEBUG0(LX_CFG_KEY("storeKeyValueSuccess", m_configUri, key));
        return true;
    } else if (canDrop && RetryableOperationResult::Cleanup == result) {
        if (deleteValueWithRetries(executor, key)) {
            ACSDK_DEBUG0(LX_CFG_KEY("storeKeyDropValueSuccess", m_configUri, key));
            return true;
        } else {
            ACSDK_DEBUG0(LX_CFG_KEY("storeKeyDropValueError", m_configUri, key));
            return false;
        }
    } else {
        ACSDK_ERROR(LX_CFG_KEY("storeKeyValueError", m_configUri, key));
        return false;
    }
}

bool EncryptedProperties::loadValueWithRetries(RetryExecutor& executor, const std::string& key, Bytes& data) noexcept {
    return executeKeyOperationWithRetries(
        executor, "loadValue", key, [this, &key, &data]() -> bool { return m_innerProperties->getBytes(key, data); });
}

bool EncryptedProperties::deleteValueWithRetries(RetryExecutor& executor, const std::string& key) noexcept {
    return executeKeyOperationWithRetries(
        executor, "removeKey", key, [this, &key]() -> bool { return m_innerProperties->remove(key); });
}

bool EncryptedProperties::executeKeyOperationWithRetries(
    RetryExecutor& executor,
    const std::string& operationName,
    const std::string& key,
    const std::function<bool()>& operation) noexcept {
    auto res = executor.execute(
        operationName,
        [this, &operationName, &key, &operation]() -> StatusCodeWithRetry {
            // Suppress unused variable errors when the code is built with logging disabled.
            ACSDK_UNUSED_VARIABLE(this);
            ACSDK_UNUSED_VARIABLE(operationName);
            ACSDK_UNUSED_VARIABLE(key);
            if (!operation()) {
                ACSDK_DEBUG9(
                    LX_CFG_KEY("keyOperationRetryableFailure", m_configUri, key).d("operation", operationName));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            } else {
                ACSDK_DEBUG9(LX_CFG_KEY("keyOperationSuccess", m_configUri, key).d("operation", operationName));
                return RetryExecutor::SUCCESS;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success == res) {
        ACSDK_DEBUG0(LX_CFG_KEY("keyOperationSuccess", m_configUri, key).d("operation", operationName));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("keyOperationFailure", m_configUri, key).d("operation", operationName));
        return false;
    }
}

bool EncryptedProperties::clearAllValuesWithRetries(RetryExecutor& executor) noexcept {
    return executeKeyOperationWithRetries(
        executor, "clear", "", [this]() -> bool { return m_innerProperties->clear(); });
}

bool EncryptedProperties::clear() noexcept {
    RetryExecutor executor{OperationType::Put, m_configUri};
    if (doClear(executor)) {
        ACSDK_DEBUG0(LX_CFG("clearSuccess", m_configUri));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG("clearFailed", m_configUri));
        return false;
    }
}

bool EncryptedProperties::doClear(RetryExecutor& executor) noexcept {
    auto result = clearAllValuesWithRetries(executor);
    if (!result) {
        ACSDK_ERROR(LX_CFG("doClearFailed", m_configUri));
        return false;
    }

    auto statusCode = generateAndStoreDataKeyWithRetries(executor);

    if (StatusCode::SUCCESS != statusCode) {
        ACSDK_ERROR(LX_CFG("doClearStoreKeyFailed", m_configUri));
        return false;
    }

    ACSDK_DEBUG0(LX("doClearSuccess"));
    return true;
}

bool EncryptedProperties::remove(const std::string& key) noexcept {
    if (key == KEY_PROPERTY_NAME) {
        ACSDK_ERROR(LX("removeFailed").m("propertyKeyForbidden"));
        return false;
    }

    // Remove calls are considered put.
    RetryExecutor executor{OperationType::Put, m_configUri};
    if (deleteValueWithRetries(executor, key)) {
        ACSDK_DEBUG0(LX_CFG_KEY("removeSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("removeFailed", m_configUri, key));
        return false;
    }
}

bool EncryptedProperties::getKeys(std::unordered_set<std::string>& keys) noexcept {
    RetryExecutor executor{OperationType::Get, m_configUri};
    if (loadKeysWithRetries(executor, keys)) {
        ACSDK_DEBUG0(LX_CFG("getKeysSuccess", m_configUri));
        keys.erase(KEY_PROPERTY_NAME);
        return true;
    } else {
        ACSDK_ERROR(LX_CFG("getKeysFailed", m_configUri));
        return false;
    }
}

bool EncryptedProperties::loadKeysWithRetries(RetryExecutor& executor, std::unordered_set<std::string>& keys) noexcept {
    auto result = executor.execute(
        "getKeys",
        [this, &keys]() -> StatusCodeWithRetry {
            if (m_innerProperties->getKeys(keys)) {
                ACSDK_DEBUG9(LX_CFG("lgetKeysSuccess", m_configUri));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG("getKeysRetryableFailure", m_configUri));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success == result) {
        ACSDK_DEBUG0(LX_CFG("loadKeysSuccess", m_configUri));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG("loadKeysFailure", m_configUri));
        return false;
    }
}

bool EncryptedProperties::init() noexcept {
    if (!m_innerProperties) {
        ACSDK_ERROR(LX_CFG("initFailed", m_configUri).m("innerPropertiesNull"));
        return false;
    }
    if (!m_cryptoFactory) {
        ACSDK_ERROR(LX_CFG("initFailed", m_configUri).m("cryptoFactoryNull"));
        return false;
    }
    if (!m_keyStore) {
        ACSDK_ERROR(LX_CFG("initFailed", m_configUri).m("keyStoreNull"));
        return false;
    }
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Open, m_configUri};
    // Check if container is encrypted.
    // This operation checks if the encryption property is present. The operation is retryable.

    // Collection of existing keys
    std::unordered_set<std::string> keys;
    if (!loadKeysWithRetries(executor, keys)) {
        ACSDK_ERROR(LX_CFG("initFailed", m_configUri).m("getKeysFailed"));
        return false;
    }

    const bool encryptedStorage = keys.find(KEY_PROPERTY_NAME) != keys.end();

    StatusCode openStatus;
    if (encryptedStorage) {
        // Key is present, so the storage is encrypted.
        openStatus = loadAndDecryptDataKey(executor);
    } else {
        // key is not present - try to upgrade the storage
        openStatus = upgradeEncryption(executor, keys);
    }
    if (StatusCode::SUCCESS == openStatus) {
        ACSDK_DEBUG0(LX_CFG("initSuccess", m_configUri));
        return true;
    } else {
        // If we manage to cleanup - it is successful. If we fail to cleanup, config is unusable.
        if (doClear(executor)) {
            ACSDK_DEBUG0(LX_CFG("initSuccessDataCleanup", m_configUri));
            return true;
        } else {
            ACSDK_ERROR(LX_CFG("initFailed", m_configUri));
            return false;
        }
    }
}

StatusCode EncryptedProperties::loadAndDecryptDataKey(RetryExecutor& executor) noexcept {
    Bytes encryptionProperty;
    if (!loadValueWithRetries(executor, KEY_PROPERTY_NAME, encryptionProperty)) {
        ACSDK_DEBUG9(LX_CFG("loadDataKeyFailed", m_configUri));
        return StatusCode::INNER_PROPERTIES_ERROR;
    }

    // We need to decode and decrypt data key.
    // If decoding fails, or there is a digest error we can try to reload property.
    auto decryptStatus = decodeAndDecryptDataKey(encryptionProperty);
    if (StatusCode::SUCCESS == decryptStatus) {
        // Data key is available.
        return StatusCode::SUCCESS;
    } else {
        ACSDK_DEBUG9(LX_CFG("decryptDataKeyFailed", m_configUri));
        return decryptStatus;
    }
}

bool EncryptedProperties::generateDataKeyWithRetries(RetryExecutor& executor) noexcept {
    m_dataAlgorithmType = DEFAULT_ALGORITHM_FOR_PROPERTIES;

    auto result = executor.execute(
        "generateDataKeyWithRetries",
        [this]() -> StatusCodeWithRetry {
            m_dataKey.clear();
            auto keyFactory = m_cryptoFactory->getKeyFactory();
            if (!keyFactory) {
                ACSDK_WARN(LX("generateDataKeyWithRetriesFailed").m("keyFactoryNull"));
                return RetryExecutor::NON_RETRYABLE_CRYPTO_ERROR;
            }
            if (!keyFactory->generateKey(m_dataAlgorithmType, m_dataKey)) {
                ACSDK_WARN(
                    LX("generateDataKeyWithRetriesFailed").d("algorithmType", static_cast<int>(m_dataAlgorithmType)));
                return RetryExecutor::NON_RETRYABLE_CRYPTO_ERROR;
            }
            return {StatusCode::SUCCESS, false};
        },
        Action::RETRY);

    if (RetryableOperationResult::Success != result) {
        ACSDK_ERROR(LX("generateDataKeyFailed").d("algorithmType", static_cast<int>(m_dataAlgorithmType)));
        return false;
    }

    return true;
}

bool EncryptedProperties::encryptAndEncodeDataKeyWithRetries(RetryExecutor& executor, Bytes& encoded) noexcept {
    std::string mainKeyAlias;
    KeyChecksum mainKeyChecksum;
    AlgorithmType algorithmType;
    IV iv;
    DataBlock ciphertext;
    Tag tag;

    // Attempt to encrypt data key. By default, we try once, but error callback may override this action.
    auto result = executor.execute(
        "encryptAndEncodeDataKeyWithRetries",
        [this, &mainKeyAlias, &mainKeyChecksum, &algorithmType, &iv, &ciphertext, &tag]() -> StatusCodeWithRetry {
            mainKeyChecksum.clear();
            iv.clear();
            ciphertext.clear();
            tag.clear();
            if (!encryptDataKey(mainKeyAlias, algorithmType, mainKeyChecksum, iv, ciphertext, tag)) {
                ACSDK_ERROR(LX("encryptAndEncodeDataKeyFailed").m("failedToEncryptDataKey"));
                return RetryExecutor::RETRYABLE_HSM_ERROR;
            } else {
                return RetryExecutor::SUCCESS;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        return false;
    }

    // Attempt to encode data key. By default, we try once, but error callback may override this action.
    result = executor.execute(
        "encryptAndEncodeDataKeyWithRetries",
        [this, &mainKeyAlias, &mainKeyChecksum, &algorithmType, &iv, &ciphertext, &encoded, &tag]()
            -> StatusCodeWithRetry {
            encoded.clear();
            if (!EncryptionKeyPropertyCodec::encode(
                    m_cryptoFactory,
                    mainKeyAlias,
                    mainKeyChecksum,
                    algorithmType,
                    iv,
                    ciphertext,
                    tag,
                    m_dataAlgorithmType,
                    encoded)) {
                ACSDK_ERROR(LX("encryptAndEncodeDataKeyFailed").m("failedToDerEncode"));
                return RetryExecutor::RETRYABLE_HSM_ERROR;
            } else {
                return RetryExecutor::SUCCESS;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        return false;
    }

    return true;
}

StatusCode EncryptedProperties::decodeAndDecryptDataKey(const Bytes& encoded) noexcept {
    std::string mainKeyAlias;
    KeyChecksum mainKeyChecksum;
    AlgorithmType dataKeyAlg;
    IV dataKeyIV;
    DataBlock dataKeyCiphertext;
    DataBlock digest;
    DataBlock actualDigest;
    Tag dataKeyTag;

    if (!EncryptionKeyPropertyCodec::decode(
            m_cryptoFactory,
            encoded,
            mainKeyAlias,
            mainKeyChecksum,
            dataKeyAlg,
            dataKeyIV,
            dataKeyCiphertext,
            dataKeyTag,
            m_dataAlgorithmType,
            digest,
            actualDigest)) {
        ACSDK_ERROR(LX("decodeAndDecryptDataKeyFailed").m("failedToDecodeDER"));
        return StatusCode::UNKNOWN_ERROR;
    }

    if (digest != actualDigest) {
        ACSDK_ERROR(LX("decodeAndDecryptDataKey").m("failedToVerifyDataKeyDigest"));
        return StatusCode::DIGEST_ERROR;
    }

    if (decryptDataKey(mainKeyAlias, dataKeyAlg, mainKeyChecksum, dataKeyIV, dataKeyCiphertext, dataKeyTag)) {
        return StatusCode::SUCCESS;
    } else {
        ACSDK_ERROR(LX("decodeAndDecryptDataKey").m("failedToDecryptDataKey"));
        m_innerProperties->clear();
        return StatusCode::HSM_ERROR;
    }
}

bool EncryptedProperties::encryptDataKey(
    std::string& mainKeyAlias,
    AlgorithmType& algorithmType,
    KeyChecksum& mainKeyChecksum,
    IV& dataKeyIV,
    Bytes& dataKeyCiphertext,
    Tag& dataKeyTag) noexcept {
    if (!m_keyStore->getDefaultKeyAlias(mainKeyAlias)) {
        ACSDK_ERROR(LX("encryptDataKeyFailed").m("defaultKeyAliasError"));
        return false;
    }

    algorithmType = DEFAULT_ALGORITHM_FOR_KEYS;

    auto keyFactory = m_cryptoFactory->getKeyFactory();
    if (!keyFactory || !keyFactory->generateIV(algorithmType, dataKeyIV)) {
        ACSDK_ERROR(LX("encryptDataKeyFailed").m("dataKeyIVGenerateFailed"));
        return false;
    }

    CryptoCodecInterface::DataBlock aadBinary{
        reinterpret_cast<CryptoCodecInterface::DataBlock::const_pointer>(m_configUri.data()),
        reinterpret_cast<CryptoCodecInterface::DataBlock::const_pointer>(m_configUri.data()) + m_configUri.size()};

    mainKeyChecksum.clear();
    dataKeyCiphertext.clear();
    dataKeyTag.clear();
    if (!m_keyStore->encryptAE(
            mainKeyAlias,
            algorithmType,
            dataKeyIV,
            aadBinary,
            m_dataKey,
            mainKeyChecksum,
            dataKeyCiphertext,
            dataKeyTag)) {
        ACSDK_ERROR(LX("encryptDataKeyFailed").m("mainKeyEncryptionFailed"));
        return false;
    }

    return true;
}

bool EncryptedProperties::decryptDataKey(
    const std::string& mainKeyAlias,
    AlgorithmType dataKeyAlgorithm,
    const KeyChecksum& keyChecksum,
    const std::vector<uint8_t>& dataKeyIV,
    const std::vector<uint8_t>& keyCiphertext,
    const Tag& dataKeyTag) noexcept {
    CryptoCodecInterface::DataBlock aadBinary{
        reinterpret_cast<CryptoCodecInterface::DataBlock::const_pointer>(m_configUri.data()),
        reinterpret_cast<CryptoCodecInterface::DataBlock::const_pointer>(m_configUri.data()) + m_configUri.size()};
    m_dataKey.clear();
    if (!m_keyStore->decryptAD(
            mainKeyAlias, dataKeyAlgorithm, keyChecksum, dataKeyIV, aadBinary, keyCiphertext, dataKeyTag, m_dataKey)) {
        ACSDK_ERROR(LX_CFG("decryptDataKeyFailed", m_configUri));
        return false;
    }

    return true;
}

StatusCode EncryptedProperties::generateAndStoreDataKeyWithRetries(RetryExecutor& executor) noexcept {
    if (!generateDataKeyWithRetries(executor)) {
        ACSDK_ERROR(LX_CFG("generateAndStoreDataKeyWithRetriesFailed", m_configUri));
        return StatusCode::CRYPTO_ERROR;
    }

    Bytes dataKeyPropertyValue;
    if (!encryptAndEncodeDataKeyWithRetries(executor, dataKeyPropertyValue)) {
        ACSDK_ERROR(LX_CFG("generateAndStoreDataKeyWithRetriesFailed", m_configUri));
        return StatusCode::HSM_ERROR;
    }

    if (!storeValueWithRetries(executor, KEY_PROPERTY_NAME, dataKeyPropertyValue, true)) {
        ACSDK_ERROR(LX_CFG("generateAndStoreDataKeyWithRetriesFailed", m_configUri));
        return StatusCode::INNER_PROPERTIES_ERROR;
    }

    ACSDK_DEBUG0(LX_CFG("generateAndStoreDataKeyWithRetriesSuccess", m_configUri));
    return StatusCode::SUCCESS;
}

StatusCode EncryptedProperties::upgradeEncryption(
    RetryExecutor& executor,
    const std::unordered_set<std::string>& keys) noexcept {
    std::unordered_map<std::string, Bytes> unencryptedValues;
    // Load unencrypted properties
    for (auto it = keys.cbegin(); it != keys.cend(); ++it) {
        Bytes binary;

        // When loading unencrypted properties the API tries to load value as a string, and then as a binary.
        // This sequence is for a better compatibility when upgrading unencrypted data into encrypted form,
        // as binary data can be mapped into Base64 textual form, and it is possible to get false positives
        // when requesting data as binary.

        const auto& key = *it;

        auto result = executor.execute(
            "upgradeEncryptionLoadKey",
            [this, &binary, &key, &unencryptedValues]() -> StatusCodeWithRetry {
                std::string string;
                if (m_innerProperties->getString(key, string)) {
                    ACSDK_DEBUG9(LX("upgradeEncryptionString").d("key", key));
                    binary.assign(
                        reinterpret_cast<Bytes::const_pointer>(string.c_str()),
                        reinterpret_cast<Bytes::const_pointer>(string.c_str()) + string.size());
                    unencryptedValues.insert({key, binary});
                    return RetryExecutor::SUCCESS;
                }
                binary.clear();
                if (m_innerProperties->getBytes(key, binary)) {
                    ACSDK_DEBUG9(LX("upgradeEncryptionBinary").d("key", key));
                    unencryptedValues.insert({key, binary});
                    return {StatusCode::SUCCESS, false};
                }
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            },
            Action::RETRY);

        if (RetryableOperationResult::Success != result) {
            ACSDK_WARN(LX("upgradeEncryptionEntryLost").d("key", key));
        }
    }

    if (!clearAllValuesWithRetries(executor)) {
        return StatusCode::INNER_PROPERTIES_ERROR;
    }

    auto statusCode = generateAndStoreDataKeyWithRetries(executor);

    if (StatusCode::SUCCESS != statusCode) {
        return statusCode;
    }

    // Store encrypted properties
    for (auto it = unencryptedValues.cbegin(); it != unencryptedValues.cend(); ++it) {
        ACSDK_DEBUG9(LX_CFG_KEY("upgradeEncryptionStoreCiphertext", m_configUri, it->first));
        Bytes encoded;
        encryptAndEncodePropertyValue(it->first, it->second, encoded);
        storeValueWithRetries(executor, it->first, encoded, false);
    }

    return StatusCode::SUCCESS;
}

bool EncryptedProperties::encryptAndEncodePropertyValue(
    const std::string& key,
    const Bytes& plaintext,
    Bytes& encodedCiphertext) noexcept {
    // Crypto Encoder
    const auto cipher = m_cryptoFactory->createEncoder(m_dataAlgorithmType);
    if (!cipher) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("createEncoderFailed"));
        return false;
    }

    // Initialization vector.
    IV iv;
    // Encrypted value.
    DataBlock ciphertext;
    // Tag for ADAD algorithms or authentication code for others.
    Tag tag;

    auto keyFactory = m_cryptoFactory->getKeyFactory();

    if (!keyFactory || !keyFactory->generateIV(m_dataAlgorithmType, iv)) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("ivGenerateFailed"));
        return false;
    }
    if (!cipher->init(m_dataKey, iv)) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("cipherInitFailed"));
        return false;
    }

    if (!cipher->processAAD(DataBlock{key.data(), key.data() + key.size()})) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("processAADError"));
        return false;
    }

    CryptoCodecInterface::DataBlock binaryPlaintext;
    binaryPlaintext.assign(plaintext.data(), plaintext.data() + plaintext.size());

    if (!cipher->process(binaryPlaintext, ciphertext)) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("cipherUpdateFailed"));
        return false;
    }

    if (!cipher->finalize(ciphertext)) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("cipherFinalizeFailed"));
        return false;
    }

    if (!cipher->getTag(tag)) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("getTagFailed"));
        return false;
    }

    if (!DataPropertyCodec::encode(m_cryptoFactory, iv, ciphertext, tag, encodedCiphertext)) {
        ACSDK_ERROR(LX("encryptAndEncodePropertyValueFailed").m("derEncodeFailed"));
        return false;
    }

    return true;
}

bool EncryptedProperties::decodeAndDecryptPropertyValue(
    const std::string& key,
    const Bytes& encodedCiphertext,
    Bytes& plaintext) noexcept {
    CryptoCodecInterface::IV iv;
    CryptoCodecInterface::DataBlock ciphertext;
    CryptoCodecInterface::Tag tag;
    DigestInterface::DataBlock digest;
    DigestInterface::DataBlock actualDigest;

    if (!DataPropertyCodec::decode(m_cryptoFactory, encodedCiphertext, iv, ciphertext, tag, digest, actualDigest)) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("propertyValueDerDecodingFailed"));
        return false;
    }
    if (digest != actualDigest) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("propertyValueDigestCheckFailed"));
        return false;
    }
    auto codec = m_cryptoFactory->createDecoder(m_dataAlgorithmType);
    if (!codec) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("decoderCreateFailed"));
        return false;
    }
    if (!codec->init(m_dataKey, iv)) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("decoderInitFailed"));
        return false;
    }
    DataBlock aad{key.data(), key.data() + key.size()};
    if (!codec->processAAD(aad)) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("decoderProcessFailed"));
        return false;
    }
    if (!codec->process(ciphertext, plaintext)) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("decoderProcessFailed"));
        return false;
    }
    if (!codec->setTag(tag)) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("decoderSetTagFailed"));
        return false;
    }
    if (!codec->finalize(plaintext)) {
        ACSDK_ERROR(LX("decodeAndDecryptPropertyValue").m("decoderFinalizeFailed"));
        return false;
    }

    return true;
}

}  // namespace properties
}  // namespace alexaClientSDK
