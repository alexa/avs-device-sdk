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

#include <chrono>

#include <AVSCommon/Utils/Power/AggregatedPowerResourceManager.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "AggregatedPowerResourceManager"

/// Prefix to uniquely identify the resource.
static const std::string PREFIX = "ACSDK_";

/// The timeout to use for logging active power resources.
static const std::chrono::minutes LOG_INTERVAL{10};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

AggregatedPowerResourceManager::PowerResourceInfo::PowerResourceInfo(
    bool isRefCounted,
    avsCommon::sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level) :
        isRefCounted{isRefCounted},
        level{level},
        refCount{0},
        lastAcquired{std::chrono::system_clock::time_point::min()} {
}

void AggregatedPowerResourceManager::PowerResourceInfo::updateLastAcquiredTimepoint() {
    lastAcquired = std::chrono::system_clock::now();
}

std::shared_ptr<AggregatedPowerResourceManager> AggregatedPowerResourceManager::create(
    std::shared_ptr<PowerResourceManagerInterface> powerResourceManager) {
    ACSDK_DEBUG5(LX(__func__));
    if (!powerResourceManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullPowerResourceManager"));
        return nullptr;
    } else {
        return std::shared_ptr<AggregatedPowerResourceManager>(
            new AggregatedPowerResourceManager(powerResourceManager));
    }
}

AggregatedPowerResourceManager::~AggregatedPowerResourceManager() {
    m_timer.stop();
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it : m_aggregatedPowerResources) {
        auto id = it.second;
        m_appPowerResourceManager->close(id);
    }
}

AggregatedPowerResourceManager::AggregatedPowerResourceManager(
    std::shared_ptr<PowerResourceManagerInterface> powerResourceManager) :
        m_appPowerResourceManager{powerResourceManager} {
    m_timer.start(
        LOG_INTERVAL,
        LOG_INTERVAL,
        avsCommon::utils::timing::Timer::PeriodType::RELATIVE,
        avsCommon::utils::timing::Timer::getForever(),
        std::bind(&AggregatedPowerResourceManager::logActivePowerResources, this));
}

void AggregatedPowerResourceManager::acquirePowerResource(
    const std::string& component,
    const PowerResourceLevel level) {
    ACSDK_ERROR(LX(__func__).m("API is deprecated.Please see PowerResourceManagerInterface for alternatives"));
}

void AggregatedPowerResourceManager::releasePowerResource(const std::string& component) {
    ACSDK_ERROR(LX(__func__).m("API is deprecated.Please see PowerResourceManagerInterface for alternatives"));
};

bool AggregatedPowerResourceManager::isPowerResourceAcquired(const std::string& component) {
    ACSDK_ERROR(LX(__func__).m("API is deprecated.Please see PowerResourceManagerInterface for alternatives"));
    return false;
}

std::shared_ptr<PowerResourceManagerInterface::PowerResourceId> AggregatedPowerResourceManager::
    getAggregatedPowerResourceIdLocked(const PowerResourceManagerInterface::PowerResourceLevel level) {
    ACSDK_DEBUG9(LX(__func__));

    auto it = m_aggregatedPowerResources.find(level);
    if (m_aggregatedPowerResources.end() != it) {
        return it->second;
    }

    ACSDK_DEBUG0(LX(__func__).d("reason", "generatingNewAggregateResource").d("level", level));

    auto aggregatedPowerResourceId =
        m_appPowerResourceManager->create(PREFIX + powerResourceLevelToString(level), true, level);

    m_aggregatedPowerResources.insert({level, aggregatedPowerResourceId});
    return aggregatedPowerResourceId;
}

std::shared_ptr<PowerResourceManagerInterface::PowerResourceId> AggregatedPowerResourceManager::create(
    const std::string& resourceId,
    bool isRefCounted,
    const PowerResourceManagerInterface::PowerResourceLevel level) {
    ACSDK_DEBUG5(LX(__func__).d("id", resourceId).d("isRefCounted", isRefCounted).d("level", level));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_ids.count(resourceId) != 0) {
        ACSDK_ERROR(LX(__func__).d("reason", "resourceIdExists").d("id", resourceId));
        return nullptr;
    }

    m_ids.insert({resourceId, PowerResourceInfo(isRefCounted, level)});

    getAggregatedPowerResourceIdLocked(level);

    auto powerId = std::make_shared<PowerResourceId>(resourceId);
    return powerId;
}

