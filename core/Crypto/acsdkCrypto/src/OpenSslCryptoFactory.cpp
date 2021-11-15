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

#include <acsdkCrypto/private/Logging.h>
#include <acsdkCrypto/private/OpenSslCryptoCodec.h>
#include <acsdkCrypto/private/OpenSslCryptoFactory.h>
#include <acsdkCrypto/private/OpenSslDigest.h>
#include <acsdkCrypto/private/OpenSslKeyFactory.h>

namespace alexaClientSDK {
namespace acsdkCrypto {

using namespace ::alexaClientSDK::acsdkCryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
static const std::string TAG{"OpenSSL::CryptoFactory"};

std::shared_ptr<CryptoFactoryInterface> OpenSslCryptoFactory::create() noexcept {
    auto res = std::shared_ptr<OpenSslCryptoFactory>(new OpenSslCryptoFactory);
    if (!res->init()) {
        ACSDK_ERROR(LX("createFailed"));
        res.reset();
    }

    return res;
}

OpenSslCryptoFactory::OpenSslCryptoFactory() noexcept {
}

bool OpenSslCryptoFactory::init() noexcept {
    m_keyFactory = OpenSslKeyFactory::create();
    if (!m_keyFactory) {
        ACSDK_ERROR(LX("keyFactoryCreateFailed"));
        return false;
    }

    return true;
}

std::unique_ptr<CryptoCodecInterface> OpenSslCryptoFactory::createEncoder(AlgorithmType type) noexcept {
    return OpenSslCryptoCodec::createEncoder(type);
}

std::unique_ptr<CryptoCodecInterface> OpenSslCryptoFactory::createDecoder(AlgorithmType type) noexcept {
    return OpenSslCryptoCodec::createDecoder(type);
}

std::unique_ptr<DigestInterface> OpenSslCryptoFactory::createDigest(DigestType type) noexcept {
    return OpenSslDigest::create(type);
}

std::shared_ptr<KeyFactoryInterface> OpenSslCryptoFactory::getKeyFactory() noexcept {
    return m_keyFactory;
}

}  // namespace acsdkCrypto
}  // namespace alexaClientSDK
