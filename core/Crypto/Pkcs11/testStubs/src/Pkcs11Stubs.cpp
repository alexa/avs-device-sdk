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

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <AVSCommon/Utils/Logger/Logger.h>

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 * @ingroup Lib_acsdkPkcs11Stubs
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// @addtogroup Lib_acsdkPkcs11Stubs
/// @{
// Define platform-specific macros for PKCS#11 API (UNIX)
#define CK_PTR *
#define CK_DECLARE_FUNCTION(returnType, name) returnType name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) returnType(*name)
#define CK_CALLBACK_FUNCTION(returnType, name) returnType(*name)
#define NULL_PTR nullptr
/// @}

#ifdef _WIN32
#pragma pack(push, cryptoki, 1)
#endif
#include <pkcs11.h>
#ifdef _WIN32
#pragma pack(pop, cryptoki)
#endif

#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/Crypto/CryptoFactory.h>

using namespace ::alexaClientSDK::cryptoInterfaces;

/// @addtogroup Lib_acsdkPkcs11Stubs
/// @{

/// @private
#define TAG "pkcs11:HSMStub"

/**
 * @brief PKCS11 function list table.
 *
 * This table is returned to PKCS11 client from @a C_Initialize call.
 */
static CK_FUNCTION_LIST FUNCTION_LIST{
    {2, 40},
    C_Initialize,         // C_Initialize
    C_Finalize,           // C_Finalize
    nullptr,              // C_GetInfo
    C_GetFunctionList,    // C_GetFunctionList
    C_GetSlotList,        // C_GetSlotList
    nullptr,              // C_GetSlotInfo
    C_GetTokenInfo,       // C_GetTokenInfo
    nullptr,              // C_GetMechanismList
    nullptr,              // C_GetMechanismInfo
    nullptr,              // C_InitToken
    nullptr,              // C_InitPin
    nullptr,              // C_SetPin
    C_OpenSession,        // C_OpenSession
    C_CloseSession,       // C_CloseSession
    nullptr,              // C_CloseAllSessions
    nullptr,              // C_GetSessionInfo
    nullptr,              // C_GetOperationState
    nullptr,              // C_SetOperationState
    C_Login,              // C_Login
    C_Logout,             // C_Logout
    nullptr,              // C_CreateObject
    nullptr,              // C_CopyObject
    nullptr,              // C_DestroyObject
    nullptr,              // C_GetObjectSize
    C_GetAttributeValue,  // C_GetAttributeValue
    nullptr,              // C_SetAttributeValue
    C_FindObjectsInit,    // C_FindObjectsInit
    C_FindObjects,        // C_FindObjects
    C_FindObjectsFinal,   // C_FindObjectsFinal
    C_EncryptInit,        // C_EncryptInit
    C_Encrypt,            // C_Encrypt
    nullptr,              // C_EncryptUpdate
    nullptr,              // C_EncryptFinal
    C_DecryptInit,        // C_DecryptInit
    C_Decrypt,            // C_Decrypt
    nullptr,              // C_DecryptUpdate
    nullptr,              // C_DecryptFinal
    nullptr,              // C_DigestInit
    nullptr,              // C_Digest
    nullptr,              // C_DigestUpdate
    nullptr,              // C_DigestKey
    nullptr,              // C_DigestFinal
    nullptr,              // C_SignInit
    nullptr,              // C_Sign
    nullptr,              // C_SignUpdate
    nullptr,              // C_SignFinal
    nullptr,              // C_SignRecoverInit
    nullptr,              // C_SignRecover
    nullptr,              // C_VerifyInit
    nullptr,              // C_Verify
    nullptr,              // C_VerifyUpdate
    nullptr,              // C_VerifyFinal
    nullptr,              // C_VerifyRecoverInit
    nullptr,              // C_VerifyRecover
    nullptr,              // C_DigestEncryptUpdate
    nullptr,              // C_DecryptDigestUpdate
    nullptr,              // C_SignEncryptUpdate
    nullptr,              // C_DecryptVerifyUpdate
    nullptr,              // C_GenerateKey
    nullptr,              // C_GenerateKeyPair
    nullptr,              // C_WrapKey
    nullptr,              // C_UnwrapKey
    nullptr,              // C_DeriveKey
    nullptr,              // C_SeedRandom
    nullptr,              // C_GenerateRandom
    nullptr,              // C_GetFunctionStatus
    nullptr,              // C_CancelFunction
    nullptr,              // C_WaitForSlotEvent
};

