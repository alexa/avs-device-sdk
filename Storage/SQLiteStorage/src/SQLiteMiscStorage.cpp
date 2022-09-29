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

#include <SQLiteStorage/SQLiteMiscStorage.h>

#include <algorithm>
#include <cctype>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <SQLiteStorage/SQLiteStatement.h>
#include <SQLiteStorage/SQLiteUtils.h>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
#define TAG "SQLiteMiscStorage"

/// Names of the columns in the database
static const std::string KEY_COLUMN_NAME = "key";
static const std::string VALUE_COLUMN_NAME = "value";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of settings for this database.
static const std::string MISC_DATABASE_CONFIGURATION_ROOT_KEY = "miscDatabase";
/// The key in our config file to find the database file path.
static const std::string MISC_DATABASE_DB_FILE_PATH_KEY = "databaseFilePath";
/// Component and table name separator in DB table name.
static const std::string MISC_DATABASE_DB_COMPONENT_TABLE_NAMES_SEPARATOR = "_";

/// String key/value type
static const std::string STRING_KEY_VALUE_TYPE = "STRING";
/// Unknown key/value type
static const std::string UNKNOWN_KEY_VALUE_TYPE = "UNKNOWN";

/// DB type
static const std::string TEXT_DB_TYPE = "TEXT";

/// Boolean to check if table exists
static const bool CHECK_TABLE_EXISTS = true;
/// Boolean to check if table doesn't exist
static const bool CHECK_TABLE_NOT_EXISTS = false;

/**
 * Helper method that will check basic things about the DB.
 *
 * @param db The SQLiteDatabase handle for MiscDB
 * @param componentName The component name.
 * @param tableName The table name to check.
 * @return an error message if the checks fail, else a blank string
 */
static std::string basicDBChecksLocked(
    SQLiteDatabase& db,
    const std::string& componentName,
    const std::string& tableName);

/**
 * Helper method that will check basic things about the DB.
 *
 * @param db The SQLiteDatabase handle for MiscDB
 * @param componentName The component name.
 * @param tableName The table name to check.
 * @param tableShouldExist If true, checks if the table should exist. If false, it checks the opposite.
 * @return an error message if the checks fail, else a blank string
 */
static std::string basicDBChecksLocked(
    SQLiteDatabase& db,
    const std::string& componentName,
    const std::string& tableName,
    bool tableShouldExist);

/**
 * Helper method that will get the table name as it is in the DB.
 * @param componentName The component name.
 * @param tableName The table name to check
 *
 * @return the table name as it is in the DB, else a blank string
 */
static std::string getDBTableName(const std::string& componentName, const std::string& tableName);

/**
 * Gets the key type as a string
 *
 * @param keyType The key type enum.
 * @return the key type as a string
 */
static std::string getKeyTypeString(SQLiteMiscStorage::KeyType keyType);

/**
 * Gets the value type as a string.
 *
 * @param valueType The value type enum.
 * @return the value type as a string
 */
static std::string getValueTypeString(SQLiteMiscStorage::ValueType valueType);

/**
 * Gets the DB type given a key/value type.
 *
 * @param keyValueType The key/value type as a string.
 * @return the DB type
 */
static std::string getDBDataType(const std::string& keyValueType);

std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> SQLiteMiscStorage::createMiscStorageInterface(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot) {
    if (!configurationRoot) {
        ACSDK_ERROR(LX("createMiscStorageInterfaceFailed").d("reason", "nullConfigurationRoot"));
        return nullptr;
    }
    return create(*configurationRoot);
}

std::unique_ptr<SQLiteMiscStorage> SQLiteMiscStorage::create(const ConfigurationNode& configurationRoot) {
    auto miscDatabaseConfigurationRoot = configurationRoot[MISC_DATABASE_CONFIGURATION_ROOT_KEY];
    if (!miscDatabaseConfigurationRoot) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "Could not load config for misc database")
                        .d("key", MISC_DATABASE_CONFIGURATION_ROOT_KEY));
        return nullptr;
    }

    std::string miscDbFilePath;
    if (!miscDatabaseConfigurationRoot.getString(MISC_DATABASE_DB_FILE_PATH_KEY, &miscDbFilePath) ||
        miscDbFilePath.empty()) {
        ACSDK_ERROR(
            LX("createFailed").d("reason", "Could not load config value").d("key", MISC_DATABASE_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return create(miscDbFilePath);
}

std::unique_ptr<SQLiteMiscStorage> SQLiteMiscStorage::create(const std::string& databasePath) {
    return std::unique_ptr<SQLiteMiscStorage>(new SQLiteMiscStorage(databasePath));
}

SQLiteMiscStorage::SQLiteMiscStorage(const std::string& dbFilePath) : m_db{dbFilePath} {
}

SQLiteMiscStorage::~SQLiteMiscStorage() {
    close();
}

bool SQLiteMiscStorage::open() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return openLocked();
}

bool SQLiteMiscStorage::openLocked() {
    if (!m_db.open()) {
        ACSDK_DEBUG0(LX("openDatabaseFailed"));
        return false;
    }
    return true;
}

void SQLiteMiscStorage::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return closeLocked();
}

