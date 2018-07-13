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

/// The key in our config file to find the root of settings.
static const std::string SETTINGS_CONFIGURATION_ROOT_KEY = "settings";
/// The key in our config file to find the database file path.
static const std::string SETTINGS_DB_FILE_PATH_KEY = "databaseFilePath";

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

std::unique_ptr<SQLiteSettingStorage> SQLiteSettingStorage::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot) {
    auto settingsConfigurationRoot = configurationRoot[SETTINGS_CONFIGURATION_ROOT_KEY];
    if (!settingsConfigurationRoot) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "Could not load config for the Settings capability agent")
                        .d("key", SETTINGS_CONFIGURATION_ROOT_KEY));
        return nullptr;
    }

    std::string settingDbFilePath;
    if (!settingsConfigurationRoot.getString(SETTINGS_DB_FILE_PATH_KEY, &settingDbFilePath) ||
        settingDbFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Could not load config value").d("key", SETTINGS_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return std::unique_ptr<SQLiteSettingStorage>(new SQLiteSettingStorage(settingDbFilePath));
}

SQLiteSettingStorage::SQLiteSettingStorage(const std::string& databaseFilePath) : m_database{databaseFilePath} {
}

bool SQLiteSettingStorage::createDatabase() {
    if (!m_database.initialize()) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "SQLiteCreateDatabaseFailed"));
        return false;
    }

    if (!m_database.performQuery(CREATE_SETTINGS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "PerformQueryFailed"));
        close();
        return false;
    }

    return true;
}

bool SQLiteSettingStorage::open() {
    return m_database.open();
}

void SQLiteSettingStorage::close() {
    m_database.close();
}

bool SQLiteSettingStorage::settingExists(const std::string& key) {
    std::string sqlString = "SELECT COUNT(*) FROM " + SETTINGS_TABLE_NAME + " WHERE " + SETTING_KEY + "=?;";

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("settingExistsFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("settingExistsFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("settingExistsFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    const int RESULT_COLUMN_POSITION = 0;
    std::string rowValue = statement->getColumnText(RESULT_COLUMN_POSITION);

    int countValue = 0;
    if (!stringToInt(rowValue.c_str(), &countValue)) {
        ACSDK_ERROR(LX("settingExistsFailed").d("StringToIntFailed", rowValue));
        return false;
    }

    statement->finalize();
    return countValue > 0;
}

bool SQLiteSettingStorage::store(const std::string& key, const std::string& value) {
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

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam++, key) || !statement->bindStringParameter(boundParam, value)) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("storeFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    statement->finalize();
    return true;
}

bool SQLiteSettingStorage::load(std::unordered_map<std::string, std::string>* mapOfSettings) {
    std::string sqlString = "SELECT * FROM " + SETTINGS_TABLE_NAME + ";";

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    std::string key;
    std::string value;

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement->getColumnName(i);

            if (SETTING_KEY == columnName) {
                key = statement->getColumnText(i);
            } else if (SETTING_VALUE == columnName) {
                value = statement->getColumnText(i);
            }
        }

        mapOfSettings->insert(make_pair(key, value));

        statement->step();
    }

    statement->finalize();
    return true;
}

bool SQLiteSettingStorage::modify(const std::string& key, const std::string& value) {
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

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam++, value) || !statement->bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("modifyFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    statement->finalize();
    return true;
}

bool SQLiteSettingStorage::erase(const std::string& key) {
    if (key.empty()) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "SettingKeyEmpty"));
        return false;
    }

    if (!settingExists(key)) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "SettingDoesNotExistInDatabase").d("key", key));
        return false;
    }

    std::string sqlString = "DELETE FROM " + SETTINGS_TABLE_NAME + " WHERE " + SETTING_KEY + "=?;";

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "SQliteStatementInvalid"));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "BindParameterFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseFailed").d("reason", "StepToRowFailed"));
        return false;
    }

    statement->finalize();
    return true;
}

bool SQLiteSettingStorage::clearDatabase() {
    if (!m_database.clearTable(SETTINGS_TABLE_NAME)) {
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
