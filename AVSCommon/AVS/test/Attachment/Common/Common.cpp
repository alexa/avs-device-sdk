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

#include "Common.h"

#include <random>
#include <climits>
#include <algorithm>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace alexaClientSDK::avsCommon::utils::sds;

std::unique_ptr<InProcessSDS> createSDS(int desiredSize) {
    auto bufferSize = InProcessSDS::calculateBufferSize(desiredSize);
    auto buffer = std::make_shared<InProcessSDSTraits::Buffer>(bufferSize);

    return InProcessSDS::create(buffer);
}

std::vector<uint8_t> createTestPattern(int patternSize) {
    std::vector<uint8_t> ret(patternSize);
    std::vector<uint16_t> vec(patternSize);
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint16_t> engine;

    std::generate(begin(vec), end(vec), std::ref(engine));
    for (size_t i = 0; i < vec.size(); i++) {
        ret[i] = static_cast<uint8_t>(vec[i]);
    }
    return ret;
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
