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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11KEYDESCRIPTOR_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11KEYDESCRIPTOR_H_

#include <functional>
#include <string>

#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/Pkcs11/private/PKCS11API.h>

namespace alexaClientSDK {
namespace pkcs11 {

/**
 * @brief Class to identify key object in HSM.
 *
 * HSM objects do not have unique parameters other than object ID. So several HSM objects may have the same label, but
 * different types or the same label and type, and different size.
 *
 * This object provides criteria for looking up key objects in HSM.
 */
struct PKCS11KeyDescriptor {
    /**
     * @brief Key object label.
     */
    std::string objectLabel;

    /**
     * @brief Key object type.
     *
     * AES ciphers use @a CKK_AES. HMAC-SHA-256 digest may use @a CKK_GENERIC_SECRET or @a CKK_SHA256_HMAC.
     */
    CK_KEY_TYPE keyType;

    /**
     * @brief Key length in bytes.
     */
    CK_ULONG keyLen;

    /**
     * @brief Default move constructor.
     *
     * @param[in] arg Object to move values from.
     */
    PKCS11KeyDescriptor(PKCS11KeyDescriptor&& arg) noexcept = default;

    /**
     * @brief Create object with alias and encryption algorithm.
     *
     * @param[in] objectLabel Object label.
     * @param[in] algorithmType Algorithm type.
     */
    PKCS11KeyDescriptor(const std::string& objectLabel, cryptoInterfaces::AlgorithmType algorithmType) noexcept;

    /**
     * @brief Create object with given parameters.
     *
     * @param[in] objectLabel Object label.
     * @param[in] keyType PKCS#11 key type.
     * @param[in] keyLen PKCS#11 key size.
     */
    PKCS11KeyDescriptor(const std::string& objectLabel, CK_KEY_TYPE keyType, CK_ULONG keyLen) noexcept;

    /**
     * @brief Maps algorithm type into key type and length
     *
     * @param[in] algorithmType Algorithm type.
     * @param[out] keyType Key type
     * @param[out] keyLen Key length.
     *
     * @return Returns true on success.
     */
    static bool mapAlgorithmToKeyParams(
        cryptoInterfaces::AlgorithmType algorithmType,
        CK_KEY_TYPE& keyType,
        CK_ULONG& keyLen);
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

namespace std {

/**
 * @brief Hash support for PKCS11KeyDescriptor.
 */
template <>
struct hash<alexaClientSDK::pkcs11::PKCS11KeyDescriptor> {
    /**
     * @brief Compute hash code.
     *
     * @param[in] arg Key descriptor.
     *
     * @return Computed hash code.
     */
    std::size_t operator()(const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg) const noexcept;
};

/**
 * @brief Comparison support PKCS11KeyDescriptor.
 */
template <>
struct equal_to<alexaClientSDK::pkcs11::PKCS11KeyDescriptor> {
    /**
     * @brief Compares two PKCS11KeyDescriptor instances.
     *
     * @param[in] arg1 First key descriptor to compare.
     * @param[in] arg2 Second key descriptor to compare.
     *
     * @return True if instances are equal.
     */
    bool operator()(
        const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg1,
        const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg2) const noexcept;
};

// Dumps PKCS11KeyDescriptor data into stream (for logging).
std::ostream& operator<<(std::ostream& out, const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg) noexcept;

}  // namespace std

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11KEYDESCRIPTOR_H_
