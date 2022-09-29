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

#include <acsdkAuthorization/LWA/LWAAuthorizationStorage.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

using namespace acsdkAuthorization::lwa;
using namespace acsdkAuthorizationInterfaces;
using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
#define TAG "SQLiteCBLAuthDelegateStorage"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for CBLAuthDelegate
static const std::string CONFIG_KEY_CBL_AUTH_DELEGATE = "cblAuthDelegate";

std::shared_ptr<CBLAuthDelegateStorageInterface> SQLiteCBLAuthDelegateStorage::createCBLAuthDelegateStorageInterface(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
    const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<cryptoInterfaces::KeyStoreInterface>& keyStore) {
    if (!configurationRoot) {
        ACSDK_ERROR(LX("createCBLAuthDelegateStorageInterfaceFailed").d("reason", "nullConfigurationRoot"));
        return nullptr;
    }

    auto lwaStorage = LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(
        configurationRoot, CONFIG_KEY_CBL_AUTH_DELEGATE, cryptoFactory, keyStore);

    if (!lwaStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "createLWAStorageFailed"));
        return nullptr;
    }

    return std::shared_ptr<SQLiteCBLAuthDelegateStorage>(new SQLiteCBLAuthDelegateStorage(lwaStorage));
}

SQLiteCBLAuthDelegateStorage::~SQLiteCBLAuthDelegateStorage() {
    ACSDK_DEBUG5(LX("~SQLiteCBLAuthDelegateStorage"));
    m_lwaStorage.reset();
}

bool SQLiteCBLAuthDelegateStorage::createDatabase() {
    ACSDK_DEBUG5(LX("createDatabase"));
    return m_lwaStorage->createDatabase();
}

bool SQLiteCBLAuthDelegateStorage::open() {
    ACSDK_DEBUG5(LX("open"));
    return m_lwaStorage->open();
}

bool SQLiteCBLAuthDelegateStorage::setRefreshToken(const std::string& refreshToken) {
    ACSDK_DEBUG5(LX("setRefreshToken"));
    return m_lwaStorage->setRefreshToken(refreshToken);
}

bool SQLiteCBLAuthDelegateStorage::clearRefreshToken() {
    ACSDK_DEBUG5(LX("clearRefreshToken"));
    return m_lwaStorage->clearRefreshToken();
}

bool SQLiteCBLAuthDelegateStorage::getRefreshToken(std::string* refreshToken) {
    ACSDK_DEBUG5(LX("getRefreshToken"));
    return m_lwaStorage->getRefreshToken(refreshToken);
}

bool SQLiteCBLAuthDelegateStorage::clear() {
    ACSDK_DEBUG5(LX("clear"));
    return m_lwaStorage->clear();
}

SQLiteCBLAuthDelegateStorage::SQLiteCBLAuthDelegateStorage(
    const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>& lwaStorage) :
        m_lwaStorage{lwaStorage} {
}

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK
