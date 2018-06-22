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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_SAFECTIMEACCESS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_SAFECTIMEACCESS_H_

#include <ctime>
#include <memory>
#include <mutex>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

/**
 * This class allows access safe access to the multithreaded-unsafe time functions.  It is a singleton because there
 * needs to be a single lock that protects the time functions.  That single lock is contained within this class.
 */
class SafeCTimeAccess {
public:
    /**
     * The method for accessing the singleton SafeCTimeAccess class.  It returns a shared_ptr so classes that depend on
     * this class can keep it alive until they are destroyed.
     *
     * @return std::shared_ptr to the singleton SafeCTimeAccess object.
     */
    static std::shared_ptr<SafeCTimeAccess> instance();

    /**
     * Function to safely call std::gmtime.  std::gmtime uses a static internal data structure that is not thread-safe.
     *
     * @param time The time since epoch to convert.
     * @param[out] time The output in calendar time, expressed in UTC time.
     * @return true if successful, false otherwise.
     */
    bool getGmtime(const std::time_t& time, std::tm* calendarTime);

    /**
     * Function to safely call std::localtime.  std::localtime uses a static internal data structure that is not
     * thread-safe.
     *
     * @param time The time since epoch to convert.
     * @param[out] time The output in calendar time, expressed in UTC time.
     * @return true if successful, false otherwise.
     */
    bool getLocaltime(const std::time_t& time, std::tm* calendarTime);

private:
    SafeCTimeAccess() = default;

    /**
     * Helper function to eliminate duplicate code, because getGmtime and getLocaltime are almost identical.
     *
     * @param timeAccessFunction One of two funtions (std::gmtime and std::localtime) that need to be safely accessed.
     * @param time The time since epoch to convert.
     * @param[out] time The output in calendar time, expressed in UTC time.
     * @return true if successful, false otherwise.
     */
    bool safeAccess(
        std::tm* (*timeAccessFunction)(const std::time_t* time),
        const std::time_t& time,
        std::tm* calendarTime);

    /// Mutex used to protect access to the ctime functions.
    std::mutex m_timeLock;
};

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TIMING_SAFECTIMEACCESS_H_
