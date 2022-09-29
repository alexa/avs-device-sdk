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

#include <random>
#include <chrono>
#include <sstream>
#include <string>
#include <iomanip>
#include <cmath>
#include <mutex>
#include <climits>
#include <algorithm>
#include <functional>
#include <list>

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace uuidGeneration {

/// String to identify log entries originating from this file.
#define TAG "UUIDGeneration"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The UUID version (Version 4), shifted into the correct position in the byte.
static const uint8_t UUID_VERSION_VALUE = 4 << 4;

/// The UUID variant (Variant 1), shifted into the correct position in the byte.
static const size_t UUID_VARIANT_VALUE = 2 << 6;

/// Separator used between UUID fields.
static const std::string SEPARATOR("-");

/// Number of bits in the replacement value.
static const size_t MAX_NUM_REPLACEMENT_BITS = CHAR_BIT;

/// Number of bits in a hex digit.
static const size_t BITS_IN_HEX_DIGIT = 4;

/// Indicates if next UUID should be seeded. Must not be accessed unless g_mutex is locked.
static bool g_seedNeeded = true;

/// Lock to avoid collisions in generationing uuid and setting seed.
static std::mutex g_mutex;

/// Entropy Threshold for sufficient uniqueness. Value chosen by experiment.
static const double ENTROPY_THRESHOLD = 600;

/// extra seeds
static const size_t MAX_SEEDS_POOL_SIZE = 1024;

/// pool of seeds
static std::list<uint32_t> seedsPool;

/// last time generateUUID was invoked
static uint64_t lastInvokeTime;

/// Catch for platforms where entropy is a hard coded value. Value chosen by experiment.
static const int ENTROPY_REPEAT_THRESHOLD = 16;

/// The default read entropy function. This can be customized using UUIDGeneration::setEntropyReader().
static std::function<double(void)> readEntropyFunc = []() {
    std::random_device rd;
    return rd.entropy();
};

void setEntropyReader(std::function<double(void)> func) {
    readEntropyFunc = func;
}

void setSalt(const std::string& newSalt) {
    std::unique_lock<std::mutex> lock(g_mutex);
    std::copy_n(newSalt.begin(), std::min(newSalt.size(), MAX_SEEDS_POOL_SIZE), std::front_inserter(seedsPool));
    g_seedNeeded = true;
}

void addSeeds(const std::vector<uint32_t>& seeds) {
    std::unique_lock<std::mutex> lock(g_mutex);
    std::copy_n(seeds.begin(), std::min(seeds.size(), MAX_SEEDS_POOL_SIZE), std::front_inserter(seedsPool));
    g_seedNeeded = true;
}

/**
 * Randomly generate a string of hex digits. Before the conversion of hex to string,
 * the function allows replacement of the bits of the first two generated hex digits.
 * Replacement happens starting at the most significant, to the least significant bit.
 *
 * @param ibe A random number generator.
 * @param numDigits The number of hex digits [0-9],[a-f] to generate.
 * @param replacementBits The replacement value for up to the first two digits generated.
 * @param numReplacementBits The number of bits of @c replacementBits to use.
 *
 * @return A hex string of length @c numDigits.
 */
static const std::string generateHexWithReplacement(
    std::independent_bits_engine<std::mt19937, CHAR_BIT, uint32_t>& ibe,
    unsigned int numDigits,
    uint8_t replacementBits,
    uint8_t numReplacementBits) {
    if (numReplacementBits > MAX_NUM_REPLACEMENT_BITS) {
        ACSDK_ERROR(LX("generateHexWithReplacementFailed")
                        .d("reason", "replacingMoreBitsThanProvided")
                        .d("numReplacementBitsLimit", MAX_NUM_REPLACEMENT_BITS)
                        .d("numReplacementBits", numReplacementBits));
        return "";
    }

    if (numReplacementBits > (numDigits * BITS_IN_HEX_DIGIT)) {
        ACSDK_ERROR(LX("generateHexWithReplacementFailed")
                        .d("reason", "replacingMoreBitsThanGenerated")
                        .d("numDigitsInBits", numDigits * BITS_IN_HEX_DIGIT)
                        .d("numReplacementBits", numReplacementBits));
        return "";
    }

    const size_t arrayLen = (numDigits + 1) / 2;

    std::vector<uint8_t> bytes(arrayLen);
    std::generate(bytes.begin(), bytes.end(), std::ref(ibe));

    // Replace the specified number of bits from the first byte.
    bytes.at(0) &= (0xff >> numReplacementBits);
    replacementBits &= (0xff << (MAX_NUM_REPLACEMENT_BITS - numReplacementBits));
    bytes.at(0) |= replacementBits;

    std::ostringstream oss;
    for (const auto& byte : bytes) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }

    std::string bytesText = oss.str();
    // Remove the last digit for odd numDigits case.
    bytesText.resize(numDigits);

    return bytesText;
}

