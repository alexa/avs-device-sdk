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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11SLOT_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11SLOT_H_

#include <memory>
#include <string>

#include <acsdk/Pkcs11/private/PKCS11API.h>

namespace alexaClientSDK {
namespace pkcs11 {

class PKCS11Functions;
class PKCS11Session;

/**
 * @brief PKCS11 slot wrapper.
 *
 * This class wraps PKCS#11 slot handle and related operations.
 * @ingroup Lib_acsdkPkcs11
 */
class PKCS11Slot {
public:
    /**
     * @brief Constructs slot object.
     *
     * @param[in] functions Owner with function table.
     * @param[in] slotId    PKCS11 slot identifier.
     */
    PKCS11Slot(std::shared_ptr<PKCS11Functions>&& functions, CK_SLOT_ID slotId) noexcept;

    /**
     * @brief Returns token name.
     *
     * @param[out] tokenName Token name.
     *
     * @return True if operation is successful.
     */
    bool getTokenName(std::string& tokenName) noexcept;

    /**
     * @brief Opens a new session for this slot.
     *
     * @return Session reference or nullptr on error.
     */
    std::shared_ptr<PKCS11Session> openSession() noexcept;

private:
    /// @brief Owner object.
    std::shared_ptr<PKCS11Functions> m_functions;

    /// @brief Slot identifier
    CK_SLOT_ID m_slotID;
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11SLOT_H_
