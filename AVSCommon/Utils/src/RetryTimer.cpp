/*
 * RetryTimer.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <chrono>
#include <random>

#include "AVSCommon/Utils/RetryTimer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

RetryTimer::RetryTimer(int* retryTable, int retrySize) :
        m_RetryTable{retryTable},
        m_RetrySize{retrySize},
        m_RetryDecreaseFactor{1 / (0.5 + 1)},
        m_RetryIncreaseFactor{1 + 0.5} {
}

RetryTimer::RetryTimer(int* retryTable, int retrySize, double randomizationFactor) :
        m_RetryTable{retryTable},
        m_RetrySize{retrySize},
        m_RetryDecreaseFactor{1 / (randomizationFactor + 1)},
        m_RetryIncreaseFactor{1 + randomizationFactor} {
}

RetryTimer::RetryTimer(int* retryTable, int retrySize, double decreaseFactor, double increaseFactor) :
        m_RetryTable{retryTable},
        m_RetrySize{retrySize},
        m_RetryDecreaseFactor{decreaseFactor},
        m_RetryIncreaseFactor{increaseFactor} {
}

std::chrono::milliseconds RetryTimer::calculateTimeToRetry(int retryCount) {
    if (retryCount < 0) {
        retryCount = 0;
    } else if (retryCount >= m_RetrySize) {
        retryCount = m_RetrySize - 1;
    }

    std::mt19937 generator(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> distribution(
        static_cast<int>(m_RetryTable[retryCount] * m_RetryDecreaseFactor),
        static_cast<int>(m_RetryTable[retryCount] * m_RetryIncreaseFactor));
    auto delayMs = std::chrono::milliseconds(distribution(generator));
    return delayMs;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
