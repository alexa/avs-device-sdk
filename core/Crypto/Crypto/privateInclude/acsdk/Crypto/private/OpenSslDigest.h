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

#ifndef ACSDK_CRYPTO_PRIVATE_OPENSSLDIGEST_H_
#define ACSDK_CRYPTO_PRIVATE_OPENSSLDIGEST_H_

#include <memory>

#include <openssl/evp.h>

#include <acsdk/CryptoInterfaces/DigestInterface.h>
#include <acsdk/CryptoInterfaces/DigestType.h>

namespace alexaClientSDK {
namespace crypto {

using alexaClientSDK::cryptoInterfaces::DigestInterface;
using alexaClientSDK::cryptoInterfaces::DigestType;

/**
 * @brief Digest implementation based on OpenSSL.
 *
 * @ingroup Lib_acsdkCrypto
 */
class OpenSslDigest : public DigestInterface {
public:
    /**
     * @brief Creates a new digest instance.
     *
     * @param[in] type Digest type.
     *
     * @return Digest reference or nullptr on error.
     */
    static std::unique_ptr<OpenSslDigest> create(DigestType type) noexcept;

    /// @name DigestInterface methods.
    ///@{
    ~OpenSslDigest() noexcept override;
    bool process(const DataBlock& data_in) noexcept override;
    bool process(DataBlock::const_iterator begin, DataBlock::const_iterator end) noexcept override;
    bool processByte(unsigned char value) noexcept override;
    bool processUInt8(uint8_t value) noexcept override;
    bool processUInt16(uint16_t value) noexcept override;
    bool processUInt32(uint32_t value) noexcept override;
    bool processUInt64(uint64_t value) noexcept override;
    bool processString(const std::string& value) noexcept override;
    bool finalize(DataBlock& digest) noexcept override;
    bool reset() noexcept override;
    ///@}
private:
    /// @brief Private constructor.
    OpenSslDigest() noexcept;

    /**
     * @brief Initializes digest.
     *
     * @param[in] type Digest type.
     *
     * @return Boolean indicating operation success.
     */
    bool init(DigestType type) noexcept;

    /// Digest algorithm reference.
    const EVP_MD* m_md;

    /// Digest context structure.
    EVP_MD_CTX* m_ctx;
};

}  // namespace crypto
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTO_PRIVATE_OPENSSLDIGEST_H_
