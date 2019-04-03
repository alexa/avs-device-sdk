/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace avsCommon::utils::file;
using namespace alexaClientSDK::storage::sqliteStorage;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteCBLAuthDelegateStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of @c ConfigurationNode for CBLAuthDelegate
static const std::string CONFIG_KEY_CBL_AUTH_DELEGATE = "cblAuthDelegate";

/// Name of @c databaseFilePath value in in CBLAuthDelegate's @c ConfigurationNode.
static const std::string CONFIG_KEY_DB_FILE_PATH_KEY = "databaseFilePath";

/// The name of the refreshToken table.
#define REFRESH_TOKEN_TABLE_NAME "refreshToken"

/// The name of the refreshToken column.
#define REFRESH_TOKEN_COLUMN_NAME "refreshToken"

/// String for creating the refreshToken table
static const std::string CREATE_REFRESH_TOKEN_TABLE_SQL_STRING =
    "CREATE TABLE " REFRESH_TOKEN_TABLE_NAME " (" REFRESH_TOKEN_COLUMN_NAME " TEXT);";

std::unique_ptr<SQLiteCBLAuthDelegateStorage> SQLiteCBLAuthDelegateStorage::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot) {
    auto cblAuthDelegateConfigurationRoot = configurationRoot[CONFIG_KEY_CBL_AUTH_DELEGATE];
    if (!cblAuthDelegateConfigurationRoot) {
        ACSDK_ERROR(LX("createFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_CBL_AUTH_DELEGATE));
        return nullptr;
    }

    std::string databaseFilePath;
    if (!cblAuthDelegateConfigurationRoot.getString(CONFIG_KEY_DB_FILE_PATH_KEY, &databaseFilePath) ||
        databaseFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "missingConfigurationValue").d("key", CONFIG_KEY_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return std::unique_ptr<SQLiteCBLAuthDelegateStorage>(new SQLiteCBLAuthDelegateStorage(databaseFilePath));
}

SQLiteCBLAuthDelegateStorage::~SQLiteCBLAuthDelegateStorage() {
    ACSDK_DEBUG5(LX("~SQLiteCBLAuthDelegateStorage"));

    close();
}

bool SQLiteCBLAuthDelegateStorage::createDatabase() {
    ACSDK_DEBUG5(LX("createDatabase"));

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

    return true;
}

bool SQLiteCBLAuthDelegateStorage::open() {
    ACSDK_DEBUG5(LX("open"));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_database.open()) {
        ACSDK_DEBUG0(LX("openFailed").d("reason", "openSQLiteDatabaseFailed"));
        return false;
    }

    if (!m_database.tableExists(REFRESH_TOKEN_TABLE_NAME)) {
        ACSDK_ERROR(LX("openFailed").d("reason", "missingTable").d("name", REFRESH_TOKEN_TABLE_NAME));
        return false;
    }

    return true;
}

bool SQLiteCBLAuthDelegateStorage::setRefreshToken(const std::string& refreshToken) {
    ACSDK_DEBUG5(LX("setRefreshToken"));

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

bool SQLiteCBLAuthDelegateStorage::clearRefreshToken() {
    ACSDK_DEBUG5(LX("clearRefreshToken"));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_database.clearTable(REFRESH_TOKEN_TABLE_NAME)) {
        ACSDK_ERROR(LX("clearRefreshTokenFailed").d("reason", "clearTableFailed"));
        return false;
    }

    return true;
}

bool SQLiteCBLAuthDelegateStorage::getRefreshToken(std::string* refreshToken) {
    ACSDK_DEBUG5(LX("getRefreshToken"));

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

bool SQLiteCBLAuthDelegateStorage::clear() {
    ACSDK_DEBUG5(LX("clear"));

    return clearRefreshToken();
}

void SQLiteCBLAuthDelegateStorage::close() {
    ACSDK_DEBUG5(LX("close"));

    std::lock_guard<std::mutex> lock(m_mutex);

    m_database.close();
}

SQLiteCBLAuthDelegateStorage::SQLiteCBLAuthDelegateStorage(const std::string& databaseFilePath) :
        m_database{databaseFilePath} {
}

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK
