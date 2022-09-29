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

#ifndef ACSDK_PROPERTIES_PRIVATE_ENCRYPTIONKEYPROPERTYCODECSTATE_H_
#define ACSDK_PROPERTIES_PRIVATE_ENCRYPTIONKEYPROPERTYCODECSTATE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <acsdk/Properties/private/Asn1Types.h>
#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>

namespace alexaClientSDK {
namespace properties {

using cryptoInterfaces::AlgorithmType;
using cryptoInterfaces::DigestType;

/**
 * @brief ASN.1 Codec state for encryption key property.
 *
 * This class contains data for DER encoding and decoding of encryption key property.
 *
 * @sa EncryptionKeyPropertyCodec
 * @ingroup Lib_acsdkProperties
 */
class EncryptionKeyPropertyCodecState {
public:
    /// Initialization vector data type.
    typedef cryptoInterfaces::KeyStoreInterface::IV IV;
    /// Key checksum data type.
    typedef cryptoInterfaces::KeyStoreInterface::KeyChecksum KeyChecksum;
    /// Byte vector data type.
    typedef cryptoInterfaces::KeyStoreInterface::DataBlock DataBlock;
    /// Data tag type.
    typedef cryptoInterfaces::CryptoCodecInterface::Tag Tag;

    EncryptionKeyPropertyCodecState() noexcept;
    ~EncryptionKeyPropertyCodecState() noexcept;

    /**
     * @brief Prepares object for encoding.
     *
     * This method allocates internal structures to hold properties before they are set for encoding. This method
     * must be called before any setter method.
     *
     * @return True on success.
     */
    bool prepareForEncoding() noexcept;

    /**
     * @brief Sets version property for encoding.
     *
     * @param[in] version Version value.
     *
     * @return True on success.
     */
    bool setVersion(int64_t version) noexcept;

    /**
     * @brief Get version property after decoding.
     *
     * @param[out] version Version value.
     *
     * @return True on success.
     */
    bool getVersion(int64_t& version) noexcept;

    /**
     * @brief Sets main key alias for encoding.
     *
     * @param[in] mainKeyAlias Main key alias.
     *
     * @return True on success.
     */
    bool setMainKeyAlias(const std::string& mainKeyAlias) noexcept;

    /**
     * @brief Get main key alias after decoding.
     *
     * @param[out] mainKeyAlias Main key alias.
     *
     * @return True on success.
     */
    bool getMainKeyAlias(std::string& mainKeyAlias) noexcept;

    /**
     * @brief Set main key checksum for encoding.
     *
     * @param[in] mainKeyChecksum Main key checksum.
     *
     * @return True on success.
     */
    bool setMainKeyChecksum(const KeyChecksum& mainKeyChecksum) noexcept;

    /**
     * @brief Get main key checksum after decoding.
     *
     * @param[out] mainKeyChecksum Main key checksum.
     *
     * @return True on success.
     */
    bool getMainKeyChecksum(KeyChecksum& mainKeyChecksum) noexcept;

    /**
     * @brief Set data key wrapping algorithm for encoding.
     *
     * @param[in] type Data key wrapping algorithm.
     *
     * @return True on success.
     */
    bool setDataKeyAlgorithm(AlgorithmType type) noexcept;

    /**
     * @brief Get data key wrapping algorithm after decoding.
     *
     * @param[out] type Data key wrapping algorithm.
     *
     * @return True on success.
     */
    bool getDataKeyAlgorithm(AlgorithmType& type) noexcept;

    /**
     * @brief Set data key IV for encoding.
     *
     * @param[in] dataKeyIV Initialization vector to unwrap data key.
     *
     * @return True on success.
     */
    bool setDataKeyIV(const IV& dataKeyIV) noexcept;

    /**
     * @brief Set data key IV for encoding.
     *
     * @param[out] dataKeyIV Initialization vector to unwrap data key.
     *
     * @return True on success.
     */
    bool getDataKeyIV(IV& dataKeyIV) noexcept;

    /**
     * @brief Set data key ciphertext for encoding.
     *
     * @param[in] dataKeyCiphertext Wrapped data key.
     *
     * @return True on success.
     */
    bool setDataKeyCiphertext(const DataBlock& dataKeyCiphertext) noexcept;

    /**
     * @brief Get data ciphertext after decoding.
     *
     * @param[out] dataKeyCiphertext Wrapped data key.
     *
     * @return True on success.
     */
    bool getDataKeyCiphertext(DataBlock& dataKeyCiphertext) noexcept;

    /**
     * @brief Set data key tag for encoding.
     *
     * @param[in] dataKeyTag Data key tag.
     *
     * @return True on success.
     */
    bool setDataKeyTag(const Tag& dataKeyTag) noexcept;

    /**
     * @brief Get data key tag after decoding.
     *
     * @param[out] dataKeyTag Data key tag.
     *
     * @return True on success.
     */
    bool getDataKeyTag(Tag& dataKeyTag) noexcept;

    /**
     * @brief Set data algorithm for encoding.
     *
     * @param[in] type Data encryption algorithm.
     *
     * @return True on success.
     */
    bool setDataAlgorithm(AlgorithmType type) noexcept;

    /**
     * @brief Get data algorithm after decoding.
     *
     * @param[in] type Data encryption algorithm.
     *
     * @return True on success.
     */
    bool getDataAlgorithm(AlgorithmType& type) noexcept;

    /**
     * @brief Set digest type for encoding.
     *
     * @param[in] type Digest type.
     *
     * @return True on success.
     */
    bool setDigestType(DigestType type) noexcept;

    /**
     * @brief Get digest type after decoding.
     *
     * @param[out] type Digest type.
     *
     * @return True on success.
     */
    bool getDigestType(DigestType& type) noexcept;

    /**
     * @brief Set digest for encoding.
     *
     * @param[in] digest Digest value.
     *
     * @return True on success.
     */
    bool setDigest(const DataBlock& digest) noexcept;

    /**
     * @brief Get digest after decoding.
     *
     * @param[out] digest Digest value.
     * @return True on success.
     */
    bool getDigest(DataBlock& digest) noexcept;

    /**
     * @brief Encodes data block for digest computation.
     *
     * This method encodes data block (payload without digest fields) for digest computation. Digest is computed using
     * DER-encoded input.
     *
     * @param[out] der Encoded data block.
     * @return True on success.
     */
    bool encodeEncInfo(DataBlock& der) noexcept;

    /**
     * @brief Encodes stored properties in DER form.
     *
     * @param[out] der DER-encoded properties.
     *
     * @return True on success.
     */
    bool encode(DataBlock& der) noexcept;

    /**
     * @brief Decodes DER input and internally stores decoded properties.
     *
     * @param[in] der DER-encoded properties.
     *
     * @return True on success.
     */
    bool decode(const DataBlock& der) noexcept;

private:
    EncryptionProperty* m_asn1Data;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_ENCRYPTIONKEYPROPERTYCODECSTATE_H_
