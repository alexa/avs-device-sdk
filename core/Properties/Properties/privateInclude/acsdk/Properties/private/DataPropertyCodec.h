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

#ifndef ACSDK_PROPERTIES_PRIVATE_DATAPROPERTYCODEC_H_
#define ACSDK_PROPERTIES_PRIVATE_DATAPROPERTYCODEC_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief ASN.1 Encoder/Decoder for encrypted property value.
 *
 * This class provides top-level functions to encode encryption property value into DER format or decode it from DER
 * format.
 *
 * @sa DataPropertyCodecState
 * @ingroup Lib_acsdkProperties
 */
struct DataPropertyCodec {
    /// Initialization vector data type.
    typedef cryptoInterfaces::CryptoCodecInterface::IV IV;
    /// Byte vector data type.
    typedef cryptoInterfaces::CryptoCodecInterface::DataBlock DataBlock;
    /// Tag data type.
    typedef cryptoInterfaces::CryptoCodecInterface::Tag Tag;

    /**
     * @brief Encodes encrypted property value into DER form.
     *
     * @param[in] cryptoFactory Crypto factory for digest operations. Must not be nullptr.
     * @param[in] dataIV Initialization vector for encrypted data.
     * @param[in] dataCiphertext Encrypted data.
     * @param[in] dataTag Data tag.
     * @param[out] derEncoded Reference to output data buffer.
     *
     * @return True if operation is successful.
     */
    static bool encode(
        const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const IV& dataIV,
        const DataBlock& dataCiphertext,
        const Tag& dataTag,
        DataBlock& derEncoded) noexcept;

    /**
     * @brief Decodes encrypted property value from DER form.
     *
     * @param[in] cryptoFactory Crypto factory for digest operations. Must not be nullptr.
     * @param[in] derEncoded DER-encoded property value.
     * @param[out] dataIV Reference to container for initialization vector of encrypted data.
     * @param[out] dataCiphertext Reference to container for encrypted data.
     * @param[out] dataTag Reference to container for data tag.
     * @param[out] digestDecoded Reference to container for decoded digest (from the DER message).
     * @param[out] digestActual Reference to container for actual (recomputed) digest.
     *
     * @return True if operation is successful.
     */
    static bool decode(
        const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::vector<uint8_t>& derEncoded,
        IV& dataIV,
        DataBlock& dataCiphertext,
        Tag& dataTag,
        DataBlock& digestDecoded,
        DataBlock& digestActual) noexcept;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_DATAPROPERTYCODEC_H_
