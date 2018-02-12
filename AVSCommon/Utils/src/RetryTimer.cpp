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

#include <random>

#include "AVSCommon/Utils/RetryTimer.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

RetryTimer::RetryTimer(const std::vector<int>& retryTable) :
        m_RetryTable{retryTable},
        m_RetrySize{retryTable.size()},
        m_RetryDecreasePercentage{(100 * 100) / (100 + 50)},
        m_RetryIncreasePercentage{100 + 50} {
}

RetryTimer::RetryTimer(const std::vector<int>& retryTable, int randomizationPercentage) :
        m_RetryTable{retryTable},
        m_RetrySize{retryTable.size()},
        m_RetryDecreasePercentage{(100 * 100) / (100 + randomizationPercentage)},
        m_RetryIncreasePercentage{100 + randomizationPercentage} {
}

RetryTimer::RetryTimer(const std::vector<int>& retryTable, int decreasePercentage, int increasePercentage) :
        m_RetryTable{retryTable},
        m_RetrySize{retryTable.size()},
        m_RetryDecreasePercentage{decreasePercentage},
        m_RetryIncreasePercentage{increasePercentage} {
}

std::chrono::milliseconds RetryTimer::calculateTimeToRetry(int retryCount) const {
    if (retryCount < 0) {
        retryCount = 0;
    } else if ((size_t)retryCount >= m_RetrySize) {
        retryCount = m_RetrySize - 1;
    }

    std::mt19937 generator(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> distribution(
        (m_RetryTable[retryCount] * m_RetryDecreasePercentage) / 100,
        (m_RetryTable[retryCount] * m_RetryIncreasePercentage) / 100);
    auto delayMs = std::chrono::milliseconds(distribution(generator));
    return delayMs;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
