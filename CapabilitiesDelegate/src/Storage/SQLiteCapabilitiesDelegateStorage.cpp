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

#include "CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace storage {

using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteCapabilitiesDelegateStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of settings for CapabilitiesDelegate
static const std::string CAPABILITIES_DELEGATE_CONFIGURATION_ROOT_KEY = "capabilitiesDelegate";

/// The key in our config file to find the database file path.
static const std::string DB_FILE_PATH = "databaseFilePath";

/// The name of the capabilitiesDelegate table.
static const std::string ENDPOINT_CONFIG_TABLE_NAME = "endpointConfigTable";
/// The name of the 'endpointid' field we will use as the primary key in our tables.
static const std::string DATABASE_COLUMN_ENDPOINT_ID_NAME = "endpointId";
/// The name of the 'endpointConfig' field we will.
static const std::string DATABASE_COLUMN_ENDPOINT_CONFIG_NAME = "endpointConfig";
/// The SQL string to create the alerts table.
static const std::string CREATE_ENDPOINT_CONFIG_TABLE_SQL_STRING =
    std::string("CREATE TABLE ") + ENDPOINT_CONFIG_TABLE_NAME + " (" + DATABASE_COLUMN_ENDPOINT_ID_NAME +
    " TEXT NOT NULL UNIQUE," + DATABASE_COLUMN_ENDPOINT_CONFIG_NAME + " TEXT NOT NULL);";

std::unique_ptr<SQLiteCapabilitiesDelegateStorage> SQLiteCapabilitiesDelegateStorage::create(
    const ConfigurationNode& configurationRoot) {
    ACSDK_DEBUG5(LX(__func__));

    auto capabilitiesStorageRoot = configurationRoot[CAPABILITIES_DELEGATE_CONFIGURATION_ROOT_KEY];
    if (!capabilitiesStorageRoot) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Could not load capabilitiesDelegate root"));
        return nullptr;
    }

    std::string dbFilePath;
    if (!capabilitiesStorageRoot.getString(DB_FILE_PATH, &dbFilePath) || dbFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Could not load database file path"));
        return nullptr;
    }

    return std::unique_ptr<SQLiteCapabilitiesDelegateStorage>(new SQLiteCapabilitiesDelegateStorage(dbFilePath));
}

SQLiteCapabilitiesDelegateStorage::SQLiteCapabilitiesDelegateStorage(const std::string& dbFilePath) :
        m_database{dbFilePath} {
}

SQLiteCapabilitiesDelegateStorage::~SQLiteCapabilitiesDelegateStorage() {
    close();
}

bool SQLiteCapabilitiesDelegateStorage::createDatabase() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_database.initialize()) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "unable to initialize Database"));
        return false;
    }

    if (!m_database.performQuery(CREATE_ENDPOINT_CONFIG_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "unable to create Endpoint Config table."));
        close();
        return false;
    }

    return true;
}
bool SQLiteCapabilitiesDelegateStorage::open() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_database.open();
}
void SQLiteCapabilitiesDelegateStorage::close() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_database.close();
}

bool SQLiteCapabilitiesDelegateStorage::storeLocked(const std::string& endpointId, const std::string& endpointConfig) {
    ACSDK_DEBUG5(LX(__func__));
    const std::string sqlString = "REPLACE INTO " + ENDPOINT_CONFIG_TABLE_NAME + " (" +
                                  DATABASE_COLUMN_ENDPOINT_ID_NAME + ", " + DATABASE_COLUMN_ENDPOINT_CONFIG_NAME +
                                  ") VALUES (?, ?);";

    auto statement = m_database.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX("storeFailed").m("Could not create statement"));
        return false;
    }

    int ENDPOINT_ID_INDEX = 1;
    int ENDPOINT_CONFIG_INDEX = 2;

    if (!statement->bindStringParameter(ENDPOINT_ID_INDEX, endpointId) ||
        !statement->bindStringParameter(ENDPOINT_CONFIG_INDEX, endpointConfig)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not bind parameter"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("storeFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

bool SQLiteCapabilitiesDelegateStorage::store(const std::string& endpointId, const std::string& endpointConfig) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    return storeLocked(endpointId, endpointConfig);
}

bool SQLiteCapabilitiesDelegateStorage::store(
    const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto it = endpointIdToConfigMap.begin(); it != endpointIdToConfigMap.end(); ++it) {
        if (!storeLocked(it->first, it->second)) {
            ACSDK_ERROR(LX("storeFailed").m("Could not store endpointConfigMap"));
            return false;
        }
    }

    return true;
}

bool SQLiteCapabilitiesDelegateStorage::load(std::unordered_map<std::string, std::string>* endpointConfigMap) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!endpointConfigMap || !endpointConfigMap->empty()) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "Invalid endpointConfigMap"));
        return false;
    }

    const std::string sqlString = "SELECT * FROM " + ENDPOINT_CONFIG_TABLE_NAME + ";";

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadFailed").m("Could not create statement."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadFailed").m("Could not perform step."));
        return false;
    }

    std::string endpointId;
    std::string endpointConfig;
    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        for (int i = 0; i < numberColumns; ++i) {
            std::string columnName = statement->getColumnName(i);

            if (DATABASE_COLUMN_ENDPOINT_ID_NAME == columnName) {
                endpointId = statement->getColumnText(i);
            } else if (DATABASE_COLUMN_ENDPOINT_CONFIG_NAME == columnName) {
                endpointConfig = statement->getColumnText(i);
            }
        }

        endpointConfigMap->insert({endpointId, endpointConfig});

        statement->step();
    }

    return true;
}

bool SQLiteCapabilitiesDelegateStorage::load(const std::string& endpointId, std::string* endpointConfig) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock{m_mutex};
    if (!endpointConfig) {
        ACSDK_ERROR(LX("loadFailed").d("reason", "Invalid endpointConfig"));
        return false;
    }

    const std::string sqlString =
        "SELECT * FROM " + ENDPOINT_CONFIG_TABLE_NAME + " WHERE " + DATABASE_COLUMN_ENDPOINT_ID_NAME + "=?;";

    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadFailed").m("Could not create statement."));
        return false;
    }

    int ENDPOINT_ID_INDEX = 1;
    if (!statement->bindStringParameter(ENDPOINT_ID_INDEX, endpointId)) {
        ACSDK_ERROR(LX("loadFailed").m("Could not bind endpointId"));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadFailed").m("Could not perform step."));
        return false;
    }

    if (SQLITE_ROW == statement->getStepResult()) {
        *endpointConfig = statement->getColumnText(1);
    }

    return true;
}

bool SQLiteCapabilitiesDelegateStorage::eraseLocked(const std::string& endpointId) {
    ACSDK_DEBUG5(LX(__func__));
    const std::string sqlString =
        "DELETE FROM " + ENDPOINT_CONFIG_TABLE_NAME + " WHERE " + DATABASE_COLUMN_ENDPOINT_ID_NAME + "=?;";
    auto statement = m_database.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseFailed").m("Could not create statement."));
        return false;
    }

    int ENDPOINT_ID_INDEX = 1;
    if (!statement->bindStringParameter(ENDPOINT_ID_INDEX, endpointId)) {
        ACSDK_DEBUG5(LX("eraseFailed").m("Could not bind endpointId."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

bool SQLiteCapabilitiesDelegateStorage::erase(const std::string& endpointId) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    return eraseLocked(endpointId);
}

bool SQLiteCapabilitiesDelegateStorage::erase(
    const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto endpointIdToConfig : endpointIdToConfigMap) {
        if (!eraseLocked(endpointIdToConfig.first)) {
            return false;
        }
    }

    return true;
}

bool SQLiteCapabilitiesDelegateStorage::clearDatabase() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_database.clearTable(ENDPOINT_CONFIG_TABLE_NAME);
}

}  // namespace storage
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
