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

#include "Alerts/Storage/SQLiteAlertStorage.h"
#include "Alerts/Alarm.h"
#include "Alerts/Reminder.h"
#include "Alerts/Timer.h"

#include <SQLiteStorage/SQLiteStatement.h>
#include <SQLiteStorage/SQLiteUtils.h>

#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include <fstream>
#include <set>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace storage {

using namespace avsCommon::utils::file;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace alexaClientSDK::storage::sqliteStorage;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteAlertStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of settings for this Capability Agent.
static const std::string ALERTS_CAPABILITY_AGENT_CONFIGURATION_ROOT_KEY = "alertsCapabilityAgent";

/// The key in our config file to find the database file path.
static const std::string ALERTS_CAPABILITY_AGENT_DB_FILE_PATH_KEY = "databaseFilePath";

/// A definition which we will store in the database to indicate Alarm type.
static const int ALERT_EVENT_TYPE_ALARM = 1;
/// A definition which we will store in the database to indicate Timer type.
static const int ALERT_EVENT_TYPE_TIMER = 2;
/// A definition which we will store in the database to indicate Reminder type.
static const int ALERT_EVENT_TYPE_REMINDER = 3;

/// This is the string value this code will expect form an Alert of Alarm type.
static const std::string ALERT_EVENT_TYPE_ALARM_STRING = "ALARM";
/// This is the string value this code will expect form an Alert of Timer type.
static const std::string ALERT_EVENT_TYPE_TIMER_STRING = "TIMER";
/// This is the string value this code will expect form an Alert of Reminder type.
static const std::string ALERT_EVENT_TYPE_REMINDER_STRING = "REMINDER";

/// A definition which we will store in the database to indicate an Alert's Unset state.
static const int ALERT_STATE_UNSET = 1;
/// A definition which we will store in the database to indicate an Alert's Set state.
static const int ALERT_STATE_SET = 2;
/// A definition which we will store in the database to indicate an Alert's Activating state.
static const int ALERT_STATE_ACTIVATING = 3;
/// A definition which we will store in the database to indicate an Alert's Active state.
static const int ALERT_STATE_ACTIVE = 4;
/// A definition which we will store in the database to indicate an Alert's Snoozing state.
static const int ALERT_STATE_SNOOZING = 5;
/// A definition which we will store in the database to indicate an Alert's Snoozed state.
static const int ALERT_STATE_SNOOZED = 6;
/// A definition which we will store in the database to indicate an Alert's Stopping state.
static const int ALERT_STATE_STOPPING = 7;
/// A definition which we will store in the database to indicate an Alert's Stopped state.
static const int ALERT_STATE_STOPPED = 8;
/// A definition which we will store in the database to indicate an Alert's Completed state.
static const int ALERT_STATE_COMPLETED = 9;
/// A definition which we will store in the database to indicate an Alert's Ready state.
static const int ALERT_STATE_READY = 10;

/// The name of the 'id' field we will use as the primary key in our tables.
static const std::string DATABASE_COLUMN_ID_NAME = "id";

/// A symbolic name for version one of our database.
static const int ALERTS_DATABASE_VERSION_ONE = 1;
/// A symbolic name for version two of our database.
static const int ALERTS_DATABASE_VERSION_TWO = 2;

/// The name of the alerts table.
static const std::string ALERTS_TABLE_NAME = "alerts";

/// The name of the alerts (v2) table.
static const std::string ALERTS_V2_TABLE_NAME = "alerts_v2";
/// The SQL string to create the alerts table.
// clang-format off
static const std::string CREATE_ALERTS_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        ALERTS_V2_TABLE_NAME + " (" +
        DATABASE_COLUMN_ID_NAME + " INT PRIMARY KEY NOT NULL," +
        "token TEXT NOT NULL," +
        "type INT NOT NULL," +
        "state INT NOT NULL," +
        "scheduled_time_unix INT NOT NULL," +
        "scheduled_time_iso_8601 TEXT NOT NULL," +
        "asset_loop_count INT NOT NULL," +
        "asset_loop_pause_milliseconds INT NOT NULL," +
        "background_asset TEXT NOT NULL);";
// clang-format on

/// The name of the alertAssets table.
static const std::string ALERT_ASSETS_TABLE_NAME = "alertAssets";
/// The SQL string to create the alertAssets table.
// clang-format off
static const std::string CREATE_ALERT_ASSETS_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
                                                                ALERT_ASSETS_TABLE_NAME + " (" +
                                                                "id INT PRIMARY KEY NOT NULL," +
                                                                "alert_id INT NOT NULL," +
                                                                "avs_id TEXT NOT NULL," +
                                                                "url TEXT NOT NULL);";
