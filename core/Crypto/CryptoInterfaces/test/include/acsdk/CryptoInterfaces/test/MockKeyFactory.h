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

#ifndef ACSDK_CRYPTOINTERFACES_TEST_MOCKKEYFACTORY_H_
#define ACSDK_CRYPTOINTERFACES_TEST_MOCKKEYFACTORY_H_

#include <acsdk/CryptoInterfaces/KeyFactoryInterface.h>
#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace cryptoInterfaces {
namespace test {

/**
 * Mock class for @c KeyFactoryInterface.
 */
class MockKeyFactory : public KeyFactoryInterface {
public:
    MOCK_NOEXCEPT_METHOD2(generateKey, bool(AlgorithmType type, Key& key));
    MOCK_NOEXCEPT_METHOD2(generateIV, bool(AlgorithmType type, IV& iv));
};

}  // namespace test
}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_TEST_MOCKKEYFACTORY_H_
