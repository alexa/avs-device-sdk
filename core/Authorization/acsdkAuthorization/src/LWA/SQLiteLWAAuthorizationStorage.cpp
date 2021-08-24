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

#include <SQLiteStorage/SQLiteStatement.h>
#include <SQLiteStorage/SQLiteUtils.h>
#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "acsdkAuthorization/LWA/SQLiteLWAAuthorizationStorage.h"

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

using namespace acsdkAuthorizationInterfaces;
using namespace acsdkAuthorizationInterfaces::lwa;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace avsCommon::utils::file;
using namespace alexaClientSDK::storage::sqliteStorage;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteLWAAuthorizationStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of default @c ConfigurationNode for LWA
static const std::string CONFIG_KEY_LWA_AUTHORIZATION = "lwaAuthorization";

/// Name of @c databaseFilePath value in the @c ConfigurationNode.
static const std::string CONFIG_KEY_DB_FILE_PATH_KEY = "databaseFilePath";

/// The name of the refreshToken table.
#define REFRESH_TOKEN_TABLE_NAME "refreshToken"

/// The name of the refreshToken column.
#define REFRESH_TOKEN_COLUMN_NAME "refreshToken"

/// The name of the userId table.
#define USER_ID_TABLE_NAME "userId"

/// The name of the userId column.
#define USER_ID_COLUMN_NAME "userId"

/// String for creating the refreshToken table
static const std::string CREATE_REFRESH_TOKEN_TABLE_SQL_STRING =
    "CREATE TABLE " REFRESH_TOKEN_TABLE_NAME " (" REFRESH_TOKEN_COLUMN_NAME " TEXT);";

/// String for creating the userId table
static const std::string CREATE_USER_ID_TABLE_SQL_STRING =
    "CREATE TABLE " USER_ID_TABLE_NAME " (" USER_ID_COLUMN_NAME " TEXT);";

std::shared_ptr<LWAAuthorizationStorageInterface> SQLiteLWAAuthorizationStorage::createLWAAuthorizationStorageInterface(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRootPtr,
    const std::string& storageRootKey) {
    ACSDK_DEBUG5(LX(__func__));

    if (!configurationRootPtr) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullConfigurationRoot"));
        return nullptr;
    }

    auto configurationRoot = *configurationRootPtr;

    std::string key = storageRootKey.empty() ? CONFIG_KEY_LWA_AUTHORIZATION : storageRootKey;

    auto storageConfigRoot = configurationRoot[key];
    if (!storageConfigRoot) {
        ACSDK_ERROR(LX(__func__).d("reason", "missingConfigurationValue").d("key", key));
        return nullptr;
    }

    std::string databaseFilePath;
    if (!storageConfigRoot.getString(CONFIG_KEY_DB_FILE_PATH_KEY, &databaseFilePath) || databaseFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return std::shared_ptr<SQLiteLWAAuthorizationStorage>(new SQLiteLWAAuthorizationStorage(databaseFilePath));
}

SQLiteLWAAuthorizationStorage::~SQLiteLWAAuthorizationStorage() {
    ACSDK_DEBUG5(LX(__func__));

    close();
}

bool SQLiteLWAAuthorizationStorage::openOrCreate() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_database.open()) {
        if (!m_database.initialize()) {
            ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "SQLiteCreateDatabaseFailed"));
            return false;
        }
    }

    if (!m_database.tableExists(REFRESH_TOKEN_TABLE_NAME)) {
        if (!m_database.performQuery(CREATE_REFRESH_TOKEN_TABLE_SQL_STRING)) {
            ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create refreshToken table"));
            close();
            return false;
        }
    }

    if (!m_database.tableExists(USER_ID_TABLE_NAME)) {
        if (!m_database.performQuery(CREATE_USER_ID_TABLE_SQL_STRING)) {
            ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create userId table"));
            close();
            return false;
        }
    }

    return true;
}

bool SQLiteLWAAuthorizationStorage::createDatabase() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_database.initialize()) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "SQLiteCreateDatabaseFailed"));
        return false;
    }

    if (!m_database.performQuery(CREATE_REFRESH_TOKEN_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create refreshToken table"));
        close();
        return false;
    }

    if (!m_database.performQuery(CREATE_USER_ID_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create userId table"));
        close();
        return false;
    }

    return true;
}

bool SQLiteLWAAuthorizationStorage::open() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_database.open()) {
        ACSDK_DEBUG0(LX("openFailed").d("reason", "openSQLiteDatabaseFailed"));
        return false;
    }

    if (!m_database.tableExists(REFRESH_TOKEN_TABLE_NAME)) {
        if (!m_database.performQuery(CREATE_REFRESH_TOKEN_TABLE_SQL_STRING)) {
            ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create refreshToken table"));
            close();
            return false;
        }
    }

    if (!m_database.tableExists(USER_ID_TABLE_NAME)) {
        if (!m_database.performQuery(CREATE_USER_ID_TABLE_SQL_STRING)) {
            ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create userId table"));
            close();
            return false;
        }
    }

    return true;
}

