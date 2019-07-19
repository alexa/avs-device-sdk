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

#include "Bluetooth/SQLiteBluetoothStorage.h"
#include "Bluetooth/BluetoothStorageInterface.h"
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
static const std::string TAG{"SQLiteBluetoothStorage"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {

using namespace alexaClientSDK::storage::sqliteStorage;

/// Configuration root.
static const std::string BLUETOOTH_CONFIGURATION_ROOT_KEY = "bluetooth";

/// The node identifying the database.
static const std::string BLUETOOTH_DB_FILE_PATH_KEY = "databaseFilePath";

/// Table name.
static const std::string UUID_TABLE_NAME = "uuidMapping";

/// The UUID column.
static const std::string COLUMN_UUID = "uuid";

/// The MAC address column.
static const std::string COLUMN_MAC = "mac";

std::unique_ptr<SQLiteBluetoothStorage> SQLiteBluetoothStorage::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot) {
    ACSDK_DEBUG5(LX(__func__));
    auto bluetoothConfigurationRoot = configurationRoot[BLUETOOTH_CONFIGURATION_ROOT_KEY];
    if (!bluetoothConfigurationRoot) {
        ACSDK_ERROR(LX(__func__).d("reason", "loadConfigFailed").d("key", BLUETOOTH_CONFIGURATION_ROOT_KEY));
        return nullptr;
    }

    std::string filePath;
    if (!bluetoothConfigurationRoot.getString(BLUETOOTH_DB_FILE_PATH_KEY, &filePath) || filePath.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "retrieveValueFailed").d("key", BLUETOOTH_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return std::unique_ptr<SQLiteBluetoothStorage>(new SQLiteBluetoothStorage(filePath));
}

bool SQLiteBluetoothStorage::createDatabase() {
    ACSDK_DEBUG5(LX(__func__));

    // clang-format off
    const std::string sqlString = "CREATE TABLE " + UUID_TABLE_NAME + "(" +
        COLUMN_UUID + " text not null unique, " + COLUMN_MAC + " text not null unique);";
    // clang-format on

    std::lock_guard<std::mutex> lock(m_databaseMutex);
    if (!m_db.initialize()) {
        ACSDK_ERROR(LX(__func__).d("reason", "initializeDBFailed"));
        return false;
    }

    if (!m_db.performQuery(sqlString)) {
        ACSDK_ERROR(LX(__func__).d("reason", "createTableFailed"));
        close();
        return false;
    }

    return true;
}

bool SQLiteBluetoothStorage::open() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_databaseMutex);

    return m_db.open();
}

void SQLiteBluetoothStorage::close() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_databaseMutex);

    return m_db.close();
}

bool SQLiteBluetoothStorage::clear() {
    ACSDK_DEBUG5(LX(__func__));

    const std::string sqlString = "DELETE FROM " + UUID_TABLE_NAME + ";";

    std::lock_guard<std::mutex> lock(m_databaseMutex);
    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX(__func__).d("reason", "createStatementFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX(__func__).d("reason", "stepFailed"));
        return false;
    }

    return true;
}

bool SQLiteBluetoothStorage::getSingleRowLocked(
    std::unique_ptr<storage::sqliteStorage::SQLiteStatement>& statement,
    std::unordered_map<std::string, std::string>* row) {
    ACSDK_DEBUG5(LX(__func__));
    if (!statement) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullStatement"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX(__func__).d("reason", "stepFailed"));
        return false;
    }

    // Not necessarily an error as there may just be no row.
    if (SQLITE_ROW != statement->getStepResult()) {
        ACSDK_INFO(LX(__func__).d("reason", "getStepResultFailed"));
        return false;
    }

    for (int i = 0; i < statement->getColumnCount(); i++) {
        row->insert({statement->getColumnName(i), statement->getColumnText(i)});
    }

    return true;
}

bool SQLiteBluetoothStorage::getUuid(const std::string& mac, std::string* uuid) {
    ACSDK_DEBUG5(LX(__func__));
    if (!uuid) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullUuid"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_databaseMutex);
    return getAssociatedDataLocked(COLUMN_MAC, mac, COLUMN_UUID, uuid);
}

bool SQLiteBluetoothStorage::getMac(const std::string& uuid, std::string* mac) {
    ACSDK_DEBUG5(LX(__func__));
    if (!mac) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMac"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_databaseMutex);
    return getAssociatedDataLocked(COLUMN_UUID, uuid, COLUMN_MAC, mac);
}

bool SQLiteBluetoothStorage::getAssociatedDataLocked(
    const std::string& constraintKey,
    const std::string& constraintVal,
    const std::string& resultKey,
    std::string* resultVal) {
    ACSDK_DEBUG5(LX(__func__));
    if (!resultVal) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullResult"));
        return false;
    }

    const std::string sqlString =
        "SELECT " + resultKey + " FROM " + UUID_TABLE_NAME + " WHERE " + constraintKey + " IS ?;";
    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX(__func__).d("reason", "createStatementFailed"));
        return false;
    }

    const int VALUE_INDEX = 1;

    if (!statement->bindStringParameter(VALUE_INDEX, constraintVal)) {
        ACSDK_ERROR(LX(__func__).d("reason", "bindFailed"));
        return false;
    }

    std::unordered_map<std::string, std::string> row;

    if (getSingleRowLocked(statement, &row) && row.count(resultKey) > 0) {
        *resultVal = row.at(resultKey);
        return true;
    }

    return false;
}