// clang-format on

/// The name of the alertAssetPlayOrderItems table.
static const std::string ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME = "alertAssetPlayOrderItems";
/// The SQL string to create the alertAssetPlayOrderItems table.
// clang-format off
static const std::string CREATE_ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME + " (" +
        "id INT PRIMARY KEY NOT NULL," +
        "alert_id INT NOT NULL," +
        "asset_play_order_position INT NOT NULL," +
        "asset_play_order_token TEXT NOT NULL);";
// clang-format on

struct AssetOrderItem {
    int index;
    std::string name;
};

struct AssetOrderItemCompare {
    bool operator()(const AssetOrderItem& lhs, const AssetOrderItem& rhs) const {
        return lhs.index < rhs.index;
    }
};

/**
 * Utility function to convert an alert type string into a value we can store in the database.
 *
 * @param alertType A string reflecting the type of an alert.
 * @param[out] dbType The destination of the converted type which may be stored in a database.
 * @return Whether the type was successfully converted.
 */
static bool alertTypeToDbField(const std::string& alertType, int* dbType) {
    if (ALERT_EVENT_TYPE_ALARM_STRING == alertType) {
        *dbType = ALERT_EVENT_TYPE_ALARM;
    } else if (ALERT_EVENT_TYPE_TIMER_STRING == alertType) {
        *dbType = ALERT_EVENT_TYPE_TIMER;
    } else if (ALERT_EVENT_TYPE_REMINDER_STRING == alertType) {
        *dbType = ALERT_EVENT_TYPE_REMINDER;
    } else {
        ACSDK_ERROR(
            LX("alertTypeToDbFieldFailed").m("Could not determine alert type.").d("alert type string", alertType));
        return false;
    }

    return true;
}

/**
 * Utility function to convert an alert state into a value we can store in the database.
 *
 * @param state The state of an alert.
 * @param[out] dbState The destination of the converted state which may be stored in a database.
 * @return Whether the state was successfully converted.
 */
static bool alertStateToDbField(Alert::State state, int* dbState) {
    switch (state) {
        case Alert::State::UNSET:
            *dbState = ALERT_STATE_UNSET;
            return true;
        case Alert::State::SET:
            *dbState = ALERT_STATE_SET;
            return true;
        case Alert::State::READY:
            *dbState = ALERT_STATE_READY;
            return true;
        case Alert::State::ACTIVATING:
            *dbState = ALERT_STATE_ACTIVATING;
            return true;
        case Alert::State::ACTIVE:
            *dbState = ALERT_STATE_ACTIVE;
            return true;
        case Alert::State::SNOOZING:
            *dbState = ALERT_STATE_SNOOZING;
            return true;
        case Alert::State::SNOOZED:
            *dbState = ALERT_STATE_SNOOZED;
            return true;
        case Alert::State::STOPPING:
            *dbState = ALERT_STATE_STOPPING;
            return true;
        case Alert::State::STOPPED:
            *dbState = ALERT_STATE_STOPPED;
            return true;
        case Alert::State::COMPLETED:
            *dbState = ALERT_STATE_COMPLETED;
            return true;
    }

    ACSDK_ERROR(LX("alertStateToDbFieldFailed")
                    .m("Could not convert alert state.")
                    .d("alert object state", static_cast<int>(state)));
    return false;
}

/**
 * Utility function to convert a database value for an alert state into its @c Alert equivalent.
 *
 * @param dbState The state of an alert, as stored in the database.
 * @param[out] dbState The destination of the converted state.
 * @return Whether the database value was successfully converted.
 */
static bool dbFieldToAlertState(int dbState, Alert::State* state) {
    switch (dbState) {
        case ALERT_STATE_UNSET:
            *state = Alert::State::UNSET;
            return true;
        case ALERT_STATE_SET:
            *state = Alert::State::SET;
            return true;
        case ALERT_STATE_READY:
            *state = Alert::State::READY;
            return true;
        case ALERT_STATE_ACTIVATING:
            *state = Alert::State::ACTIVATING;
            return true;
        case ALERT_STATE_ACTIVE:
            *state = Alert::State::ACTIVE;
            return true;
        case ALERT_STATE_SNOOZING:
            *state = Alert::State::SNOOZING;
            return true;
        case ALERT_STATE_SNOOZED:
            *state = Alert::State::SNOOZED;
            return true;
        case ALERT_STATE_STOPPING:
            *state = Alert::State::STOPPING;
            return true;
        case ALERT_STATE_STOPPED:
            *state = Alert::State::STOPPED;
            return true;
        case ALERT_STATE_COMPLETED:
            *state = Alert::State::COMPLETED;
            return true;
    }

    ACSDK_ERROR(LX("dbFieldToAlertStateFailed").m("Could not convert db value.").d("db value", dbState));
    return false;
}

