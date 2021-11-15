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

#ifndef ACSDKCRYPTOINTERFACES_TEST_MOCKCRYPTOCODEC_H_
#define ACSDKCRYPTOINTERFACES_TEST_MOCKCRYPTOCODEC_H_

#include <acsdkCryptoInterfaces/CryptoCodecInterface.h>
#include <gmock/gmock.h>

namespace alexaClientSDK {
namespace acsdkCryptoInterfaces {
namespace test {

/**
 * Mock class for @c CryptoCodecInterface.
 */
class MockCryptoCodec : public CryptoCodecInterface {
public:
    MOCK_METHOD2(_init, bool(const Key&, const IV&));
    MOCK_METHOD1(_processAAD, bool(const DataBlock&));
    MOCK_METHOD2(_processAAD, bool(const DataBlock::const_iterator&, const DataBlock::const_iterator&));
    MOCK_METHOD2(_process, bool(const DataBlock&, DataBlock&));
    MOCK_METHOD3(_process, bool(const DataBlock::const_iterator&, const DataBlock::const_iterator&, DataBlock&));
    MOCK_METHOD1(_finalize, bool(DataBlock&));
    MOCK_METHOD1(_getTag, bool(Tag&));
    MOCK_METHOD1(_setTag, bool(const Tag&));

    bool init(const Key& key, const IV& iv) noexcept override;
    bool processAAD(const DataBlock& dataIn) noexcept override;
    bool processAAD(DataBlock::const_iterator dataInBegin, DataBlock::const_iterator dataInEnd) noexcept override;
    bool process(const DataBlock& dataIn, DataBlock& dataOut) noexcept override;
    bool process(
        DataBlock::const_iterator dataInBegin,
        DataBlock::const_iterator dataInEnd,
        DataBlock& dataOut) noexcept override;
    bool finalize(DataBlock& dataOut) noexcept override;
    bool getTag(DataBlock& tag) noexcept override;
    bool setTag(const DataBlock& tag) noexcept override;
};

inline bool MockCryptoCodec::init(const Key& key, const IV& iv) noexcept {
    return _init(key, iv);
}

inline bool MockCryptoCodec::process(const DataBlock& dataIn, DataBlock& dataOut) noexcept {
    return _process(dataIn, dataOut, append);
}

inline bool MockCryptoCodec::process(
    DataBlock::const_iterator dataInBegin,
    DataBlock::const_iterator dataInEnd,
    DataBlock& dataOut) noexcept {
    return _process(dataInBegin, dataInEnd, dataOut);
}

inline bool MockCryptoCodec::finalize(DataBlock& dataOut) noexcept {
    return _finalize(dataOut);
}

inline bool MockCryptoCodec::getTag(Tag& tag) noexcept {
    return _getTag(tag);
}

inline bool MockCryptoCodec::setTag(const Tag& tag) noexcept {
    return _setTag(tag);
}

}  // namespace test
}  // namespace acsdkCryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCRYPTOINTERFACES_TEST_MOCKCRYPTOCODEC_H_
