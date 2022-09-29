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

#include <acsdk/Crypto/CryptoFactory.h>
#include <acsdk/Crypto/private/OpenSslCryptoFactory.h>

namespace alexaClientSDK {
namespace crypto {

using namespace alexaClientSDK::cryptoInterfaces;

std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface> createCryptoFactory() noexcept {
    return OpenSslCryptoFactory::create();
}

}  // namespace crypto
}  // namespace alexaClientSDK
