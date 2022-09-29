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

#ifndef ACSDK_CRYPTO_PRIVATE_OPENSSLKEYFACTORY_H_
#define ACSDK_CRYPTO_PRIVATE_OPENSSLKEYFACTORY_H_

#include <memory>

#include <acsdk/CryptoInterfaces/KeyFactoryInterface.h>

namespace alexaClientSDK {
namespace crypto {

using alexaClientSDK::cryptoInterfaces::AlgorithmType;
using alexaClientSDK::cryptoInterfaces::KeyFactoryInterface;

/**
 * @brief Key factory implementation based on OpenSSL.
 *
 * @ingroup Lib_acsdkCrypto
 */
class OpenSslKeyFactory : public KeyFactoryInterface {
public:
    /**
     * @brief Factory method.
     *
     * @return Key factory reference or nullptr.
     */
    static std::shared_ptr<KeyFactoryInterface> create() noexcept;

    /// @name KeyFactoryInterface methods.
    ///@{
    bool generateKey(AlgorithmType type, Key& key) noexcept override;
    bool generateIV(AlgorithmType type, IV& iv) noexcept override;
    ///@}
private:
    /// Private constructor.
    OpenSslKeyFactory() noexcept;

    /**
     * @brief Helper method to generate random output.
     *
     * @param[out]  data    Random data destination.
     * @param[in]   size    Result size.
     *
     * @return Boolean, indicating operation success.
     */
    bool generateRandom(std::vector<unsigned char>& data, int size) noexcept;
};

}  // namespace crypto
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTO_PRIVATE_OPENSSLKEYFACTORY_H_
