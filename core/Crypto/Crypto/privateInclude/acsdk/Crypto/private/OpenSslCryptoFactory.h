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

#ifndef ACSDK_CRYPTO_PRIVATE_OPENSSLCRYPTOFACTORY_H_
#define ACSDK_CRYPTO_PRIVATE_OPENSSLCRYPTOFACTORY_H_

#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>

namespace alexaClientSDK {
namespace crypto {

/**
 * @brief Cryptography factory implementation based on OpenSSL.
 *
 * @ingroup Lib_acsdkCrypto
 */
class OpenSslCryptoFactory : public alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface {
public:
    /**
     * @brief Initializes OpenSSL library and returns factory interface.
     *
     * @return Factory reference or nullptr on error.
     */
    static std::shared_ptr<CryptoFactoryInterface> create() noexcept;

    /// @name CryptoCodecInterface methods.
    ///@{
    std::unique_ptr<alexaClientSDK::cryptoInterfaces::CryptoCodecInterface> createEncoder(
        alexaClientSDK::cryptoInterfaces::AlgorithmType type) noexcept override;
    std::unique_ptr<alexaClientSDK::cryptoInterfaces::CryptoCodecInterface> createDecoder(
        alexaClientSDK::cryptoInterfaces::AlgorithmType type) noexcept override;
    std::unique_ptr<alexaClientSDK::cryptoInterfaces::DigestInterface> createDigest(
        alexaClientSDK::cryptoInterfaces::DigestType type) noexcept override;
    std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyFactoryInterface> getKeyFactory() noexcept override;
    ///@}
private:
    /// Private object constructor.
    OpenSslCryptoFactory() noexcept;

    /**
     * @brief Initializes crypto library.
     *
     * @return Boolean indicating operation success.
     */
    bool init() noexcept;

    /// Singleton instance of key factory.
    std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyFactoryInterface> m_keyFactory;
};

}  // namespace crypto
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTO_PRIVATE_OPENSSLCRYPTOFACTORY_H_
