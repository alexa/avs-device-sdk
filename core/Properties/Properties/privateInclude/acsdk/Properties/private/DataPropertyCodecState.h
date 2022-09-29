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

#ifndef ACSDK_PROPERTIES_PRIVATE_DATAPROPERTYCODECSTATE_H_
#define ACSDK_PROPERTIES_PRIVATE_DATAPROPERTYCODECSTATE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <acsdk/Properties/private/Asn1Types.h>
#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>

namespace alexaClientSDK {
namespace properties {

using cryptoInterfaces::DigestType;

/**
 * @brief Helper state for holding ASN.1 structures of DER codec for encrypted property value.
 *
 * @sa DataPropertyCodec
 * @ingroup Lib_acsdkProperties
 */
class DataPropertyCodecState {
public:
    /// @brief Initialization vector data type.
    typedef cryptoInterfaces::CryptoCodecInterface::IV IV;

    /// @brief Byte vector data type.
    typedef cryptoInterfaces::CryptoCodecInterface::DataBlock DataBlock;

    /// @brief Data tag type.
    typedef cryptoInterfaces::CryptoCodecInterface::Tag Tag;

    /// @brief Object constructor
    DataPropertyCodecState() noexcept;

    /// @brief Releases internal structures.
    ~DataPropertyCodecState() noexcept;

    /**
     * @brief Prepares structure for encoding operations.
     *
     * This method allocates internal structures and must be called before any setter method.
     *
     * @return True if operation is successful.
     */
    bool prepareForEncode() noexcept;

    /**
     * @brief Sets encoding version property.
     *
     * This method must be called after #prepareForEncode().
     *
     * @param[in] version Version number.
     *
     * @return True if operation is successful.
     */
    bool setVersion(int64_t version) noexcept;

    /**
     * @brief Sets initialization vector property.
     *
     * This method must be called after #prepareForEncode().
     *
     * @param[in] dataIV Initialization vector data.
     *
     * @return True if operation is successful.
     */
    bool setDataIV(const IV& dataIV) noexcept;

    /**
     * @brief Sets data ciphertext property.
     *
     * This method must be called after #prepareForEncode().
     *
     * @param[in] dataCiphertext Ciphertext data.
     *
     * @return True if operation is successful.
     */
    bool setDataCiphertext(const DataBlock& dataCiphertext) noexcept;

    /**
     * @brief Sets data tag property.
     *
     * This method must be called after #prepareForEncode().
     *
     * @param[in] dataTag Tag data.
     *
     * @return True if operation is successful.
     */
    bool setDataTag(const Tag& dataTag) noexcept;

    /**
     * @brief Sets digest type property.
     *
     * This method must be called after #prepareForEncode().
     *
     * @param[in] type Digest type.
     *
     * @return True if operation is successful.
     */
    bool setDigestType(DigestType type) noexcept;

    /**
     * @brief Sets digest property.
     *
     * This method must be called after #prepareForEncode().
     *
     * @param[in] digest Digest data.
     *
     * @return True if operation is successful.
     */
    bool setDigest(const DataBlock& digest) noexcept;

    /**
     * @brief Produces DER format according to stored properties.
     *
     * @param[out] der Reference to store  DER-encoded data.
     *
     * @return True if operation is successful.
     */
    bool encode(DataBlock& der) noexcept;

    /**
     * @brief Method provides encoding version property.
     *
     * @param[out] version Reference to store encoding version result.
     *
     * @return True if operation is successful.
     */
    bool getVersion(int64_t& version) noexcept;

    /**
     * @brief Method provides data initialization vector.
     *
     * @param[out] dataIV Reference to store data initialization vector result.
     *
     * @return True if operation is successful.
     */
    bool getDataIV(IV& dataIV) noexcept;

    /**
     * @brief Method provides data ciphertext.
     *
     * @param[out] dataCiphertext Reference to store data ciphertext result.
     *
     * @return True if operation is successful.
     */
    bool getDataCiphertext(DataBlock& dataCiphertext) noexcept;

    /**
     * @brief Method provides data tag.
     *
     * @param[out] dataTag Reference to store data tag result.
     *
     * @return True if operation is successful.
     */
    bool getDataTag(Tag& dataTag) noexcept;

    /**
     * @brief Method provides digest type.
     *
     * @param[out] type Reference to store digest type.
     *
     * @return True if operation is successful.
     */
    bool getDigestType(DigestType& type) noexcept;

    /**
     * @brief Method provides digest value.
     *
     * @param[out] digest Reference to store digest value.
     * @return True if operation is successful.
     */
    bool getDigest(DataBlock& digest) noexcept;

    /**
     * @brief Method to decode property fields from DER-encoded input.
     *
     * @param[in] der Der-encoded input.
     *
     * @return True if operation is successful.
     */
    bool decode(const DataBlock& der) noexcept;

    /**
     * Method encodes payload sequence for computing digest. DER-specification doesn't let multiple ways
     * to encode the same data set, so the result will depend only on supplied values (either from setters
     * or from decoding result).
     *
     * @param[out] der Reference to store result.
     * @return True if operation is successful.
     */
    bool encodeEncInfo(DataBlock& der) noexcept;

private:
    DataProperty* m_asn1Data;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_DATAPROPERTYCODECSTATE_H_
