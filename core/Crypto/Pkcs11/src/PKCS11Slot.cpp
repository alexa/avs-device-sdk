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
#include <acsdk/Pkcs11/private/PKCS11Slot.h>
#include <acsdk/Pkcs11/private/PKCS11Session.h>
#include <acsdk/Pkcs11/private/PKCS11Functions.h>

namespace alexaClientSDK {
namespace pkcs11 {

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::Slot"

PKCS11Slot::PKCS11Slot(std::shared_ptr<PKCS11Functions>&& functions, CK_SLOT_ID slotId) noexcept :
        m_functions(functions),
        m_slotID(slotId) {
}

bool PKCS11Slot::getTokenName(std::string& tokenName) noexcept {
    CK_TOKEN_INFO tokenInfo;
    CK_RV rv;

    rv = m_functions->m_pkcs11Functions->C_GetTokenInfo(m_slotID, &tokenInfo);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("getTokenInfoFailed").d("CK_RV", rv));
        return false;
    }

    std::string tmp;
    tmp.assign((const char*)&tokenInfo.label[0], (const char*)&tokenInfo.label[sizeof(tokenInfo.label)]);

    for (auto it = tmp.rbegin(); it != tmp.rend();) {
        if (*it != ' ') {
            break;
        } else {
            tmp.pop_back();
            it = tmp.rbegin();
        }
    }
    tokenName = tmp;

    return true;
}

std::shared_ptr<PKCS11Session> PKCS11Slot::openSession() noexcept {
    CK_RV rv;

    CK_SESSION_HANDLE sessionHandle;
    CK_FLAGS flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;

    auto session = std::shared_ptr<PKCS11Session>(new PKCS11Session(m_functions, CK_INVALID_HANDLE));

    rv = m_functions->m_pkcs11Functions->C_OpenSession(m_slotID, flags, session.get(), nullptr, &sessionHandle);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("openSessionFailed").d("CK_RV", rv));
        return nullptr;
    }
    session->m_sessionHandle = sessionHandle;

    return session;
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