void SQLiteMiscStorage::closeLocked() {
    m_db.close();
}

bool SQLiteMiscStorage::createDatabase() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return createDatabaseLocked();
}

bool SQLiteMiscStorage::createDatabaseLocked() {
    if (!m_db.initialize()) {
        ACSDK_ERROR(LX("createDatabaseFailed"));
        return false;
    }
    return true;
}

std::string getDBTableName(const std::string& componentName, const std::string& tableName) {
    if (componentName.empty() || tableName.empty()) {
        std::string emptyParam;
        if (componentName.empty() && tableName.empty()) {
            emptyParam = "Component and table";
        } else {
            if (componentName.empty()) {
                emptyParam = "Component";
            } else {
                emptyParam = "Table";
            }
        }
        ACSDK_ERROR(LX("getDBTableNameError").d("reason", emptyParam + " name can't be empty."));
        return "";
    }

    return (componentName + MISC_DATABASE_DB_COMPONENT_TABLE_NAMES_SEPARATOR + tableName);
}

std::string basicDBChecksLocked(SQLiteDatabase& db, const std::string& componentName, const std::string& tableName) {
    if (!db.isDatabaseReady()) {
        return "Database is not ready";
    }

    if (componentName.empty()) {
        return "Component name is empty";
    }

    if (tableName.empty()) {
        return "Table name is empty";
    }

    return "";
}

std::string basicDBChecksLocked(
    SQLiteDatabase& db,
    const std::string& componentName,
    const std::string& tableName,
    bool tableShouldExist) {
    const std::string errorReason = basicDBChecksLocked(db, componentName, tableName);
    if (!errorReason.empty()) {
        return errorReason;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);
    bool tableExists = db.tableExists(dbTableName);
    if (tableShouldExist && !tableExists) {
        return "Table does not exist";
    }
    if (!tableShouldExist && tableExists) {
        return "Table already exists";
    }

    return "";
}

std::string getKeyTypeString(SQLiteMiscStorage::KeyType keyType) {
    switch (keyType) {
        case SQLiteMiscStorage::KeyType::STRING_KEY:
            return STRING_KEY_VALUE_TYPE;
        case SQLiteMiscStorage::KeyType::UNKNOWN_KEY:
            return UNKNOWN_KEY_VALUE_TYPE;
    }

    return UNKNOWN_KEY_VALUE_TYPE;
}

std::string getValueTypeString(SQLiteMiscStorage::ValueType valueType) {
    switch (valueType) {
        case SQLiteMiscStorage::ValueType::STRING_VALUE:
            return STRING_KEY_VALUE_TYPE;
        case SQLiteMiscStorage::ValueType::UNKNOWN_VALUE:
            return UNKNOWN_KEY_VALUE_TYPE;
    }

    return UNKNOWN_KEY_VALUE_TYPE;
}

std::string getDBDataType(const std::string& keyValueType) {
    if (keyValueType == STRING_KEY_VALUE_TYPE) {
        return TEXT_DB_TYPE;
    }

    return "";
}

