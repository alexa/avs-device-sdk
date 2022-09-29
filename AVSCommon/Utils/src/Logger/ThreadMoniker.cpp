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

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iomanip>
#include <sstream>

#if defined(_WIN32) || defined(__QNX__)
#include <mutex>
#include <thread>
#include <unordered_map>
#endif

#include <AVSCommon/Utils/Logger/ThreadMoniker.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

/// Space character for moniker formatting.
static constexpr char CHAR_SPACE = ' ';

/// Colon character for moniker prefix separation.
static constexpr char CHAR_COLON = ':';

/// Size of formatted moniker in characters.
static constexpr size_t MONIKER_SIZE_CHARS = 5u;

#if defined(_WIN32) || defined(__QNX__)
/// Map each thread to a moniker.
static std::unordered_map<std::thread::id, std::string> monikerMap;
/// Lock used to synchronize access to the local maps.
static std::mutex monikerMutex;

/**
 * Return the moniker value reference for the current thread for OS that don't support thread local variables.
 *
 * @return The moniker for the @c std::this_thread.
 */
static inline std::string getMonikerValue() noexcept {
    auto id = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock{monikerMutex};
    auto entry = monikerMap.find(id);
    if (entry == monikerMap.end()) {
        entry = monikerMap.emplace(std::make_pair(id, ThreadMoniker::generateMoniker())).first;
    }
    return entry->second;
}

/**
 * @brief Set the moniker value reference for the current thread for OS that doesn't support thread local variables.
 *
 * @param moniker Moniker value to set.
 */
static inline void setMonikerValue(const std::string& moniker) noexcept {
    auto id = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock{monikerMutex};
    auto entry = monikerMap.find(id);
    if (entry == monikerMap.end()) {
        monikerMap.emplace(std::make_pair(id, moniker));
    } else {
        entry->second = moniker;
    }
}
#else

/**
 * @brief Thread-local moniker value.
 *
 * Thread local destructors are called before static member destructors. If client code contains any static instance,
 * that attempts to use Logging API from destructor, and exit() call is issued, the @a threadMoniker variable is
 * released and running threads attempt to access string data through dangling pointers.
 *
 * To address the issue we use a type with a trivial destructor.
 *
 * @note This variable is defined only for platforms, that support @a thread_local.
 *
 * @see [basic.start.term]/p1 from C++ spec.
 * @private
 */
static thread_local struct {
    // Number of characters in @a value. It can be between 0 and sizeof(value) - 1.
    std::size_t len;
    // Moniker value. This value is not null-terminated.
    char value[16];
} threadMoniker = {0, {0}};

/**
 * @brief Changes moniker value for the caller's thread.
 *
 * @param moniker Value to set.
 */
static inline void setMonikerValue(const std::string& moniker) noexcept {
    auto& tm = threadMoniker;
    tm.len = std::min(sizeof(tm.value), moniker.length());
    std::memcpy(tm.value, moniker.data(), tm.len);
}

/**
 * @brief Returns reference to this thread's moniker value.
 */
static inline std::string getMonikerValue() noexcept {
    auto& tm = threadMoniker;
    if (!tm.len) {
        setMonikerValue(ThreadMoniker::generateMoniker());
    }
    return std::string(tm.value, tm.value + tm.len);
}

#endif

std::string ThreadMoniker::generateMoniker(char prefix) noexcept {
    /// Counter to generate (small) unique thread monikers.
    static std::atomic<int> g_nextThreadMoniker(1);

    std::stringstream ss;
    if (prefix) {
        ss << prefix << CHAR_COLON;
    }
    ss << std::hex << g_nextThreadMoniker++;
    auto nextMoniker = ss.str();

    if (nextMoniker.size() < MONIKER_SIZE_CHARS) {
        nextMoniker.reserve(MONIKER_SIZE_CHARS);
        nextMoniker.insert(nextMoniker.begin(), MONIKER_SIZE_CHARS - nextMoniker.size(), CHAR_SPACE);
    }

    return nextMoniker;
}

std::string ThreadMoniker::getThisThreadMoniker() noexcept {
    return getMonikerValue();
}

void ThreadMoniker::setThisThreadMoniker(const std::string& moniker) noexcept {
    setMonikerValue(moniker);
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
