/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <string>
#include <future>
#include <vector>
#include <unordered_set>
#include <cctype>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace testing;
using namespace avsCommon::utils::uuidGeneration;

/// The version of the UUID generated.
static const std::string UUID_VERSION("4");

/// The variant of the UUID generated.
static const unsigned int UUID_VARIANT(8);

/// The offset of the UUID version in the string.
static const unsigned int UUID_VERSION_OFFSET(14);

/// The offset of the UUID variant in the string.
static const unsigned int UUID_VARIANT_OFFSET(19);

/// Hyphen.
static const std::string HYPHEN("-");

/// Position of first hyphen.
static const unsigned int HYPHEN1_POSITION(8);

/// Position of second hyphen.
static const unsigned int HYPHEN2_POSITION(13);

/// Position of third hyphen.
static const unsigned int HYPHEN3_POSITION(18);

/// Position of fourth hyphen.
static const unsigned int HYPHEN4_POSITION(23);

/// The length of the UUID string - 32 hexadecimal digits and 4 hyphens.
static const unsigned int UUID_LENGTH(36);

/// The maximum UUIDs to generate to test for uniqueness.
static const unsigned int MAX_UUIDS_TO_GENERATE(100);

/// The maximum threads to test with.
static const unsigned int MAX_TEST_THREADS(10);

/// The maximum number of retries.
static const unsigned int MAX_RETRIES(20);

class UUIDGenerationTest : public ::testing::Test {};

/**
 * Call @c generateUUID and expect a string of length @c UUID_LENGTH.
 */
TEST_F(UUIDGenerationTest, testUUIDStringLength) {
    ASSERT_EQ(UUID_LENGTH, generateUUID().length());
}

/**
 * Call @c generateUUID and expect a string of length @c UUID_LENGTH. Check that each character in the string
 * is a hexedecimal number except for the hyphens.
 */
TEST_F(UUIDGenerationTest, testUUIDContainsOnlyHexCharacters) {
    auto uuid = generateUUID();
    ASSERT_EQ(UUID_LENGTH, uuid.length());
    for (unsigned int i = 0; i < uuid.length(); i++) {
        if (i == HYPHEN1_POSITION || i == HYPHEN2_POSITION || i == HYPHEN3_POSITION || i == HYPHEN4_POSITION) {
            ASSERT_EQ(HYPHEN, uuid.substr(i, 1));
        } else {
            ASSERT_TRUE(isxdigit(uuid[i]));
        }
    }
}

/**
 * Call @c generateUUID and check that the version is set correctly.
 */
TEST_F(UUIDGenerationTest, testUUIDVersion) {
    ASSERT_EQ(UUID_VERSION, generateUUID().substr(UUID_VERSION_OFFSET, 1));
}

/**
 * Call @c generateUUID and check the variant is set correctly.
 */
TEST_F(UUIDGenerationTest, testUUIDVariant) {
    ASSERT_EQ(UUID_VARIANT, strtoul(generateUUID().substr(UUID_VARIANT_OFFSET, 1).c_str(), nullptr, 16) & UUID_VARIANT);
}

/**
 * Call @c generateUUID and check that the hyphens are in the right positions.
 */
TEST_F(UUIDGenerationTest, testUUIDHyphens) {
    std::string uuid = generateUUID();
    ASSERT_EQ(HYPHEN, uuid.substr(HYPHEN1_POSITION, 1));
    ASSERT_EQ(HYPHEN, uuid.substr(HYPHEN2_POSITION, 1));
    ASSERT_EQ(HYPHEN, uuid.substr(HYPHEN3_POSITION, 1));
    ASSERT_EQ(HYPHEN, uuid.substr(HYPHEN4_POSITION, 1));
}

/**
 * Call @c generateUUID multiple times and check the version and variant are set correctly.
 * Check for uniqueness of the UUIDs generated.
 */
TEST_F(UUIDGenerationTest, testMultipleRequests) {
    std::unordered_set<std::string> uuidsGenerated;

    for (unsigned int i = 0; i < MAX_UUIDS_TO_GENERATE; ++i) {
        unsigned int prevSizeOfSet = uuidsGenerated.size();
        auto uuid = generateUUID();
        uuidsGenerated.insert(uuid);
        ASSERT_EQ(UUID_LENGTH, uuid.length());
        ASSERT_EQ(UUID_VERSION, uuid.substr(UUID_VERSION_OFFSET, 1));
        ASSERT_EQ(UUID_VARIANT, strtoul(uuid.substr(UUID_VARIANT_OFFSET, 1).c_str(), nullptr, 16) & UUID_VARIANT);
        ASSERT_EQ(prevSizeOfSet + 1, uuidsGenerated.size());
    }
}

/**
 * Call @c generateUUID from multiple threads and check the version and variant are set correctly.
 * Check for uniqueness of the UUIDs generated.
 */
TEST_F(UUIDGenerationTest, testMultipleConcurrentRequests) {
    int no_of_threads = MAX_TEST_THREADS;
    std::vector<std::future<std::string>> uuidRequesters;
    std::unordered_set<std::string> uuidsGenerated;

    for (int i = 0; i < no_of_threads; ++i) {
        auto future = std::async(std::launch::async, []() { return generateUUID(); });
        uuidRequesters.push_back(std::move(future));
    }

    for (auto& future : uuidRequesters) {
        unsigned int prevSizeOfSet = uuidsGenerated.size();
        auto uuid = future.get();
        uuidsGenerated.insert(uuid);
        ASSERT_EQ(UUID_LENGTH, uuid.length());
        ASSERT_EQ(UUID_VERSION, uuid.substr(UUID_VERSION_OFFSET, 1));
        ASSERT_EQ(UUID_VARIANT, strtoul(uuid.substr(UUID_VARIANT_OFFSET, 1).c_str(), nullptr, 16) & UUID_VARIANT);
        ASSERT_EQ(prevSizeOfSet + 1, uuidsGenerated.size());
    }
}

/**
 * Call @c generateUUID and ensure all hex values are generated. Will retry @c MAX_RETRIES times.
 */
TEST_F(UUIDGenerationTest, testAllHexValuesGenerated) {
    std::unordered_set<char> hexCharacters = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (unsigned int retry = 0; retry < MAX_RETRIES && !hexCharacters.empty(); retry++) {
        std::string uuid = generateUUID();
        for (const char& digit : uuid) {
            hexCharacters.erase(digit);
            if (hexCharacters.empty()) {
                break;
            }
        }
    }

    ASSERT_TRUE(hexCharacters.empty());
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
