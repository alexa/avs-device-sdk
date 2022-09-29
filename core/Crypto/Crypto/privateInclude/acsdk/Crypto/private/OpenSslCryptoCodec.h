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

#ifndef ACSDK_CRYPTO_PRIVATE_OPENSSLCRYPTOCODEC_H_
#define ACSDK_CRYPTO_PRIVATE_OPENSSLCRYPTOCODEC_H_

#include <memory>

#include <openssl/evp.h>

#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/CryptoInterfaces/CryptoCodecInterface.h>
#include <acsdk/Crypto/private/OpenSslTypes.h>

namespace alexaClientSDK {
namespace crypto {

using alexaClientSDK::cryptoInterfaces::AlgorithmType;
using alexaClientSDK::cryptoInterfaces::CryptoCodecInterface;

/**
 * @brief Binary codec implementation.
 *
 * This class uses EVP API from OpenSSL library. We can add new algorithms as needed.
 *
 * @ingroup Lib_acsdkCrypto
 */
class OpenSslCryptoCodec : public CryptoCodecInterface {
public:
    /**
     * @brief Create decoder.
     *
     * Factory method to create decoder for an encryption algorithm.
     *
     * @param[in] type Encryption algorithm.
     *
     * @return Codec reference or nullptr on error.
     */
    static std::unique_ptr<OpenSslCryptoCodec> createDecoder(AlgorithmType type) noexcept;
    /**
     * @brief Create encoder.
     *
     * Factory method to create encoder for an encryption algorithm.
     *
     * @param[in] type Encryption algorithm.
     *
     * @return Codec reference or nullptr on error.
     */
    static std::unique_ptr<OpenSslCryptoCodec> createEncoder(AlgorithmType type) noexcept;

    /// @name CryptoCodecInterface methods.
    ///@{
    ~OpenSslCryptoCodec() noexcept override;
    bool init(const Key& key, const IV& iv) noexcept override;
    bool processAAD(const DataBlock& dataIn) noexcept override;
    bool processAAD(DataBlock::const_iterator dataInBegin, DataBlock::const_iterator dataInEnd) noexcept override;
    bool process(const DataBlock& dataIn, DataBlock& dataOut) noexcept override;
    bool process(
        DataBlock::const_iterator dataInBegin,
        DataBlock::const_iterator dataInEnd,
        DataBlock& dataOut) noexcept override;
    bool finalize(DataBlock& dataOut) noexcept override;
    bool getTag(Tag& tag) noexcept override;
    bool setTag(const Tag& tag) noexcept override;
    ///@}

private:
    /**
     * @brief Creates encoder or decoder.
     *
     * Creates encoder or decoder depending on arguments.
     *
     * @param[in] type      Algorithm type.
     * @param[in] codecType Codec type.
     *
     * @return Codec reference or nullptr on error.
     */
    static std::unique_ptr<OpenSslCryptoCodec> createCodec(AlgorithmType type, CodecType codecType) noexcept;

    /**
     * @brief Private constructor.
     *
     * @param[in] codecType     Codec type.
     * @param[in] algorithmType Cipher type.
     */
    OpenSslCryptoCodec(CodecType codecType, AlgorithmType algorithmType) noexcept;

    /**
     * @brief Checks if the algorithm is authenticated encryption and decryption.
     *
     * @return True, if @a m_algorithmType is from AEAD family.
     */
    bool isAEADCipher() noexcept;

    /// Codec type.
    const CodecType m_codecType;

    /// Codec cipher type.
    const AlgorithmType m_algorithmType;

    /// Encryption context reference.
    EVP_CIPHER_CTX* m_cipherCtx;

    /// Codec state.
    bool m_initDone;

    /// OpenSSL cipher object.
    const EVP_CIPHER* m_cipher;
};

}  // namespace crypto
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTO_PRIVATE_OPENSSLCRYPTOCODEC_H_
