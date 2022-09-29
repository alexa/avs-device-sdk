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

#ifndef ACSDK_CRYPTOINTERFACES_CRYPTOFACTORYINTERFACE_H_
#define ACSDK_CRYPTOINTERFACES_CRYPTOFACTORYINTERFACE_H_

#include <memory>

#include "AlgorithmType.h"
#include "KeyFactoryInterface.h"
#include "CryptoCodecInterface.h"
#include "DigestInterface.h"
#include "DigestType.h"

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Crypto API factory interface.
 *
 * This interface allows construction of platform-specific crypto facilities.
 *
 * ### Thread Safety
 *
 * This interface is thread safe and can be used concurrently by different threads.
 *
 * @ingroup Lib_acsdkCryptoInterfaces
 * @sa Lib_acsdkCrypto
 */
class CryptoFactoryInterface {
public:
    /// @brief Default destructor.
    virtual ~CryptoFactoryInterface() noexcept = default;

    /**
     * @brief Create new encoder cipher.
     *
     * Creates a new encoder instance for a given algorithm type to encrypt data.
     *
     * @param[in] type Encryption algorithm type.
     *
     * @return Encoder reference or nullptr on error.
     */
    virtual std::unique_ptr<CryptoCodecInterface> createEncoder(AlgorithmType type) noexcept = 0;

    /**
     * @brief Create new decodec cipher.
     *
     * Creates a new decoder instance for a given algorithm type to decrypt data.
     *
     * @param[in] type Decryption algorithm type.
     *
     * @return Decoder reference or nullptr on error.
     */
    virtual std::unique_ptr<CryptoCodecInterface> createDecoder(AlgorithmType type) noexcept = 0;

    /**
     * @brief Create new hash function.
     *
     * Creates a new digest instance for a given digest type.
     *
     * @param[in] type Digest type.
     *
     * @return Digest reference or nullptr on error.
     */
    virtual std::unique_ptr<DigestInterface> createDigest(DigestType type) noexcept = 0;

    /**
     * @brief Provides key factory.
     *
     * Provides a key factory interface. Key factory allows creation of random keys and initialization vectors.
     *
     * @return Key factory reference or nullptr on error.
     */
    virtual std::shared_ptr<KeyFactoryInterface> getKeyFactory() noexcept = 0;
};

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_CRYPTOFACTORYINTERFACE_H_