bool SQLiteBluetoothStorage::getMappingsLocked(
    const std::string& keyPreference,
    std::unordered_map<std::string, std::string>* mappings) {
    ACSDK_DEBUG5(LX(__func__).d("keyPreference", keyPreference));

    if (!mappings) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMappings"));
        return false;
    }

    if (COLUMN_UUID != keyPreference && COLUMN_MAC != keyPreference) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidKeyPreference").d("keyPreference", keyPreference));

        return false;
    }

    const std::string sqlString = "SELECT * FROM " + UUID_TABLE_NAME + ";";
    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX(__func__).d("reason", "createStatementFailed"));
        return false;
    }

    std::unordered_map<std::string, std::string> row;
    while (getSingleRowLocked(statement, &row)) {
        if (0 == row.count(COLUMN_MAC) || 0 == row.count(COLUMN_UUID)) {
            ACSDK_ERROR(LX(__func__)
                            .d("reason", "missingData")
                            .d("macPresent", row.count(COLUMN_MAC))
                            .d("uuidPresent", row.count(COLUMN_UUID)));
            continue;
        }

        if (COLUMN_UUID == keyPreference) {
            mappings->insert({row.at(COLUMN_UUID), row.at(COLUMN_MAC)});
        } else if (COLUMN_MAC == keyPreference) {
            mappings->insert({row.at(COLUMN_MAC), row.at(COLUMN_UUID)});
        } else {
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedData").d("keyPreference", keyPreference));
        }
        row.clear();
    }

    return true;
}

bool SQLiteBluetoothStorage::getMacToUuid(std::unordered_map<std::string, std::string>* macToUuid) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_databaseMutex);
    return getMappingsLocked(COLUMN_MAC, macToUuid);
}

bool SQLiteBluetoothStorage::getUuidToMac(std::unordered_map<std::string, std::string>* uuidToMac) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_databaseMutex);
    return getMappingsLocked(COLUMN_UUID, uuidToMac);
}

bool SQLiteBluetoothStorage::getOrderedMac(bool ascending, std::list<std::string>* macs) {
    ACSDK_DEBUG5(LX(__func__));

    if (!macs) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMacs"));
        return false;
    }

    const std::string order = ascending ? "ASC" : "DESC";
    const std::string sqlString = "SELECT * FROM " + UUID_TABLE_NAME + " ORDER BY rowid " + order + ";";

    std::lock_guard<std::mutex> lock(m_databaseMutex);

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX(__func__).d("reason", "createStatementFailed"));
        return false;
    }

    std::unordered_map<std::string, std::string> row;
    while (getSingleRowLocked(statement, &row)) {
        if (row.count(COLUMN_MAC) > 0 && !row.at(COLUMN_MAC).empty()) {
            macs->push_back(row.at(COLUMN_MAC));
        }
        row.clear();
    }

    return true;
};

bool SQLiteBluetoothStorage::insertByMac(const std::string& mac, const std::string& uuid, bool overwrite) {
    ACSDK_DEBUG5(LX(__func__));
    const std::string operation = overwrite ? "REPLACE" : "INSERT";

    // clang-format off
    const std::string sqlString = operation + " INTO " + UUID_TABLE_NAME +
            " (" + COLUMN_UUID + "," + COLUMN_MAC + ") VALUES (?,?);";
    // clang-format on

    std::lock_guard<std::mutex> lock(m_databaseMutex);

    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX(__func__).d("reason", "createStatementFailed"));
        return false;
    }

    const int UUID_INDEX = 1;
    const int MAC_INDEX = 2;

    if (!statement->bindStringParameter(UUID_INDEX, uuid) || !statement->bindStringParameter(MAC_INDEX, mac)) {
        ACSDK_ERROR(LX(__func__).d("reason", "bindParameterFailed"));
        return false;
    }

    // This could be due to a mac or uuid already existing in the db.
    if (!statement->step()) {
        ACSDK_ERROR(LX(__func__).d("reason", "stepFailed"));
        return false;
    }

    return true;
}

bool SQLiteBluetoothStorage::remove(const std::string& mac) {
    ACSDK_DEBUG5(LX(__func__));
    const std::string sqlString = "DELETE FROM " + UUID_TABLE_NAME + " WHERE " + COLUMN_MAC + "=?;";

    std::lock_guard<std::mutex> lock(m_databaseMutex);

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("removeFailed").d("reason", "createStatementFailed"));
        return false;
    }

    const int MAC_INDEX = 1;

    if (!statement->bindStringParameter(MAC_INDEX, mac)) {
        ACSDK_ERROR(LX("removeFailed").d("reason", "bindFailed"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("removeFailed").d("reason", "stepFailed"));
        return false;
    }

    return true;
}

SQLiteBluetoothStorage::SQLiteBluetoothStorage(const std::string& filePath) : m_db{filePath} {
}

}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