/// Constant to indicate unspecified value for object class attribute.
static constexpr CK_OBJECT_CLASS UNSPECIFIED_OBJECT_CLASS = (CK_OBJECT_CLASS)-1;
/// Constant to indicate unspecified value for key type attribute.
static constexpr CK_KEY_TYPE UNSPECIFIED_KEY_TYPE = (CK_KEY_TYPE)-1;
/// Constant to indicate unspecified value for value length attribute.
static constexpr CK_ULONG UNSPECIFIED_VALUE_LEN = (CK_ULONG)-1;
/// Default slot id.
static constexpr CK_SLOT_ID DEFAULT_SLOT_ID = 1;
/// AES256 key object handle.
static constexpr CK_OBJECT_HANDLE AES256_KEY_OBJECT_HANDLE = 2;
/// AES128 key object handle.
static constexpr CK_OBJECT_HANDLE AES128_KEY_OBJECT_HANDLE = 3;
/// AES block size in bytes.
static constexpr CK_ULONG AES_BLOCK_SIZE = 16;
/// AES GCM tag size in bytes.
static constexpr int AES_GCM_TAG_SIZE = 16;
/// Key size in bytes for AES 256.
static constexpr CK_ULONG AES256_KEY_SIZE = 32;
/// Key size in bytes for AES 128.
static constexpr CK_ULONG AES128_KEY_SIZE = 16;

/// Crypto factory to HSM function simulations
static std::shared_ptr<CryptoFactoryInterface> c_cryptoFactory;
/// AES 256 key value. This stub generates key value on initialization.
static KeyFactoryInterface::Key c_aes256Key;
/// AES 128 key value. This stub generates key value on initialization.
static KeyFactoryInterface::Key c_aes128Key;
/// AES 256 key checksum. It is three bytes, that correspond to \a c_mainKey data. Unlike PKCS#11 spec, we use SHA-256
/// algorithm to produce checksum instead of SHA-1.
static DigestInterface::DataBlock c_aes256Checksum;
/// AES 128 key checksum.
static DigestInterface::DataBlock c_aes128Checksum;
/// Counter to generate unique session handle values.
static CK_ULONG c_sessionCounter;

/// Session state object.
/**
 * This object contains session state essential for stub operations.
 */
struct SessionStub {
    /// Flag if login has been performed.
    bool m_login = false;
    /// Flag if C_FindObjectsInit has been called.
    bool m_findObjectsInit = false;
    /// Encoder or decoder reference
    std::unique_ptr<CryptoCodecInterface> m_cryptoCodec;
    /// Algorithm type.
    AlgorithmType m_algorithmType;
    /// Filter for object lookup filter by object class.
    CK_OBJECT_CLASS m_findObjectClass = UNSPECIFIED_OBJECT_CLASS;
    /// Filter for object lookup filter by key type.
    CK_KEY_TYPE m_findKeyType = UNSPECIFIED_KEY_TYPE;
    /// Filter for object lookup filter by value length.
    CK_ULONG m_findValueLen = UNSPECIFIED_VALUE_LEN;
    /// Filter for object lookup filter by label.
    std::string m_findLabel = "";
};
/// Session map.
static std::unordered_map<CK_ULONG, std::shared_ptr<SessionStub>> c_sessions;
/// Session map mutex
static std::mutex c_sessionsMutex;

/// Helper to find session by handle
/**
 * This method looks up session object in session map.
 *
 * @param sessionHandle Session handle.
 *
 * @return Session pointer or nullptr on error.
 */
