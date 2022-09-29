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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11KEY_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11KEY_H_

#include <memory>
#include <string>

#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/Pkcs11/private/PKCS11API.h>

namespace alexaClientSDK {
namespace pkcs11 {

class PKCS11Session;

using alexaClientSDK::cryptoInterfaces::AlgorithmType;

/**
 * @brief PKCS11 key object wrapper.
 *
 * This class wraps PKCS#11 key handle and related operations.
 * @ingroup Lib_acsdkPkcs11
 */
class PKCS11Key {
public:
    /**
     * @brief Create key object with parameters.
     *
     * @param[in] session Owner session.
     * @param[in] keyHandle PKCS11 ket object handle.
     */
    PKCS11Key(std::shared_ptr<PKCS11Session>&& session, CK_OBJECT_HANDLE keyHandle) noexcept;

    /**
     * @brief Method to check if key has a correct type and supports given algorithm type.
     *
     * @param[in] type Algorithm type.
     *
     * @return True if key supports given algorithm type, False on error or if key doesn't support the algorithm.
     */
    bool isCompatible(AlgorithmType type) noexcept;

    /**
     * @brief Method to query key attributes.
     *
     * This method queries key CKA_CHECKSUM (if it supported) and CKA_NEVER_EXTRACTABLE flags.
     *
     * @param[out] checksum         Key checksum if it is available. The value can be empty if HSM doesn't support
     *                              checksums.
     * @param[out] neverExtractable Flag if the key has never been extracted.
     *
     * @return True on success, False on error.
     */
    bool getAttributes(std::vector<unsigned char>& checksum, bool& neverExtractable) noexcept;

    /**
     * @brief Function to encrypt data with given parameters.
     *
     * @param[in]   algorithmType   Algorithm to use.
     * @param[in]   iv              Initialization vector.
     * @param[in]   aad             Additional authenticated data.
     * @param[in]   plaintext       Unencrypted data.
     * @param[out]  ciphertext      Encrypted data.
     * @param[out]  tag             Message authentication code.
     *
     * @return True if operation is successful.
     */
    bool encrypt(
        AlgorithmType algorithmType,
        const std::vector<unsigned char>& iv,
        const std::vector<unsigned char>& aad,
        const std::vector<unsigned char>& plaintext,
        std::vector<unsigned char>& ciphertext,
        std::vector<unsigned char>& tag) noexcept;

    /**
     * @brief Function to decrypt data with given parameters.
     *
     * @param[in]   algorithmType   Algorithm to use.
     * @param[in]   iv              Initialization vector.
     * @param[in]   aad             Additional authenticated data.
     * @param[in]   ciphertext      Encrypted data.
     * @param[in]   tag             Authentication tag.
     * @param[out]  plaintext       Decrypted data.
     *
     * @return True if operation is successful.
     */
    bool decrypt(
        AlgorithmType algorithmType,
        const std::vector<unsigned char>& iv,
        const std::vector<unsigned char>& aad,
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& tag,
        std::vector<unsigned char>& plaintext) noexcept;

    /**
     * @brief Configure PKCS#11 mechanism according to parameters.
     *
     * @param[in]   mechanismType   Type of encryption.
     * @param[in]   iv              Initialization vector.
     * @param[in]   aad             Additional authenticaiton data.
     * @param[out]  params          Mechanism parameters for PKCS#11 calls.
     * @param[out]  gcmParams       GCM-specific parameters for PKCS#11 calls.
     *
     * @return True if operation is successful.
     */
    bool configureMechanism(
        CK_MECHANISM_TYPE mechanismType,
        const std::vector<unsigned char>& iv,
        const std::vector<unsigned char>& aad,
        CK_MECHANISM& params,
        CK_GCM_PARAMS& gcmParams) noexcept;

private:
    /// Owner session object.
    std::shared_ptr<PKCS11Session> m_session;

    /// PKCS11 key handle.
    CK_OBJECT_HANDLE m_keyHandle;
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11KEY_H_
