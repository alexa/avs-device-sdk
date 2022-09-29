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

#include <climits>

#include <acsdk/Pkcs11/private/Logging.h>
#include <acsdk/Pkcs11/private/PKCS11Key.h>
#include <acsdk/Pkcs11/private/PKCS11Session.h>
#include <acsdk/Pkcs11/private/PKCS11Slot.h>
#include <acsdk/Pkcs11/private/PKCS11Functions.h>

namespace alexaClientSDK {
namespace pkcs11 {

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::Key"

using namespace alexaClientSDK::cryptoInterfaces;

/// @private
static constexpr size_t AES_CBC_IV_SIZE = 16;
/// @private
static constexpr size_t AES_256_KEY_SIZE = 32;
/// @private
static constexpr size_t AES_128_KEY_SIZE = 16;
/// @private
static constexpr size_t AES_GCM_TAG_SIZE = 16;
/// @private
static constexpr size_t AES_GCM_IV_SIZE = 12;

PKCS11Key::PKCS11Key(std::shared_ptr<PKCS11Session>&& session, CK_OBJECT_HANDLE keyHandle) noexcept :
        m_session(std::move(session)),
        m_keyHandle(keyHandle) {
}

bool PKCS11Key::isCompatible(AlgorithmType type) noexcept {
    CK_OBJECT_CLASS actualObjectClass = UNDEFINED_OBJECT_CLASS;
    CK_KEY_TYPE actualKeyType = UNDEFINED_KEY_TYPE;
    CK_ULONG keyLengthBytes = 0;

    CK_ATTRIBUTE dataTemplate[] = {
        {CKA_CLASS, &actualObjectClass, sizeof(actualObjectClass)},
        {CKA_KEY_TYPE, &actualKeyType, sizeof(actualKeyType)},
        {CKA_VALUE_LEN, &keyLengthBytes, sizeof(keyLengthBytes)},
    };

    std::lock_guard<std::mutex> lock(m_session->m_mutex);

    CK_RV rv = m_session->m_functions->m_pkcs11Functions->C_GetAttributeValue(
        m_session->m_sessionHandle, m_keyHandle, dataTemplate, sizeof(dataTemplate) / sizeof(dataTemplate[0]));

    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("getAttributeValueFailed").d("CK_RV", rv));
        return false;
    }

    ACSDK_INFO(LX("foundObject")
                   .d("objectClass", actualObjectClass)
                   .d("keyType", actualKeyType)
                   .d("valueLen", keyLengthBytes));

    if (CKO_SECRET_KEY != actualObjectClass) {
        ACSDK_ERROR(LX("objectClassMismatch").d("objectClass", actualObjectClass));
        return false;
    }

    CK_KEY_TYPE expectedKeyType;
    CK_ULONG expectedKeySize;

    switch (type) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_256_GCM:
            expectedKeyType = CKK_AES;
            expectedKeySize = AES_256_KEY_SIZE;
            break;
        case AlgorithmType::AES_128_CBC:
        case AlgorithmType::AES_128_CBC_PAD:
        case AlgorithmType::AES_128_GCM:
            expectedKeyType = CKK_AES;
            expectedKeySize = AES_128_KEY_SIZE;
            break;
        default:
            return false;
    }

    if (expectedKeyType != actualKeyType) {
        ACSDK_ERROR(LX("keyTypeMismatch").d("keyType", actualKeyType));
        return false;
    }
    if (expectedKeySize != keyLengthBytes) {
        ACSDK_ERROR(LX("keySizeMismatch").d("keySize", keyLengthBytes));
        return false;
    }

    return true;
}

bool PKCS11Key::getAttributes(std::vector<unsigned char>& checksum, bool& neverExtractable) noexcept {
    CK_BYTE actualChecksum[3] = {0, 0, 0};
    CK_BBOOL actualNeverExtractable = CK_FALSE;
    CK_ATTRIBUTE dataTemplate[] = {
        {CKA_CHECK_VALUE, &actualChecksum, sizeof(actualChecksum)},
        {CKA_NEVER_EXTRACTABLE, &actualNeverExtractable, sizeof(actualNeverExtractable)},
    };

    std::lock_guard<std::mutex> lock(m_session->m_mutex);

    CK_RV rv = m_session->m_functions->m_pkcs11Functions->C_GetAttributeValue(
        m_session->m_sessionHandle, m_keyHandle, dataTemplate, sizeof(dataTemplate) / sizeof(dataTemplate[0]));

    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("getAttributeValueFailed").d("CK_RV", rv));
        return false;
    }

    size_t checksumOffset = checksum.size();
    checksum.reserve(checksum.size() + checksumOffset);
    checksum.insert(checksum.end(), actualChecksum, actualChecksum + sizeof(actualChecksum));
    neverExtractable = actualNeverExtractable != CK_FALSE;

    return true;
}