bool SQLiteMiscStorage::getKeyValueTypesLocked(
    const std::string& componentName,
    const std::string& tableName,
    KeyType* keyType,
    ValueType* valueType) {
    const std::string errorEvent = "getKeyValueTypesFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    if (!keyType || !valueType) {
        ACSDK_ERROR(LX(errorEvent).m("Key/value pointers are null"));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string sqlString = "PRAGMA table_info(" + dbTableName + ");";

    auto sqlStatement = m_db.createStatement(sqlString);

    if ((!sqlStatement) || (!sqlStatement->step())) {
        ACSDK_ERROR(LX(errorEvent).d("Could not get metadata of table", tableName));
        return false;
    }

    const std::string tableInfoColumnName = "name";
    const std::string tableInfoColumnType = "type";

    std::string columnName, columnType;

    while (SQLITE_ROW == sqlStatement->getStepResult()) {
        int numberColumns = sqlStatement->getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string tableInfoColumn = sqlStatement->getColumnName(i);

            if (tableInfoColumnName == tableInfoColumn) {
                columnName = sqlStatement->getColumnText(i);

            } else if (tableInfoColumnType == tableInfoColumn) {
                columnType = sqlStatement->getColumnText(i);
                std::transform(columnType.begin(), columnType.end(), columnType.begin(), ::toupper);
            }
        }

        if (!(columnName.empty()) && !(columnType.empty())) {
            if (KEY_COLUMN_NAME == columnName) {
                if (TEXT_DB_TYPE == columnType) {
                    *keyType = KeyType::STRING_KEY;
                } else {
                    *keyType = KeyType::UNKNOWN_KEY;
                }
            } else if (VALUE_COLUMN_NAME == columnName) {
                if (TEXT_DB_TYPE == columnType) {
                    *valueType = ValueType::STRING_VALUE;
                } else {
                    *valueType = ValueType::UNKNOWN_VALUE;
                }
            }
        }

        columnName.clear();
        columnType.clear();

        sqlStatement->step();
    }

    return true;
}

std::string SQLiteMiscStorage::checkKeyTypeLocked(
    const std::string& componentName,
    const std::string& tableName,
    KeyType keyType) {
    if (keyType == KeyType::UNKNOWN_KEY) {
        return "Cannot check for unknown key column type";
    }

    const std::string basicDBChecksError = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);
    if (!basicDBChecksError.empty()) {
        return basicDBChecksError;
    }

    KeyType keyColumnType;
    ValueType valueColumnType;

    if (!getKeyValueTypesLocked(componentName, tableName, &keyColumnType, &valueColumnType)) {
        return "Unable to get key column type";
    }

    if (keyColumnType == KeyType::UNKNOWN_KEY) {
        return "Unknown key column type";
    }

    if (keyColumnType != keyType) {
        return "Unexpected key column type";
    }

    return "";
}

std::string SQLiteMiscStorage::checkValueTypeLocked(
    const std::string& componentName,
    const std::string& tableName,
    ValueType valueType) {
    if (valueType == ValueType::UNKNOWN_VALUE) {
        return "Cannot check for unknown value column type";
    }

    const std::string basicDBChecksError = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);
    if (!basicDBChecksError.empty()) {
        return basicDBChecksError;
    }

    KeyType keyColumnType;
    ValueType valueColumnType;

    if (!getKeyValueTypesLocked(componentName, tableName, &keyColumnType, &valueColumnType)) {
        return "Unable to get value column type";
    }

    if (valueColumnType == ValueType::UNKNOWN_VALUE) {
        return "Unknown value column type";
    }

    if (valueColumnType != valueType) {
        return "Unexpected value column type";
    }

    return "";
}

std::string SQLiteMiscStorage::checkKeyValueTypeLocked(
    const std::string& componentName,
    const std::string& tableName,
    KeyType keyType,
    ValueType valueType) {
    if (keyType == KeyType::UNKNOWN_KEY) {
        return "Cannot check for unknown key column type";
    }
    if (valueType == ValueType::UNKNOWN_VALUE) {
        return "Cannot check for unknown value column type";
    }

    const std::string basicDBChecksError = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);
    if (!basicDBChecksError.empty()) {
        return basicDBChecksError;
    }

    KeyType keyColumnType;
    ValueType valueColumnType;

    if (!getKeyValueTypesLocked(componentName, tableName, &keyColumnType, &valueColumnType)) {
        return "Unable to get key/value column types";
    }

    if (keyColumnType == KeyType::UNKNOWN_KEY) {
        return "Unknown key column type";
    }
    if (valueColumnType == ValueType::UNKNOWN_VALUE) {
        return "Unknown value column type";
    }

    if (keyColumnType != keyType) {
        return "Unexpected key column type";
    }
    if (valueColumnType != valueType) {
        return "Unexpected value column type";
    }

    return "";
}

bool SQLiteMiscStorage::createTable(
    const std::string& componentName,
    const std::string& tableName,
    KeyType keyType,
    ValueType valueType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return createTableLocked(componentName, tableName, keyType, valueType);
}

