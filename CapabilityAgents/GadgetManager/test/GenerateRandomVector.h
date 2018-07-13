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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_GADGETMANAGER_TEST_GENERATERANDOMVECTOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_GADGETMANAGER_TEST_GENERATERANDOMVECTOR_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace gadgetManager {
namespace test {

/**
 * Utility function that is used in multiple tests for generating random vectors of bytes
 */
std::vector<uint8_t> generateRandomVector(size_t length) {
    // First create an instance of an engine.
    std::random_device rnd_device;
    // Specify the engine and distribution.
    std::mt19937 mersenne_engine(rnd_device());
    std::uniform_int_distribution<uint8_t> dist(0, 0xFF);

    auto gen = std::bind(dist, mersenne_engine);

    std::vector<uint8_t> vec(length);
    std::generate(std::begin(vec), std::end(vec), gen);
    return vec;
}

}  // namespace test
}  // namespace gadgetManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_GADGETMANAGER_TEST_GENERATERANDOMVECTOR_H_