std::unique_ptr<SQLiteAlertStorage> SQLiteAlertStorage::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory) {
    auto alertsConfigurationRoot = configurationRoot[ALERTS_CAPABILITY_AGENT_CONFIGURATION_ROOT_KEY];
    if (!alertsConfigurationRoot) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "Could not load config for the Alerts capability agent")
                        .d("key", ALERTS_CAPABILITY_AGENT_CONFIGURATION_ROOT_KEY));
        return nullptr;
    }

    std::string alertDbFilePath;
    if (!alertsConfigurationRoot.getString(ALERTS_CAPABILITY_AGENT_DB_FILE_PATH_KEY, &alertDbFilePath) ||
        alertDbFilePath.empty()) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "Could not load config value")
                        .d("key", ALERTS_CAPABILITY_AGENT_DB_FILE_PATH_KEY));
        return nullptr;
    }

    return std::unique_ptr<SQLiteAlertStorage>(new SQLiteAlertStorage(alertDbFilePath, alertsAudioFactory));
}

SQLiteAlertStorage::SQLiteAlertStorage(
    const std::string& dbFilePath,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory) :
        m_alertsAudioFactory{alertsAudioFactory},
        m_db{dbFilePath} {
}

SQLiteAlertStorage::~SQLiteAlertStorage() {
    close();
}

/**
 * Utility function to create the Alerts table within the database.
 *
 * @param db The SQLiteDatabase object.
 * @return Whether the table was successfully created.
 */
static bool createAlertsTable(SQLiteDatabase* db) {
    if (!db) {
        ACSDK_ERROR(LX("createAlertsTableFailed").m("null db"));
        return false;
    }

    if (!db->performQuery(CREATE_ALERTS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createAlertsTableFailed").m("Table could not be created."));
        return false;
    }

    return true;
}

/**
 * Utility function to create the AlertAssets table within the database.
 *
 * @param db The SQLiteDatabase object.
 * @return Whether the table was successfully created.
 */
static bool createAlertAssetsTable(SQLiteDatabase* db) {
    if (!db) {
        ACSDK_ERROR(LX("createAlertAssetsTableFailed").m("null db"));
        return false;
    }

    if (!db->performQuery(CREATE_ALERT_ASSETS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createAlertAssetsTableFailed").m("Table could not be created."));
        return false;
    }

    return true;
}

/**
 * Utility function to create the AlertAssetPlayOrderItems table within the database.
 *
 * @param db The SQLiteDatabase object.
 * @return Whether the table was successfully created.
 */
static bool createAlertAssetPlayOrderItemsTable(SQLiteDatabase* db) {
    if (!db) {
        ACSDK_ERROR(LX("createAlertAssetPlayOrderItemsTableFailed").m("null db"));
        return false;
    }

    if (!db->performQuery(CREATE_ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createAlertAssetPlayOrderItemsTableFailed").m("Table could not be created."));
        return false;
    }

    return true;
}

bool SQLiteAlertStorage::createDatabase() {
    if (!m_db.initialize()) {
        ACSDK_ERROR(LX("createDatabaseFailed"));
        return false;
    }

    if (!createAlertsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("Alerts table could not be created."));
        close();
        return false;
    }

    if (!createAlertAssetsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("AlertAssets table could not be created."));
        close();
        return false;
    }

    if (!createAlertAssetPlayOrderItemsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("AlertAssetPlayOrderItems table could not be created."));
        close();
        return false;
    }

    return true;
}

