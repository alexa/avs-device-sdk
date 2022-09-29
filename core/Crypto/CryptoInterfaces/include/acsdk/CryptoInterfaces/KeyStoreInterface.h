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

#ifndef ACSDK_CRYPTOINTERFACES_KEYSTOREINTERFACE_H_
#define ACSDK_CRYPTOINTERFACES_KEYSTOREINTERFACE_H_

#include <memory>
#include <string>
#include <vector>

#include "AlgorithmType.h"

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Key Store Interface.
 *
 * Interface provides integration with platform-specific key storage and operations. The vendor can choose how to
 * implement this interface for a best security.
 *
 * This interface enables data encryption and decryption without accessing encryption key data. Keys must be provided
 * by device manufacturer (vendor), and cryptography functions access those keys through key aliases.
 *
 * ACSDK provides a reference implementation of this interface to integrate with Hardware Security Module through
 * PKCS#11 API.
 *
 * ### Thread Safety
 *
 * This interface is thread safe and can be used concurrently by different threads.
 *
 * @sa Lib_acsdkPkcs11
 * @ingroup Lib_acsdkCryptoInterfaces
 */
class KeyStoreInterface {
public:
    /// @brief Data type for data block (encrypted or unencrypted).
    typedef std::vector<unsigned char> DataBlock;

    /// @brief Data type for initialization vector data.
    typedef std::vector<unsigned char> IV;

    /// @brief Data type for key checksum.
    typedef std::vector<unsigned char> KeyChecksum;

    /// @brief Data type for tag.
    /// Tag (known as Message Authentication Code) is used with AEAD mode of operation like with Galois/Counter mode.
    typedef std::vector<unsigned char> Tag;

    /// @brief Default destructor.
    virtual ~KeyStoreInterface() noexcept = default;

    /**
     * @brief Encrypts data block.
     *
     * This method encrypts data block. The method locates the key, checks if the key type supports the algorithm,
     * and performs encryption using provided initialization vector. As a result, the method provides key checksum
     * (if supported), and encrypted content.
     *
     * @param[in]   keyAlias    Key alias.
     * @param[in]   type        Algorithm type to use. The method will fail, if @a type is AEAD algorithm like AES-GCM.
     * @param[in]   iv          Initialization vector.
     * @param[in]   plaintext   Data to encrypt.
     * @param[out]  checksum    Key checksum. The method appends data to @a checksum if this attribute is supported by
     *                          implementation.
     * @param[out]  ciphertext  Encrypted data. The method appends data to @a ciphertext container.
     *
     * @return Boolean indicating operation success. If operation fails, the contents of @a checksum and @a ciphertext
     * are undefined.
     */
    virtual bool encrypt(
        const std::string& keyAlias,
        AlgorithmType type,
        const IV& iv,
        const DataBlock& plaintext,
        KeyChecksum& checksum,
        DataBlock& ciphertext) noexcept = 0;

    /**
     * @brief Encrypts data block using authenticated encryption algorithm.
     *
     * Method encrypts data block using authenticated encryption. The method locates the key, checks if the key type
     * supports the algorithm, and performs encryption using provided initialization vector and additional
     * authenticated data. As a result, the method provides key checksum (if supported), authentication tag (also known
     * as Message Authentication Code/MAC), and encrypted content.
     *
     * @param[in]    keyAlias    Key alias.
     * @param[in]    type        Algorithm type to use. The method will fail, if @a type is not AEAD algorithm like
     *                           AES-GCM.
     * @param[in]    iv          Initialization vector.
     * @param[in]    aad         Additional authenticated data. This data works as an input to encryption function to
     *                           ensure that the resulting ciphertext can be decrypted only with the same AAD.
     * @param[in]    plaintext   Data to encrypt.
     * @param[out]   checksum    Key checksum. The method appends data to @a checksum if this attribute is supported by
     *                           implementation.
     * @param[out]   ciphertext  Encrypted data. The method appends data to @a ciphertext container.
     * @param[out]   tag         Authentication tag (also known as MAC). Authentication tag must be provided to
     *                           decryption function to prevent data tampering. The method appends data to @a tag
     *                           container.
     *
     * @return Boolean indicating operation success. If operation fails, the contents of @a checksum, @a ciphertext, and
     * @a tag are undefined.
     *
     * @see #decryptAD()
     */
    virtual bool encryptAE(
        const std::string& keyAlias,
        AlgorithmType type,
        const IV& iv,
        const DataBlock& aad,
        const DataBlock& plaintext,
        KeyChecksum& checksum,
        DataBlock& ciphertext,
        Tag& tag) noexcept = 0;

