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

#include <acsdkAuthorization/AuthorizationManagerStorage.h>
#include <acsdkAuthorization/private/Logging.h>

/// String to identify log entries originating from this file.
#define TAG "AuthorizationManagerStorage"

namespace alexaClientSDK {
namespace acsdkAuthorization {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::storage;

/// The component name used for @c MiscStorageInterface.
static const std::string COMPONENT_NAME = "AuthorizationManager";

/// The table name used for @c MiscStorageInterface.
static const std::string AUTH_STATE_TABLE = "authorizationState";

/// The authAdapterId key associated with the adapterId.
static const std::string AUTH_ADAPTER_ID_KEY = "authAdapterId";

/// The authAdapterId key associated with the storage userId.
static const std::string USER_ID_KEY = "userId";

std::shared_ptr<AuthorizationManagerStorage> AuthorizationManagerStorage::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage) {
    ACSDK_DEBUG5(LX("create"));

    if (!storage) {
        ACSDK_ERROR(LX("createFailed").d("isStorageNull", !storage));

        return nullptr;
    }

    auto authMgrStorage = std::shared_ptr<AuthorizationManagerStorage>(new AuthorizationManagerStorage(storage));
    if (!authMgrStorage->initializeDatabase()) {
        return nullptr;
    }

    return authMgrStorage;
}

AuthorizationManagerStorage::AuthorizationManagerStorage(
    const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage) :
        m_storage{storage} {
}

bool AuthorizationManagerStorage::initializeDatabase() {
    ACSDK_DEBUG5(LX("initializeDatabase"));
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!openLocked()) {
        if (!m_storage->createDatabase() || !openLocked()) {
            ACSDK_ERROR(LX("initializeDatabaseFailed").d("reason", "createDatabaseFailed"));
            return false;
        }
    }

    bool exists = false;

    if (!m_storage->tableExists(COMPONENT_NAME, AUTH_STATE_TABLE, &exists)) {
        ACSDK_ERROR(LX("initializeDatabaseFailed").d("reason", "checkTableExistenceFailed"));
        return false;
    }

    if (!exists) {
        if (!m_storage->createTable(
                COMPONENT_NAME,
                AUTH_STATE_TABLE,
                MiscStorageInterface::KeyType::STRING_KEY,
                MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("initializeDatabaseFailed").d("reason", "createTableFailed"));
            return false;
        }
    }

    return true;
}

bool AuthorizationManagerStorage::openLocked() {
    ACSDK_DEBUG5(LX("openLocked"));

    return m_storage->isOpened() || m_storage->open();
}

bool AuthorizationManagerStorage::store(const std::string& adapterId, const std::string& userId) {
    ACSDK_DEBUG5(LX("store"));
    std::lock_guard<std::mutex> lock(m_mutex);

    std::unordered_map<std::string, std::string> valueContainer;

    m_storage->load(COMPONENT_NAME, AUTH_STATE_TABLE, &valueContainer);

    if (valueContainer.size() != 0) {
        ACSDK_WARN(LX("storeFailed").d("reason", "tableNotEmpty"));
    }

    if (!m_storage->put(COMPONENT_NAME, AUTH_STATE_TABLE, AUTH_ADAPTER_ID_KEY, adapterId)) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "storeAdapterIdFailed").d("adapterId", adapterId));

        return false;
    }

    if (!m_storage->put(COMPONENT_NAME, AUTH_STATE_TABLE, USER_ID_KEY, userId)) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "storeUserIdFailed").d("userId", userId));
        return false;
    }

    return true;
}

bool AuthorizationManagerStorage::load(std::string& adapterId, std::string& userId) {
    ACSDK_DEBUG5(LX("load"));

    std::lock_guard<std::mutex> lock(m_mutex);

    std::unordered_map<std::string, std::string> valueContainer;
    if (!m_storage->load(COMPONENT_NAME, AUTH_STATE_TABLE, &valueContainer)) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "storageLoadError"));
        return false;
    }

    auto it = valueContainer.find(AUTH_ADAPTER_ID_KEY);
    if (it == valueContainer.end()) {
        ACSDK_DEBUG0(LX("loadFailed").d("reason", "missingAuthAdapterId"));
        adapterId.clear();
    } else {
        adapterId = it->second;
    }

    it = valueContainer.find(USER_ID_KEY);
    if (it == valueContainer.end()) {
        ACSDK_DEBUG0(LX("loadFailed").d("reason", "missingUserId"));
        userId.clear();
    } else {
        userId = it->second;
    }

    ACSDK_DEBUG5(LX("loadFailed").d("authAdapterId", adapterId).d("userId", userId));

    return true;
}

void AuthorizationManagerStorage::clear() {
    ACSDK_DEBUG5(LX("clear"));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage->clearTable(COMPONENT_NAME, AUTH_STATE_TABLE);
}

}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
