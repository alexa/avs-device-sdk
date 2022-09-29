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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Power/PowerMonitor.h"
#include "AVSCommon/Utils/Power/AggregatedPowerResourceManager.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

/// String to identify log entries originating from this file.
#define TAG "PowerMonitor"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::mutex PowerMonitor::m_mutex;
std::shared_ptr<PowerMonitor> PowerMonitor::m_monitor;

std::shared_ptr<PowerMonitor> PowerMonitor::getInstance() {
    ACSDK_DEBUG9(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_monitor) {
        PowerMonitor::m_monitor = std::shared_ptr<PowerMonitor>(new PowerMonitor());
    }

    return m_monitor;
}

void PowerMonitor::activate(std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> powerManager) {
    ACSDK_DEBUG9(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (isActiveLocked()) {
        ACSDK_ERROR(LX(__func__).d("reason", "alreadyActive"));
        return;
    }

    if (powerManager) {
        m_powerManager = AggregatedPowerResourceManager::create(powerManager);
    } else {
        ACSDK_ERROR(LX(__func__).d("reason", "nullPowerManager"));
    }
}

bool PowerMonitor::isActive() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isActiveLocked();
}

bool PowerMonitor::isActiveLocked() {
    bool active = m_powerManager != nullptr;
    ACSDK_DEBUG9(LX(__func__).d("isActiveLocked", active));
    return active;
}

void PowerMonitor::deactivate() {
    ACSDK_DEBUG9(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_powerManager.reset();
    m_threadPowerResources.clear();
}

std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> PowerMonitor::getPowerResourceManager() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_powerManager;
}

std::shared_ptr<PowerResource> PowerMonitor::createLocalPowerResource(
    const std::string& identifier,
    sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level) {
    ACSDK_DEBUG9(LX(__func__).d("identifier", identifier));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!isActiveLocked()) {
        return nullptr;
    }

    return PowerResource::create(identifier, m_powerManager, level);
}

std::shared_ptr<PowerResource> PowerMonitor::getThreadPowerResource() {
    ACSDK_DEBUG9(LX(__func__).d("threadId", std::this_thread::get_id()));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!isActiveLocked()) {
        return nullptr;
    }

    auto threadId = std::this_thread::get_id();
    std::shared_ptr<PowerResource> resource;

    auto it = m_threadPowerResources.find(threadId);
    if (m_threadPowerResources.end() != it) {
        resource = it->second;
    }

    return resource;
}

std::shared_ptr<PowerResource> PowerMonitor::assignThreadPowerResource(std::shared_ptr<PowerResource> powerResource) {
    ACSDK_DEBUG9(LX(__func__).d("threadId", std::this_thread::get_id()));

    auto threadId = std::this_thread::get_id();
    std::shared_ptr<PowerResource> ret;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!isActiveLocked()) {
        return ret;
    } else if (!powerResource) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullPowerResource"));
        return ret;
    }

    if (m_threadPowerResources.count(threadId) != 0) {
        auto existingResource = m_threadPowerResources.at(threadId);

        ACSDK_ERROR(LX(__func__)
                        .d("threadId", threadId)
                        .d("reason", "threadAlreadyHasResource")
                        .d("resourceId", existingResource->getId()));
    } else {
        m_threadPowerResources.insert({threadId, powerResource});
        ret = powerResource;
    }

    return ret;
}

std::shared_ptr<PowerResource> PowerMonitor::getThreadPowerResourceOrCreate(
    const std::string& identifier,
    sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level) {
    ACSDK_DEBUG9(LX(__func__));

    if (!isActive()) {
        return nullptr;
    }

    auto powerResource = getThreadPowerResource();

    // Either the wrong identifier was passed in or the thread did not cleanup its resource.
    // Will fail gracefully, but this is not behavior guaranteed by the method.
    if (powerResource && powerResource->getId() != identifier) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "mismatchedIdentifier")
                        .d("existingIdentifier", powerResource->getId())
                        .d("identifier", identifier)
                        .d("action", "deletingExistingResource"));

        removeThreadPowerResource();
        powerResource.reset();
    }

    if (!powerResource) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_powerManager) {
            ACSDK_ERROR(LX(__func__).d("reason", "nullPowerManager"));
            return nullptr;
        }

        ACSDK_DEBUG9(LX(__func__)
                         .d("reason", "creatingPowerResource")
                         .d("identifier", identifier)
                         .d("threadId", std::this_thread::get_id()));
        powerResource = PowerResource::create(identifier, m_powerManager, level);

        m_threadPowerResources.insert({std::this_thread::get_id(), powerResource});
    }

    return powerResource;
}

void PowerMonitor::removeThreadPowerResource() {
    ACSDK_DEBUG9(LX(__func__).d("threadId", std::this_thread::get_id()));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadPowerResources.erase(std::this_thread::get_id());
}

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