bool SQLiteAlertStorage::migrateAlertsDbFromV1ToV2() {
    // The good case - the db file is already up to date.
    if (m_db.tableExists(ALERTS_V2_TABLE_NAME)) {
        return true;
    }

    if (!createAlertsTable(&m_db)) {
        ACSDK_ERROR(LX("migrateAlertsDbFromV1ToV2Failed").m("Alert table could not be created."));
        return false;
    }

    if (!m_db.tableExists(ALERT_ASSETS_TABLE_NAME)) {
        if (!createAlertAssetsTable(&m_db)) {
            ACSDK_ERROR(LX("migrateAlertsDbFromV1ToV2Failed").m("AlertAssets table could not be created."));
            return false;
        }
    }

    if (!m_db.tableExists(ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME)) {
        if (!createAlertAssetPlayOrderItemsTable(&m_db)) {
            ACSDK_ERROR(
                LX("migrateAlertsDbFromV1ToV2Failed").m("AlertAssetPlayOrderItems table could not be created."));
            return false;
        }
    }

    // We have created the new alerts tables, and the expectation is that the old alerts table exists.
    // Just for an edge case where it does not, let's not fail out if the old table does not exist - in that case,
    // the migration is fine.
    if (m_db.tableExists(ALERTS_TABLE_NAME)) {
        std::vector<std::shared_ptr<Alert>> alertContainer;
        if (!loadHelper(ALERTS_DATABASE_VERSION_ONE, &alertContainer)) {
            ACSDK_ERROR(LX("migrateAlertsDbFromV1ToV2Failed").m("Could not load V1 alert records."));
            return false;
        }

        for (auto& alert : alertContainer) {
            if (!store(alert)) {
                ACSDK_ERROR(LX("migrateAlertsDbFromV1ToV2Failed").m("Could not migrate alert to V2 database."));
                alert->printDiagnostic();
                return false;
            }
        }

        const std::string sqlString = "DROP TABLE IF EXISTS " + ALERTS_TABLE_NAME + ";";

        if (!m_db.performQuery(sqlString)) {
            ACSDK_ERROR(LX("migrateAlertsDbFromV1ToV2Failed").m("Alerts table could not be dropped."));
            return false;
        }
    }

    return true;
}

bool SQLiteAlertStorage::open() {
    return m_db.open();
}

void SQLiteAlertStorage::close() {
    m_db.close();
}

bool SQLiteAlertStorage::alertExists(const std::string& token) {
    const std::string sqlString = "SELECT COUNT(*) FROM " + ALERTS_V2_TABLE_NAME + " WHERE token=?;";
    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, token)) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Could not step to next row."));
        return false;
    }

    const int RESULT_COLUMN_POSITION = 0;
    std::string rowValue = statement->getColumnText(RESULT_COLUMN_POSITION);

    int countValue = 0;
    if (!stringToInt(rowValue.c_str(), &countValue)) {
        ACSDK_ERROR(LX("alertExistsFailed").d("Could not convert string to integer", rowValue));
        return false;
    }

    return countValue > 0;
}

static bool storeAlertAssets(
    SQLiteDatabase* db,
    int alertId,
    const std::unordered_map<std::string, Alert::Asset>& assets) {
    if (assets.empty()) {
        return true;
    }

    // clang-format off
    const std::string sqlString = "INSERT INTO " + ALERT_ASSETS_TABLE_NAME + " (" +
                            "id, alert_id, avs_id, url" +
                            ") VALUES (" +
                            "?, ?, ?, ?" +
                            ");";
    // clang-format on

    int id = 0;
    if (!getTableMaxIntValue(db, ALERT_ASSETS_TABLE_NAME, DATABASE_COLUMN_ID_NAME, &id)) {
        ACSDK_ERROR(LX("storeAlertAssetsFailed").m("Cannot generate asset id."));
        return false;
    }
    id++;

    auto statement = db->createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX("storeAlertAssetsFailed").m("Could not create statement."));
        return false;
    }

    // go through each asset in the alert, and store in the database.
    for (auto& assetIter : assets) {
        auto& asset = assetIter.second;

        int boundParam = 1;
        if (!statement->bindIntParameter(boundParam++, id) || !statement->bindIntParameter(boundParam++, alertId) ||
            !statement->bindStringParameter(boundParam++, asset.id) ||
            !statement->bindStringParameter(boundParam, asset.url)) {
            ACSDK_ERROR(LX("storeAlertAssetsFailed").m("Could not bind a parameter."));
            return false;
        }

        if (!statement->step()) {
            ACSDK_ERROR(LX("storeAlertAssetsFailed").m("Could not step to next row."));
            return false;
        }

        if (!statement->reset()) {
            ACSDK_ERROR(LX("storeAlertAssetsFailed").m("Could not reset the statement."));
            return false;
        }

        id++;
    }

    return true;
}

