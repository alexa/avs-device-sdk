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

#include <gtest/gtest.h>

#include <acsdk/Crypto/private/OpenSslKeyFactory.h>

namespace alexaClientSDK {
namespace crypto {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::cryptoInterfaces;

static constexpr size_t AES_256_KEY_SIZE = 32u;  // 256 bits
static constexpr size_t AES_128_KEY_SIZE = 16u;  // 128 bits
static constexpr size_t AES_CBC_IV_SIZE = 16u;   // 128 bits
static constexpr size_t AES_GCM_IV_SIZE = 12u;   // 96 bits
static constexpr AlgorithmType BAD_ALGORITHM_TYPE = static_cast<AlgorithmType>(0);

/**
 * Test fixture for generic parameter error tests.
 *
 * @private
 */
class KeyFactoryTest : public TestWithParam<AlgorithmType> {};

TEST_P(KeyFactoryTest, testCreateUniqueKeys) {
    auto factory = OpenSslKeyFactory::create();
    ASSERT_NE(nullptr, factory);
    KeyFactoryInterface::Key key1;
    KeyFactoryInterface::Key key2;
    ASSERT_TRUE(factory->generateKey(GetParam(), key1));
    ASSERT_TRUE(factory->generateKey(GetParam(), key2));
    size_t expectedSize = 0;
    switch (GetParam()) {
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_256_GCM:
            expectedSize = AES_256_KEY_SIZE;
            break;
        case AlgorithmType::AES_128_CBC_PAD:
        case AlgorithmType::AES_128_CBC:
        case AlgorithmType::AES_128_GCM:
            expectedSize = AES_128_KEY_SIZE;
            break;
        default:
            ASSERT_TRUE(false);
            break;
    }
    ASSERT_EQ(expectedSize, key1.size());
    ASSERT_EQ(expectedSize, key2.size());
    ASSERT_NE(key1, key2);
}

TEST_P(KeyFactoryTest, testCreateUniqueIVs) {
    auto factory = OpenSslKeyFactory::create();
    ASSERT_NE(nullptr, factory);
    KeyFactoryInterface::IV iv1;
    KeyFactoryInterface::IV iv2;
    ASSERT_TRUE(factory->generateIV(GetParam(), iv1));
    ASSERT_TRUE(factory->generateIV(GetParam(), iv2));
    size_t expectedSize = 0;
    switch (GetParam()) {
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_128_CBC_PAD:
        case AlgorithmType::AES_128_CBC:
            expectedSize = AES_CBC_IV_SIZE;
            break;
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            expectedSize = AES_GCM_IV_SIZE;
            break;
        default:
            ASSERT_TRUE(false);
            break;
    }

    ASSERT_EQ(expectedSize, iv1.size());
    ASSERT_EQ(expectedSize, iv2.size());
    ASSERT_NE(iv1, iv2);
}

TEST_F(KeyFactoryTest, test_createKeyUnknown) {
    auto factory = OpenSslKeyFactory::create();
    ASSERT_NE(nullptr, factory);
    KeyFactoryInterface::Key key1;
    ASSERT_FALSE(factory->generateKey(BAD_ALGORITHM_TYPE, key1));
}

TEST_F(KeyFactoryTest, test_createIvUnknown) {
    auto factory = OpenSslKeyFactory::create();
    ASSERT_NE(nullptr, factory);
    KeyFactoryInterface::IV iv1;
    ASSERT_FALSE(factory->generateIV(BAD_ALGORITHM_TYPE, iv1));
}

INSTANTIATE_TEST_CASE_P(
    OpenSslKeyFactoryTest,
    KeyFactoryTest,
    Values(
        AlgorithmType::AES_256_CBC,
        AlgorithmType::AES_256_CBC_PAD,
        AlgorithmType::AES_128_CBC,
        AlgorithmType::AES_128_CBC_PAD,
        AlgorithmType::AES_128_GCM,
        AlgorithmType::AES_256_GCM));

}  // namespace test
}  // namespace crypto
}  // namespace alexaClientSDK
