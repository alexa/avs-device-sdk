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

#include "AVSCommon/Utils/RequiresShutdown.h"
#include "AVSCommon/Utils/Logger/Logger.h"

#include <mutex>
#include <unordered_set>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// String to identify log entries originating from this file.
static const std::string TAG("RequiresShutdown");

/**
 * Create a @c LogEntry using this file's @c TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Class used to track whether @c RequiresShutdown objects have been shut down correctly.
class ShutdownMonitor {
public:
    /**
     * Adds a @c RequiresShutdown object to the set of objects being tracked.  This must be called at construction of
     * a @c RequiresShutdown object.
     *
     * @param object A pointer to the object to track.
     */
    void add(const RequiresShutdown* object);

    /**
     * Removes a @c RequiresShutdown object from the set of objects being tracked.  This must be called at destruction
     * of a @c RequiresShutdown object.
     *
     * @param object A pointer to the object to track.
     */
    void remove(const RequiresShutdown* object);

    /// Constructor
    ShutdownMonitor();

    /// Destructor.
    ~ShutdownMonitor();

private:
    /// Protects access to @c m_objects.
    std::mutex m_mutex;

    /// Alias to the container type used to hold objects.
    using Objects = std::unordered_set<const RequiresShutdown*>;

    /// The @c RequiredShutdown objects being tracked.
    Objects m_objects;

    // a @c Logger instance to be used on the destructor
    alexaClientSDK::avsCommon::utils::logger::ModuleLogger m_destructorLogger;
};

void ShutdownMonitor::add(const RequiresShutdown* object) {
    if (nullptr == object) {
        ACSDK_ERROR(LX("addFailed").d("reason", "nullptrObject"));
    }
    bool inserted = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        inserted = m_objects.insert(object).second;
    }
    if (!inserted) {
        ACSDK_ERROR(LX("addFailed").d("reason", "alreadyAdded").d("name", object->name()));
    }
}

void ShutdownMonitor::remove(const RequiresShutdown* object) {
    if (nullptr == object) {
        ACSDK_ERROR(LX("removeFailed").d("reason", "nullptrObject"));
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_objects.erase(object) == 0) {
        ACSDK_ERROR(LX("removeFailed").d("reason", "notFound").d("name", object->name()));
    }
}

ShutdownMonitor::ShutdownMonitor() : m_destructorLogger(ACSDK_STRINGIFY(ACSDK_LOG_MODULE)) {
}

ShutdownMonitor::~ShutdownMonitor() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto object : m_objects) {
        if (!object->isShutdown()) {
            m_destructorLogger.logAtExit(
                alexaClientSDK::avsCommon::utils::logger::Level::WARN,
                LX("ShutdownMonitor").d("reason", "no shutdown() call").d("name: ", object->name()));
        }
        m_destructorLogger.logAtExit(
            alexaClientSDK::avsCommon::utils::logger::Level::WARN,
            LX("ShutdownMonitor").d("reason", "never deleted").d("name", object->name()));
    }
}

/// The global @c ShutdownMonitor used to track all @c RequiresShutdown objects.
static ShutdownMonitor g_shutdownMonitor;

RequiresShutdown::RequiresShutdown(const std::string& name) : m_name{name}, m_isShutdown{false} {
    g_shutdownMonitor.add(this);
}

RequiresShutdown::~RequiresShutdown() {
    if (!m_isShutdown) {
        ACSDK_ERROR(LX("~RequiresShutdownFailed").d("reason", "notShutdown").d("name", name()));
    }
    g_shutdownMonitor.remove(this);
}

const std::string& RequiresShutdown::name() const {
    return m_name;
}

void RequiresShutdown::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isShutdown) {
        ACSDK_ERROR(LX("shutdownFailed").d("reason", "alreadyShutdown").d("name", name()));
        return;
    }
    doShutdown();
    m_isShutdown = true;
}

bool RequiresShutdown::isShutdown() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isShutdown;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
