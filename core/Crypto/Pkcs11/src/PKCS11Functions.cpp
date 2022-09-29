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
#include <acsdk/Pkcs11/private/PKCS11Functions.h>
#include <acsdk/Pkcs11/private/PKCS11Slot.h>

namespace alexaClientSDK {
namespace pkcs11 {

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::Functions"

std::shared_ptr<PKCS11Functions> PKCS11Functions::create(const std::string& libpath) noexcept {
    auto res = std::shared_ptr<PKCS11Functions>(new PKCS11Functions);
    if (res->loadLibraryAndGetFunctions(libpath)) {
        if (!res->initialize()) {
            ACSDK_ERROR(LX("libraryInitFailed"));
            res.reset();
        }
    } else {
        ACSDK_ERROR(LX("libraryLoadFailed").sensitive("path", libpath));
        res.reset();
    }
    return res;
}

PKCS11Functions::PKCS11Functions() noexcept : m_libraryHandle(nullptr), m_pkcs11Functions(nullptr) {
}

PKCS11Functions::~PKCS11Functions() noexcept {
    finalizeOperations();
    unloadLibrary();
}

bool PKCS11Functions::initialize() noexcept {
    CK_C_INITIALIZE_ARGS initArgs;
    memset(&initArgs, 0, sizeof(initArgs));

    initArgs.flags = CKF_OS_LOCKING_OK;

    CK_RV rv = m_pkcs11Functions->C_Initialize(&initArgs);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("pkcs11InitFailed").d("CK_RV", rv));
        m_pkcs11Functions = nullptr;
        return false;
    }

    return true;
}

void PKCS11Functions::finalizeOperations() noexcept {
    if (m_pkcs11Functions) {
        m_pkcs11Functions->C_Finalize(nullptr);
        m_pkcs11Functions = nullptr;
    }
}

bool PKCS11Functions::listSlots(bool tokenPresent, std::vector<std::shared_ptr<PKCS11Slot>>& slots) noexcept {
    CK_BBOOL tokenPresentFlag = CK_FALSE;
    CK_ULONG slotCount = 0;
    CK_RV rv;

    if (tokenPresent) {
        // CK_TRUE is defined as a signed constant, which causes a Fortify warning
        tokenPresentFlag = static_cast<CK_BBOOL>(CK_TRUE);
    }

    rv = m_pkcs11Functions->C_GetSlotList(tokenPresentFlag, nullptr, &slotCount);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("getSlotCountFailed").d("CK_RV", rv));
        return false;
    }

    std::vector<CK_SLOT_ID> slotsIds;
    slotsIds.resize(slotCount);

    rv = m_pkcs11Functions->C_GetSlotList(tokenPresentFlag, slotsIds.data(), &slotCount);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("getSlotListFailed").d("CK_RV", rv));
        return false;
    }

    slots.clear();
    slots.reserve(slotCount);

    auto transform = [this](CK_SLOT_ID slotId) -> std::shared_ptr<PKCS11Slot> {
        return std::shared_ptr<PKCS11Slot>(new PKCS11Slot(shared_from_this(), slotId));
    };
    std::transform(slotsIds.begin(), slotsIds.end(), std::back_inserter(slots), transform);

    return true;
}

bool PKCS11Functions::findSlotByTokenName(const std::string& tokenName, std::shared_ptr<PKCS11Slot>& slot) noexcept {
    std::vector<std::shared_ptr<PKCS11Slot>> slots;

    if (!listSlots(true, slots)) {
        ACSDK_ERROR(LX("listSlotsFailed"));
        return false;
    }

    auto name_matches = [&tokenName](const std::shared_ptr<PKCS11Slot>& slot) -> bool {
        std::string name;
        if (slot->getTokenName(name)) {
            return name == tokenName;
        } else {
            return false;
        }
    };
    auto it = std::find_if(slots.begin(), slots.end(), name_matches);
    if (it == slots.end()) {
        ACSDK_ERROR(LX("slotNotFound").d("tokenName", tokenName));
        slot.reset();
    } else {
        slot = *it;
    }
    return true;
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
