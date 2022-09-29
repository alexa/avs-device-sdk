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

#ifndef ACSDK_CRYPTOINTERFACES_TEST_MOCKDIGEST_H_
#define ACSDK_CRYPTOINTERFACES_TEST_MOCKDIGEST_H_

#include <acsdk/CryptoInterfaces/DigestInterface.h>
#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace cryptoInterfaces {
namespace test {

/**
 * Mock class for @c DigestInterface.
 */
class MockDigest : public DigestInterface {
public:
    MOCK_NOEXCEPT_METHOD1(process, bool(const DataBlock&));
    MOCK_NOEXCEPT_METHOD2(process, bool(const DataBlock::const_iterator, const DataBlock::const_iterator));
    MOCK_NOEXCEPT_METHOD1(processByte, bool(unsigned char));
    MOCK_NOEXCEPT_METHOD1(processUInt8, bool(uint8_t));
    MOCK_NOEXCEPT_METHOD1(processUInt16, bool(uint16_t));
    MOCK_NOEXCEPT_METHOD1(processUInt32, bool(uint32_t));
    MOCK_NOEXCEPT_METHOD1(processUInt64, bool(uint64_t));
    MOCK_NOEXCEPT_METHOD1(processString, bool(const std::string&));
    MOCK_NOEXCEPT_METHOD1(finalize, bool(DataBlock&));
    MOCK_NOEXCEPT_METHOD0(reset, bool());
};

}  // namespace test
}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_TEST_MOCKDIGEST_H_