bool AggregatedPowerResourceManager::acquire(
    const std::shared_ptr<PowerResourceId>& id,
    const std::chrono::milliseconds autoReleaseTimeout) {
    if (!id) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullId"));
        return false;
    }

    ACSDK_DEBUG9(LX(__func__).d("id", id->getResourceId()));
    std::lock_guard<std::mutex> lock(m_mutex);

    auto callerIt = m_ids.find(id->getResourceId());
    if (m_ids.end() == callerIt) {
        ACSDK_ERROR(LX(__func__).d("reason", "nonExistentId").d("id", id->getResourceId()));
        return false;
    }

    auto& callerPowerResourceInfo = callerIt->second;
    std::shared_ptr<PowerResourceId> aggregatedPowerResourceId =
        getAggregatedPowerResourceIdLocked(callerPowerResourceInfo.level);

    // Do not handle any auto release timer cases.
    if (autoReleaseTimeout != std::chrono::milliseconds::zero()) {
        return m_appPowerResourceManager->acquire(aggregatedPowerResourceId, autoReleaseTimeout);
    }

    // Do not dedupe  acquire calls if refcount enabled. Let the application PowerResourceManagerInterface
    // handle that if desired.
    if (callerPowerResourceInfo.isRefCounted || callerPowerResourceInfo.refCount == 0) {
        callerPowerResourceInfo.updateLastAcquiredTimepoint();
        callerPowerResourceInfo.refCount++;
        m_appPowerResourceManager->acquire(aggregatedPowerResourceId);
    }

    return true;
}

bool AggregatedPowerResourceManager::release(const std::shared_ptr<PowerResourceId>& id) {
    if (!id) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullId"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG9(LX(__func__).d("id", id->getResourceId()).d("total resources", m_ids.size()));

    auto callerIt = m_ids.find(id->getResourceId());
    if (m_ids.end() == callerIt) {
        ACSDK_ERROR(LX(__func__).d("reason", "nonExistentId").d("id", id->getResourceId()));
        return false;
    }

    auto& callerPowerResourceInfo = callerIt->second;
    std::shared_ptr<PowerResourceId> aggregatedPowerResourceId =
        getAggregatedPowerResourceIdLocked(callerPowerResourceInfo.level);

    if (callerPowerResourceInfo.refCount > 0) {
        callerPowerResourceInfo.refCount--;
        m_appPowerResourceManager->release(aggregatedPowerResourceId);
    }

    return true;
}

bool AggregatedPowerResourceManager::close(const std::shared_ptr<PowerResourceId>& id) {
    if (!id) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullId"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const std::string idString = id->getResourceId();
    ACSDK_DEBUG5(LX(__func__).d("id", id->getResourceId()));

    auto callerIt = m_ids.find(idString);
    if (m_ids.end() == callerIt) {
        ACSDK_ERROR(LX(__func__).d("reason", "nonExistentId").d("id", idString));
        return false;
    }

    auto& callerPowerResourceInfo = callerIt->second;
    auto level = callerPowerResourceInfo.level;

    std::shared_ptr<PowerResourceId> aggregatedPowerResourceId = getAggregatedPowerResourceIdLocked(level);

    for (unsigned int i = 0; i < callerPowerResourceInfo.refCount; i++) {
        m_appPowerResourceManager->release(aggregatedPowerResourceId);
    }

    // We do not close the underlying aggregated PowerResource to prevent any latency hit from dynamically
    // creating/closing.
    m_ids.erase(idString);

    return true;
}

void AggregatedPowerResourceManager::logActivePowerResources() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::stringstream log;
    size_t numActive = 0;
    for (const auto& id : m_ids) {
        if (id.second.refCount > 0) {
            auto dateTime = m_logFormatter.getDateTimeString(id.second.lastAcquired);
            auto milliseconds = m_logFormatter.getMillisecondString(id.second.lastAcquired);
            log << id.first << " acquired " << (dateTime.empty() ? "ERROR: Date and time not logged." : dateTime) << "."
                << (milliseconds.empty() ? "ERROR: Milliseconds not logged." : milliseconds) << ",";

            numActive++;
        }
    }

    log << "total active=" << numActive;

    ACSDK_INFO(LX(__func__).m(log.str()));
}

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