static std::shared_ptr<SessionStub> findSession(CK_SESSION_HANDLE sessionHandle) {
    std::unique_lock<std::mutex> sessionLock(c_sessionsMutex);
    auto it = c_sessions.find(sessionHandle);
    if (it == c_sessions.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

/**
 * @brief Provides function table.
 *
 * This method provides function table for PKCS#11 interface.
 *
 * @param[out] result Pointer to store function table.
 *
 * @return CRK_OK on success.
 */
CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR_PTR result) {
    ACSDK_DEBUG0(LX("C_GetFunctionList"));
    if (!result) {
        ACSDK_ERROR(LX("C_GetFunctionListFailed").d("reason", "resultNull"));
        return CKR_ARGUMENTS_BAD;
    }
    *result = &FUNCTION_LIST;
    return CKR_OK;
}

// Helper to create key and compute checksum.
static void initializeKey(AlgorithmType type, CryptoCodecInterface::Key& key, DigestInterface::DataBlock& checksum) {
    auto keyFactory = c_cryptoFactory->getKeyFactory();
    keyFactory->generateKey(type, key);
    auto digest = c_cryptoFactory->createDigest(DigestType::SHA_256);
    DigestInterface::DataBlock value;
    digest->process(key);
    digest->finalize(checksum);
    checksum.resize(3);
}

/**
 * @brief Initializes module.
 *
 * This method generates a new unique key and computes key signature.
 *
 * @param reserved Unused parameter.
 *
 * @return CKR_OK on success, error code on failure.
 */
CK_RV C_Initialize(CK_VOID_PTR reserved) {
    ACSDK_DEBUG0(LX("C_Initialize"));

    std::unique_lock<std::mutex> sessionLock(c_sessionsMutex);
    c_cryptoFactory = alexaClientSDK::crypto::createCryptoFactory();
    initializeKey(AlgorithmType::AES_256_CBC, c_aes256Key, c_aes256Checksum);
    initializeKey(AlgorithmType::AES_128_CBC, c_aes128Key, c_aes128Checksum);
    return CKR_OK;
}

/**
 * @brief Releases module data.
 *
 * @param reserved Unused parameter. The value must be nullptr.
 *
 * @return CKR_OK on success, error code on failure.
 */
CK_RV C_Finalize(CK_VOID_PTR reserved) {
    ACSDK_DEBUG0(LX("C_Finalize"));
    if (reserved) {
        return CKR_ARGUMENTS_BAD;
    }
    std::unique_lock<std::mutex> sessionLock(c_sessionsMutex);

    c_aes256Key.clear();
    c_aes128Key.clear();
    c_aes256Checksum.clear();
    c_aes128Checksum.clear();
    c_cryptoFactory.reset();
    c_sessions.clear();
    return CKR_OK;
}

/**
 * @brief Provides slot list.
 *
 * This method returns a single hardcoded slot id.
 *
 * @param           tokenPresent    Flag if the slot must have token.
 * @param[out]      slotList        Optional pointer for slot ids with at least \a slotListSize elements
 * @param[in,out]   slotListSize    Number of elements in \a slotList.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_GetSlotList(CK_BBOOL tokenPresent, CK_SLOT_ID_PTR slotList, CK_ULONG_PTR slotListSize) {
    ACSDK_DEBUG0(LX("C_GetSlotList"));
    if (!slotListSize) {
        ACSDK_ERROR(LX("C_GetSlotListFailed").d("reason", "slotListSizeNull"));
        return CKR_ARGUMENTS_BAD;
    }
    *slotListSize = 1;
    if (slotList) {
        slotList[0] = DEFAULT_SLOT_ID;
    }
    return CKR_OK;
}

/**
 * @brief Provide token info.
 *
 * Provides token info for supported slot.
 *
 * @param[in]   slotID      Slot id.
 * @param[out]  tokenInfo Token information.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_GetTokenInfo(CK_SLOT_ID slotID, CK_TOKEN_INFO_PTR tokenInfo) {
    ACSDK_DEBUG0(LX("C_GetTokenInfo"));
    if (!tokenInfo) {
        ACSDK_ERROR(LX("C_GetTokenInfoFailed").d("reason", "tokenInfoNull"));
        return CKR_ARGUMENTS_BAD;
    }
    if (slotID == DEFAULT_SLOT_ID) {
        std::memset(tokenInfo, 0, sizeof(*tokenInfo));
        std::memset(tokenInfo->label, ' ', sizeof(tokenInfo->label));
        std::memset(tokenInfo->manufacturerID, ' ', sizeof(tokenInfo->manufacturerID));
        std::memset(tokenInfo->serialNumber, ' ', sizeof(tokenInfo->serialNumber));
        std::memset(tokenInfo->model, ' ', sizeof(tokenInfo->model));
        std::memcpy(tokenInfo->label, "ACSDK", 5);
        return CKR_OK;
    } else {
        ACSDK_ERROR(LX("C_GetTokenInfoFailed").d("reason", "badSlotId"));
        return CKR_SLOT_ID_INVALID;
    }
}

/**
 * @brief Opens a new session.
 *
 * Method allocates and registers new session object and provides session handle.
 *
 * @param[in]   slotID          Slot id.
 * @param[in]   flags           Session flags.
 * @param[in]   application     Optional application-specific pointer for callbacks.
 * @param[in]   notify          Optional callback function.
 * @param[out]  sessionHandle   Session handle.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_OpenSession(
    CK_SLOT_ID slotID,
    CK_FLAGS flags,
    CK_VOID_PTR application,
    CK_NOTIFY notify,
    CK_SESSION_HANDLE_PTR sessionHandle) {
    ACSDK_DEBUG0(LX("C_OpenSession"));
    if (slotID != DEFAULT_SLOT_ID) {
        ACSDK_ERROR(LX("C_OpenSessionFailed").d("reason", "badSlotId"));
        return CKR_SLOT_ID_INVALID;
    }
    std::unique_lock<std::mutex> sessionLock(c_sessionsMutex);

    *sessionHandle = ++c_sessionCounter;
    c_sessions[*sessionHandle] = std::make_shared<SessionStub>();
    return CKR_OK;
}

/**
 * @brief Terminates session.
 *
 * @param[in] sessionHandle Session handle.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_CloseSession(CK_SESSION_HANDLE sessionHandle) {
    ACSDK_DEBUG0(LX("C_CloseSession"));
    std::unique_lock<std::mutex> sessionsLock(c_sessionsMutex);
    auto it = c_sessions.find(sessionHandle);
    if (it == c_sessions.end()) {
        ACSDK_ERROR(LX("C_CloseSessionFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }

    c_sessions.erase(it);
    return CKR_OK;
}

/**
 * @brief Performs login.
 *
 * @param[in] sessionHandle Session handle.
 * @param[in] type          Login type.
 * @param[in] pin           User pin.
 * @param[in] pinLen        Length of user pin.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_Login(CK_SESSION_HANDLE sessionHandle, CK_USER_TYPE type, CK_UTF8CHAR_PTR pin, CK_ULONG pinLen) {
    ACSDK_DEBUG0(LX("C_Logout"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_LoginFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }
    if (session->m_login) {
        ACSDK_ERROR(LX("C_LoginFailed").d("reason", "alreadyLoggedIn"));
        return CKR_USER_ALREADY_LOGGED_IN;
    }
    if (CKU_USER != type) {
        ACSDK_ERROR(LX("C_LoginFailed").d("reason", "soLoginUnsupported"));
        return CKR_GENERAL_ERROR;
    }
    if (!pin) {
        ACSDK_ERROR(LX("C_LoginFailed").d("reason", "pinNull"));
        return CKR_ARGUMENTS_BAD;
    }
    if (4 != pinLen || std::memcmp(pin, "1234", 4)) {
        ACSDK_ERROR(LX("C_LoginFailed").d("reason", "pinError"));
        return CKR_PIN_INCORRECT;
    }

    session->m_login = true;
    return CKR_OK;
}

/**
 * Performs logout.
 *
 * @param[in] sessionHandle Session handle.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_Logout(CK_SESSION_HANDLE sessionHandle) {
    ACSDK_DEBUG0(LX("C_Logout"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_LogoutFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }
    if (!session->m_login) {
        ACSDK_ERROR(LX("C_LogoutFailed").d("reason", "notLoggedIn"));
        return CKR_USER_NOT_LOGGED_IN;
    }
    session->m_login = false;
    return CKR_OK;
}

/**
 * Method returns object attributes. This implementation supports only subset of attributes.
 *
 * @param[in]       sessionHandle   Session handle.
 * @param[in]       objectHandle    Object handle.
 * @param[in,out]   attributes      Attributes to query.
 * @param[in]       attributeCount  Number of attributes to query.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_GetAttributeValue(
    CK_SESSION_HANDLE sessionHandle,
    CK_OBJECT_HANDLE objectHandle,
    CK_ATTRIBUTE_PTR attributes,
    CK_ULONG attributeCount) {
    ACSDK_DEBUG0(LX("C_GetAttributeValue"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_GetAttributeValueFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }

    DigestInterface::DataBlock* checksum = nullptr;
    CK_ULONG keySize = 0;

    switch (objectHandle) {
        case AES128_KEY_OBJECT_HANDLE:
            checksum = &c_aes128Checksum;
            keySize = AES128_KEY_SIZE;
            break;
        case AES256_KEY_OBJECT_HANDLE:
            checksum = &c_aes256Checksum;
            keySize = AES256_KEY_SIZE;
            break;
        default:
            ACSDK_ERROR(LX("C_GetAttributeValueFailed").d("reason", "badObjectHandle"));
            return CKR_OBJECT_HANDLE_INVALID;
    }

    for (CK_ULONG i = 0; i < attributeCount; ++i) {
        switch (attributes[i].type) {
            case CKA_NEVER_EXTRACTABLE:
                if (attributes[i].ulValueLen != sizeof(CK_BBOOL)) {
                    ACSDK_ERROR(LX("C_GetAttributeValueFailed")
                                    .d("reason", "badAttributeSize")
                                    .d("attr", "CKA_NEVER_EXTRACTABLE"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                } else {
                    *(CK_BBOOL*)attributes[i].pValue = CK_TRUE;
                }
                break;
            case CKA_CHECK_VALUE:
                if (attributes[i].ulValueLen != checksum->size()) {
                    ACSDK_ERROR(
                        LX("C_GetAttributeValueFailed").d("reason", "badAttributeSize").d("attr", "CKA_CHECK_VALUE"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                } else {
                    std::memcpy(attributes[i].pValue, checksum->data(), checksum->size());
                }
                break;
            case CKA_CLASS:
                if (attributes[i].ulValueLen != sizeof(CK_OBJECT_CLASS)) {
                    ACSDK_ERROR(LX("C_GetAttributeValueFailed").d("reason", "badAttributeSize").d("attr", "CKA_CLASS"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                } else {
                    *(CK_OBJECT_CLASS*)attributes[i].pValue = CKO_SECRET_KEY;
                }
                break;
            case CKA_KEY_TYPE:
                if (attributes[i].ulValueLen != sizeof(CK_KEY_TYPE)) {
                    ACSDK_ERROR(
                        LX("C_GetAttributeValueFailed").d("reason", "badAttributeSize").d("attr", "CKA_KEY_TYPE"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                } else {
                    *(CK_KEY_TYPE*)attributes[i].pValue = CKK_AES;
                }
                break;
            case CKA_VALUE_LEN:
                if (attributes[i].ulValueLen != sizeof(CK_ULONG)) {
                    ACSDK_ERROR(
                        LX("C_GetAttributeValueFailed").d("reason", "badAttributeSize").d("attr", "CKA_VALUE_LEN"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                } else {
                    *(CK_ULONG*)attributes[i].pValue = keySize;
                }
                break;
            default:
                ACSDK_ERROR(
                    LX("C_GetAttributeValueFailed").d("reason", "unsupportedAttribute").d("type", attributes[i].type));
                return CKR_ATTRIBUTE_TYPE_INVALID;
        }
    }

    return CKR_OK;
}

/**
 * @brief Initializes object search.
 *
 * This method configures object search parametrs.
 *
 * @param[in] sessionHandle     Session handle.
 * @param[in] attributes        Attributes to match.
 * @param[in] attributeCount    Number of attributes to query.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_FindObjectsInit(CK_SESSION_HANDLE sessionHandle, CK_ATTRIBUTE_PTR attributes, CK_ULONG attributeCount) {
    ACSDK_DEBUG0(LX("C_FindObjectsInit"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_FindObjectsInitFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }

    session->m_findObjectsInit = false;
    session->m_findObjectClass = UNSPECIFIED_OBJECT_CLASS;
    session->m_findKeyType = UNSPECIFIED_KEY_TYPE;
    session->m_findValueLen = UNSPECIFIED_VALUE_LEN;

    for (CK_ULONG i = 0; i < attributeCount; ++i) {
        if (!attributes[i].pValue) {
            ACSDK_ERROR(LX("C_FindObjectsInitFailed").d("reason", "pValueNull"));
            return CKR_ATTRIBUTE_VALUE_INVALID;
        }
        switch (attributes[i].type) {
            case CKA_CLASS:
                if (attributes[i].ulValueLen == sizeof(CK_OBJECT_CLASS)) {
                    session->m_findObjectClass = *(CK_OBJECT_CLASS_PTR)attributes[i].pValue;
                } else {
                    ACSDK_ERROR(LX("C_FindObjectsInitFailed").d("reason", "classSizeInvalid"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                }
                break;
            case CKA_KEY_TYPE:
                if (attributes[i].ulValueLen == sizeof(CK_KEY_TYPE)) {
                    session->m_findKeyType = *(CK_KEY_TYPE*)attributes[i].pValue;
                } else {
                    ACSDK_ERROR(LX("C_FindObjectsInitFailed").d("reason", "keyTypeSizeInvalid"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                }
                break;
            case CKA_VALUE_LEN:
                if (attributes[i].ulValueLen == sizeof(CK_ULONG)) {
                    session->m_findValueLen = *(CK_ULONG_PTR)attributes[i].pValue;
                } else {
                    ACSDK_ERROR(LX("C_FindObjectsInitFailed").d("reason", "valueLenSizeInvalid"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                }
                break;
            case CKA_LABEL:
                if (attributes[i].ulValueLen < 128) {
                    session->m_findLabel.assign(
                        (CK_BYTE_PTR)attributes[i].pValue,
                        (CK_BYTE_PTR)attributes[i].pValue + attributes[i].ulValueLen);
                } else {
                    ACSDK_ERROR(LX("C_FindObjectsInitFailed").d("reason", "valueLenSizeInvalid"));
                    return CKR_ATTRIBUTE_VALUE_INVALID;
                }
                break;
            default:
                ACSDK_ERROR(
                    LX("C_FindObjectsInitFailed").d("reason", "unsupportedAttribute").d("type", attributes[i].type));
                return CKR_ATTRIBUTE_TYPE_INVALID;
        }
    }

    session->m_findObjectsInit = true;
    return CKR_OK;
}

/**
 * @brief Finds objects matching search criteria.
 *
 * This method provides object handles that match search criteria.
 *
 * @param[in]   sessionHandle   Session handle.
 * @param[out]  objectHandles   Discovered object handles.
 * @param[in]   maxObjectCount  Maximum number of objects to locate.
 * @param[out]  objectCount     Number of objects located.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_FindObjects(
    CK_SESSION_HANDLE sessionHandle,
    CK_OBJECT_HANDLE_PTR objectHandles,
    CK_ULONG maxObjectCount,
    CK_ULONG_PTR objectCount) {
    ACSDK_DEBUG0(LX("C_FindObjects"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_FindObjectsFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }
    if (!session->m_findObjectsInit) {
        ACSDK_ERROR(LX("C_FindObjectsFailed").d("reason", "findNotInitialized"));
        return CKR_FUNCTION_REJECTED;
    }

    session->m_findObjectsInit = false;

    if (!session->m_login) {
        ACSDK_ERROR(LX("C_FindObjectsFailed").d("reason", "notLoggedIn"));
        return CKR_USER_NOT_LOGGED_IN;
    }

    if (maxObjectCount < 1) {
        ACSDK_ERROR(LX("C_FindObjectsFailed").d("reason", "bufferTooSmall"));
        return CKR_BUFFER_TOO_SMALL;
    }

    if (!session->m_findLabel.empty() && session->m_findLabel != "TEST_KEY") {
        *objectCount = 0;
        return CKR_OK;
    }

    if ((session->m_findObjectClass == UNSPECIFIED_OBJECT_CLASS || session->m_findObjectClass == CKO_SECRET_KEY) &&
        (session->m_findKeyType == UNSPECIFIED_KEY_TYPE || session->m_findKeyType == CKK_AES)) {
        if (session->m_findValueLen == UNSPECIFIED_VALUE_LEN) {
            constexpr CK_ULONG totalKeys = 2u;
            *objectCount = std::min(maxObjectCount, totalKeys);
            objectHandles[0] = AES256_KEY_OBJECT_HANDLE;
            if (*objectCount > 1u) {
                objectHandles[1] = AES128_KEY_OBJECT_HANDLE;
            }
            return CKR_OK;
        } else if (session->m_findValueLen == AES128_KEY_SIZE) {
            *objectCount = 1u;
            objectHandles[0] = AES128_KEY_OBJECT_HANDLE;
            return CKR_OK;
        } else if (session->m_findValueLen == AES256_KEY_SIZE) {
            *objectCount = 1u;
            objectHandles[0] = AES256_KEY_OBJECT_HANDLE;
            return CKR_OK;
        }
    }

    *objectCount = 0;
    return CKR_OK;
}

/**
 * @brief Finishes object search.
 *
 * @param[in] sessionHandle Session handle.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE sessionHandle) {
    ACSDK_DEBUG0(LX("C_FindObjectsFinal"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_FindObjectsFinalFailed").d("reason", "sessionNull"));
        return CKR_SESSION_HANDLE_INVALID;
    }
    return CKR_OK;
}

/**
 * @brief Initializes encryption operation.
 *
 * @param[in] sessionHandle Session handle.
 * @param[in] mechanism     Encryption parameters.
 * @param[in] keyHandle     Key handle.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_EncryptInit(CK_SESSION_HANDLE sessionHandle, CK_MECHANISM_PTR mechanism, CK_OBJECT_HANDLE keyHandle) {
    ACSDK_DEBUG0(LX("C_EncryptInit"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "sessionNotFound"));
        return CKR_SESSION_HANDLE_INVALID;
    }
    if (!session->m_login) {
        ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "notLoggedIn"));
        return CKR_USER_NOT_LOGGED_IN;
    }
    KeyFactoryInterface::Key* key;
    bool use256Key;
    switch (keyHandle) {
        case AES128_KEY_OBJECT_HANDLE:
            use256Key = false;
            key = &c_aes128Key;
            break;
        case AES256_KEY_OBJECT_HANDLE:
            use256Key = true;
            key = &c_aes256Key;
            break;
        default:
            ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "keyHandleInvalid"));
            return CKR_KEY_HANDLE_INVALID;
    }

    switch (mechanism->mechanism) {
        case CKM_AES_CBC:
            session->m_algorithmType = use256Key ? AlgorithmType::AES_256_CBC : AlgorithmType::AES_128_CBC;
            break;
        case CKM_AES_CBC_PAD:
            session->m_algorithmType = use256Key ? AlgorithmType::AES_256_CBC_PAD : AlgorithmType::AES_128_CBC_PAD;
            break;
        case CKM_AES_GCM:
            session->m_algorithmType = use256Key ? AlgorithmType::AES_256_GCM : AlgorithmType::AES_128_GCM;
            break;
        default:
            ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "mechanismInvalid"));
            return CKR_MECHANISM_INVALID;
    }

    session->m_cryptoCodec = c_cryptoFactory->createEncoder(session->m_algorithmType);

    if (CKM_AES_GCM == mechanism->mechanism) {
        const CK_GCM_PARAMS& gcmParams = *(const CK_GCM_PARAMS*)(mechanism->pParameter);

        ACSDK_DEBUG5(LX("C_EncryptInit").d("ivLen", gcmParams.ulIvLen).d("aadLen", gcmParams.ulAADLen));

        CryptoCodecInterface::IV iv{gcmParams.pIv, gcmParams.pIv + gcmParams.ulIvLen};
        if (!session->m_cryptoCodec->init(*key, iv)) {
            ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "codecInitError"));
            return CKR_GENERAL_ERROR;
        }
        CryptoCodecInterface::DataBlock aad{gcmParams.pAAD, gcmParams.pAAD + gcmParams.ulAADLen};
        if (!session->m_cryptoCodec->processAAD(aad)) {
            ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "codecProcessAADError"));
            return CKR_GENERAL_ERROR;
        }
    } else {
        CryptoCodecInterface::IV iv{
            (CryptoCodecInterface::IV::const_pointer)mechanism->pParameter,
            (CryptoCodecInterface::IV::const_pointer)mechanism->pParameter + mechanism->ulParameterLen};
        if (!session->m_cryptoCodec->init(*key, iv)) {
            ACSDK_ERROR(LX("C_EncryptInitFailed").d("reason", "codecInitError"));
            return CKR_GENERAL_ERROR;
        }
    }

    return CKR_OK;
}

/**
 * @brief Performs encryption.
 *
 * Method encrypts data block or signal an error. Any result except CKR_BUFFER_TOO_SMALL terminates encryption
 * operation.
 *
 * @param[in]       sessionHandle   Session handle.
 * @param[in]       plaintext       Plaintext data.
 * @param[in]       plaintextLen    Size of \a plaintext.
 * @param[out]      ciphertext      Optional ciphertext output.
 * @param[in,out]   ciphertextLen   Size of \a ciphertext buffer on input, and required size on output.
 *
 * @return CKR_OK on success, or error code on failure.
 * @retval CKR_BUFFER_TOO_SMALL Indicates the \a ciphertextLen was too small.
 */
CK_RV C_Encrypt(
    CK_SESSION_HANDLE sessionHandle,
    CK_BYTE_PTR plaintext,
    CK_ULONG plaintextLen,
    CK_BYTE_PTR ciphertext,
    CK_ULONG_PTR ciphertextLen) {
    ACSDK_DEBUG0(LX("C_Encrypt").d("mode", ciphertext ? "estimate" : "encrypt"));

    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "sessionHandleInvalid"));
        return CKR_SESSION_HANDLE_INVALID;
    }

    CK_ULONG estSize;
    bool useGcm;
    switch (session->m_algorithmType) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
            estSize = plaintextLen;
            if (plaintextLen % AES_BLOCK_SIZE) {
                ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "inputBlockSize"));
                return CKR_DATA_INVALID;
            }
            useGcm = false;
            break;
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            estSize = plaintextLen + AES_BLOCK_SIZE - plaintextLen % AES_BLOCK_SIZE;
            useGcm = false;
            break;
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            estSize = plaintextLen + AES_GCM_TAG_SIZE;
            useGcm = true;
            break;
        default:
            ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "unknownAlgorithmType").d("type", session->m_algorithmType));
            return CKR_GENERAL_ERROR;
    }

    if (ciphertext) {
        if (*ciphertextLen < estSize) {
            ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "bufferTooSmall"));
            return CKR_BUFFER_TOO_SMALL;
        }
        CryptoCodecInterface::DataBlock in;
        in.assign(plaintext, plaintext + plaintextLen);
        CryptoCodecInterface::DataBlock res;
        if (!session->m_cryptoCodec->process(in, res)) {
            ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "codecProcessError"));
            return CKR_GENERAL_ERROR;
        }
        if (!session->m_cryptoCodec->finalize(res)) {
            ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "codecFinalizeError"));
            return CKR_GENERAL_ERROR;
        }
        if (useGcm) {
            CryptoCodecInterface::Tag tag;
            if (!session->m_cryptoCodec->getTag(tag)) {
                ACSDK_ERROR(LX("C_EncryptFailed").d("reason", "codecGetTagError"));
                return CKR_GENERAL_ERROR;
            }
            res.reserve(res.size() + tag.size());
            res.insert(res.end(), tag.cbegin(), tag.cend());
        }
        *ciphertextLen = static_cast<CK_ULONG>(res.size());
        ::memcpy(ciphertext, res.data(), res.size());

        session->m_cryptoCodec.reset();
    } else {
        *ciphertextLen = estSize;
    }

    return CKR_OK;
}

