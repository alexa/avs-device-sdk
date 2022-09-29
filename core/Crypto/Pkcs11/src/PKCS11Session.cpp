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
#include <acsdk/Pkcs11/private/PKCS11Functions.h>
#include <acsdk/Pkcs11/private/PKCS11Session.h>
#include <acsdk/Pkcs11/private/PKCS11Key.h>

namespace alexaClientSDK {
namespace pkcs11 {

using namespace alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::Session"

PKCS11Session::PKCS11Session(
    const std::shared_ptr<PKCS11Functions>& functions,
    CK_SESSION_HANDLE sessionHandle) noexcept :
        m_functions(functions),
        m_sessionHandle(sessionHandle) {
}

PKCS11Session::~PKCS11Session() noexcept {
    closeSession();
}

bool PKCS11Session::logIn(const std::string& userPin) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<CK_UTF8CHAR> tmp;
    tmp.reserve(userPin.size());
    std::transform(userPin.cbegin(), userPin.cend(), std::back_inserter(tmp), [](char ch) -> CK_UTF8CHAR {
        return static_cast<CK_UTF8CHAR>(ch);
    });

    CK_RV rv;

    rv = m_functions->m_pkcs11Functions->C_Login(m_sessionHandle, CKU_USER, &tmp[0], static_cast<CK_ULONG>(tmp.size()));
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("logInFailed").d("CK_RV", rv));
        return false;
    }

    return true;
}

bool PKCS11Session::logOut() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);

    CK_RV rc;
    rc = m_functions->m_pkcs11Functions->C_Logout(m_sessionHandle);

    if (CKR_OK != rc) {
        ACSDK_ERROR(LX("logOutFailed").d("CK_RV", rc));
        return false;
    }

    return true;
}

bool PKCS11Session::closeSession() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (CK_INVALID_HANDLE != m_sessionHandle) {
        m_functions->m_pkcs11Functions->C_CloseSession(m_sessionHandle);
        m_sessionHandle = CK_INVALID_HANDLE;
    }

    return true;
}

std::unique_ptr<PKCS11Key> PKCS11Session::findKey(const PKCS11KeyDescriptor& descriptor) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);

    CK_RV rv;

    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    std::vector<CK_ATTRIBUTE> objectMask;
    objectMask.reserve(4);
    objectMask.push_back({CKA_CLASS, &keyClass, static_cast<CK_ULONG>(sizeof(keyClass))});
    objectMask.push_back({CKA_KEY_TYPE,
                          const_cast<CK_KEY_TYPE*>(&descriptor.keyType),
                          static_cast<CK_ULONG>(sizeof(descriptor.keyType))});
    objectMask.push_back({CKA_LABEL,
                          const_cast<char*>(descriptor.objectLabel.data()),
                          static_cast<CK_ULONG>(descriptor.objectLabel.size())});
    objectMask.push_back({CKA_VALUE_LEN,
                          const_cast<CK_KEY_TYPE*>(&descriptor.keyLen),
                          static_cast<CK_ULONG>(sizeof(descriptor.keyLen))});

    rv = m_functions->m_pkcs11Functions->C_FindObjectsInit(
        m_sessionHandle, objectMask.data(), static_cast<CK_ULONG>(objectMask.size()));
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("findObjectsInitFailed").d("CK_RV", rv));
        return nullptr;
    }

    CK_ULONG maxObjectCount = 1;
    CK_ULONG objectCount = 1;
    CK_OBJECT_HANDLE keyHandle = CK_INVALID_HANDLE;

    rv = m_functions->m_pkcs11Functions->C_FindObjects(m_sessionHandle, &keyHandle, maxObjectCount, &objectCount);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("findObjectsFailed").d("CK_RV", rv));
        rv = m_functions->m_pkcs11Functions->C_FindObjectsFinal(m_sessionHandle);
        if (rv != CKR_OK) {
            ACSDK_ERROR(LX("findObjectsFinalFailed").d("CK_RV", rv));
        }
        return nullptr;
    }

    rv = m_functions->m_pkcs11Functions->C_FindObjectsFinal(m_sessionHandle);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("findObjectsFinalFailed").d("CK_RV", rv));
        return nullptr;
    }

    if (!objectCount) {
        ACSDK_ERROR(LX("objectNotFound").sensitive("descriptor", descriptor));
        return nullptr;
    }

    return std::unique_ptr<PKCS11Key>(new PKCS11Key(shared_from_this(), keyHandle));
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
