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

#include <acsdk/Pkcs11/private/Logging.h>
#include <acsdk/Pkcs11/private/ErrorCleanupGuard.h>
#include <acsdk/Pkcs11/private/PKCS11KeyStore.h>
#include <acsdk/Pkcs11/private/PKCS11Config.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>

namespace alexaClientSDK {
namespace pkcs11 {

using namespace alexaClientSDK::cryptoInterfaces;
using namespace alexaClientSDK::avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::KeyStore"

/// The activity name for encryption.
/// @private
static const std::string ACTIVITY_ENCRYPT{"PKCS11-ENCRYPT"};

/// The activity name for decryption.
/// @private
static const std::string ACTIVITY_DECRYPT{"PKCS11-DECRYPT"};

/// Metric dimension for checksum errors.
/// @private
static const std::string CHECKSUM_ERROR{"CHECKSUM_ERROR"};

/// Metric dimension for get key errors.
/// @private
static const std::string GET_KEY_ERROR{"GET_KEY_ERROR"};

/// Metric dimension for get checksum errors.
/// @private
static const std::string GET_CHECKSUM_ERROR{"GET_CHECKSUM_ERROR"};

/// Metric dimension for decrypt errors.
/// @private
static const std::string DECRYPT_ERROR{"DECRYPT_ERROR"};

/// Metric dimension for encrypt errors.
/// @private
static const std::string ENCRYPT_ERROR{"ENCRYPT_ERROR"};

/// Metric dimension for failure metrics.
/// @private
static const std::string FAILURE{"FAILURE"};

/// Metric dimension for extractable key metrics.
/// @private
static const std::string EXTRACTABLE_KEY{"EXTRACTABLE_KEY"};

std::shared_ptr<KeyStoreInterface> PKCS11KeyStore::create(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder) noexcept {
    ACSDK_INFO(LX("create"));
    auto res = std::shared_ptr<PKCS11KeyStore>(new PKCS11KeyStore(metricRecorder));
    if (!res->init()) {
        ACSDK_ERROR(LX("createFailed"));
        res.reset();
    }
    return res;
}

PKCS11KeyStore::PKCS11KeyStore(const std::shared_ptr<MetricRecorderInterface>& metricRecorder) noexcept :
        m_metricRecorder{metricRecorder} {
}

bool PKCS11KeyStore::init() noexcept {
    auto config = PKCS11Config::create();
    if (!config) {
        ACSDK_ERROR(LX("configNull"));
        return false;
    }
    m_defaultKeyAlias = config->getDefaultKeyName();

    m_functions = PKCS11Functions::create(config->getLibraryPath());
    if (!m_functions) {
        ACSDK_ERROR(LX("functionsLoadFailed"));
        return false;
    }
    bool success = false;
    ErrorCleanupGuard cleanupFunctions{success, [this]() -> void { m_functions.reset(); }};

    std::shared_ptr<PKCS11Slot> slot;
    if (!m_functions->findSlotByTokenName(config->getTokenName(), slot)) {
        ACSDK_ERROR(LX("slotLookupFailed"));
        return false;
    }
    if (!slot) {
        ACSDK_ERROR(LX("slotIsNotFound"));
        return false;
    }

    m_session = slot->openSession();
    if (!m_session) {
        ACSDK_ERROR(LX("openSessionFailed"));
        return false;
    }

    ErrorCleanupGuard cleanupSession{success, [this]() -> void { m_session.reset(); }};

    if (!m_session->logIn(config->getUserPin())) {
        ACSDK_ERROR(LX("logInFailed"));
        return false;
    }
    success = true;
    return success;
}

PKCS11KeyStore::~PKCS11KeyStore() noexcept {
}

std::shared_ptr<PKCS11Key> PKCS11KeyStore::loadKey(const std::string& objectLabel, AlgorithmType type) noexcept {
    std::lock_guard<std::mutex> keyAccessLock(m_keysMutex);
    return loadKeyLocked({objectLabel, type});
}

std::shared_ptr<PKCS11Key> PKCS11KeyStore::loadKeyLocked(PKCS11KeyDescriptor&& descriptor) noexcept {
    PKCS11KeyDescriptor localDescriptor{std::move(descriptor)};
    auto keyRef = m_keys.find(localDescriptor);
    if (keyRef == m_keys.end()) {
        auto key = m_session->findKey(localDescriptor);
        if (!key) {
            ACSDK_ERROR(LX("loadKeyLockedFailed").sensitive("descriptor", localDescriptor));
            return nullptr;
        }
        ACSDK_DEBUG0(LX("loadKeyLockedSuccess").sensitive("descriptor", localDescriptor));

        std::shared_ptr<PKCS11Key> result = std::move(key);
        m_keys[std::move(localDescriptor)] = result;
        return result;
    } else {
        return keyRef->second;
    }
}

bool PKCS11KeyStore::encrypt(
    const std::string& keyAlias,
    AlgorithmType type,
    const IV& iv,
    const DataBlock& plaintext,
    KeyChecksum& checksum,
    DataBlock& ciphertext) noexcept {
    Tag tag;
    DataBlock aad;

    return encryptAE(keyAlias, type, iv, aad, plaintext, checksum, ciphertext, tag);
}

bool PKCS11KeyStore::encryptAE(
    const std::string& keyAlias,
    AlgorithmType type,
    const IV& iv,
    const DataBlock& aad,
    const DataBlock& plaintext,
    KeyChecksum& checksum,
    DataBlock& ciphertext,
    Tag& tag) noexcept {
    auto key = loadKey(keyAlias, type);
    if (!key) {
        ACSDK_ERROR(LX("keyIsNotLoaded").sensitive("keyAlias", keyAlias));
        submitMetric(ACTIVITY_ENCRYPT, GET_KEY_ERROR, 1, true);
        return false;
    }

    bool neverExtractable;
    if (!key->getAttributes(checksum, neverExtractable)) {
        ACSDK_ERROR(LX("encryptFailed").sensitive("keyAlias", keyAlias).d("reason", "getAttributesFailed"));
        submitMetric(ACTIVITY_ENCRYPT, GET_CHECKSUM_ERROR, 1, true);
        return false;
    }

    if (!neverExtractable) {
        ACSDK_WARN(LX("encryptInsecure").sensitive("keyAlias", keyAlias).d("reason", "keyWasExtractable"));
        submitMetric(ACTIVITY_ENCRYPT, EXTRACTABLE_KEY, 1, false);
    }

    if (!key->encrypt(type, iv, aad, plaintext, ciphertext, tag)) {
        submitMetric(ACTIVITY_ENCRYPT, ENCRYPT_ERROR, 1, true);
        return false;
    }

    return true;
}

bool PKCS11KeyStore::decrypt(
    const std::string& keyAlias,
    AlgorithmType type,
    const KeyChecksum& checksum,
    const IV& iv,
    const DataBlock& ciphertext,
    DataBlock& plaintext) noexcept {
    Tag tag;
    DataBlock aad;
    return decryptAD(keyAlias, type, checksum, iv, aad, ciphertext, tag, plaintext);
}

bool PKCS11KeyStore::decryptAD(
    const std::string& keyAlias,
    AlgorithmType type,
    const KeyChecksum& checksum,
    const IV& iv,
    const DataBlock& aad,
    const DataBlock& ciphertext,
    const Tag& tag,
    DataBlock& plaintext) noexcept {
    auto key = loadKey(keyAlias, type);
    if (!key) {
        ACSDK_ERROR(LX("keyIsNotLoaded").sensitive("keyAlias", keyAlias));
        submitMetric(ACTIVITY_DECRYPT, GET_KEY_ERROR, 1, true);
        return false;
    }
    KeyChecksum keyChecksum;
    bool neverExtractable = false;
    if (!key->getAttributes(keyChecksum, neverExtractable)) {
        ACSDK_ERROR(LX("decryptFailed").sensitive("keyAlias", keyAlias).d("reason", "getChecksumFailed"));
        submitMetric(ACTIVITY_DECRYPT, GET_CHECKSUM_ERROR, 1, true);
        return false;
    }
    if (!neverExtractable) {
        ACSDK_WARN(LX("encryptInsecure").sensitive("keyAlias", keyAlias).d("reason", "keyWasExtractable"));
        submitMetric(ACTIVITY_DECRYPT, EXTRACTABLE_KEY, 1, false);
    }
    if (checksum != keyChecksum) {
        ACSDK_ERROR(LX("decryptFailed").sensitive("keyAlias", keyAlias).d("reason", "keyChecksumMismatch"));
        submitMetric(ACTIVITY_DECRYPT, CHECKSUM_ERROR, 1, true);
        return false;
    }

    if (!key->decrypt(type, iv, aad, ciphertext, tag, plaintext)) {
        submitMetric(ACTIVITY_DECRYPT, DECRYPT_ERROR, 1, true);
        return false;
    }

    return true;
}

void PKCS11KeyStore::submitMetric(
    const std::string& activity,
    const std::string& eventName,
    uint64_t count,
    bool failure) noexcept {
    if (!m_metricRecorder) {
        return;
    }

    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(activity)
                           .addDataPoint(DataPointCounterBuilder{}.setName(eventName).increment(count).build())
                           .addDataPoint(DataPointCounterBuilder{}.setName(FAILURE).increment(failure ? 1 : 0).build())
                           .build();

    if (!metricEvent) {
        ACSDK_ERROR(LX("submitMetricFailed").d("reason", "metricEventNull"));
        return;
    }

    recordMetric(m_metricRecorder, metricEvent);
}

bool PKCS11KeyStore::getDefaultKeyAlias(std::string& keyAlias) noexcept {
    keyAlias = m_defaultKeyAlias;
    return true;
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