static bool storeAlertAssetPlayOrderItems(
    SQLiteDatabase* db,
    int alertId,
    const std::vector<std::string>& assetPlayOrderItems) {
    if (assetPlayOrderItems.empty()) {
        return true;
    }

    // clang-format off
    const std::string sqlString = "INSERT INTO " + ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME + " (" +
                            "id, alert_id, asset_play_order_position, asset_play_order_token" +
                            ") VALUES (" +
                            "?, ?, ?, ?" +
                            ");";
    // clang-format on

    int id = 0;
    if (!getTableMaxIntValue(db, ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME, DATABASE_COLUMN_ID_NAME, &id)) {
        ACSDK_ERROR(LX("storeAlertAssetPlayOrderItemsFailed").m("Cannot generate asset id."));
        return false;
    }
    id++;

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("storeAlertAssetPlayOrderItemsFailed").m("Could not create statement."));
        return false;
    }

    // go through each assetPlayOrderItem in the alert, and store in the database.
    int itemIndex = 1;
    for (auto& assetId : assetPlayOrderItems) {
        int boundParam = 1;
        if (!statement->bindIntParameter(boundParam++, id) || !statement->bindIntParameter(boundParam++, alertId) ||
            !statement->bindIntParameter(boundParam++, itemIndex) ||
            !statement->bindStringParameter(boundParam, assetId)) {
            ACSDK_ERROR(LX("storeAlertAssetPlayOrderItemsFailed").m("Could not bind a parameter."));
            return false;
        }

        if (!statement->step()) {
            ACSDK_ERROR(LX("storeAlertAssetPlayOrderItemsFailed").m("Could not step to next row."));
            return false;
        }

        if (!statement->reset()) {
            ACSDK_ERROR(LX("storeAlertAssetPlayOrderItemsFailed").m("Could not reset the statement."));
            return false;
        }

        id++;
        itemIndex++;
    }

    return true;
}

bool SQLiteAlertStorage::store(std::shared_ptr<Alert> alert) {
    if (!alert) {
        ACSDK_ERROR(LX("storeFailed").m("Alert parameter is nullptr"));
        return false;
    }

    if (alertExists(alert->m_token)) {
        ACSDK_ERROR(LX("storeFailed").m("Alert already exists.").d("token", alert->m_token));
        return false;
    }

    // clang-format off
    const std::string sqlString = "INSERT INTO " + ALERTS_V2_TABLE_NAME + " (" +
                            "id, token, type, state, " +
                            "scheduled_time_unix, scheduled_time_iso_8601, asset_loop_count, " +
                            "asset_loop_pause_milliseconds, background_asset"
                            ") VALUES (" +
                            "?, ?, ?, ?, " +
                            "?, ?, ?," +
                            "?, ?" +
                            ");";
    // clang-format on

    int id = 0;
    if (!getTableMaxIntValue(&m_db, ALERTS_V2_TABLE_NAME, DATABASE_COLUMN_ID_NAME, &id)) {
        ACSDK_ERROR(LX("storeFailed").m("Cannot generate alert id."));
        return false;
    }
    id++;

    int alertType = ALERT_EVENT_TYPE_ALARM;
    if (!alertTypeToDbField(alert->getTypeName(), &alertType)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not convert type name to db field."));
        return false;
    }

    int alertState = ALERT_STATE_SET;
    if (!alertStateToDbField(alert->m_state, &alertState)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not convert alert state to db field."));
        return false;
    }

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("storeFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    auto token = alert->m_token;
    auto iso8601 = alert->getScheduledTime_ISO_8601();
    auto assetId = alert->getBackgroundAssetId();
    if (!statement->bindIntParameter(boundParam++, id) || !statement->bindStringParameter(boundParam++, token) ||
        !statement->bindIntParameter(boundParam++, alertType) ||
        !statement->bindIntParameter(boundParam++, alertState) ||
        !statement->bindInt64Parameter(boundParam++, alert->getScheduledTime_Unix()) ||
        !statement->bindStringParameter(boundParam++, iso8601) ||
        !statement->bindIntParameter(boundParam++, alert->getLoopCount()) ||
        !statement->bindIntParameter(boundParam++, alert->getLoopPause().count()) ||
        !statement->bindStringParameter(boundParam, assetId)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not bind parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("storeFailed").m("Could not perform step."));
        return false;
    }

    // capture the generated database id in the alert object.
    alert->m_dbId = id;

    if (!storeAlertAssets(&m_db, id, alert->m_assetConfiguration.assets)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not store alertAssets."));
        return false;
    }

    if (!storeAlertAssetPlayOrderItems(&m_db, id, alert->m_assetConfiguration.assetPlayOrderItems)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not store alertAssetPlayOrderItems."));
        return false;
    }

    return true;
}

