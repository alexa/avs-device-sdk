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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_RETRYTIMER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_RETRYTIMER_H_

#include <chrono>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * A @c RetryTimer holds information needed to compute a retry time for a thread
 * when waiting on events.
 */
class RetryTimer {
public:
    /**
     * Constructor.
     *
     * @param retryTable The table with entries for retry times.
     */
    RetryTimer(const std::vector<int>& retryTable);

    /**
     * Constructor.
     *
     * @param retryTable The table with entries for retry times.
     * @param randomizationPercentage The randomization percentage to be used while computing the distribution range
     * around the retry time.
     */
    RetryTimer(const std::vector<int>& retryTable, int randomizationPercentage);

    /**
     * Constructor.
     *
     * @param retryTable The table with entries for retry times.
     * @param decreasePercentage The lower bound of the retry time duration.
     * @param increasePercentage The upper bound of the retry time duration.
     */
    RetryTimer(const std::vector<int>& retryTable, int decreasePercentage, int increasePercentage);

    /**
     * Method to return a randomized delay in milliseconds when threads are waiting on an event.
     *
     * @param retryCount The number of retries.
     * @return delay in milliseconds.
     */
    std::chrono::milliseconds calculateTimeToRetry(int retryCount) const;

private:
    /// Retry table with retry time in milliseconds.
    const std::vector<int> m_RetryTable;

    /// Size of the retry table.
    const size_t m_RetrySize;

    /// The lower bound (as a percentage) for randomizing the next retry time.
    const int m_RetryDecreasePercentage;

    /// The upper bound (as a percentage) for randomizing the next retry time.
    const int m_RetryIncreasePercentage;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_RETRYTIMER_H_