bool SQLiteMiscStorage::createTableLocked(
    const std::string& componentName,
    const std::string& tableName,
    KeyType keyType,
    ValueType valueType) {
    const std::string errorEvent = "createTableFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_NOT_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    if (keyType == KeyType::UNKNOWN_KEY) {
        ACSDK_ERROR(LX(errorEvent).m("Unknown key type"));
        return false;
    }

    if (valueType == ValueType::UNKNOWN_VALUE) {
        ACSDK_ERROR(LX(errorEvent).m("Unknown value type"));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string sqlString = "CREATE TABLE " + dbTableName + " (" + KEY_COLUMN_NAME + " " +
                                  getDBDataType(getKeyTypeString(keyType)) + " PRIMARY KEY NOT NULL," +
                                  VALUE_COLUMN_NAME + " " + getDBDataType(getValueTypeString(valueType)) +
                                  " NOT NULL);";

    if (!m_db.performQuery(sqlString)) {
        ACSDK_ERROR(LX(errorEvent).d("Could not create table", tableName));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::clearTable(const std::string& componentName, const std::string& tableName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return clearTableLocked(componentName, tableName);
}

bool SQLiteMiscStorage::clearTableLocked(const std::string& componentName, const std::string& tableName) {
    const std::string errorEvent = "clearTableFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    if (!m_db.clearTable(dbTableName)) {
        ACSDK_ERROR(LX(errorEvent).d("Could not clear table", tableName));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::deleteTable(const std::string& componentName, const std::string& tableName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return deleteTableLocked(componentName, tableName);
}

bool SQLiteMiscStorage::deleteTableLocked(const std::string& componentName, const std::string& tableName) {
    const std::string errorEvent = "deleteTableFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    int numOfTableEntries = 0;
    if (!getNumberTableRows(&m_db, dbTableName, &numOfTableEntries)) {
        ACSDK_ERROR(LX(errorEvent).m("Failed to count rows in table"));
        return false;
    }

    if (numOfTableEntries > 0) {
        ACSDK_ERROR(LX(errorEvent).m("Unable to delete table that is not empty"));
        return false;
    }

    if (!m_db.dropTable(dbTableName)) {
        ACSDK_ERROR(LX(errorEvent).d("Could not delete table", tableName));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::get(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    std::string* value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getLocked(componentName, tableName, key, value);
}

bool SQLiteMiscStorage::getLocked(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    std::string* value) {
    const std::string errorEvent = "getFromTableFailed";

    if (!value) {
        ACSDK_ERROR(LX(errorEvent).m("Value is nullptr."));
        return false;
    }

    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);
    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string keyTypeError = checkKeyTypeLocked(componentName, tableName, KeyType::STRING_KEY);
    if (!keyTypeError.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(keyTypeError));
        return false;
    }

    const std::string sqlString = "SELECT value FROM " + dbTableName + " WHERE " + KEY_COLUMN_NAME + "=?;";
    const int keyIndex = 1;

    auto sqliteStatement = m_db.createStatement(sqlString);
    if (!sqliteStatement) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Create statement failed."));
        return false;
    }

    if (!sqliteStatement->bindStringParameter(keyIndex, key)) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Bind parameter failed."));
        return false;
    }

    if (!sqliteStatement->step()) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Step failed."));
        return false;
    }

    if (SQLITE_ROW == sqliteStatement->getStepResult()) {
        const int RESULT_COLUMN_POSITION = 0;
        *value = sqliteStatement->getColumnText(RESULT_COLUMN_POSITION);
    }

    return true;
}

bool SQLiteMiscStorage::tableEntryExists(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    bool* tableEntryExistsValue) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return tableEntryExistsLocked(componentName, tableName, key, tableEntryExistsValue);
}

bool SQLiteMiscStorage::tableEntryExistsLocked(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    bool* tableEntryExistsValue) {
    const std::string errorEvent = "tableEntryExistsFailed";

    if (!tableEntryExistsValue) {
        ACSDK_ERROR(LX(errorEvent).m("tableEntryExistsValue is nullptr."));
        return false;
    }

    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);
    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    KeyType keyColumnType;
    ValueType valueColumnType;
    if (!getKeyValueTypesLocked(componentName, tableName, &keyColumnType, &valueColumnType)) {
        ACSDK_ERROR(LX(errorEvent).m("Unable to get key/value column types"));
        return false;
    }
    if (keyColumnType != KeyType::STRING_KEY) {
        ACSDK_ERROR(LX(errorEvent).m("Unexpected key column types"));
        return false;
    }

    if (valueColumnType == ValueType::STRING_VALUE) {
        std::string tableEntry;
        if (!getLocked(componentName, tableName, key, &tableEntry)) {
            ACSDK_ERROR(LX(errorEvent).m("Unable to get table entry"));
            return false;
        }
        *tableEntryExistsValue = !tableEntry.empty();
        return true;
    }

    return false;
}