bool PKCS11Key::encrypt(
    AlgorithmType algorithmType,
    const std::vector<unsigned char>& iv,
    const std::vector<unsigned char>& aad,
    const std::vector<unsigned char>& plaintext,
    std::vector<unsigned char>& ciphertext,
    std::vector<unsigned char>& tag) noexcept {
    std::lock_guard<std::mutex> lock(m_session->m_mutex);

    size_t ivSize = 0;
    CK_MECHANISM_TYPE mechanismType = 0;
    bool useGcm = false;

    switch (algorithmType) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
            mechanismType = CKM_AES_CBC;
            ivSize = AES_CBC_IV_SIZE;
            break;
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            mechanismType = CKM_AES_CBC_PAD;
            ivSize = AES_CBC_IV_SIZE;
            break;
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            mechanismType = CKM_AES_GCM;
            ivSize = AES_GCM_IV_SIZE;
            useGcm = true;
            break;
        default:
            ACSDK_ERROR(LX("algorithmTypeError").d("algorithmType", algorithmType));
            return false;
    }
    if (iv.size() != ivSize) {
        ACSDK_ERROR(LX("ivSizeError").d("size", iv.size()));
        return false;
    }

    CK_MECHANISM mechanism;
    CK_GCM_PARAMS gcmParams;

    if (!configureMechanism(mechanismType, iv, aad, mechanism, gcmParams)) {
        ACSDK_ERROR(LX("encryptFailed").d("reason", "configureError"));
        return false;
    }

    CK_RV rv;

    rv = m_session->m_functions->m_pkcs11Functions->C_EncryptInit(m_session->m_sessionHandle, &mechanism, m_keyHandle);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("encryptInitFailed").d("RC_RV", rv));
        return false;
    }

    CK_ULONG ulEncryptedDataLen = 0;
    rv = m_session->m_functions->m_pkcs11Functions->C_Encrypt(
        m_session->m_sessionHandle,
        const_cast<CK_BYTE_PTR>(plaintext.data()),
        static_cast<CK_LONG>(plaintext.size()),
        nullptr,
        &ulEncryptedDataLen);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("encryptGetSizeFailed").d("RC_RV", rv));
        return false;
    }

    size_t ciphertextOffset = ciphertext.size();
    ciphertext.resize(ciphertextOffset + ulEncryptedDataLen);

    rv = m_session->m_functions->m_pkcs11Functions->C_Encrypt(
        m_session->m_sessionHandle,
        (CK_BYTE_PTR)plaintext.data(),
        static_cast<CK_ULONG>(plaintext.size()),
        static_cast<CK_BYTE_PTR>(ciphertext.data()) + ciphertextOffset,
        &ulEncryptedDataLen);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("encryptFailed").d("RC_RV", rv));
        return false;
    }

    if (useGcm && &ciphertext != &tag) {
        // ciphertext and tag may be the same reference. If they are not, then remove tag into own reference.

        if (ciphertext.size() < AES_GCM_TAG_SIZE) {
            ACSDK_ERROR(LX("encryptFailed").d("reason", "tooSmallCiphertext"));
            return false;
        }

        size_t tagOffset = tag.size();
        tag.reserve(tagOffset + AES_GCM_TAG_SIZE);
        tag.insert(tag.end(), ciphertext.end() - AES_GCM_TAG_SIZE, ciphertext.end());
        ciphertext.resize(ciphertext.size() - AES_GCM_TAG_SIZE);
    }

    return true;
}

