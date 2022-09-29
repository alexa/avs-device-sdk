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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11FUNCTIONS_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11FUNCTIONS_H_

#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <acsdk/Pkcs11/private/PKCS11API.h>

namespace alexaClientSDK {
namespace pkcs11 {

class PKCS11Key;
class PKCS11Slot;
class PKCS11Session;
class PKCS11Token;

/**
 * @brief PKCS11 API Wrapper.
 *
 * This class manages library load, initialization, and slot operations.
 *
 * @ingroup Lib_acsdkPkcs11
 */
class PKCS11Functions : public std::enable_shared_from_this<PKCS11Functions> {
public:
    /**
     * Creates object.
     * @param libpath File path to PKCS11 library.
     * @return
     */
    static std::shared_ptr<PKCS11Functions> create(const std::string& libpath) noexcept;

    /**
     * @brief Releases library.
     */
    virtual ~PKCS11Functions() noexcept;

    /**
     * @brief Lists available PKCS11 slots by type.
     *
     * @param[in]   tokenPresent    Flag, if list slots with tokens (when true), or all slots.
     * @param[out]  slots           Discovered slot names.
     *
     * @return True if operation is successful.
     */
    bool listSlots(bool tokenPresent, std::vector<std::shared_ptr<PKCS11Slot>>& slots) noexcept;

    /**
     * @brief Finds PKCS11 slot by name.
     *
     * @param[in]   tokenName   Token name.
     * @param[out]  slot        Discovered slot or nullptr if slot with given name doesn't exist.
     *
     * @return True if operation is successful. If operation fails, the value of \a slot is unchanged.
     */
    bool findSlotByTokenName(const std::string& tokenName, std::shared_ptr<PKCS11Slot>& slot) noexcept;

private:
    friend class PKCS11Key;
    friend class PKCS11Session;
    friend class PKCS11Token;
    friend class PKCS11Slot;

    /// @brief Private constructor.
    PKCS11Functions() noexcept;

    /**
     * @brief Helper to initialize object and prepare for operations.
     *
     * @return True if opeation is successful.
     */
    bool initialize() noexcept;
    /**
     * Helper to load PKCS11 library and discover function table.
     *
     * @param[in] libpath File path to PKCS11 interface library.
     *
     * @return True if operation is successful.
     */
    bool loadLibraryAndGetFunctions(const std::string& libpath) noexcept;

    /**
     * @brief Method to finalize operations and release PKCS11 module.
     */
    void finalizeOperations() noexcept;

    /**
     * @brief Helper to unload PKCS11 libary.
     */
    void unloadLibrary() noexcept;

#ifdef _WIN32
    /// @brief Loaded library handle.
    HMODULE m_libraryHandle;
#else
    /// @brief Loaded library handle.
    void* m_libraryHandle;
#endif
    /// @brief PKCS11 function table.
    CK_FUNCTION_LIST* m_pkcs11Functions;
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11FUNCTIONS_H_
