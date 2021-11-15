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

#ifndef ACSDKCRYPTOINTERFACES_TEST_MOCKCRYPTOFACTORY_H_
#define ACSDKCRYPTOINTERFACES_TEST_MOCKCRYPTOFACTORY_H_

#include <unordered_map>
#include <string>
#include <memory>
#include <gmock/gmock.h>

#include <acsdkCryptoInterfaces/AlgorithmType.h>
#include <acsdkCryptoInterfaces/DigestType.h>
#include <acsdkCryptoInterfaces/DigestInterface.h>
#include <acsdkCryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkCryptoInterfaces/CryptoCodecInterface.h>
#include <acsdkCryptoInterfaces/KeyFactoryInterface.h>

namespace alexaClientSDK {
namespace acsdkCryptoInterfaces {
namespace test {

/**
 * Mock class for @c CryptoFactoryInterface.
 */
class MockCryptoFactory : public CryptoFactoryInterface {
public:
    MOCK_METHOD1(_createEncoder, std::unique_ptr<CryptoCodecInterface>(AlgorithmType));
    MOCK_METHOD1(_createDecoder, std::unique_ptr<CryptoCodecInterface>(AlgorithmType));
    MOCK_METHOD1(_createDigest, std::unique_ptr<DigestInterface>(DigestType));
    MOCK_METHOD0(_getKeyFactory, std::shared_ptr<KeyFactoryInterface>());

    std::unique_ptr<CryptoCodecInterface> createEncoder(AlgorithmType type) noexcept override;
    std::unique_ptr<CryptoCodecInterface> createDecoder(AlgorithmType type) noexcept override;
    std::unique_ptr<DigestInterface> createDigest(DigestType type) noexcept override;
    std::shared_ptr<KeyFactoryInterface> getKeyFactory() noexcept override;
};

inline std::unique_ptr<CryptoCodecInterface> MockCryptoFactory::createEncoder(AlgorithmType type) noexcept {
    return _createEncoder(type);
}

inline std::unique_ptr<CryptoCodecInterface> MockCryptoFactory::createDecoder(AlgorithmType type) noexcept {
    return _createDecoder(type);
}

inline std::unique_ptr<DigestInterface> MockCryptoFactory::createDigest(DigestType type) noexcept {
    return _createDigest(type);
}

inline std::shared_ptr<KeyFactoryInterface> MockCryptoFactory::getKeyFactory() noexcept {
    return _getKeyFactory();
}

}  // namespace test
}  // namespace acsdkCryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTOINTERFACES_TEST_MOCKCRYPTOFACTORY_H_
