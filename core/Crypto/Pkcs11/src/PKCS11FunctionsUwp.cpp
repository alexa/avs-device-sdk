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

#include <stdlib.h>

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
static bool isAbsolutePath(const std::string& libpath) noexcept {
    // File path is absolute, if it starts with a drive letter followed by colon, and then has path separator.
    // Examples: c:/ or F:\.
    return libpath.size() > 2u && ':' == libpath[1] && ('\\' == libpath[2] || '/' == libpath[2]);
}

bool PKCS11Functions::loadLibraryAndGetFunctions(const std::string& libpath) noexcept {
    if (!isAbsolutePath(libpath)) {
        ACSDK_ERROR(LX("libraryLoadFailed").d("reason", "pathMustBeAbsolute"));
        return false;
    }
#if defined(UNICODE)
    size_t wSize = 0;
    if (::mbstowcs_s(&wSize, nullptr, 0, libpath.c_str(), libpath.size())) {
        ACSDK_ERROR(LX("libraryLoadFailed").m("failedToEstimateSize"));
        return false;
    }
    std::wstring libWPath;
    libWPath.resize(wSize);
    if (::mbstowcs_s(&wSize, &libWPath[0], libWPath.size(), libpath.c_str(), libpath.size())) {
        ACSDK_ERROR(LX("libraryLoadFailed").m("failedToConvert"));
        return false;
    }
#endif
#if WINAPI_FAMILY == WINAPI_FAMILY_APP
    m_libraryHandle = LoadPackagedLibrary(libWPath.c_str(), 0);
#elif defined(UNICODE)
    m_libraryHandle = LoadLibraryEx(libWPath.c_str(), NULL, 0);
#else
    m_libraryHandle = LoadLibraryEx(libpath.c_str(), NULL, 0);
#endif
    if (!m_libraryHandle) {
        ACSDK_ERROR(LX("libraryLoadFailed").sensitive("path", libpath).d("lastError", GetLastError()));
        return false;
    }
    bool success = false;
    ErrorCleanupGuard unloadCleanup(success, std::bind(&PKCS11Functions::unloadLibrary, this));

    CK_C_Initialize pFunc = reinterpret_cast<CK_C_Initialize>(GetProcAddress(m_libraryHandle, "C_GetFunctionList"));
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
        FreeLibrary(m_libraryHandle);
        m_libraryHandle = nullptr;
    }
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
