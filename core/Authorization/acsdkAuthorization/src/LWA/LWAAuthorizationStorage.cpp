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

#include <fstream>

#include <acsdkAuthorization/LWA/LWAAuthorizationStorage.h>
#include <acsdkAuthorization/private/Logging.h>
#include <acsdkAuthorization/private/LWA/LWAStorageConstants.h>
#include <acsdkAuthorization/private/LWA/LWAStorageDataMigration.h>
#include <acsdk/Properties/EncryptedPropertiesFactories.h>
#include <acsdk/Properties/MiscStorageAdapter.h>
#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

using namespace ::alexaClientSDK::acsdkAuthorizationInterfaces;
using namespace ::alexaClientSDK::acsdkAuthorizationInterfaces::lwa;
using namespace ::alexaClientSDK::properties;
using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::avsCommon::utils::configuration;
using namespace ::alexaClientSDK::avsCommon::utils::logger;
using namespace ::alexaClientSDK::storage;

/// String to identify log entries originating from this file.
#define TAG "LWAAuthorizationStorage"

/// Name of default @c ConfigurationNode for LWA
static const std::string CONFIG_KEY_LWA_AUTHORIZATION = "lwaAuthorization";

/// Name of @c databaseFilePath value in the @c ConfigurationNode.
static const std::string CONFIG_KEY_DB_FILE_PATH_KEY = "databaseFilePath";

std::shared_ptr<LWAAuthorizationStorageInterface> LWAAuthorizationStorage::createStorage(
    const std::shared_ptr<propertiesInterfaces::PropertiesFactoryInterface>& propertiesFactory) {
    if (!propertiesFactory) {
        ACSDK_ERROR(LX("createStorageFailed").d("reason", "propertiesFactoryNull"));
        return nullptr;
    }

    return std::shared_ptr<LWAAuthorizationStorageInterface>(new LWAAuthorizationStorage(propertiesFactory));
}

bool LWAAuthorizationStorage::createStorageFileAndSetPermissions(const std::string& filepath) noexcept {
    using namespace avsCommon::utils::filesystem;

    if (exists(filepath)) {
        ACSDK_DEBUG9(
            LX("createStorageFileAndSetPermissionsSuccess").d("reason", "fileExists").sensitive("path", filepath));
        return true;
    }

    std::ofstream file{filepath, std::ofstream::out | std::ofstream::binary};
    if (!file.is_open()) {
        ACSDK_DEBUG9(
            LX("createStorageFileAndSetPermissionsFailed").d("reason", "createError").sensitive("path", filepath));
        return false;
    }
    file.close();

    if (!changePermissions(filepath, OWNER_READ | OWNER_WRITE)) {
        ACSDK_DEBUG9(LX("createStorageFileAndSetPermissionsFailed").sensitive("path", filepath));
        return false;
    }

    ACSDK_DEBUG9(LX("createStorageFileAndSetPermissionsSuccess").sensitive("path", filepath));

    return true;
}

std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage> LWAAuthorizationStorage::createSQLiteStorage(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
    const std::string& storageRootKey) {
    ACSDK_DEBUG5(LX("createSQLiteStorage"));

    if (!configurationRoot) {
        ACSDK_ERROR(LX("createSQLiteStorageFailed").d("reason", "nullConfigurationRoot"));
        return nullptr;
    }

    std::string key = storageRootKey.empty() ? CONFIG_KEY_LWA_AUTHORIZATION : storageRootKey;

    auto configNode = (*configurationRoot)[key];
    if (!configNode) {
        ACSDK_ERROR(LX("createSQLiteStorageFailed").d("reason", "missingConfigurationValue").d("key", key));
        return nullptr;
    }

    std::string databaseFilePath;
    if (!configNode.getString(CONFIG_KEY_DB_FILE_PATH_KEY, &databaseFilePath) || databaseFilePath.empty()) {
        ACSDK_ERROR(LX("createSQLiteStorageFailed")
                        .d("reason", "missingConfigurationValue")
                        .d("key", CONFIG_KEY_DB_FILE_PATH_KEY));
        return nullptr;
    }

    if (!createStorageFileAndSetPermissions(databaseFilePath)) {
        ACSDK_ERROR(
            LX("createSQLiteStorageFailed").d("reason", "failedToCreateDBFile").sensitive("path", databaseFilePath));
        return nullptr;
    }

    auto storage = storage::sqliteStorage::SQLiteMiscStorage::create(databaseFilePath);
    if (!storage || !storage->open()) {
        ACSDK_ERROR(LX("createSQLiteStorageFailed").d("reason", "openFailed").sensitive("path", databaseFilePath));
        return nullptr;
    }

    return std::move(storage);
}

