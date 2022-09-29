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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11SESSION_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11SESSION_H_

#include <memory>
#include <mutex>
#include <string>

#include <acsdk/Pkcs11/private/PKCS11KeyDescriptor.h>

namespace alexaClientSDK {
namespace pkcs11 {

class PKCS11Functions;
class PKCS11Key;
using alexaClientSDK::cryptoInterfaces::AlgorithmType;

/**
 * @brief PKCS11 session wrapper.
 *
 * This class wraps PKCS#11 API session handle and related operations.
 * @ingroup Lib_acsdkPkcs11
 */
class PKCS11Session : public std::enable_shared_from_this<PKCS11Session> {
public:
    /// Public destructor (closes session).
    ~PKCS11Session() noexcept;

    /**
     * @brief Log into HSM.
     *
     * @param[in] userPin User PIN.
     *
     * @return True if operation is successful.
     */
    bool logIn(const std::string& userPin) noexcept;

    /**
     * @brief Log out from HSM.
     *
     * @return True if operation is successful.
     */
    bool logOut() noexcept;

    /**
     * @brief Loads existing key.
     *
     * @param[in] descriptor Key parameters.
     *
     * @return Key reference or nullptr on error.
     */
    std::unique_ptr<PKCS11Key> findKey(const PKCS11KeyDescriptor& descriptor) noexcept;

private:
    friend class PKCS11Key;
    friend class PKCS11Slot;

    /**
     * @brief Creates session object.
     *
     * @param[in] functions Owner object.
     * @param[in] sessionHandle Session handle.
     */
    PKCS11Session(const std::shared_ptr<PKCS11Functions>& functions, CK_SESSION_HANDLE sessionHandle) noexcept;

    /**
     * @brief Closes session.
     *
     * @return True if operation is successful.
     */
    bool closeSession() noexcept;

    /// Mutex object to ensure session calls are serialzied.
    std::mutex m_mutex;

    /// Owner object reference.
    std::shared_ptr<PKCS11Functions> m_functions;

    /// PKCS11 session handle.
    CK_SESSION_HANDLE m_sessionHandle;
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11SESSION_H_