    /**
     * @brief Decrypts data block.
     *
     * Method decrypts data block. The method locates the key, checks if key type supports requested algorithm and has
     * matching checksum (if checksum is supported), and performs decryption.
     *
     * @param[in]   keyAlias    Key alias.
     * @param[in]   type        Algorithm type to use. The method will fail, if @a type is AEAD algorithm like AES-GCM.
     * @param[in]   checksum    Key checksum if available. If implementation doesn't support checksum, the value of
     *                          this parameter is ignored. The system checks checksum against checksum of a currently
     *                          available key before decrypting data to ensure we don't try to use a different key,
     *                          then the one, that has been used during encryption.
     * @param[in]   iv          Initialization vector. This vector must match have the same value, as the one used when
     *                          encrypting data.
     * @param[in]   ciphertext  Data to decrypt.
     * @param[out]  plaintext   Decrypted data. This method appends data to @a plaintext.
     *
     * @return Boolean indicating operation success. If operation fails, the contents of @a plaintext is undefined.
     */
    virtual bool decrypt(
        const std::string& keyAlias,
        AlgorithmType type,
        const KeyChecksum& checksum,
        const IV& iv,
        const DataBlock& ciphertext,
        DataBlock& plaintext) noexcept = 0;

    /**
     * @brief Decrypts data block using authenticated decryption algorithm.
     *
     * Method decrypts data block using additional authenticated data and authentication tag (also known as Message
     * Authentication Code/MAC). This method locates the key, checks if key type supports requested algorithm and has
     * matching checksum (if checksum is supported), and performs decryption.
     *
     * @param[in]   keyAlias    Key alias.
     * @param[in]   type        Algorithm type to use. The method will fail, if @a type is not AEAD algorithm like
     *                          AES-GCM.
     * @param[in]   checksum    Key checksum if available. If implementation doesn't support checksum, the value of this
     *                          parameter is ignored. The system checks checksum against checksum of a currently
     *                          available key before decrypting data to ensure we don't try to use a different key, then
     *                          the one, that has been used during encryption.
     * @param[in]   iv          Initialization vector. This vector must match have the same value, as the one used when
     *                          encrypting data.
     * @param[in]   aad         Additional authenticated data. This data must match AAD used when encrypting the
     *                          content. Decryption will fail if the data doesn't match.
     * @param[in]   ciphertext  Data to decrypt.
     * @param[in]   tag         Authentication tag (also known as MAC). The algorithm uses tag from encryption
     *                          algorithm to check if the data has been tampered.
     * @param[in]   plaintext   Decrypted data. This method appends data to @a plaintext.
     *
     * @return Boolean indicating operation success. If operation fails, the contents of @a plaintext is undefined.
     *
     * @see #encryptAE()
     */
    virtual bool decryptAD(
        const std::string& keyAlias,
        AlgorithmType type,
        const KeyChecksum& checksum,
        const IV& iv,
        const DataBlock& aad,
        const DataBlock& ciphertext,
        const Tag& tag,
        DataBlock& plaintext) noexcept = 0;

    /**
     * @brief Returns default key alias.
     *
     * Get default key alias. Any component can have component-specific configuration or use default configuration.
     *
     * Default key alias is a platform configuration parameter, and may change over time. When the alias changes,
     * implementation must use new alias to encrypt new data, and must use old alias to decrypt existing data as long
     * as the old key exists.
     *
     * @param[out] keyAlias Reference to key alias. The method replaces contents of @a keyAlias.
     *
     * @return Returns true if main key alias is stored into @a keyAlias. Returns false on error.
     */
    virtual bool getDefaultKeyAlias(std::string& keyAlias) noexcept = 0;
};

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_KEYSTOREINTERFACE_H_