static bool loadAlertAssets(SQLiteDatabase* db, std::map<int, std::vector<Alert::Asset>>* alertAssetsMap) {
    const std::string sqlString = "SELECT * FROM " + ALERT_ASSETS_TABLE_NAME + ";";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadAlertAssetsFailed").m("Could not create statement."));
        return false;
    }

    int alertId = 0;
    std::string avsId;
    std::string url;

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadAlertAssetsFailed").m("Could not perform step."));
        return false;
    }

    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement->getColumnName(i);

            if ("alert_id" == columnName) {
                alertId = statement->getColumnInt(i);
            } else if ("avs_id" == columnName) {
                avsId = statement->getColumnText(i);
            } else if ("url" == columnName) {
                url = statement->getColumnText(i);
            }
        }

        (*alertAssetsMap)[alertId].push_back(Alert::Asset(avsId, url));

        statement->step();
    }

    return true;
}

static bool loadAlertAssetPlayOrderItems(
    SQLiteDatabase* db,
    std::map<int, std::set<AssetOrderItem, AssetOrderItemCompare>>* alertAssetOrderItemsMap) {
    const std::string sqlString = "SELECT * FROM " + ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME + ";";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadAlertAssetPlayOrderItemsFailed").m("Could not create statement."));
        return false;
    }

    int alertId = 0;
    int playOrderPosition = 0;
    std::string playOrderToken;

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadAlertAssetPlayOrderItemsFailed").m("Could not perform step."));
        return false;
    }

    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement->getColumnName(i);

            if ("alert_id" == columnName) {
                alertId = statement->getColumnInt(i);
            } else if ("asset_play_order_position" == columnName) {
                playOrderPosition = statement->getColumnInt(i);
            } else if ("asset_play_order_token" == columnName) {
                playOrderToken = statement->getColumnText(i);
            }
        }

        (*alertAssetOrderItemsMap)[alertId].insert(AssetOrderItem{playOrderPosition, playOrderToken});

        statement->step();
    }

    return true;
}

bool SQLiteAlertStorage::loadHelper(int dbVersion, std::vector<std::shared_ptr<Alert>>* alertContainer) {
    if (!alertContainer) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Alert container parameter is nullptr."));
        return false;
    }

    if (dbVersion != ALERTS_DATABASE_VERSION_ONE && dbVersion != ALERTS_DATABASE_VERSION_TWO) {
        ACSDK_ERROR(LX("loadHelperFailed").d("Invalid version", dbVersion));
        return false;
    }

    std::string alertsTableName = ALERTS_TABLE_NAME;
    if (ALERTS_DATABASE_VERSION_TWO == dbVersion) {
        alertsTableName = ALERTS_V2_TABLE_NAME;
    }

    const std::string sqlString = "SELECT * FROM " + alertsTableName + ";";

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not create statement."));
        return false;
    }

    // local values which we will use to capture what we read from the database.
    int id = 0;
    std::string token;
    int type = 0;
    int state = 0;
    std::string scheduledTime_ISO_8601;
    int loopCount = 0;
    int loopPauseInMilliseconds = 0;
    std::string backgroundAssetId;

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not perform step."));
        return false;
    }

    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement->getColumnName(i);

            if ("id" == columnName) {
                id = statement->getColumnInt(i);
            } else if ("token" == columnName) {
                token = statement->getColumnText(i);
            } else if ("type" == columnName) {
                type = statement->getColumnInt(i);
            } else if ("state" == columnName) {
                state = statement->getColumnInt(i);
            } else if ("scheduled_time_iso_8601" == columnName) {
                scheduledTime_ISO_8601 = statement->getColumnText(i);
            } else if ("asset_loop_count" == columnName) {
                loopCount = statement->getColumnInt(i);
            } else if ("asset_loop_pause_milliseconds" == columnName) {
                loopPauseInMilliseconds = statement->getColumnInt(i);
            } else if ("background_asset" == columnName) {
                backgroundAssetId = statement->getColumnText(i);
            }
        }

        std::shared_ptr<Alert> alert;
        if (ALERT_EVENT_TYPE_ALARM == type) {
            alert = std::make_shared<Alarm>(m_alertsAudioFactory->alarmDefault(), m_alertsAudioFactory->alarmShort());
        } else if (ALERT_EVENT_TYPE_TIMER == type) {
            alert = std::make_shared<Timer>(m_alertsAudioFactory->timerDefault(), m_alertsAudioFactory->timerShort());
        } else if (ALERT_EVENT_TYPE_REMINDER == type) {
            alert = std::make_shared<Reminder>(
                m_alertsAudioFactory->reminderDefault(), m_alertsAudioFactory->reminderShort());
        } else {
            ACSDK_ERROR(
                LX("loadHelperFailed").m("Could not instantiate an alert object.").d("type read from database", type));
            return false;
        }

        alert->m_dbId = id;
        alert->m_token = token;
        alert->setTime_ISO_8601(scheduledTime_ISO_8601);
        alert->setLoopCount(loopCount);
        alert->setLoopPause(std::chrono::milliseconds{loopPauseInMilliseconds});
        alert->setBackgroundAssetId(backgroundAssetId);

        if (!dbFieldToAlertState(state, &(alert->m_state))) {
            ACSDK_ERROR(LX("loadHelperFailed").m("Could not convert alert state."));
            return false;
        }

        alertContainer->push_back(alert);

        statement->step();
    }

    statement->finalize();

    std::map<int, std::vector<Alert::Asset>> alertAssetsMap;
    if (!loadAlertAssets(&m_db, &alertAssetsMap)) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not load alert assets."));
        return false;
    }

    std::map<int, std::set<AssetOrderItem, AssetOrderItemCompare>> alertAssetOrderItemsMap;
    if (!loadAlertAssetPlayOrderItems(&m_db, &alertAssetOrderItemsMap)) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not load alert asset play order items."));
        return false;
    }

    for (auto alert : *alertContainer) {
        if (alertAssetsMap.find(alert->m_dbId) != alertAssetsMap.end()) {
            for (auto& mapEntry : alertAssetsMap[alert->m_dbId]) {
                alert->m_assetConfiguration.assets[mapEntry.id] = mapEntry;
            }
        }
        if (alertAssetOrderItemsMap.find(alert->m_dbId) != alertAssetOrderItemsMap.end()) {
            for (auto& mapEntry : alertAssetOrderItemsMap[alert->m_dbId]) {
                alert->m_assetConfiguration.assetPlayOrderItems.push_back(mapEntry.name);
            }
        }
    }

    return true;
}

