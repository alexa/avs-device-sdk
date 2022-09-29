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

#ifndef ACSDK_CRYPTOINTERFACES_KEYFACTORYINTERFACE_H_
#define ACSDK_CRYPTOINTERFACES_KEYFACTORYINTERFACE_H_

#include <vector>

#include "AlgorithmType.h"

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Key factory interface.
 *
 * This interface allows construction of new keys and initialization vectors.
 *
 * ### Thread Safety
 *
 * This interface is thread safe and can be used concurrently by different threads.
 *
 * @ingroup Lib_acsdkCryptoInterfaces
 */
class KeyFactoryInterface {
public:
    /// @brief Key data.
    /// Key is a sequence of bytes, and the size depends on an encryption algorithm.
    typedef std::vector<unsigned char> Key;

    /// @brief Initialization vector type.
    /// IV is a sequence of bytes, and the size depends on a encryption algorithm.
    typedef std::vector<unsigned char> IV;

    /// Default destructor.
    virtual ~KeyFactoryInterface() noexcept = default;

    /**
     * @brief Generates a new key.
     *
     * Generates a new key for a given algorithm.
     *
     * @param[in]   type    Encryption algorithm type.
     * @param[out]  key     Key data.
     *
     * @return Boolean indicating operation success. If operation fails, the state of
     *         @a key is undefined.
     */
    virtual bool generateKey(AlgorithmType type, Key& key) noexcept = 0;

    /**
     * @brief Generates a new initialization vector.
     *
     * Generate random initialization vector.
     *
     * @param[in]   type    Algorithm type.
     * @param[out]  iv      Initialization vector.
     *
     * @return Boolean indicating operation success. If operation fails, the state of
     *         @a iv is undefined.
     */
    virtual bool generateIV(AlgorithmType type, IV& iv) noexcept = 0;
};

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_KEYFACTORYINTERFACE_H_