/**
 * @brief Initializes decryption operation.
 *
 * @param[in] sessionHandle Session handle.
 * @param[in] mechanism     Encryption parameters.
 * @param[in] keyHandle     Key handle.
 *
 * @return CKR_OK on success, or error code on failure.
 */
CK_RV C_DecryptInit(CK_SESSION_HANDLE sessionHandle, CK_MECHANISM_PTR mechanism, CK_OBJECT_HANDLE keyHandle) {
    ACSDK_DEBUG0(LX("C_DecryptInit"));
    auto session = findSession(sessionHandle);
    if (!session) {
        ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "sessionHandleInvalid"));
        return CKR_SESSION_HANDLE_INVALID;
    }
    if (!session->m_login) {
        ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "notLoggedIn"));
        return CKR_USER_NOT_LOGGED_IN;
    }

    KeyFactoryInterface::Key* key;
    bool use256Key;
    switch (keyHandle) {
        case AES128_KEY_OBJECT_HANDLE:
            use256Key = false;
            key = &c_aes128Key;
            break;
        case AES256_KEY_OBJECT_HANDLE:
            use256Key = true;
            key = &c_aes256Key;
            break;
        default:
            ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "unknownHandle").d("handle", keyHandle));
            return CKR_KEY_HANDLE_INVALID;
    }
    switch (mechanism->mechanism) {
        case CKM_AES_CBC:
            session->m_algorithmType = use256Key ? AlgorithmType::AES_256_CBC : AlgorithmType::AES_128_CBC;
            break;
        case CKM_AES_CBC_PAD:
            session->m_algorithmType = use256Key ? AlgorithmType::AES_256_CBC_PAD : AlgorithmType::AES_128_CBC_PAD;
            break;
        case CKM_AES_GCM:
            session->m_algorithmType = use256Key ? AlgorithmType::AES_256_GCM : AlgorithmType::AES_128_GCM;
            break;
        default:
            ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "unknownMechanism").d("type", mechanism->mechanism));
            return CKR_MECHANISM_INVALID;
    }

    session->m_cryptoCodec = c_cryptoFactory->createDecoder(session->m_algorithmType);
    if (!session->m_cryptoCodec) {
        ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "createDecoderFailed").d("type", session->m_algorithmType));
        return CKR_GENERAL_ERROR;
    }

    if (CKM_AES_GCM == mechanism->mechanism) {
        const CK_GCM_PARAMS& gcmParams = *(const CK_GCM_PARAMS*)(mechanism->pParameter);
        ACSDK_DEBUG5(LX("C_DecryptInit").d("ivLen", gcmParams.ulIvLen).d("aadLen", gcmParams.ulAADLen));
        CryptoCodecInterface::IV iv{gcmParams.pIv, gcmParams.pIv + gcmParams.ulIvLen};
        if (!session->m_cryptoCodec->init(*key, iv)) {
            ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "codecInitFailed"));
            return CKR_GENERAL_ERROR;
        }
        CryptoCodecInterface::DataBlock aad{gcmParams.pAAD, gcmParams.pAAD + gcmParams.ulAADLen};
        if (!session->m_cryptoCodec->processAAD(aad)) {
            ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "codecProcessAADFailed"));
            return CKR_GENERAL_ERROR;
        }
    } else {
        CryptoCodecInterface::IV iv{
            (CryptoCodecInterface::IV::const_pointer)mechanism->pParameter,
            (CryptoCodecInterface::IV::const_pointer)mechanism->pParameter + mechanism->ulParameterLen};
        if (!session->m_cryptoCodec->init(*key, iv)) {
            ACSDK_ERROR(LX("C_DecryptInitFailed").d("reason", "codecInitFailed"));
            return CKR_GENERAL_ERROR;
        }
    }

    return CKR_OK;
}

