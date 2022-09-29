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

#include <dlfcn.h>

#include <acsdk/Pkcs11/private/ErrorCleanupGuard.h>
#include <acsdk/Pkcs11/private/Logging.h>
#include <acsdk/Pkcs11/private/PKCS11Functions.h>

namespace alexaClientSDK {
namespace pkcs11 {

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::Functions"

/**
 * @brief Check if the path is absolute.
 *
 * @param[in] libPath Path to library.
 *
 * @return True if path is absolute.
 * @private
 */
static bool isAbsolutePath(const std::string& libPath) noexcept {
    return !libPath.empty() && '/' == libPath[0];
}

bool PKCS11Functions::loadLibraryAndGetFunctions(const std::string& libpath) noexcept {
    if (libpath.empty()) {
        m_libraryHandle = RTLD_DEFAULT;
    } else {
        if (!isAbsolutePath(libpath)) {
            ACSDK_ERROR(LX("libraryLoadFailed").d("reason", "pathMustBeAbsolute"));
            return false;
        }

        m_libraryHandle = dlopen(libpath.c_str(), RTLD_NOW);
        if (!m_libraryHandle) {
            ACSDK_ERROR(LX("libraryLoadFailed").sensitive("libpath", libpath).d("errno", errno));
            return false;
        }
    }
    bool success = false;
    ErrorCleanupGuard unloadCleanup(success, std::bind(&PKCS11Functions::unloadLibrary, this));

    CK_C_Initialize pFunc = reinterpret_cast<CK_C_Initialize>(dlsym(m_libraryHandle, "C_GetFunctionList"));
    if (!pFunc) {
        ACSDK_ERROR(LX("functionListNotFound"));
        return false;
    }

    CK_RV rv = pFunc(&m_pkcs11Functions);
    if (CKR_OK != rv) {
        ACSDK_ERROR(LX("functionListInitFailed").d("CK_RV", rv));
        m_pkcs11Functions = nullptr;
        return false;
    }

    success = true;
    return success;
}

void PKCS11Functions::unloadLibrary() noexcept {
    if (m_libraryHandle) {
        if (RTLD_DEFAULT != m_libraryHandle) {
            dlclose(m_libraryHandle);
        }
        m_libraryHandle = nullptr;
    }
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