bool SQLiteAlertStorage::load(std::vector<std::shared_ptr<Alert>>* alertContainer) {
    return loadHelper(ALERTS_DATABASE_VERSION_TWO, alertContainer);
}

bool SQLiteAlertStorage::modify(std::shared_ptr<Alert> alert) {
    if (!alert) {
        ACSDK_ERROR(LX("modifyFailed").m("Alert parameter is nullptr."));
        return false;
    }

    if (!alertExists(alert->m_token)) {
        ACSDK_ERROR(LX("modifyFailed").m("Cannot modify alert.").d("token", alert->m_token));
        return false;
    }

    const std::string sqlString = "UPDATE " + ALERTS_V2_TABLE_NAME + " SET " +
                                  "state=?, scheduled_time_unix=?, scheduled_time_iso_8601=? " + "WHERE id=?;";

    int alertState = ALERT_STATE_SET;
    if (!alertStateToDbField(alert->m_state, &alertState)) {
        ACSDK_ERROR(LX("modifyFailed").m("Cannot convert state."));
        return false;
    }

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("modifyFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    auto iso8601 = alert->getScheduledTime_ISO_8601();
    if (!statement->bindIntParameter(boundParam++, alertState) ||
        !statement->bindInt64Parameter(boundParam++, alert->getScheduledTime_Unix()) ||
        !statement->bindStringParameter(boundParam++, iso8601) ||
        !statement->bindIntParameter(boundParam++, alert->m_dbId)) {
        ACSDK_ERROR(LX("modifyFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("modifyFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

/**
 * A utility function to delete alert records from the database for a given alert id.
 * This function will clean up records in the alerts table.
 *
 * @param db The database object.
 * @param alertId The alert id of the alert to be deleted.
 * @return Whether the delete operation was successful.
 */
static bool eraseAlert(SQLiteDatabase* db, int alertId) {
    const std::string sqlString = "DELETE FROM " + ALERTS_V2_TABLE_NAME + " WHERE id=?;";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindIntParameter(boundParam, alertId)) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

/**
 * A utility function to delete alert records from the database for a given alert id.
 * This function will clean up records in the alertAssets table.
 *
 * @param db The database object.
 * @param alertId The alert id of the alert to be deleted.
 * @return Whether the delete operation was successful.
 */
static bool eraseAlertAssets(SQLiteDatabase* db, int alertId) {
    const std::string sqlString = "DELETE FROM " + ALERT_ASSETS_TABLE_NAME + " WHERE alert_id=?;";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseAlertAssetsFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindIntParameter(boundParam, alertId)) {
        ACSDK_ERROR(LX("eraseAlertAssetsFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseAlertAssetsFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

/**
 * A utility function to delete alert records from the database for a given alert id.
 * This function will clean up records in the alertAssetPlayOrderItems table.
 *
 * @param db The database object.
 * @param alertId The alert id of the alert to be deleted.
 * @return Whether the delete operation was successful.
 */
static bool eraseAlertAssetPlayOrderItems(SQLiteDatabase* db, int alertId) {
    const std::string sqlString = "DELETE FROM " + ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME + " WHERE alert_id=?;";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseAlertAssetPlayOrderItemsFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindIntParameter(boundParam, alertId)) {
        ACSDK_ERROR(LX("eraseAlertAssetPlayOrderItemsFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseAlertAssetPlayOrderItemsFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

/**
 * A utility function to delete an alert from the database for a given alert id.  This will clean up records in
 * all tables which are associated with the alert.
 *
 * @param db The DB object.
 * @param alertId The alert id of the alert to be deleted.
 * @return Whether the delete operation was successful.
 */
static bool eraseAlertByAlertId(SQLiteDatabase* db, int alertId) {
    if (!db) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("db is nullptr."));
        return false;
    }

    if (!eraseAlert(db, alertId)) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("Could not erase alert table items."));
        return false;
    }

    if (!eraseAlertAssets(db, alertId)) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("Could not erase alertAsset table items."));
        return false;
    }

    if (!eraseAlertAssetPlayOrderItems(db, alertId)) {
        ACSDK_ERROR(LX("eraseAlertByAlertIdFailed").m("Could not erase alertAssetPlayOrderItems table items."));
        return false;
    }

    return true;
}

bool SQLiteAlertStorage::erase(std::shared_ptr<Alert> alert) {
    if (!alert) {
        ACSDK_ERROR(LX("eraseFailed").m("Alert parameter is nullptr."));
        return false;
    }

    if (!alertExists(alert->m_token)) {
        ACSDK_ERROR(LX("eraseFailed").m("Cannot delete alert - not in database.").d("token", alert->m_token));
        return false;
    }

    return eraseAlertByAlertId(&m_db, alert->m_dbId);
}

bool SQLiteAlertStorage::clearDatabase() {
    const std::vector<std::string> tablesToClear = {
        ALERTS_V2_TABLE_NAME, ALERT_ASSETS_TABLE_NAME, ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME};
    for (auto& tableName : tablesToClear) {
        if (!m_db.clearTable(tableName)) {
            ACSDK_ERROR(LX("clearDatabaseFailed").d("could not clear table", tableName));
            return false;
        }
    }

    return true;
}

/**
 * Utility diagnostic function to print a one-line summary of all alerts in the database.
 *
 * @param db The database object.
 */
static void printOneLineSummary(SQLiteDatabase* db) {
    int numberAlerts = 0;

    if (!getNumberTableRows(db, ALERTS_V2_TABLE_NAME, &numberAlerts)) {
        ACSDK_ERROR(LX("printOneLineSummaryFailed").m("could not read number of alerts."));
        return;
    }

    ACSDK_INFO(LX("ONE-LINE-STAT: Number of alerts:" + std::to_string(numberAlerts)));
}

/**
 * Utility diagnostic function to print the details of all the alerts stored in the database.
 *
 * @param db The database object.
 * @param shouldPrintEverything If @c true, then all details of an alert will be printed.  If @c false, then
 * summary information will be printed instead.
 */
static void printAlertsSummary(
    SQLiteDatabase* db,
    const std::vector<std::shared_ptr<Alert>>& alerts,
    bool shouldPrintEverything = false) {
    printOneLineSummary(db);

    for (auto& alert : alerts) {
        alert->printDiagnostic();
    }
}

void SQLiteAlertStorage::printStats(StatLevel level) {
    std::vector<std::shared_ptr<Alert>> alerts;
    load(&alerts);
    switch (level) {
        case StatLevel::ONE_LINE:
            printOneLineSummary(&m_db);
            break;
        case StatLevel::ALERTS_SUMMARY:
            printAlertsSummary(&m_db, alerts, false);
            break;
        case StatLevel::EVERYTHING:
            printAlertsSummary(&m_db, alerts, true);
            break;
    }
}

}  // namespace storage
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
