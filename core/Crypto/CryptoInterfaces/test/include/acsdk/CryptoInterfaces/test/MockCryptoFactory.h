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

#ifndef ACSDK_CRYPTOINTERFACES_TEST_MOCKCRYPTOFACTORY_H_
#define ACSDK_CRYPTOINTERFACES_TEST_MOCKCRYPTOFACTORY_H_

#include <unordered_map>
#include <string>
#include <memory>

#include <acsdk/CryptoInterfaces/AlgorithmType.h>
#include <acsdk/CryptoInterfaces/DigestType.h>
#include <acsdk/CryptoInterfaces/DigestInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/CryptoInterfaces/CryptoCodecInterface.h>
#include <acsdk/CryptoInterfaces/KeyFactoryInterface.h>
#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace cryptoInterfaces {
namespace test {

/**
 * Mock class for @c CryptoFactoryInterface.
 */
class MockCryptoFactory : public CryptoFactoryInterface {
public:
    MOCK_NOEXCEPT_METHOD1(createEncoder, std::unique_ptr<CryptoCodecInterface>(AlgorithmType));
    MOCK_NOEXCEPT_METHOD1(createDecoder, std::unique_ptr<CryptoCodecInterface>(AlgorithmType));
    MOCK_NOEXCEPT_METHOD1(createDigest, std::unique_ptr<DigestInterface>(DigestType));
    MOCK_NOEXCEPT_METHOD0(getKeyFactory, std::shared_ptr<KeyFactoryInterface>());
};

}  // namespace test
}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_TEST_MOCKCRYPTOFACTORY_H_