std::shared_ptr<LWAAuthorizationStorageInterface> LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
    const std::string& storageRootKey,
    const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<cryptoInterfaces::KeyStoreInterface>& keyStore) {
    ACSDK_DEBUG0(LX("createLWAAuthorizationStorageInterface"));

    bool useEncryptionAtRest = true;
    if (!cryptoFactory) {
        ACSDK_WARN(LX("createLWAAuthorizationStorageInterface")
                       .m("encryptionAtRestDisabled")
                       .d("reason", "cryptoFactoryNull"));
        useEncryptionAtRest = false;
    }
    if (!keyStore) {
        ACSDK_WARN(
            LX("createLWAAuthorizationStorageInterface").m("encryptionAtRestDisabled").d("reason", "keyStoreNull"));
        useEncryptionAtRest = false;
    }
    auto storage = createSQLiteStorage(configurationRoot, storageRootKey);
    if (!storage) {
        ACSDK_ERROR(LX("createLWAAuthorizationStorageInterfaceFailed").d("reason", "storageNull"));
        return nullptr;
    }

    std::shared_ptr<PropertiesFactoryInterface> propertiesFactory;
    if (useEncryptionAtRest) {
        ACSDK_INFO(LX("createLWAAuthorizationStorageInterface").m("encryptionAtRestEnabled"));
        propertiesFactory =
            createEncryptedPropertiesFactory(storage, SimpleMiscStorageUriMapper::create(), cryptoFactory, keyStore);
    } else {
        propertiesFactory = createPropertiesFactory(storage, SimpleMiscStorageUriMapper::create());
    }

    if (!propertiesFactory) {
        ACSDK_ERROR(LX("createLWAAuthorizationStorageInterfaceFailed").d("reason", "propertiesFactoryNull"));
        return nullptr;
    }

    LWAStorageDataMigration{storage, propertiesFactory}.upgradeStorage();

    return createStorage(propertiesFactory);
}

LWAAuthorizationStorage::LWAAuthorizationStorage(const std::shared_ptr<PropertiesFactoryInterface>& propertiesFactory) :
        m_propertiesFactory{propertiesFactory} {
    ACSDK_DEBUG5(LX("LWAAuthorizationStorage"));
}

LWAAuthorizationStorage::~LWAAuthorizationStorage() {
    ACSDK_DEBUG5(LX("~LWAAuthorizationStorage"));
}

bool LWAAuthorizationStorage::openOrCreate() {
    ACSDK_DEBUG5(LX("openOrCreate"));

    m_properties = m_propertiesFactory->getProperties(CONFIG_URI);
    if (!m_properties) {
        ACSDK_ERROR(LX("openOrCreateFailed").d("reason", "propertiesGetError"));
        return false;
    }

    return true;
}

bool LWAAuthorizationStorage::createDatabase() {
    ACSDK_DEBUG5(LX("createDatabase"));

    return openOrCreate();
}

bool LWAAuthorizationStorage::open() {
    ACSDK_DEBUG5(LX("open"));

    return openOrCreate();
}

bool LWAAuthorizationStorage::setRefreshToken(const std::string& refreshToken) {
    ACSDK_DEBUG5(LX("setRefreshToken"));

    if (refreshToken.empty()) {
        ACSDK_ERROR(LX("setRefreshTokenFailed").d("reason", "refreshTokenIsEmpty"));
        return false;
    }

    if (!m_properties) {
        ACSDK_ERROR(LX("setRefreshTokenFailed").d("reason", "storageClosed"));
        return false;
    }

    if (!m_properties->putString(REFRESH_TOKEN_PROPERTY_NAME, refreshToken)) {
        ACSDK_ERROR(LX("setRefreshTokenFailed").d("reason", "clearTableFailed"));
        return false;
    }

    return true;
}

bool LWAAuthorizationStorage::clearRefreshToken() {
    ACSDK_DEBUG5(LX("clearRefreshToken"));

    if (!m_properties) {
        ACSDK_ERROR(LX("clearRefreshTokenFailed").d("reason", "storageClosed"));
        return false;
    }

    if (!m_properties->remove(REFRESH_TOKEN_PROPERTY_NAME)) {
        ACSDK_ERROR(LX("clearRefreshTokenFailed").d("reason", "clearTableFailed"));
        return false;
    }

    return true;
}

bool LWAAuthorizationStorage::getRefreshToken(std::string* refreshToken) {
    ACSDK_DEBUG5(LX("getRefreshToken"));

    if (!refreshToken) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "nullRefreshToken"));
        return false;
    }

    if (!m_properties) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "storageClosed"));
        return false;
    }

    std::string tmp;
    if (!m_properties->getString(REFRESH_TOKEN_PROPERTY_NAME, tmp)) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "stepFailed"));
        return false;
    }

    if (tmp.empty()) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "emptyValue"));
        return false;
    }

    *refreshToken = tmp;

    return true;
}

bool LWAAuthorizationStorage::setUserId(const std::string& userId) {
    ACSDK_DEBUG5(LX("setUserId").sensitive("userId", userId));

    if (!m_properties) {
        ACSDK_ERROR(LX("setUserIdFailed").d("reason", "storageClosed"));
        return false;
    }

    if (!m_properties->putString(USER_ID_PROPERTY_NAME, userId)) {
        ACSDK_ERROR(LX("setUserIdFailed").d("reason", "putStringFailed"));
        return false;
    }

    return true;
}

bool LWAAuthorizationStorage::getUserId(std::string* userId) {
    ACSDK_DEBUG5(LX("getUserId"));

    if (!userId) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "nullRefreshToken"));
        return false;
    }

    if (!m_properties) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "storageClosed"));
        return false;
    }

    std::string tmp;
    if (!m_properties->getString(USER_ID_PROPERTY_NAME, tmp)) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "createStatementFailed"));
        return false;
    }

    if (tmp.empty()) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "emptyValue"));
        return false;
    }

    *userId = tmp;

    return true;
}

bool LWAAuthorizationStorage::clear() {
    ACSDK_DEBUG5(LX("clear"));

    if (!m_properties) {
        ACSDK_ERROR(LX("clearFailed").d("reason", "storageClosed"));
        return false;
    }

    // Be careful of short circuiting here.
    return m_properties->clear();
}

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