bool SQLiteMiscStorage::tableExists(
    const std::string& componentName,
    const std::string& tableName,
    bool* tableExistsValue) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return tableExistsLocked(componentName, tableName, tableExistsValue);
}

bool SQLiteMiscStorage::tableExistsLocked(
    const std::string& componentName,
    const std::string& tableName,
    bool* tableExistsValue) {
    const std::string errorEvent = "tableExistsFailed";
    if (!tableExistsValue) {
        ACSDK_ERROR(LX(errorEvent).m("tableExistsValue is nullptr."));
        return false;
    }

    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName);
    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);
    *tableExistsValue = m_db.tableExists(dbTableName);
    return true;
}

bool SQLiteMiscStorage::add(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return addLocked(componentName, tableName, key, value);
}

bool SQLiteMiscStorage::addLocked(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    const std::string errorEvent = "addToTableFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    const std::string keyValueTypeError =
        checkKeyValueTypeLocked(componentName, tableName, KeyType::STRING_KEY, ValueType::STRING_VALUE);
    if (!keyValueTypeError.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(keyValueTypeError));
        return false;
    }

    bool tableEntryExistsValue;
    if (!tableEntryExistsLocked(componentName, tableName, key, &tableEntryExistsValue)) {
        ACSDK_ERROR(LX(errorEvent).d("Unable to get table entry information for " + key + " in table", tableName));
        return false;
    }
    if (tableEntryExistsValue) {
        ACSDK_ERROR(LX(errorEvent).d("An entry already exists for " + key + " in table", tableName));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string sqlString =
        "INSERT INTO " + dbTableName + " (" + KEY_COLUMN_NAME + ", " + VALUE_COLUMN_NAME + ") VALUES (?, ?);";

    const int keyIndex = 1;
    const int valueIndex = 2;

    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Create statement failed."));
        return false;
    }

    if (!statement->bindStringParameter(keyIndex, key) || !statement->bindStringParameter(valueIndex, value)) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Bind parameter failed."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Step failed."));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::update(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return updateLocked(componentName, tableName, key, value);
}

bool SQLiteMiscStorage::updateLocked(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    const std::string errorEvent = "updateTableEntryFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    const std::string keyValueTypeError =
        checkKeyValueTypeLocked(componentName, tableName, KeyType::STRING_KEY, ValueType::STRING_VALUE);
    if (!keyValueTypeError.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(keyValueTypeError));
        return false;
    }

    bool tableEntryExistsValue;
    if (!tableEntryExistsLocked(componentName, tableName, key, &tableEntryExistsValue)) {
        ACSDK_ERROR(LX(errorEvent).d("Unable to get table entry information for " + key + " in table", tableName));
        return false;
    }
    if (!tableEntryExistsValue) {
        ACSDK_ERROR(LX(errorEvent).d("An entry does not exist for " + key + " in table", tableName));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string sqlString =
        "UPDATE " + dbTableName + " SET " + VALUE_COLUMN_NAME + "=? WHERE " + KEY_COLUMN_NAME + "=?;";

    const int valueIndex = 1;
    const int keyIndex = 2;

    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Create statement failed."));
        return false;
    }

    if (!statement->bindStringParameter(keyIndex, key) || !statement->bindStringParameter(valueIndex, value)) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Bind parameter failed."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Step failed."));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::put(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return putLocked(componentName, tableName, key, value);
}

