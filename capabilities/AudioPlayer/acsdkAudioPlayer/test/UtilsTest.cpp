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
#include <AVSCommon/Utils/Memory/Memory.h>

#include <acsdkAudioPlayer/Util.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/Crypto/CryptoFactory.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayer {
namespace test {

using namespace ::testing;
using namespace avsCommon::utils::memory;
using namespace cryptoInterfaces;

/// @brief Test string for MD5 hashing.
/// @private
static std::string TEST_STR = "The quick brown fox jumps over the lazy dog";

/// @brief Test string for MD5 hashing.
/// @private
static std::string TEST_STR_2 = "Foo";

/// @brief Test string for MD5 hashing.
/// @private
static std::string TEST_STR_3 = "xyz";
/// @brief Expected MD5 value of test string.
/// @private
static const std::string MD5_TEST_DATA_HEX{"9e107d9d372bb6826bd81d3542a419d6"};
/// @brief Expected MD5 value of test string.
/// @private
static const std::string MD5_TEST_DATA_2_HEX{"1356c67d7ad1638d816bfb822dd2c25d"};
/// @brief Expected MD5 value of test string.
/// @private
static const std::string MD5_TEST_DATA_3_HEX{"d16fb36f0911f878998c136191af705e"};

/**
 * A test struct to encapsulate @c input and expected @c output values.
 *
 * @private
 */
struct UtilsTestData {
public:
    UtilsTestData(const std::string& input, const std::string& output) : m_inputString(input), m_outputString(output) {
    }

    std::string m_inputString;
    std::string m_outputString;
};

class UtilTestRealCryptoFactoryFixture : public testing::TestWithParam<UtilsTestData> {
public:
    void SetUp() override;

    /// Crypto Factory
    std::shared_ptr<CryptoFactoryInterface> m_cryptoFactory;
};

/**
 * Test fixture to test actual crypto implemenations
 *
 * @private
 */
void UtilTestRealCryptoFactoryFixture::SetUp() {
    m_cryptoFactory = alexaClientSDK::crypto::createCryptoFactory();
}

TEST_P(UtilTestRealCryptoFactoryFixture, test_generateMD5Hash) {
    UtilsTestData testData = GetParam();
    std::string output = Util::generateMD5Hash(testData.m_inputString, m_cryptoFactory);
    ASSERT_EQ(testData.m_outputString, output);
}

INSTANTIATE_TEST_CASE_P(
    Parameterized,
    UtilTestRealCryptoFactoryFixture,
    ::testing::Values(
        UtilsTestData(TEST_STR, MD5_TEST_DATA_HEX),
        UtilsTestData(TEST_STR_2, MD5_TEST_DATA_2_HEX),
        UtilsTestData(TEST_STR_3, MD5_TEST_DATA_3_HEX)));

}  // namespace test
}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK
