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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_THREADMONIKER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_THREADMONIKER_H_

#include <iomanip>
#include <string>
#include <sstream>
#include <thread>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/**
 * Class to provide @c std::this_thread access to unique name for itself.
 * The name ThreadMoniker is used instead of ThreadId to avoid confusion with platform specific thread identifiers
 * or the @c std::thread::id values rendered as a string.
 */
class ThreadMoniker {
public:
    /**
     * Constructor.
     */
    ThreadMoniker();

    /**
     * Get the moniker for @c std::this_thread.
     *
     * @return The moniker for @c std::this_thread.
     */
    static inline const std::string getThisThreadMoniker();

private:
    /// The current thread's moniker.
    std::string m_moniker;
};

const std::string ThreadMoniker::getThisThreadMoniker() {
#ifdef _WIN32
    std::ostringstream winThreadID;
    winThreadID << std::setw(3) << std::hex << std::right << std::this_thread::get_id();
    return winThreadID.str();
#else
    /// Per-thread static instance so that @c m_threadMoniker.m_moniker is @c std::this_thread's moniker.
    static thread_local ThreadMoniker m_threadMoniker;

    return m_threadMoniker.m_moniker;
#endif
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_THREADMONIKER_H_