/**
 * Randomly generate a string of hex digits.
 *
 * @param ibe A random number generator.
 * @param numDigits The number of hex digits [0-9],[a-f] to generate.
 *
 * @return A hex string of length @c numDigits.
 */
static const std::string generateHex(
    std::independent_bits_engine<std::mt19937, CHAR_BIT, uint32_t>& ibe,
    unsigned int numDigits) {
    return generateHexWithReplacement(ibe, numDigits, 0, 0);
}

/**
 * Add default seed to the pool, should only be used in generateUUID and locked by g_mutex.
 */
static void addDefaultSeedLocked() {
    std::random_device rd;
    uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (lastInvokeTime > 0) {
        seedsPool.push_front(static_cast<uint32_t>(timeSeed - lastInvokeTime));  // interval between two invocations
    }
    lastInvokeTime = timeSeed;
    seedsPool.push_front(static_cast<uint32_t>(timeSeed));  // lower 32bits of current time
    seedsPool.push_front(rd());                             // random device
    seedsPool.push_front(static_cast<uint32_t>(
        reinterpret_cast<std::intptr_t>(&timeSeed)));  // lower 32bits of memory address of temporary variable

    if (seedsPool.size() > MAX_SEEDS_POOL_SIZE) {
        seedsPool.resize(MAX_SEEDS_POOL_SIZE);
    }
}

const std::string generateUUID() {
    static std::independent_bits_engine<std::mt19937, CHAR_BIT, uint32_t> ibe;
    std::unique_lock<std::mutex> lock(g_mutex);

    if (g_seedNeeded) {
        static int consistentEntropyReports = 0;
        static double priorEntropyResult = 0;
        addDefaultSeedLocked();
        std::seed_seq seed(seedsPool.begin(), seedsPool.end());
        ibe.seed(seed);

        double currentEntropy = readEntropyFunc();

        if (std::fabs(currentEntropy - priorEntropyResult) < std::numeric_limits<double>::epsilon()) {
            ++consistentEntropyReports;
        } else {
            consistentEntropyReports = 0;
        }
        priorEntropyResult = currentEntropy;

        if (currentEntropy > ENTROPY_THRESHOLD) {
            g_seedNeeded = false;
        } else {
            ACSDK_INFO(LX("low entropy on call to generate UUID").d("current entropy", currentEntropy));
            if (consistentEntropyReports > ENTROPY_REPEAT_THRESHOLD) {
                g_seedNeeded = false;
                ACSDK_INFO(LX("multiple repeat values for entropy")
                               .d("current entropy", currentEntropy)
                               .d("consistent entropy reports", consistentEntropyReports));
            }
        }
    }

    std::ostringstream uuidText;
    uuidText << generateHex(ibe, 8) << SEPARATOR << generateHex(ibe, 4) << SEPARATOR
             << generateHexWithReplacement(ibe, 4, UUID_VERSION_VALUE, 4) << SEPARATOR
             << generateHexWithReplacement(ibe, 4, UUID_VARIANT_VALUE, 2) << SEPARATOR << generateHex(ibe, 12);

    lock.unlock();

    return uuidText.str();
}

}  // namespace uuidGeneration
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
