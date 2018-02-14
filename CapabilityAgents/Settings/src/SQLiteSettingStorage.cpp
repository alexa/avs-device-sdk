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

#include <SQLiteStorage/SQLiteStatement.h>
#include <SQLiteStorage/SQLiteUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "Settings/SQLiteSettingStorage.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace alexaClientSDK::storage::sqliteStorage;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteSettingsStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of the settings table.
static const std::string SETTINGS_TABLE_NAME = "settings";
/// The setting key.
static const std::string SETTING_KEY = "key";
/// The setting value.
static const std::string SETTING_VALUE = "value";
/// The SQL string to create the settings table.
static const std::string CREATE_SETTINGS_TABLE_SQL_STRING = std::string("CREATE TABLE ") + SETTINGS_TABLE_NAME + " (" +
                                                            SETTING_KEY + " TEXT PRIMARY KEY NOT NULL," +
                                                            SETTING_VALUE + " TEXT NOT NULL);";

SQLiteSettingStorage::SQLiteSettingStorage() : m_dbHandle{nullptr} {
}

/**
 * A small utility function to help determine if a file exists.
 *
 * @param filePath The path to the file being queried about.
 * @return Whether the file exists and is accessible.
 */
static bool fileExists(const std::string& filePath) {
    std::ifstream is(filePath);
    return is.good();
}

bool SQLiteSettingStorage::createDatabase(const std::string& filePath) {
    if (m_dbHandle) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "DatabaseHandleAlreadyOpen"));
        return false;
    }

    if (fileExists(filePath)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "FileAlreadyExists").d("FilePath", filePath));
        return false;
    }

    m_dbHandle = createSQLiteDatabase(filePath);
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "SQLiteCreateDatabaseFailed").d("file path", filePath));
        return false;
    }

    if (!performQuery(m_dbHandle, CREATE_SETTINGS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "PerformQueryFailed"));
        close();
        return false;
    }

    return true;
}

bool SQLiteSettingStorage::open(const std::string& filePath) {
    if (m_dbHandle) {
        ACSDK_ERROR(LX("openFailed").d("reason", "DatabaseHandleAlreadyOpen"));
        return false;
    }

    if (!fileExists(filePath)) {
        ACSDK_DEBUG(LX("openFailed").d("reason", "FileDoesNotExist").d("FilePath", filePath));
        return false;
    }

    m_dbHandle = openSQLiteDatabase(filePath);
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("openFailed").d("reason", "openSQLiteDatabaseFailed").d("FilePath", filePath));
        return false;
    }
    return true;
}

bool SQLiteSettingStorage::isOpen() {
    return (nullptr != m_dbHandle);
}

void SQLiteSettingStorage::close() {
    if (m_dbHandle) {
        closeSQLiteDatabase(m_dbHandle);
        m_dbHandle = nullptr;
    }
}

bool SQLiteSettingStorage::settingExists(const std::string& key) {
    std::string sqlString = "SELECT COUNT(*) FROM " + SETTINGS_TABLE_NAME + " WHERE " + SETTING_KEY + "=?;";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("settingExistsFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement.bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("settingExistsFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("settingExistsFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    const int RESULT_COLUMN_POSITION = 0;
    std::string rowValue = statement.getColumnText(RESULT_COLUMN_POSITION);

    int countValue = 0;
    if (!stringToInt(rowValue.c_str(), &countValue)) {
        ACSDK_ERROR(LX("settingExistsFailed").d("StringToIntFailed", rowValue));
        return false;
    }

    statement.finalize();
    return countValue > 0;
}

bool SQLiteSettingStorage::store(const std::string& key, const std::string& value) {
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "DatabaseHandleNotOpen"));
        return false;
    }

    if (value.empty()) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "SettingValueisEmpty"));
        return false;
    }

    if (settingExists(key)) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "SettingAlreadyExists").d("key", key));
        return false;
    }

    std::string sqlString = std::string("INSERT INTO " + SETTINGS_TABLE_NAME + " (") + SETTING_KEY + ", " +
                            SETTING_VALUE + ") VALUES (" + "?, ?" + ");";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement.bindStringParameter(boundParam++, key) || !statement.bindStringParameter(boundParam, value)) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    statement.finalize();
    return true;
}

bool SQLiteSettingStorage::load(std::unordered_map<std::string, std::string>* mapOfSettings) {
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "DatabaseHandleNotOpen"));
        return false;
    }

    std::string sqlString = "SELECT * FROM " + SETTINGS_TABLE_NAME + ";";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    std::string key;
    std::string value;

    if (!statement.step()) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    while (SQLITE_ROW == statement.getStepResult()) {
        int numberColumns = statement.getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement.getColumnName(i);

            if (SETTING_KEY == columnName) {
                key = statement.getColumnText(i);
            } else if (SETTING_VALUE == columnName) {
                value = statement.getColumnText(i);
            }
        }

        mapOfSettings->insert(make_pair(key, value));

        statement.step();
    }

    statement.finalize();
    return true;
}

bool SQLiteSettingStorage::modify(const std::string& key, const std::string& value) {
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "DatabaseHandleNotOpen"));
        return false;
    }

    if (value.empty()) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "SettingValueisEmpty"));
        return false;
    }

    if (!settingExists(key)) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "SettingDoesNotExistInDatabase").d("key", key));
        return false;
    }

    std::string sqlString =
        std::string("UPDATE " + SETTINGS_TABLE_NAME + " SET ") + "value=?" + "WHERE " + SETTING_KEY + "=?;";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement.bindStringParameter(boundParam++, value) || !statement.bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    statement.finalize();
    return true;
}

bool SQLiteSettingStorage::erase(const std::string& key) {
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "DatabaseHandleNotOpen"));
        return false;
    }

    if (key.empty()) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "SettingKeyEmpty"));
        return false;
    }

    if (!settingExists(key)) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "SettingDoesNotExistInDatabase").d("key", key));
        return false;
    }

    std::string sqlString = "DELETE FROM " + SETTINGS_TABLE_NAME + " WHERE " + SETTING_KEY + "=?;";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement.bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    statement.finalize();
    return true;
}

bool SQLiteSettingStorage::clearDatabase() {
    if (!clearTable(m_dbHandle, SETTINGS_TABLE_NAME)) {
        ACSDK_ERROR(LX("clearDatabaseFailed").d("reason", "SqliteClearTableFailed"));
        return false;
    }
    return true;
}

SQLiteSettingStorage::~SQLiteSettingStorage() {
    close();
}
}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