bool SQLiteLWAAuthorizationStorage::setRefreshToken(const std::string& refreshToken) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (refreshToken.empty()) {
        ACSDK_ERROR(LX("setRefreshTokenFailed").d("reason", "refreshTokenIsEmpty"));
        return false;
    }

    if (!m_database.clearTable(REFRESH_TOKEN_TABLE_NAME)) {
        ACSDK_ERROR(LX("setRefreshTokenFailed").d("reason", "clearTableFailed"));
        return false;
    }

    std::string sqlString = "INSERT INTO " REFRESH_TOKEN_TABLE_NAME " (" REFRESH_TOKEN_COLUMN_NAME ") VALUES (?);";
    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("setRefreshToken").d("reason", "createStatementFailed"));
        return false;
    }

    if (!statement->bindStringParameter(1, refreshToken)) {
        ACSDK_ERROR(LX("setRefreshToken").d("reason", "bindStringParameter"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("setRefreshToken").d("reason", "stepFailed"));
        return false;
    }

    return true;
}

bool SQLiteLWAAuthorizationStorage::clearRefreshToken() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_database.clearTable(REFRESH_TOKEN_TABLE_NAME)) {
        ACSDK_ERROR(LX("clearRefreshTokenFailed").d("reason", "clearTableFailed"));
        return false;
    }

    return true;
}

bool SQLiteLWAAuthorizationStorage::getRefreshToken(std::string* refreshToken) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!refreshToken) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "nullRefreshToken"));
        return false;
    }

    std::string sqlString = "SELECT * FROM " REFRESH_TOKEN_TABLE_NAME ";";
    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "createStatementFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "stepFailed"));
        return false;
    }

    if (statement->getStepResult() != SQLITE_ROW) {
        ACSDK_DEBUG0(LX("getRefreshTokenFailed").d("reason", "stepResultWasNotRow"));
        return false;
    }

    std::string columnName = statement->getColumnName(0);
    if (columnName != REFRESH_TOKEN_COLUMN_NAME) {
        ACSDK_ERROR(LX("getRefreshTokenFailed").d("reason", "unexpectedColumnName"));
        return false;
    }

    auto text = statement->getColumnText(0);
    *refreshToken = text;
    return true;
}

bool SQLiteLWAAuthorizationStorage::setUserId(const std::string& userId) {
    ACSDK_DEBUG5(LX(__func__).sensitive("userId", userId));

    std::lock_guard<std::mutex> lock(m_mutex);

    clearTableLocked(USER_ID_TABLE_NAME);

    std::string sqlString = "INSERT INTO " USER_ID_TABLE_NAME " (" USER_ID_COLUMN_NAME ") VALUES (?);";
    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("setUserIdFailed").d("reason", "createStatementFailed"));
        return false;
    }

    if (!statement->bindStringParameter(1, userId)) {
        ACSDK_ERROR(LX("setUserIdFailed").d("reason", "bindStringParameter"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("setUserIdFailed").d("reason", "stepFailed"));
        return false;
    }

    return true;
}

bool SQLiteLWAAuthorizationStorage::getUserId(std::string* userId) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!userId) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "nullRefreshToken"));
        return false;
    }

    std::string sqlString = "SELECT * FROM " USER_ID_TABLE_NAME ";";
    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "createStatementFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "stepFailed"));
        return false;
    }

    if (statement->getStepResult() != SQLITE_ROW) {
        ACSDK_DEBUG0(LX("getUserIdFailed").d("reason", "stepResultWasNotRow"));
        return false;
    }

    std::string columnName = statement->getColumnName(0);
    if (columnName != USER_ID_COLUMN_NAME) {
        ACSDK_ERROR(LX("getUserIdFailed").d("reason", "unexpectedColumnName").d("columnName", columnName));
        return false;
    }

    auto text = statement->getColumnText(0);
    *userId = text;
    return true;
}

bool SQLiteLWAAuthorizationStorage::clear() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    // Be careful of short circuiting here.
    auto success = clearTableLocked(REFRESH_TOKEN_TABLE_NAME);
    return clearTableLocked(USER_ID_TABLE_NAME) && success;
}

bool SQLiteLWAAuthorizationStorage::clearTableLocked(const std::string& tableName) {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_database.clearTable(tableName)) {
        ACSDK_ERROR(LX("clearUserIdFailed").d("reason", "clearTableFailed").d("table", tableName));
        return false;
    }

    return true;
}

void SQLiteLWAAuthorizationStorage::close() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    m_database.close();
}

SQLiteLWAAuthorizationStorage::SQLiteLWAAuthorizationStorage(const std::string& databaseFilePath) :
        m_database{databaseFilePath} {
}

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
