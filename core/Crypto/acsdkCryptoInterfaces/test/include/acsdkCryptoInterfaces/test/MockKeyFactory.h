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

#ifndef ACSDKCRYPTOINTERFACES_TEST_MOCKKEYFACTORY_H_
#define ACSDKCRYPTOINTERFACES_TEST_MOCKKEYFACTORY_H_

#include <acsdkCryptoInterfaces/KeyFactoryInterface.h>
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace acsdkCryptoInterfaces {
namespace test {

/**
 * Mock class for @c KeyFactoryInterface.
 */
class MockKeyFactory : public KeyFactoryInterface {
public:
    MOCK_METHOD2(_generateKey, bool(AlgorithmType type, Key& key));
    MOCK_METHOD2(_generateIV, bool(AlgorithmType type, IV& iv));

    bool generateIV(AlgorithmType type, IV& iv) noexcept override;
    bool generateKey(AlgorithmType type, Key& key) noexcept override;
};

inline bool MockKeyFactory::generateKey(AlgorithmType type, Key& key) noexcept {
    return _generateKey(type, key);
}

inline bool MockKeyFactory::generateIV(AlgorithmType type, IV& iv) noexcept {
    return _generateIV(type, iv);
}

}  // namespace test
}  // namespace acsdkCryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTOINTERFACES_TEST_MOCKKEYFACTORY_H_
