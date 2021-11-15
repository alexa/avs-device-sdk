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

#ifndef ACSDKCRYPTOINTERFACES_TEST_MOCKDIGEST_H_
#define ACSDKCRYPTOINTERFACES_TEST_MOCKDIGEST_H_

#include <acsdkCryptoInterfaces/DigestInterface.h>
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace acsdkCryptoInterfaces {
namespace test {

/**
 * Mock class for @c DigestInterface.
 */
class MockDigest : public DigestInterface {
public:
    MOCK_METHOD1(_process, bool(const DataBlock&));
    MOCK_METHOD2(_process, bool(const DataBlock::const_iterator, const DataBlock::const_iterator));
    MOCK_METHOD1(_processByte, bool(unsigned char));
    MOCK_METHOD1(_processUInt8, bool(uint8_t));
    MOCK_METHOD1(_processUInt16, bool(uint16_t));
    MOCK_METHOD1(_processUInt32, bool(uint32_t));
    MOCK_METHOD1(_processUInt64, bool(uint64_t));
    MOCK_METHOD1(_processString, bool(const std::string&));
    MOCK_METHOD1(_finalize, bool(DataBlock&));
    MOCK_METHOD0(_reset, bool());

    bool process(const DataBlock& dataIn) noexcept override;
    bool process(DataBlock::const_iterator begin, DataBlock::const_iterator end) noexcept override;
    bool processByte(unsigned char value) noexcept override;
    bool processUInt8(uint8_t value) noexcept override;
    bool processUInt16(uint16_t value) noexcept override;
    bool processUInt32(uint32_t value) noexcept override;
    bool processUInt64(uint64_t value) noexcept override;
    bool processString(const std::string& value) noexcept override;
    bool finalize(DataBlock& dataOut) noexcept override;
    bool reset() noexcept override;
};

bool MockDigest::process(const DataBlock& dataIn) noexcept {
    return _process(dataIn);
}

bool MockDigest::process(DataBlock::const_iterator begin, DataBlock::const_iterator end) noexcept {
    return _process(begin, end);
}

inline bool MockDigest::processByte(unsigned char value) noexcept {
    return _processByte(value);
}

inline bool MockDigest::processUInt8(uint8_t value) noexcept {
    return _processUInt8(value);
}

inline bool MockDigest::processUInt16(uint16_t value) noexcept {
    return _processUInt16(value);
}

inline bool MockDigest::processUInt32(uint32_t value) noexcept {
    return _processUInt32(value);
}

inline bool MockDigest::processUInt64(uint64_t value) noexcept {
    return _processUInt64(value);
}

inline bool MockDigest::processString(const std::string& value) noexcept {
    return _processString(value);
}

inline bool MockDigest::finalize(DataBlock& dataOut) noexcept {
    return _finalize(dataOut);
}

inline bool MockDigest::reset() noexcept {
    return _reset();
}

}  // namespace test
}  // namespace acsdkCryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTOINTERFACES_TEST_MOCKDIGEST_H_
