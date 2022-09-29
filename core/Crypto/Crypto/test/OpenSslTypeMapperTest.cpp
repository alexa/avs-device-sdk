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

#include <acsdk/Crypto/private/OpenSslTypeMapper.h>

namespace alexaClientSDK {
namespace crypto {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::cryptoInterfaces;

class MapDigest : public TestWithParam<std::pair<DigestType, const EVP_MD*>> {};

TEST_P(MapDigest, testMapDigest) {
    auto param = GetParam();
    auto digest = OpenSslTypeMapper::mapDigestToEvpMd(DigestType::SHA_256);
    ASSERT_EQ(param.second, digest);
}

INSTANTIATE_TEST_CASE_P(
    OpenSslTypeMapperTest,
    MapDigest,
    Values(std::pair<DigestType, const EVP_MD*>{DigestType::SHA_256, EVP_sha256()}));

TEST(OpenSslTypeMapperTest, test_unknownDigest) {
    auto digest = OpenSslTypeMapper::mapDigestToEvpMd(static_cast<DigestType>(0));
    ASSERT_EQ(nullptr, digest);
}

class MapCipher : public TestWithParam<std::pair<AlgorithmType, const EVP_CIPHER*>> {};

TEST_P(MapCipher, testCipherMap) {
    auto param = GetParam();
    auto cipher = OpenSslTypeMapper::mapAlgorithmToEvpCipher(param.first);
    ASSERT_EQ(param.second, cipher);
}

INSTANTIATE_TEST_CASE_P(
    OpenSslTypeMapperTest,
    MapCipher,
    Values(
        std::pair<AlgorithmType, const EVP_CIPHER*>{AlgorithmType::AES_256_CBC, EVP_aes_256_cbc()},
        std::pair<AlgorithmType, const EVP_CIPHER*>{AlgorithmType::AES_256_CBC_PAD, EVP_aes_256_cbc()},
        std::pair<AlgorithmType, const EVP_CIPHER*>{AlgorithmType::AES_128_CBC, EVP_aes_128_cbc()},
        std::pair<AlgorithmType, const EVP_CIPHER*>{AlgorithmType::AES_128_CBC_PAD, EVP_aes_128_cbc()}));

TEST(OpenSslTypeMapperTest, test_unknownAlgorithm) {
    auto cipher = OpenSslTypeMapper::mapAlgorithmToEvpCipher(static_cast<AlgorithmType>(0));
    ASSERT_EQ(nullptr, cipher);
}

}  // namespace test
}  // namespace crypto
}  // namespace alexaClientSDK