/**
 * @brief Performs decryption.
 *
 * Method decrypts data block or signal an error. Any result except CKR_BUFFER_TOO_SMALL terminates decryption
 * operation.
 *
 * @param[in]       sessionHandle   Session handle.
 * @param[in]       ciphertext      Optional ciphertext output.
 * @param[in]       ciphertextLen   Size of \a ciphertext buffer on input, and required size on output.
 * @param[out]      plaintext       Plaintext data.
 * @param[in,out]   plaintextLen    Size of \a plaintext.
 *
 * @return CKR_OK on success, or error code on failure.
 * @retval CKR_BUFFER_TOO_SMALL Indicates the \a plaintextLen was too small.
 */
CK_RV C_Decrypt(
    CK_SESSION_HANDLE sessionHandle,
    CK_BYTE_PTR ciphertext,
    CK_ULONG ciphertextLen,
    CK_BYTE_PTR plaintext,
    CK_ULONG_PTR plaintextLen) {
    ACSDK_DEBUG0(LX("C_Decrypt").d("mode", plaintext ? "decrypt" : "estimate"));

    auto session = findSession(sessionHandle);
    if (!session) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    if (!session->m_cryptoCodec) {
        return CKR_ACTION_PROHIBITED;
    }

    CK_ULONG estSize;
    bool useGcm;
    switch (session->m_algorithmType) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC:
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_128_CBC_PAD:
            // Overestimate the size when PKCS#7 padding is used
            estSize = ciphertextLen;
            useGcm = false;
            break;
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            estSize = ciphertextLen - AES_GCM_TAG_SIZE;
            useGcm = true;
            break;
        default:
            return CKR_GENERAL_ERROR;
    }

    if (!useGcm && (ciphertextLen % AES_BLOCK_SIZE)) {
        return CKR_ENCRYPTED_DATA_INVALID;
    }

    if (plaintext) {
        if (*plaintextLen < estSize) {
            return CKR_BUFFER_TOO_SMALL;
        }
        CryptoCodecInterface::DataBlock res;

        if (useGcm) {
            CK_ULONG actualCiphertextLen = ciphertextLen - AES_GCM_TAG_SIZE;
            CryptoCodecInterface::DataBlock in;
            in.assign(ciphertext, ciphertext + actualCiphertextLen);
            if (!session->m_cryptoCodec->process(in, res)) {
                ACSDK_ERROR(LX("C_DecryptFailed").d("reason", "codecProcessFailed"));
                return CKR_GENERAL_ERROR;
            }
            CryptoCodecInterface::Tag tag;
            tag.assign(ciphertext + actualCiphertextLen, ciphertext + ciphertextLen);
            if (!session->m_cryptoCodec->setTag(tag)) {
                ACSDK_ERROR(LX("C_DecryptFailed").d("reason", "codecSetTagFailed"));
                return CKR_GENERAL_ERROR;
            }
        } else {
            CryptoCodecInterface::DataBlock in;
            in.assign(ciphertext, ciphertext + ciphertextLen);
            if (!session->m_cryptoCodec->process(in, res)) {
                ACSDK_ERROR(LX("C_DecryptFailed").d("reason", "codecProcessFailed"));
                return CKR_GENERAL_ERROR;
            }
        }

        if (!session->m_cryptoCodec->finalize(res)) {
            ACSDK_ERROR(LX("C_DecryptFailed").d("reason", "codecFinalizeFailed"));
            return CKR_GENERAL_ERROR;
        }
        session->m_cryptoCodec.reset();
        std::memcpy(plaintext, res.data(), res.size());
        *plaintextLen = static_cast<CK_ULONG>(res.size());
    } else {
        *plaintextLen = estSize;
    }

    return CKR_OK;
}

/// @}
