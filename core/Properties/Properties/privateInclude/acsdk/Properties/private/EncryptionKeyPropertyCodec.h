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

#ifndef ACSDK_PROPERTIES_PRIVATE_ENCRYPTIONKEYPROPERTYCODEC_H_
#define ACSDK_PROPERTIES_PRIVATE_ENCRYPTIONKEYPROPERTYCODEC_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>
#include <acsdk/Properties/private/Asn1Types.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief ASN.1 Codec API for Encryption Key Property Encoding.
 *
 * This class provides top-level functions to encode encryption key property into DER format or decode it from DER
 * format.
 *
 * @sa EncryptionKeyPropertyCodecState
 * @ingroup Lib_acsdkProperties
 */
struct EncryptionKeyPropertyCodec {
    typedef cryptoInterfaces::KeyStoreInterface::KeyChecksum KeyChecksum;
    typedef cryptoInterfaces::KeyStoreInterface::DataBlock DataBlock;
    typedef cryptoInterfaces::KeyStoreInterface::IV IV;
    typedef cryptoInterfaces::CryptoCodecInterface::Tag Tag;
    typedef propertiesInterfaces::PropertiesInterface::Bytes Bytes;

    /**
     * @brief Produces encryption key property in DER form.
     *
     * @param[in] cryptoFactory Crypto API factory.
     * @param[in] mainKeyAlias Main key alias.
     * @param[in] mainKeyChecksum Main key checksum.
     * @param[in] dataKeyAlgorithm Algorithm used to wrap data key.
     * @param[in] dataKeyIV Initialization vector used to wrap data key.
     * @param[in] dataKeyCiphertext Wrapped data key.
     * @param[in] dataKeyTag Data key tag.
     * @param[in] dataAlgorithm Algorithm for data encryption.
     *
     * @param[out] derEncoded Encoded properties in DER format.
     *
     * @return True on success.
     */
    static bool encode(
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::string& mainKeyAlias,
        const KeyChecksum& mainKeyChecksum,
        alexaClientSDK::cryptoInterfaces::AlgorithmType dataKeyAlgorithm,
        const IV& dataKeyIV,
        const DataBlock& dataKeyCiphertext,
        const Tag& dataKeyTag,
        alexaClientSDK::cryptoInterfaces::AlgorithmType dataAlgorithm,
        Bytes& derEncoded) noexcept;

    /**
     * @brief Decode encryption key property.
     *
     * This method parses DER input and extracts encoding fields. Method also computes actual digest for digest
     * verification.
     *
     * @param[in] cryptoFactory Crypto API factory.
     * @param[in] derEncoded DER-encoded properties.
     * @param[out] mainKeyAlias Parsed main key alias.
     * @param[out] mainKeyChecksum Parsed main key checksum.
     * @param[out] dataKeyAlgorithm Parsed algorithm for data key unwrapping.
     * @param[out] dataKeyIV Parsed initialization vector for data key unwrapping.
     * @param[out] dataKeyCiphertext Parsed wrapped data key.
     * @param[out] dataKeyTag Parsed data key tag.
     * @param[out] dataAlgorithm Parsed algorithm to encrypt/decrypt data.
     * @param[out] digestDecoded Parsed digest.
     * @param[out] digestActual Actual (recomputed) digest.
     *
     * @return True on success.
     */
    static bool decode(
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const Bytes& derEncoded,
        std::string& mainKeyAlias,
        KeyChecksum& mainKeyChecksum,
        alexaClientSDK::cryptoInterfaces::AlgorithmType& dataKeyAlgorithm,
        IV& dataKeyIV,
        DataBlock& dataKeyCiphertext,
        Tag& dataKeyTag,
        alexaClientSDK::cryptoInterfaces::AlgorithmType& dataAlgorithm,
        DataBlock& digestDecoded,
        DataBlock& digestActual) noexcept;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_ENCRYPTIONKEYPROPERTYCODEC_H_
