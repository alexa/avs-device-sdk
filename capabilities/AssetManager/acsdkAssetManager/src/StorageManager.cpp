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

#include "StorageManager.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>
#include <fstream>

#include "acsdkAssetManager/AssetManager.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace alexaClientSDK::avsCommon::utils;
using namespace common;

#if UNIT_TEST == 1
// For tests, because we don't want to create megabytes worth of files...
static constexpr auto BYTES_IN_MB = 1024 * 4;
#else
static constexpr auto BYTES_IN_MB = 1024 * 1024;
#endif
static const string BUDGET_FILE_SUFFIX = "/budget.config";

// when looking at how much space is available on the system, be sure to leave out a few MBs
static constexpr size_t SYSTEM_STORAGE_BUFFER = 5 * BYTES_IN_MB;

static const auto s_metrics = AmdMetricsWrapper::creator("StorageManager");

/// String to identify log entries originating from this file.
static const std::string TAG{"StorageManager"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

size_t subtractSize(size_t original, size_t subAmount) {
    if (original <= subAmount) {
        return 0;
    }
    return original - subAmount;
}

shared_ptr<StorageManager> StorageManager::create(
        const string& workingDirectory,
        const shared_ptr<AssetManager>& assetManager) {
    if (assetManager == nullptr) {
        ACSDK_ERROR(LX("create").m("Null AssetManager"));
        return nullptr;
    }
    auto budget = MAX_BUDGET_MB;
    ifstream configFile(workingDirectory + BUDGET_FILE_SUFFIX);
    if (configFile.good()) {
        configFile >> budget;
    }
    if (budget <= 0) {
        budget = MAX_BUDGET_MB;
        s_metrics().addCounter(METRIC_PREFIX_ERROR("budgetSetToZero"));
    }
    auto sm = shared_ptr<StorageManager>(new StorageManager(workingDirectory, assetManager, budget));

    if (!sm->init()) {
        ACSDK_ERROR(LX("create").m("Failed to initialize Storage Manager"));
        return nullptr;
    }

    return sm;
}

StorageManager::StorageManager(
        const string& workingDirectory,
        const shared_ptr<AssetManager>& assetManager,
        size_t budget) :
        m_workingDirectory(workingDirectory),
        m_assetManager(assetManager),
        m_budget(budget),
        m_allocatedSize(0) {
}

bool StorageManager::init() {
    if (!filesystem::makeDirectory(m_workingDirectory)) {
        ACSDK_ERROR(LX("init")
                            .m("This should never happen, failed to create working directory")
                            .d("directory", m_workingDirectory));
        return false;
    }

    auto directories = filesystem::list(m_workingDirectory, filesystem::FileType::DIRECTORY);
    for (const auto& dirName : directories) {
        auto resourceDirectory = m_workingDirectory + "/" + dirName;
        auto resource = Resource::createFromStorage(resourceDirectory);
        if (resource != nullptr) {
            ACSDK_INFO(LX("init").m("Loaded stored resource").d("resource", resource->getPath()));
            m_allocatedSize += resource->getSizeBytes();
            m_bank[resource->getId()] = resource;
        } else {
            ACSDK_ERROR(LX("init").m("Failed to load stored resource, cleaning it up!"));
            filesystem::removeAll(resourceDirectory);
        }
    }
    return true;
}

void StorageManager::purgeUnreferenced() {
    unique_lock<mutex> lock(m_allocationMutex);
    for (auto resource = m_bank.begin(); resource != m_bank.end();) {
        if (resource->second == nullptr) {
            resource = m_bank.erase(resource);
        } else if (resource->second->m_refCount <= 0) {
            m_allocatedSize -= resource->second->getSizeBytes();
            resource->second->erase();
            resource = m_bank.erase(resource);
        } else {
            ++resource;
        }
    }
}

bool StorageManager::requestSpace(const size_t requestedAmount) {
    auto assetManagerPtr = m_assetManager.lock();
    if (assetManagerPtr == nullptr) {
        ACSDK_ERROR(
                LX("requestSpace")
                        .m("This should never happen, assetManager is not available, failed to free requested space"));
        return false;
    }
    if (!assetManagerPtr->freeUpSpace(requestedAmount)) {
        ACSDK_ERROR(LX("requestSpace").m("Failed to clear requested space").d("requestedSpaceBytes", requestedAmount));
        return false;
    }

    ACSDK_DEBUG(
            LX("requestSpace").m("Was able to clear up requested space").d("RequestedNumberOfBytes", requestedAmount));
    return true;
}

size_t StorageManager::availableBudget() {
    lock_guard<mutex> lock(m_allocationMutex);
    auto availableStorage = subtractSize(filesystem::availableSpace(m_workingDirectory), SYSTEM_STORAGE_BUFFER);
    auto budgetSize = m_budget * BYTES_IN_MB;

    // if we are over our budget, then return 0
    if (budgetSize < m_allocatedSize) {
        return 0;
    }

    // return the lesser of:
    // a. how much space is available on the device, and
    // b. how many bytes we have left on our budget
    return min(budgetSize - m_allocatedSize, availableStorage);
}

void StorageManager::requestGarbageCollection(const size_t requestedAmount) {
    auto assetManagerPtr = m_assetManager.lock();
    if (assetManagerPtr == nullptr) {
        ACSDK_ERROR(LX("requestGarbageCollection")
                            .m("This should never happen, assetManager is not available, failed to request garbage "
                               "collection"));
        return;
    }

    assetManagerPtr->queueFreeUpSpace(requestedAmount);
}

shared_ptr<Resource> StorageManager::registerAndAcquireResource(
        unique_ptr<ReservationToken> reservationToken,
        const string& id,
        const string& sourcePath) {
    if (reservationToken == nullptr) {
        ACSDK_ERROR(LX("registerAndAcquireResource").m("Cannot register a new resource without first reserving space"));
        return nullptr;
    }
    // destroy the token to free up reservation space, this token is required to force users to account for how much
    // space they'll need before starting the download in order to make sure we have enough to store it.
    // when we destroy the token, it's destructor will callback to free up the reserved space (by design)
    reservationToken.reset();

    unique_lock<mutex> lock(m_allocationMutex);
    auto& resource = m_bank[id];
    if (resource != nullptr) {
        ACSDK_WARN(LX("registerAndAcquireResource")
                           .m("Attempting to register path, which already exists, ignoring...")
                           .d("path", sourcePath));
        filesystem::removeAll(sourcePath);
        resource->incrementRefCount();
        return resource;
    }

    resource = Resource::create(m_workingDirectory, id, sourcePath);
    if (resource == nullptr) {
        ACSDK_ERROR(LX("registerAndAcquireResource").m("Failed to register resource").d("resource", id));
        return nullptr;
    }

    resource->incrementRefCount();
    m_allocatedSize += resource->getSizeBytes();
    auto budgetSize = m_budget * BYTES_IN_MB;
    if (m_allocatedSize > budgetSize) {
        requestGarbageCollection(m_allocatedSize - budgetSize);
    }

    return resource;
}

shared_ptr<Resource> StorageManager::acquireResource(const string& id) {
    unique_lock<mutex> lock(m_allocationMutex);
    auto& resource = m_bank[id];
    if (resource != nullptr) {
        resource->incrementRefCount();
    }
    return resource;
}

size_t StorageManager::releaseResource(const shared_ptr<Resource>& resource) {
    if (resource == nullptr) {
        ACSDK_INFO(LX("releaseResource").m("Null resource provided, nothing to release"));
        return 0;
    }

    unique_lock<mutex> lock(m_allocationMutex);
    if (resource->decrementRefCount() > 0) {
        return 0;
    }

    ACSDK_INFO(LX("releaseResource").m("There is no usage for resource, deleting").d("resource", resource->m_id));
    auto size = resource->getSizeBytes();
    m_bank.erase(resource->m_id);
    resource->erase();

    m_allocatedSize = subtractSize(m_allocatedSize, size);
    return size;
}

unique_ptr<StorageManager::ReservationToken> StorageManager::reserveSpace(size_t requestedAmount) {
    auto available = availableBudget();
    if (requestedAmount > available && !requestSpace(requestedAmount - available)) {
        ACSDK_ERROR(LX("reserveSpace")
                            .m("Could not reserve the requested amount of space")
                            .d("requestedSpaceBytes", requestedAmount));
        return nullptr;
    }
    unique_lock<mutex> lock(m_allocationMutex);
    m_allocatedSize += requestedAmount;
    return std::unique_ptr<ReservationToken>(new ReservationToken(shared_from_this(), requestedAmount));
}

void StorageManager::freeReservedSpace(size_t size) {
    unique_lock<mutex> lock(m_allocationMutex);
    m_allocatedSize = subtractSize(m_allocatedSize, size);
}

size_t StorageManager::getBudget() {
    lock_guard<mutex> lock(m_allocationMutex);
    return m_budget;
}

void StorageManager::setBudget(size_t value) {
    ofstream configFile(m_workingDirectory + BUDGET_FILE_SUFFIX);
    if (configFile.good()) {
        configFile << value;
    }

    auto newSize = static_cast<size_t>(value) * BYTES_IN_MB;
    lock_guard<mutex> lock(m_allocationMutex);
    if (m_allocatedSize > newSize) {
        requestGarbageCollection(m_allocatedSize - newSize);
    }
    m_budget = value;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