bool SQLiteMiscStorage::putLocked(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    const std::string errorEvent = "putToTableFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    const std::string keyValueTypeError =
        checkKeyValueTypeLocked(componentName, tableName, KeyType::STRING_KEY, ValueType::STRING_VALUE);
    if (!keyValueTypeError.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(keyValueTypeError));
        return false;
    }

    bool tableEntryExistsValue;
    if (!tableEntryExistsLocked(componentName, tableName, key, &tableEntryExistsValue)) {
        ACSDK_ERROR(LX(errorEvent).d("Unable to get table entry information for " + key + " in table", tableName));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    std::string sqlString;
    std::string errorValue;

    if (!tableEntryExistsValue) {
        sqlString =
            "INSERT INTO " + dbTableName + " (" + VALUE_COLUMN_NAME + ", " + KEY_COLUMN_NAME + ") VALUES (?, ?);";
        errorValue = "Could not add entry (" + key + ", " + value + ") to table";
    } else {
        sqlString = "UPDATE " + dbTableName + " SET " + VALUE_COLUMN_NAME + "=? WHERE " + KEY_COLUMN_NAME + "=?;";
        errorValue = "Could not update entry for " + key + " in table";
    }

    const int valueIndex = 1;
    const int keyIndex = 2;

    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Create statement failed."));
        return false;
    }

    if (!statement->bindStringParameter(keyIndex, key) || !statement->bindStringParameter(valueIndex, value)) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Bind parameter failed."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Step failed (" + errorValue + ")."));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::remove(const std::string& componentName, const std::string& tableName, const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return removeLocked(componentName, tableName, key);
}

bool SQLiteMiscStorage::removeLocked(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key) {
    const std::string errorEvent = "removeTableEntryFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    const std::string keyTypeError = checkKeyTypeLocked(componentName, tableName, KeyType::STRING_KEY);
    if (!keyTypeError.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(keyTypeError));
        return false;
    }

    bool tableEntryExistsValue;
    if (!tableEntryExistsLocked(componentName, tableName, key, &tableEntryExistsValue)) {
        ACSDK_ERROR(LX(errorEvent).d("Unable to get table entry information for " + key + " in table", tableName));
        return false;
    }
    if (!tableEntryExistsValue) {
        ACSDK_ERROR(LX(errorEvent).d("An entry does not exist for " + key + " in table", tableName));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string sqlString = "DELETE FROM " + dbTableName + " WHERE " + KEY_COLUMN_NAME + "=?;";

    const int keyIndex = 1;

    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Create statement failed."));
        return false;
    }

    if (!statement->bindStringParameter(keyIndex, key)) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Bind parameter failed."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX(errorEvent).d("reason", "Step failed."));
        return false;
    }

    return true;
}

bool SQLiteMiscStorage::load(
    const std::string& componentName,
    const std::string& tableName,
    std::unordered_map<std::string, std::string>* valueContainer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return loadLocked(componentName, tableName, valueContainer);
}

bool SQLiteMiscStorage::loadLocked(
    const std::string& componentName,
    const std::string& tableName,
    std::unordered_map<std::string, std::string>* valueContainer) {
    const std::string errorEvent = "loadFromTableFailed";
    const std::string errorReason = basicDBChecksLocked(m_db, componentName, tableName, CHECK_TABLE_EXISTS);

    if (!errorReason.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(errorReason));
        return false;
    }

    if (!valueContainer) {
        ACSDK_ERROR(LX(errorEvent).m("Value container is nullptr."));
        return false;
    }

    const std::string keyValueTypeError =
        checkKeyValueTypeLocked(componentName, tableName, KeyType::STRING_KEY, ValueType::STRING_VALUE);
    if (!keyValueTypeError.empty()) {
        ACSDK_ERROR(LX(errorEvent).m(keyValueTypeError));
        return false;
    }

    std::string dbTableName = getDBTableName(componentName, tableName);

    const std::string sqlString = "SELECT * FROM " + dbTableName + ";";

    auto sqlStatement = m_db.createStatement(sqlString);

    if ((!sqlStatement) || (!sqlStatement->step())) {
        ACSDK_ERROR(LX(errorEvent).d("Could not load entries from table", tableName));
        return false;
    }

    std::string key, value;

    while (SQLITE_ROW == sqlStatement->getStepResult()) {
        int numberColumns = sqlStatement->getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = sqlStatement->getColumnName(i);

            if (KEY_COLUMN_NAME == columnName) {
                key = sqlStatement->getColumnText(i);
            } else if (VALUE_COLUMN_NAME == columnName) {
                value = sqlStatement->getColumnText(i);
            }
        }

        if (!(key.empty()) && !(value.empty())) {
            valueContainer->insert(std::make_pair(key, value));
        }

        key.clear();
        value.clear();

        sqlStatement->step();
    }

    return true;
}

bool SQLiteMiscStorage::isOpened() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return isOpenedLocked();
}

bool SQLiteMiscStorage::isOpenedLocked() {
    return m_db.isDatabaseReady();
}

SQLiteDatabase& SQLiteMiscStorage::getDatabase() {
    return m_db;
}

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK
