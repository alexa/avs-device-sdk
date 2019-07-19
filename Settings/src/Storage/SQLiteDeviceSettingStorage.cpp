/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace settings {
namespace storage {

using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteDeviceSettingStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of settings for this database.
static const std::string DEVICE_SETTING_DATABASE_CONFIGURATION_ROOT_KEY = "deviceSettings";
/// The key in our config file to find the database file path.
static const std::string DEVICE_SETTING_DATABASE_DB_FILE_PATH_KEY = "databaseFilePath";
/// Component and table name separator in DB table name.
static const std::string DEVICE_SETTING_DATABASE_DB_COMPONENT_TABLE_NAMES_SEPARATOR = "_";

/// The name of the settings table.
static const std::string DEVICE_SETTINGS_TABLE_NAME = "deviceSettings";

/// The name of the column to store the settings key.
static const std::string DEVICE_SETTINGS_KEY_COLUMN_NAME = "key";

/// The name of the column to store the settings value.
static const std::string DEVICE_SETTINGS_VALUE_COLUMN_NAME = "value";

/// The name of the column to store the settings status.
static const std::string DEVICE_SETTINGS_STATUS_COLUMN_NAME = "status";

/// The SQL string to create the alert configurations table.
// clang-format off
 static const std::string CREATE_DEVICE_SETTINGS_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
         DEVICE_SETTINGS_TABLE_NAME + " (" + 
         DEVICE_SETTINGS_KEY_COLUMN_NAME  + " TEXT PRIMARY KEY NOT NULL," +
         DEVICE_SETTINGS_VALUE_COLUMN_NAME+ " TEXT NOT NULL," +
         DEVICE_SETTINGS_STATUS_COLUMN_NAME + " TEXT NOT NULL);";
// clang-format on

std::unique_ptr<SQLiteDeviceSettingStorage> SQLiteDeviceSettingStorage::create(
    const ConfigurationNode& configurationRoot) {
    ACSDK_DEBUG5(LX(__func__));

    auto deviceSettingDatabaseConfigurationRoot = configurationRoot[DEVICE_SETTING_DATABASE_CONFIGURATION_ROOT_KEY];
    if (!deviceSettingDatabaseConfigurationRoot) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "Could not load config for deviceSetting database")
                        .d("key", DEVICE_SETTING_DATABASE_CONFIGURATION_ROOT_KEY));
        return nullptr;
    }

    std::string deviceSettingDbFilePath;
    if (!deviceSettingDatabaseConfigurationRoot.getString(
            DEVICE_SETTING_DATABASE_DB_FILE_PATH_KEY, &deviceSettingDbFilePath) ||
        deviceSettingDbFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "Could not load config value")
                        .d("key", DEVICE_SETTING_DATABASE_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return std::unique_ptr<SQLiteDeviceSettingStorage>(new SQLiteDeviceSettingStorage(deviceSettingDbFilePath));
}

SQLiteDeviceSettingStorage::SQLiteDeviceSettingStorage(const std::string& dbFilePath) : m_db{dbFilePath} {
}

SQLiteDeviceSettingStorage::~SQLiteDeviceSettingStorage() {
    close();
}

bool SQLiteDeviceSettingStorage::open() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_db.isDatabaseReady()) {
        // Already open
        return true;
    }

    if (!m_db.open()) {
        // The database probably is not initialized.
        if (!m_db.initialize()) {
            ACSDK_ERROR(LX("openFailed"));
            return false;
        }
    }

    // At this point, database is open.
    if (!m_db.tableExists(DEVICE_SETTINGS_TABLE_NAME) && !createSettingsTable()) {
        ACSDK_ERROR(LX("openFailed").m("Cannot create " + DEVICE_SETTINGS_TABLE_NAME + " table"));
        close();
        return false;
    }

    return true;
}

void SQLiteDeviceSettingStorage::close() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    m_db.close();
}

bool SQLiteDeviceSettingStorage::storeSetting(const std::string& key, const std::string& value, SettingStatus status) {
    ACSDK_DEBUG5(LX(__func__).d("key", key).d("value", value).d("status", settingStatusToString(status)));

    std::lock_guard<std::mutex> lock(m_mutex);

    // clang-format off
     const std::string sqlString = "REPLACE INTO " + DEVICE_SETTINGS_TABLE_NAME + " (" +
                            DEVICE_SETTINGS_KEY_COLUMN_NAME + ", " 
                            + DEVICE_SETTINGS_VALUE_COLUMN_NAME + ", " 
                            + DEVICE_SETTINGS_STATUS_COLUMN_NAME 
                            + ") VALUES (" 
                            + "'" + key + "', '" + value + "', '" 
                            + settingStatusToString(status) + "' );";
    // clang-format on

    auto statement = m_db.createStatement(sqlString);

    if (!statement->step()) {
        ACSDK_ERROR(LX("storeSettingFailed").d("reason", "stepFailed"));
        return false;
    }

    return true;
}

DeviceSettingStorageInterface::SettingStatusAndValue SQLiteDeviceSettingStorage::loadSetting(const std::string& key) {
    ACSDK_DEBUG5(LX(__func__).d("key", key));

    std::lock_guard<std::mutex> lock(m_mutex);

    const int VALUE_COLUMN_INDEX = 0;
    const int STATUS_COLUMN_INDEX = 1;

    if (!m_db.isDatabaseReady()) {
        const std::string error = "Database not ready";
        ACSDK_ERROR(LX("loadSettingFailed").d("reason", error));
        return std::make_pair(SettingStatus::NOT_AVAILABLE, error);
    }

    // clang-format off
     const std::string sqlString = "SELECT " + DEVICE_SETTINGS_VALUE_COLUMN_NAME 
                    + "," + DEVICE_SETTINGS_STATUS_COLUMN_NAME 
                    + " FROM " + DEVICE_SETTINGS_TABLE_NAME 
                    + " WHERE " + DEVICE_SETTINGS_KEY_COLUMN_NAME + "=?;";
    // clang-format on

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        const std::string error = "Can not create SQL Statement.";
        ACSDK_ERROR(LX("loadSettingFailed").d("reason", error).d("sql", sqlString));
        return std::make_pair(SettingStatus::NOT_AVAILABLE, error);
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, key)) {
        const std::string error = "Binding key to SQL statement failed.";
        ACSDK_ERROR(LX("loadSettingFailed").d("reason", error));
        return std::make_pair(SettingStatus::NOT_AVAILABLE, error);
    }

    if (!statement->step()) {
        const std::string error = "SQL statement execution failed.";
        ACSDK_ERROR(LX("loadSettingFailed").d("reason", error).d("sql", sqlString));
        return std::make_pair(SettingStatus::NOT_AVAILABLE, error);
    }

    if (statement->getStepResult() != SQLITE_ROW) {
        const std::string error = "Retrieving row from database failed.";
        ACSDK_ERROR(LX("loadSettingFailed").d("reason", error).d("sql", sqlString));
        return std::make_pair(SettingStatus::NOT_AVAILABLE, error);
    }

    std::string value = statement->getColumnText(VALUE_COLUMN_INDEX);
    SettingStatus status = stringToSettingStatus(statement->getColumnText(STATUS_COLUMN_INDEX));

    ACSDK_DEBUG5(LX("loadSetting").d("value", value));

    return std::make_pair(status, value);
}

bool SQLiteDeviceSettingStorage::deleteSetting(const std::string& key) {
    ACSDK_DEBUG5(LX(__func__).d("key", key));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_db.isDatabaseReady()) {
        ACSDK_ERROR(LX("deleteSettingFailed").d("reason", "Database not ready"));
        return false;
    }

    const std::string sqlString =
        "DELETE FROM " + DEVICE_SETTINGS_TABLE_NAME + " WHERE " + DEVICE_SETTINGS_KEY_COLUMN_NAME + "=?;";

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("deleteSettingFailed").d("reason", "createStatementFailed"));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, key)) {
        ACSDK_ERROR(LX("deleteSettingFailed").d("reason", "bindIntParameterFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("deleteSettingFailed").d("reason", "stepFailed"));
        return false;
    }

    return true;
}

bool SQLiteDeviceSettingStorage::createSettingsTable() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_db.performQuery(CREATE_DEVICE_SETTINGS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createSettingsTableFailed").m("Table could not be created."));
        return false;
    }

    return true;
}

bool SQLiteDeviceSettingStorage::updateSettingStatus(const std::string& key, SettingStatus status) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_db.isDatabaseReady()) {
        ACSDK_ERROR(LX("deleteSettingFailed").d("reason", "Database not ready"));
        return false;
    }

    // clang-format off
     const std::string sqlString = "UPDATE " + DEVICE_SETTINGS_TABLE_NAME +
                            " SET " + DEVICE_SETTINGS_STATUS_COLUMN_NAME +"=\"" + settingStatusToString(status)+ "\""
                             " WHERE " + DEVICE_SETTINGS_KEY_COLUMN_NAME + " = \"" + key + "\"";

    // clang-format on
    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX("updateSettingStatusFailed").d("reason", "createStatementFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("updateSettingStatusFailed").d("reason", "stepFailed").d("sql", sqlString));
        return false;
    }

    return true;
}

}  // namespace storage
}  // namespace settings
}  // namespace alexaClientSDK