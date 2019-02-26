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
 *
 * The name ThreadMoniker is used instead of ThreadId to avoid confusion with platform specific thread identifiers
 * or the @c std::thread::id values rendered as a string.
 */
class ThreadMoniker {
public:
    /**
     * Get the moniker for @c std::this_thread.
     *
     * @return The moniker for @c std::this_thread.
     */
    static inline std::string getThisThreadMoniker();

    /**
     * Generate a unique moniker.
     *
     * @return A new unique moniker.
     */
    static std::string generateMoniker();

    /**
     * Set the moniker for @c std::this_thread. This method should be called before @c getThisThreadMoniker() in order
     * to take effect.
     *
     * @param moniker The moniker for @c std::this_thread.
     */
    static inline void setThisThreadMoniker(const std::string& moniker);

private:
    /**
     * Constructor.
     *
     * @param moniker Optional moniker for this thread. If no moniker is provided, a new moniker will be provided.
     */
    ThreadMoniker(const std::string& moniker = std::string());

    /**
     * Return the @c ThreadMoniker object for the current thread.
     *
     * @param moniker Use this moniker to initialize the @c ThreadMoniker if it doesn't exist already.
     * @return The moniker for the @c std::this_thread.
     */
    static inline const ThreadMoniker& getMonikerObject(const std::string& moniker = std::string());

    /**
     * Return the @c ThreadMoniker object for the current thread for OS that don't support thread local variables.
     *
     * @param moniker Use this moniker to initialize the @c ThreadMoniker if it doesn't exist already.
     * @return The moniker for the @c std::this_thread.
     */
    static const ThreadMoniker& getMonikerObjectFromMap(const std::string& moniker = std::string());

    /// The current thread's moniker.
    std::string m_moniker;
};

std::string ThreadMoniker::getThisThreadMoniker() {
    return getMonikerObject().m_moniker;
}

void ThreadMoniker::setThisThreadMoniker(const std::string& moniker) {
    getMonikerObject(moniker);
}

const ThreadMoniker& ThreadMoniker::getMonikerObject(const std::string& moniker) {
#ifdef _WIN32
    return getMonikerObjectFromMap(moniker);
#else
    /// Per-thread static instance so that @c m_threadMoniker.m_moniker is @c std::this_thread's moniker.
    static thread_local ThreadMoniker m_threadMoniker{moniker};

    return m_threadMoniker;
#endif
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LOGGER_THREADMONIKER_H_
