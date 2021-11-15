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

#ifndef ACSDKCRYPTO_PRIVATE_OPENSSLCRYPTOFACTORY_H_
#define ACSDKCRYPTO_PRIVATE_OPENSSLCRYPTOFACTORY_H_

#include <acsdkCryptoInterfaces/CryptoFactoryInterface.h>

namespace alexaClientSDK {
namespace acsdkCrypto {

/**
 * @brief Cryptography factory implementation based on OpenSSL.
 *
 * @ingroup CryptoIMPL
 */
class OpenSslCryptoFactory : public alexaClientSDK::acsdkCryptoInterfaces::CryptoFactoryInterface {
public:
    /**
     * @brief Initializes OpenSSL library and returns factory interface.
     *
     * @return Factory reference or nullptr on error.
     */
    static std::shared_ptr<CryptoFactoryInterface> create() noexcept;

    /// @name CryptoCodecInterface methods.
    ///@{
    std::unique_ptr<alexaClientSDK::acsdkCryptoInterfaces::CryptoCodecInterface> createEncoder(
        alexaClientSDK::acsdkCryptoInterfaces::AlgorithmType type) noexcept override;
    std::unique_ptr<alexaClientSDK::acsdkCryptoInterfaces::CryptoCodecInterface> createDecoder(
        alexaClientSDK::acsdkCryptoInterfaces::AlgorithmType type) noexcept override;
    std::unique_ptr<alexaClientSDK::acsdkCryptoInterfaces::DigestInterface> createDigest(
        alexaClientSDK::acsdkCryptoInterfaces::DigestType type) noexcept override;
    std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::KeyFactoryInterface> getKeyFactory() noexcept override;
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
    std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::KeyFactoryInterface> m_keyFactory;
};

}  // namespace acsdkCrypto
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTO_PRIVATE_OPENSSLCRYPTOFACTORY_H_
