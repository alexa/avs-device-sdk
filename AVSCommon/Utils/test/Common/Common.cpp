/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/Common/Common.h"

#include <algorithm>
#include <climits>
#include <chrono>
#include <functional>
#include <random>
#include <vector>

// A constant seed for random number generator, to make the test consistent every run
static const unsigned int RANDOM_NUMBER_SEED = 1;

// The random number generator
static std::minstd_rand randGenerator;

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

std::string createRandomAlphabetString(int stringSize) {
    // First, let's efficiently generate random numbers of the appropriate size.
    std::vector<uint8_t> vec(stringSize);
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t> engine;
    std::random_device rd;
    engine.seed(
        rd() + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                   .count());
    std::generate(begin(vec), end(vec), std::ref(engine));

    // Now perform a modulo, bounding them within [a,z].
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = static_cast<uint8_t>('a' + (vec[i] % 26));
    }

    /// Convert the data into a std::string.
    char* dataBegin = reinterpret_cast<char*>(&vec[0]);

    return std::string(dataBegin, stringSize);
}

int generateRandomNumber(int min, int max) {
    if (min > max) {
        std::swap(min, max);
    }

    /// Identifier to tell if the random number generated has been initialized
    static bool randInit = false;

    if (!randInit) {
        randGenerator.seed(RANDOM_NUMBER_SEED);
        randInit = true;
    }

    return (randGenerator() % (max - min + 1)) + min;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