bool PKCS11Key::decrypt(
    AlgorithmType algorithmType,
    const std::vector<unsigned char>& iv,
    const std::vector<unsigned char>& aad,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& tag,
    std::vector<unsigned char>& plaintext) noexcept {
    std::lock_guard<std::mutex> lock(m_session->m_mutex);

    size_t ivSize = 0;
    CK_MECHANISM_TYPE mechanismType = 0;
    bool useGcm = false;

    switch (algorithmType) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
            mechanismType = CKM_AES_CBC;
            ivSize = AES_CBC_IV_SIZE;
            break;
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            mechanismType = CKM_AES_CBC_PAD;
            ivSize = AES_CBC_IV_SIZE;
            break;
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            mechanismType = CKM_AES_GCM;
            ivSize = AES_GCM_IV_SIZE;
            useGcm = true;
            break;
        default:
            ACSDK_ERROR(LX("algorithmTypeError").d("algorithmType", algorithmType));
            return false;
    }
    if (iv.size() != ivSize) {
        ACSDK_ERROR(LX("ivSizeError").d("size", iv.size()));
        return false;
    }
    CK_MECHANISM mechanism = {mechanismType, const_cast<CK_BYTE_PTR>(iv.data()), static_cast<CK_ULONG>(iv.size())};
    CK_GCM_PARAMS gcmParams;

    if (!configureMechanism(mechanismType, iv, aad, mechanism, gcmParams)) {
        ACSDK_ERROR(LX("decryptFailed").d("reason", "configureError"));
        return false;
    }

    CK_BYTE_PTR ciphertextPtr;
    CK_ULONG ciphertextSize;
    std::vector<unsigned char> gcmTmp;

    if (useGcm) {
        gcmTmp.reserve(ciphertext.size() + tag.size());
        gcmTmp.insert(gcmTmp.end(), ciphertext.cbegin(), ciphertext.cend());
        gcmTmp.insert(gcmTmp.end(), tag.cbegin(), tag.cend());
        ciphertextPtr = const_cast<CK_BYTE_PTR>(gcmTmp.data());
        ciphertextSize = static_cast<CK_ULONG>(gcmTmp.size());
    } else {
        ciphertextPtr = const_cast<CK_BYTE_PTR>(ciphertext.data());
        ciphertextSize = static_cast<CK_ULONG>(ciphertext.size());
    }

    CK_ULONG decryptedSize = 0;
    CK_RV rv;

    rv = m_session->m_functions->m_pkcs11Functions->C_DecryptInit(m_session->m_sessionHandle, &mechanism, m_keyHandle);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("decryptInitFailed").d("CK_RV", rv));
        return false;
    }

    rv = m_session->m_functions->m_pkcs11Functions->C_Decrypt(
        m_session->m_sessionHandle, ciphertextPtr, ciphertextSize, nullptr, &decryptedSize);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("decryptGetSizeFailed").d("CK_RV", rv));
        return false;
    }

    size_t plaintextOffset = plaintext.size();
    plaintext.resize(plaintextOffset + decryptedSize);

    rv = m_session->m_functions->m_pkcs11Functions->C_Decrypt(
        m_session->m_sessionHandle,
        ciphertextPtr,
        ciphertextSize,
        static_cast<CK_BYTE_PTR>(plaintext.data()) + plaintextOffset,
        &decryptedSize);

    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("decryptFailed").d("CK_RV", rv));
        return false;
    }

    plaintext.resize(plaintextOffset + decryptedSize);

    return true;
}

bool PKCS11Key::configureMechanism(
    CK_MECHANISM_TYPE mechanismType,
    const std::vector<unsigned char>& iv,
    const std::vector<unsigned char>& aad,
    CK_MECHANISM& mechanism,
    CK_GCM_PARAMS& gcmParams) noexcept {
    std::memset(&mechanism, 0, sizeof(mechanism));
    if (CKM_AES_GCM == mechanismType) {
        mechanism.mechanism = mechanismType;
        mechanism.pParameter = &gcmParams;
        mechanism.ulParameterLen = static_cast<CK_ULONG>(sizeof(gcmParams));

        std::memset(&gcmParams, 0, sizeof(gcmParams));
        gcmParams.pIv = const_cast<CK_BYTE_PTR>(iv.data());
        gcmParams.ulIvLen = static_cast<CK_ULONG>(iv.size());
        // Do not set ulIvBits as it not specified in API
        // gcmParams.ulIvBits = iv.size() * CHAR_BIT;
        gcmParams.pAAD = const_cast<CK_BYTE_PTR>(aad.data());
        gcmParams.ulAADLen = static_cast<CK_ULONG>(aad.size());
        gcmParams.ulTagBits = static_cast<CK_ULONG>(AES_GCM_TAG_SIZE) * CHAR_BIT;
    } else {
        if (!aad.empty()) {
            ACSDK_ERROR(LX("configureMechanismError").d("reason", "aadNotEmpty").d("mechanismType", mechanismType));
            return false;
        }

        mechanism.mechanism = mechanismType;
        mechanism.pParameter = const_cast<CK_BYTE_PTR>(iv.data());
        mechanism.ulParameterLen = static_cast<CK_ULONG>(iv.size());
    }

    return true;
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
