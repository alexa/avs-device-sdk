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

#include "acsdkAlerts/Storage/SQLiteAlertStorage.h"
#include "acsdkAlerts/Alarm.h"
#include "acsdkAlerts/Reminder.h"
#include "acsdkAlerts/Timer.h"

#include <SQLiteStorage/SQLiteStatement.h>
#include <SQLiteStorage/SQLiteUtils.h>

#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include <fstream>
#include <set>

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace storage {

using namespace avsCommon::utils::file;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;
using namespace alexaClientSDK::storage::sqliteStorage;
using namespace rapidjson;

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

/// The value of the offline stopped alert token key.
static const std::string OFFLINE_STOPPED_ALERT_TOKEN_KEY = "token";
/// The value of the offline stopped alert scheduledTime key.
static const std::string OFFLINE_STOPPED_ALERT_SCHEDULED_TIME_KEY = "scheduledTime";
/// The value of the offline stopped alert eventTime key.
static const std::string OFFLINE_STOPPED_ALERT_EVENT_TIME_KEY = "eventTime";
/// The value of the offline stopped alert id key.
static const std::string OFFLINE_STOPPED_ALERT_ID_KEY = "id";

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
/// The name of the 'token' field used in alerts table.
static const std::string DATABASE_COLUMN_TOKEN_NAME = "token";
/// The name of the 'type' field used in alerts table.
static const std::string DATABASE_COLUMN_TYPE_NAME = "type";
/// The name of the 'state' field used in alerts table.
static const std::string DATABASE_COLUMN_STATE_NAME = "state";
/// The name of the 'scheduled_time_unix' field used in alerts table.
static const std::string DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME = "scheduled_time_unix";
/// The name of the 'scheduled_time_iso_8601' field used in alerts table.
static const std::string DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME = "scheduled_time_iso_8601";
/// The name of the 'asset_loop_count' field used in alerts table.
static const std::string DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME = "asset_loop_count";
/// The name of the 'asset_loop_pause_milliseconds' field used in alerts table.
static const std::string DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME = "asset_loop_pause_milliseconds";
/// The name of the 'background_asset' field used in alerts table.
static const std::string DATABASE_COLUMN_BACKGROUND_ASSET_NAME = "background_asset";
/// The name of the 'original_time' field used in alerts table.
static const std::string DATABASE_COLUMN_ORIGINAL_TIME_NAME = "original_time";
/// The name of the 'label' field used in alerts table.
static const std::string DATABASE_COLUMN_LABEL_NAME = "label";
/// The name of the 'created_time_iso_8601' field used in alerts table.
static const std::string DATABASE_COLUMN_CREATED_TIME_NAME = "created_time_iso_8601";

/// A symbolic name for version three of our database.
static const int ALERTS_DATABASE_VERSION_THREE = 3;

/// The name of the alerts (v2) table.
static const std::string ALERTS_V2_TABLE_NAME = "alerts_v2";

/// The name of the alerts (v3) table.
static const std::string ALERTS_V3_TABLE_NAME = "alerts_v3";

/// The SQL string to create the alerts table.
// clang-format off
static const std::string CREATE_ALERTS_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        ALERTS_V3_TABLE_NAME + " (" +
        DATABASE_COLUMN_ID_NAME + " INT PRIMARY KEY NOT NULL," +
        DATABASE_COLUMN_TOKEN_NAME + " TEXT NOT NULL," +
        DATABASE_COLUMN_TYPE_NAME + " INT NOT NULL," +
        DATABASE_COLUMN_STATE_NAME + " INT NOT NULL," +
        DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME + " INT NOT NULL," +
        DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME + " TEXT NOT NULL," +
        DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME + " INT NOT NULL," +
        DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME + " INT NOT NULL," +
        DATABASE_COLUMN_BACKGROUND_ASSET_NAME + " TEXT NOT NULL," +
        DATABASE_COLUMN_ORIGINAL_TIME_NAME + " TEXT NOT NULL," +
        DATABASE_COLUMN_LABEL_NAME + " TEXT NOT NULL," +
        DATABASE_COLUMN_CREATED_TIME_NAME + " TEXT NOT NULL);";
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

/// The name of the offline alerts (v1) table.
static const std::string OFFLINE_ALERTS_TABLE_NAME = "offlineAlerts";

/// The name of the offline alerts (v2) table.
static const std::string OFFLINE_ALERTS_V2_TABLE_NAME = "offlineAlerts_v2";

/// A symbolic name for version one of our offline alerts database.
static const int OFFLINE_ALERTS_DATABASE_VERSION_ONE = 1;
/// A symbolic name for version two of our offline alerts database.
static const int OFFLINE_ALERTS_DATABASE_VERSION_TWO = 2;

/// The SQL string to create the offline alerts table.
// clang-format off
static const std::string CREATE_OFFLINE_ALERTS_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        OFFLINE_ALERTS_V2_TABLE_NAME + " (" +
        "id INT PRIMARY KEY NOT NULL," +
        "token TEXT NOT NULL," +
        "scheduled_time_iso_8601 TEXT NOT NULL," +
        "event_time_iso_8601 TEXT NOT NULL);";
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

/// The prefix for alert metrics.
static const std::string ALERT_METRIC_PREFIX = "ALERT-";

/// The event names for metrics.
static const std::string ALERT_DATABASE_OPEN_FAILED = "databaseOpenFailed";
static const std::string OFFLINE_ALERTS_V1ToV2_MIGRATION_FAILED = "offlineAlertsV1ToV2MigrationFailed";
static const std::string ALERTS_V2ToV3_MIGRATION_FAILED = "alertsV2ToV3MigrationFailed";
static const std::string CREATE_OFFLINE_ALERTS_V2_FAILED = "createOfflineAlertsV2Failed";
static const std::string CREATE_ALERTS_V3_FAILED = "createAlertsV3Failed";
static const std::string CREATE_DATABASE_FAILED = "createDatabaseFailed";

/// The table with entries for retry times in milliseconds.
static const std::vector<int> RETRY_TABLE = {10, 20, 40};
/// The maximum retry times.
static const size_t RETRY_TIME_MAXIMUM = RETRY_TABLE.size();

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
 * Submits a metric for a given event name.
 *
 * @param metricRecorder The @c MetricRecorderInterface used to record metrics.
 * @param eventName The name of the metric event.
 * @param count The number of data point to be added.
 */
static void submitMetric(
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::string& eventName,
    const int count) {
    if (nullptr == metricRecorder) {
        return;
    }

    auto metricEvent =
        avsCommon::utils::metrics::MetricEventBuilder{}
            .setActivityName(ALERT_METRIC_PREFIX + eventName)
            .addDataPoint(
                avsCommon::utils::metrics::DataPointCounterBuilder{}.setName(eventName).increment(count).build())
            .build();
    if (nullptr == metricEvent) {
        ACSDK_ERROR(LX("submitMetricFailed").d("reason", "metricEventNull"));
        return;
    }
    recordMetric(metricRecorder, metricEvent);
}

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

std::shared_ptr<AlertStorageInterface> SQLiteAlertStorage::createAlertStorageInterface(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>& audioFactory,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    if (!configurationRoot || !audioFactory) {
        ACSDK_ERROR(LX("createAlertStorageInterfaceFailed")
                        .d("isConfigurationRootNull", !configurationRoot)
                        .d("isAudioFactoryNull", !audioFactory));
        return nullptr;
    }

    auto alertsAudioFactory = audioFactory->alerts();

    auto storage = create(*configurationRoot, alertsAudioFactory, metricRecorder);
    return std::move(storage);
}

std::unique_ptr<SQLiteAlertStorage> SQLiteAlertStorage::create(
    const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
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

    if (!alertsAudioFactory) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAlertsAudioFactory"));
        return nullptr;
    }

    return std::unique_ptr<SQLiteAlertStorage>(
        new SQLiteAlertStorage(alertDbFilePath, alertsAudioFactory, metricRecorder));
}

SQLiteAlertStorage::SQLiteAlertStorage(
    const std::string& dbFilePath,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_alertsAudioFactory{alertsAudioFactory},
        m_db{dbFilePath},
        m_metricRecorder{metricRecorder},
        m_retryTimer{RETRY_TABLE} {
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
 * Utility function to create the offline Alerts table within the database.
 *
 * @param db The SQLiteDatabase object.
 * @return Whether the table was successfully created.
 */
static bool createOfflineAlertsTable(SQLiteDatabase* db) {
    if (!db) {
        ACSDK_ERROR(LX("createOfflineAlertsTableFailed").m("null db"));
        return false;
    }

    if (!db->performQuery(CREATE_OFFLINE_ALERTS_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createOfflineAlertsTableFailed").m("Table could not be created."));
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
        submitMetric(m_metricRecorder, CREATE_DATABASE_FAILED, 1);
        return false;
    }

    if (!createAlertsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("Alerts table could not be created."));
        close();
        submitMetric(m_metricRecorder, CREATE_ALERTS_V3_FAILED, 1);
        submitMetric(m_metricRecorder, CREATE_DATABASE_FAILED, 1);
        return false;
    }
    submitMetric(m_metricRecorder, CREATE_ALERTS_V3_FAILED, 0);

    if (!createAlertAssetsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("AlertAssets table could not be created."));
        close();
        submitMetric(m_metricRecorder, CREATE_DATABASE_FAILED, 1);
        return false;
    }

    if (!createOfflineAlertsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("OfflineAlerts table could not be created."));
        close();
        submitMetric(m_metricRecorder, CREATE_OFFLINE_ALERTS_V2_FAILED, 1);
        submitMetric(m_metricRecorder, CREATE_DATABASE_FAILED, 1);
        return false;
    }
    submitMetric(m_metricRecorder, CREATE_OFFLINE_ALERTS_V2_FAILED, 0);

    if (!createAlertAssetPlayOrderItemsTable(&m_db)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("AlertAssetPlayOrderItems table could not be created."));
        close();
        submitMetric(m_metricRecorder, CREATE_DATABASE_FAILED, 1);
        return false;
    }

    submitMetric(m_metricRecorder, CREATE_DATABASE_FAILED, 0);
    return true;
}

bool SQLiteAlertStorage::open() {
    if (!m_db.open()) {
        return false;
    }
    /// Check if any tables are missing, if they are, add them.
    /// Alerts table will be created during migration if it does not exist yet.
    if (!migrateAlertsDbFromV2ToV3()) {
        ACSDK_ERROR(LX("openFailed").m("migrateAlertsDbFromV2ToV3 failed."));
        close();
        submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 1);
        return false;
    }
    /// Drop the legacy alerts_v2 table.
    if (m_db.tableExists(ALERTS_V2_TABLE_NAME) && !m_db.dropTable(ALERTS_V2_TABLE_NAME)) {
        ACSDK_ERROR(LX("dropTableFailed").d("tableName", ALERTS_V2_TABLE_NAME));
        close();
        submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 1);
        return false;
    }

    if (!m_db.tableExists(ALERT_ASSETS_TABLE_NAME)) {
        if (!createAlertAssetsTable(&m_db)) {
            ACSDK_ERROR(LX("openFailed").m("AlertAssets table could not be created."));
            close();
            submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 1);
            return false;
        }
    }
    if (!m_db.tableExists(ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME)) {
        if (!createAlertAssetPlayOrderItemsTable(&m_db)) {
            ACSDK_ERROR(LX("openFailed").m("AlertAssetPlayOrderItems table could not be created."));
            submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 1);
            close();
            return false;
        }
    }

    /// Offline alerts table will be created during migration if it does not exist yet.
    if (!migrateOfflineAlertsDbFromV1ToV2()) {
        ACSDK_ERROR(LX("openFailed").m("migrateOfflineAlertsDbFromV1ToV2 failed."));
        close();
        submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 1);
        return false;
    }
    /// Drop the legacy offlineAlerts table.
    if (m_db.tableExists(OFFLINE_ALERTS_TABLE_NAME) && !m_db.dropTable(OFFLINE_ALERTS_TABLE_NAME)) {
        ACSDK_ERROR(LX("dropTableFailed").d("tableName", OFFLINE_ALERTS_TABLE_NAME));
        close();
        submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 1);
        return false;
    }

    submitMetric(m_metricRecorder, ALERT_DATABASE_OPEN_FAILED, 0);
    return true;
}

bool SQLiteAlertStorage::migrateOfflineAlertsDbFromV1ToV2() {
    /// Offline alerts table is up-to-date, no need to migrate.
    if (m_db.tableExists(OFFLINE_ALERTS_V2_TABLE_NAME)) {
        ACSDK_DEBUG5(LX("migrateOfflineAlertsDbFromV1ToV2").m("Offline alerts v2 table already exists."));
        return true;
    }

    if (!createOfflineAlertsTable(&m_db)) {
        ACSDK_ERROR(LX("migrateOfflineAlertsDbFromV1ToV2Failed").m("Offline alerts v2 table could not be created."));
        submitMetric(m_metricRecorder, CREATE_OFFLINE_ALERTS_V2_FAILED, 1);
        return false;
    }
    submitMetric(m_metricRecorder, CREATE_OFFLINE_ALERTS_V2_FAILED, 0);

    /// Offline alerts v1 table does not exist, nothing to be migrated.
    if (!m_db.tableExists(OFFLINE_ALERTS_TABLE_NAME)) {
        ACSDK_DEBUG5(LX("migrateOfflineAlertsDbFromV1ToV2")
                         .m("Offline alerts v1 table does not exist, nothing to be migrated."));
        submitMetric(m_metricRecorder, OFFLINE_ALERTS_V1ToV2_MIGRATION_FAILED, 0);
        return true;
    }
    bool success = retryDataMigration([this]() -> bool {
        rapidjson::Document offlineAlerts(rapidjson::kObjectType);
        auto& allocator = offlineAlerts.GetAllocator();
        rapidjson::Value alertContainer(rapidjson::kArrayType);
        loadOfflineAlertsHelper(OFFLINE_ALERTS_DATABASE_VERSION_ONE, &alertContainer, allocator);
        bool isLegacyV1 = isOfflineTableV1Legacy();

        for (const auto& alert : alertContainer.GetArray()) {
            std::string token = "";
            if (!avsCommon::utils::json::jsonUtils::retrieveValue(alert, OFFLINE_STOPPED_ALERT_TOKEN_KEY, &token)) {
                ACSDK_ERROR(LX("migrateOfflineAlertsDbFromV1ToV2Failed")
                                .m("Could not retrieve" + OFFLINE_STOPPED_ALERT_TOKEN_KEY));
                return false;
            }
            std::string scheduledTime_ISO_8601 = "";
            if (!avsCommon::utils::json::jsonUtils::retrieveValue(
                    alert, OFFLINE_STOPPED_ALERT_SCHEDULED_TIME_KEY, &scheduledTime_ISO_8601)) {
                ACSDK_ERROR(LX("migrateOfflineAlertsDbFromV1ToV2Failed")
                                .m("Could not retrieve" + OFFLINE_STOPPED_ALERT_SCHEDULED_TIME_KEY));
                return false;
            }
            std::string event_time_iso_8601 = "";
            if (!isLegacyV1 && !avsCommon::utils::json::jsonUtils::retrieveValue(
                                   alert, OFFLINE_STOPPED_ALERT_EVENT_TIME_KEY, &event_time_iso_8601)) {
                ACSDK_ERROR(LX("migrateOfflineAlertsDbFromV1ToV2Failed")
                                .m("Could not retrieve" + OFFLINE_STOPPED_ALERT_EVENT_TIME_KEY));
                return false;
            }
            if (offlineAlertExists(OFFLINE_ALERTS_DATABASE_VERSION_TWO, token)) {
                /// the offline alert may be stored successfully before retry.
                ACSDK_DEBUG7(LX("migrateOfflineAlertsDbFromV1ToV2").m("Offline alerts already exists"));
                continue;
            }
            if (!storeOfflineAlertHelper(
                    OFFLINE_ALERTS_DATABASE_VERSION_TWO, token, scheduledTime_ISO_8601, event_time_iso_8601)) {
                ACSDK_ERROR(LX("migrateOfflineAlertsDbFromV1ToV2Failed").m("Failed to store offline alert to V2."));
                return false;
            }
        }
        return true;
    });

    if (success) {
        ACSDK_DEBUG8(LX("migrateOfflineAlertsDbFromV1ToV2Succeeded"));
        submitMetric(m_metricRecorder, OFFLINE_ALERTS_V1ToV2_MIGRATION_FAILED, 0);
    } else {
        ACSDK_ERROR(LX("migrateOfflineAlertsDbFromV1ToV2Failed"));
        submitMetric(m_metricRecorder, OFFLINE_ALERTS_V1ToV2_MIGRATION_FAILED, 1);
    }

    return success;
}

bool SQLiteAlertStorage::migrateAlertsDbFromV2ToV3() {
    /// Alerts table is up-to-date, no need to migrate.
    if (m_db.tableExists(ALERTS_V3_TABLE_NAME)) {
        ACSDK_DEBUG5(LX("migrateAlertsDbFromV2ToV3").m("Alerts v3 table already exists."));
        return true;
    }

    if (!createAlertsTable(&m_db)) {
        ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed").m("Alerts v3 table could not be created."));
        submitMetric(m_metricRecorder, CREATE_ALERTS_V3_FAILED, 1);
        return false;
    }
    submitMetric(m_metricRecorder, CREATE_ALERTS_V3_FAILED, 0);

    /// Alerts v2 table does not exist, nothing to be migrated.
    if (!m_db.tableExists(ALERTS_V2_TABLE_NAME)) {
        submitMetric(m_metricRecorder, ALERTS_V2ToV3_MIGRATION_FAILED, 0);
        ACSDK_DEBUG5(LX("migrateAlertsDbFromV2ToV3").m("Alerts v2 table does not exist, nothing to be migrated."));
        return true;
    }

    bool success = retryDataMigration([this]() -> bool {
        const std::string loadSqlString = "SELECT * FROM " + ALERTS_V2_TABLE_NAME + ";";
        auto loadStatement = m_db.createStatement(loadSqlString);
        if (!loadStatement) {
            ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed").m("Could not create loadStatement."));
            return false;
        }

        if (!loadStatement->step()) {
            ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed").m("Could not perform step."));
            return false;
        }

        while (SQLITE_ROW == loadStatement->getStepResult()) {
            int numberColumns = loadStatement->getColumnCount();
            int id = 0;
            std::string token = "";
            int type = 0;
            int state = 0;
            int64_t scheduledTime_Unix = 0;
            std::string scheduledTime_ISO_8601 = "";
            int loopCount = 0;
            int loopPauseInMilliseconds = 0;
            std::string backgroundAssetId = "";

            for (int i = 0; i < numberColumns; i++) {
                std::string columnName = loadStatement->getColumnName(i);

                if (DATABASE_COLUMN_ID_NAME == columnName) {
                    id = loadStatement->getColumnInt(i);
                } else if (DATABASE_COLUMN_TOKEN_NAME == columnName) {
                    token = loadStatement->getColumnText(i);
                } else if (DATABASE_COLUMN_TYPE_NAME == columnName) {
                    type = loadStatement->getColumnInt(i);
                } else if (DATABASE_COLUMN_STATE_NAME == columnName) {
                    state = loadStatement->getColumnInt(i);
                } else if (DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME == columnName) {
                    scheduledTime_Unix = loadStatement->getColumnInt64(i);
                } else if (DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME == columnName) {
                    scheduledTime_ISO_8601 = loadStatement->getColumnText(i);
                } else if (DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME == columnName) {
                    loopCount = loadStatement->getColumnInt(i);
                } else if (DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME == columnName) {
                    loopPauseInMilliseconds = loadStatement->getColumnInt(i);
                } else if (DATABASE_COLUMN_BACKGROUND_ASSET_NAME == columnName) {
                    backgroundAssetId = loadStatement->getColumnText(i);
                }
            }

            if (alertExists(ALERTS_DATABASE_VERSION_THREE, token)) {
                loadStatement->step();
                continue;
            }

            // clang-format off
            const std::string storeSqlString = "INSERT INTO " + ALERTS_V3_TABLE_NAME + " (" +
                                    DATABASE_COLUMN_ID_NAME + ", " + DATABASE_COLUMN_TOKEN_NAME + ", " +
                                    DATABASE_COLUMN_TYPE_NAME + ", " + DATABASE_COLUMN_STATE_NAME + ", " +
                                    DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME + ", " +
                                    DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME + ", " +
                                    DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME + ", " +
                                    DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME + ", " +
                                    DATABASE_COLUMN_BACKGROUND_ASSET_NAME + ", " +
                                    DATABASE_COLUMN_ORIGINAL_TIME_NAME + ", " + DATABASE_COLUMN_LABEL_NAME + ", " +
                                    DATABASE_COLUMN_CREATED_TIME_NAME +
                                    ") VALUES (" +
                                    "?, ?, " + /// DATABASE_COLUMN_ID_NAME, DATABASE_COLUMN_TOKEN_NAME
                                    "?, ?, " + /// DATABASE_COLUMN_TYPE_NAME, DATABASE_COLUMN_STATE_NAME
                                    "?, " + /// DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME
                                    "?, " + /// DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME
                                    "?, " + /// DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME
                                    "?, " + /// DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME
                                    "?, " + /// DATABASE_COLUMN_BACKGROUND_ASSET_NAME
                                    "?, ?, " + /// DATABASE_COLUMN_ORIGINAL_TIME_NAME, DATABASE_COLUMN_LABEL_NAME
                                    "?" + /// DATABASE_COLUMN_CREATED_TIME_NAME
                                    ");";
            // clang-format on
            auto storeStatement = m_db.createStatement(storeSqlString);

            if (!storeStatement) {
                ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed").m("Could not create storeStatement."));
                return false;
            }

            int boundParam = 1;
            if (!storeStatement->bindIntParameter(boundParam++, id) ||
                !storeStatement->bindStringParameter(boundParam++, token) ||
                !storeStatement->bindIntParameter(boundParam++, type) ||
                !storeStatement->bindIntParameter(boundParam++, state) ||
                !storeStatement->bindInt64Parameter(boundParam++, scheduledTime_Unix) ||
                !storeStatement->bindStringParameter(boundParam++, scheduledTime_ISO_8601) ||
                !storeStatement->bindIntParameter(boundParam++, loopCount) ||
                !storeStatement->bindIntParameter(boundParam++, loopPauseInMilliseconds) ||
                !storeStatement->bindStringParameter(boundParam++, backgroundAssetId) ||
                !storeStatement->bindStringParameter(boundParam++, "") ||
                !storeStatement->bindStringParameter(boundParam++, "") ||
                !storeStatement->bindStringParameter(boundParam, "")) {
                ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed").m("Bind parameter failed in storeStatement."));
                return false;
            }

            if (!storeStatement->step()) {
                ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed").m("Step failed in storeStatement."));
                return false;
            }

            loadStatement->step();
        }
        return true;
    });

    if (success) {
        ACSDK_DEBUG8(LX("migrateAlertsDbFromV2toV3Succeeded"));
        submitMetric(m_metricRecorder, ALERTS_V2ToV3_MIGRATION_FAILED, 0);
    } else {
        ACSDK_ERROR(LX("migrateAlertsDbFromV2toV3Failed"));
        submitMetric(m_metricRecorder, ALERTS_V2ToV3_MIGRATION_FAILED, 1);
    }
    return success;
}

void SQLiteAlertStorage::close() {
    m_db.close();
}

bool SQLiteAlertStorage::alertExists(const int dbVersion, const std::string& token) {
    if (dbVersion != ALERTS_DATABASE_VERSION_THREE) {
        ACSDK_ERROR(LX("alertExistsFailed").d("UnsupportedDbVersion", dbVersion));
        return false;
    }

    std::string tableName = ALERTS_V3_TABLE_NAME;
    const std::string sqlString = "SELECT COUNT(*) FROM " + tableName + " WHERE token=?;";
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

bool SQLiteAlertStorage::offlineAlertExists(const int dbVersion, const std::string& token) {
    if (dbVersion != OFFLINE_ALERTS_DATABASE_VERSION_TWO) {
        ACSDK_ERROR(LX("offlineAlertExistsFailed").d("Invalid db version", dbVersion));
        return false;
    }
    std::string offlineAlertsTableName = OFFLINE_ALERTS_V2_TABLE_NAME;

    const std::string sqlString = "SELECT COUNT(*) FROM " + offlineAlertsTableName + " WHERE token=?;";
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
        ACSDK_ERROR(LX("storeAlertFailed").m("Alert parameter is nullptr"));
        return false;
    }

    if (alertExists(ALERTS_DATABASE_VERSION_THREE, alert->getToken())) {
        ACSDK_ERROR(LX("storeAlertFailed").m("Alert already exists.").d("token", alert->getToken()));
        return false;
    }

    // clang-format off
    const std::string sqlString = "INSERT INTO " + ALERTS_V3_TABLE_NAME + " (" +
                            DATABASE_COLUMN_ID_NAME + ", " + DATABASE_COLUMN_TOKEN_NAME + ", " +
                            DATABASE_COLUMN_TYPE_NAME + ", " + DATABASE_COLUMN_STATE_NAME + ", " +
                            DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME + ", " +
                            DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME + ", " +
                            DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME + ", " +
                            DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME + ", " +
                            DATABASE_COLUMN_BACKGROUND_ASSET_NAME + ", " +
                            DATABASE_COLUMN_ORIGINAL_TIME_NAME + ", " + DATABASE_COLUMN_LABEL_NAME + ", " +
                            DATABASE_COLUMN_CREATED_TIME_NAME +
                            ") VALUES (" +
                            "?, ?, " + /// DATABASE_COLUMN_ID_NAME, DATABASE_COLUMN_TOKEN_NAME
                            "?, ?, " + /// DATABASE_COLUMN_TYPE_NAME, DATABASE_COLUMN_STATE_NAME
                            "?, " + /// DATABASE_COLUMN_SCHEDULED_TIME_UNIX_NAME
                            "?, " + /// DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME
                            "?, " + /// DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME
                            "?, " + /// DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME
                            "?, " + /// DATABASE_COLUMN_BACKGROUND_ASSET_NAME
                            "?, ?, " + /// DATABASE_COLUMN_ORIGINAL_TIME_NAME, DATABASE_COLUMN_LABEL_NAME
                            "?" + /// DATABASE_COLUMN_CREATED_TIME_NAME
                            ");";
    // clang-format on

    int id = 0;
    if (!getTableMaxIntValue(&m_db, ALERTS_V3_TABLE_NAME, DATABASE_COLUMN_ID_NAME, &id)) {
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
    if (!alertStateToDbField(alert->getState(), &alertState)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not convert alert state to db field."));
        return false;
    }

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("storeFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    auto token = alert->getToken();
    auto iso8601 = alert->getScheduledTime_ISO_8601();
    auto assetId = alert->getBackgroundAssetId();
    auto originalTime =
        alert->getOriginalTime().hasValue()
            ? acsdkAlertsInterfaces::AlertObserverInterface::originalTimeToString(alert->getOriginalTime().value())
            : "";
    std::string label = alert->getLabel().valueOr("");
    std::string createdTime = "";
    if (!statement->bindIntParameter(boundParam++, id) || !statement->bindStringParameter(boundParam++, token) ||
        !statement->bindIntParameter(boundParam++, alertType) ||
        !statement->bindIntParameter(boundParam++, alertState) ||
        !statement->bindInt64Parameter(boundParam++, alert->getScheduledTime_Unix()) ||
        !statement->bindStringParameter(boundParam++, iso8601) ||
        !statement->bindIntParameter(boundParam++, alert->getLoopCount()) ||
        !statement->bindIntParameter(boundParam++, alert->getLoopPause().count()) ||
        !statement->bindStringParameter(boundParam++, assetId) ||
        !statement->bindStringParameter(boundParam++, originalTime) ||
        !statement->bindStringParameter(boundParam++, label) ||
        !statement->bindStringParameter(boundParam, createdTime)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not bind parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("storeFailed").m("Could not perform step."));
        return false;
    }

    // capture the generated database id in the alert object.
    Alert::StaticData staticData;
    alert->getAlertData(&staticData, nullptr);

    staticData.dbId = id;

    if (!alert->setAlertData(&staticData, nullptr)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not set alert data."));
        return false;
    }

    if (!storeAlertAssets(&m_db, id, alert->getAssetConfiguration().assets)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not store alertAssets."));
        return false;
    }

    if (!storeAlertAssetPlayOrderItems(&m_db, id, alert->getAssetConfiguration().assetPlayOrderItems)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not store alertAssetPlayOrderItems."));
        return false;
    }
    ACSDK_DEBUG9(LX("Successfully stored alert to " + ALERTS_V3_TABLE_NAME));
    return true;
}

bool SQLiteAlertStorage::storeOfflineAlert(
    const std::string& token,
    const std::string& scheduledTime,
    const std::string& eventTime) {
    return storeOfflineAlertHelper(OFFLINE_ALERTS_DATABASE_VERSION_TWO, token, scheduledTime, eventTime);
}

bool SQLiteAlertStorage::storeOfflineAlertHelper(
    const int dbVersion,
    const std::string& token,
    const std::string& scheduledTime,
    const std::string& eventTime) {
    if (dbVersion != OFFLINE_ALERTS_DATABASE_VERSION_TWO) {
        ACSDK_ERROR(LX("storeOfflineAlertHelperFailed").d("Invalid db version", dbVersion));
        return false;
    }

    if (offlineAlertExists(dbVersion, token)) {
        ACSDK_WARN(LX("storeOfflineAlertHelper").m("Offline alert already exists.").d("token", token));
        return true;
    }

    std::string offlineAlertsTableName = OFFLINE_ALERTS_V2_TABLE_NAME;

    // clang-format off
    std::string sqlString = "INSERT INTO " +  offlineAlertsTableName + " (" +
                                    "id, token, scheduled_time_iso_8601, event_time_iso_8601" +
                                    ") VALUES (" +
                                    "?, ?, ?, ?" +
                                    ");";
    // clang-format on

    int id = 0;
    if (!getTableMaxIntValue(&m_db, offlineAlertsTableName, DATABASE_COLUMN_ID_NAME, &id)) {
        ACSDK_ERROR(LX("storeOfflineAlertHelperFailed").m("Cannot generate alert id."));
        return false;
    }
    id++;

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("storeOfflineAlertHelperFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    auto alertToken = token;
    auto scheduledTime_ISO = scheduledTime;
    auto eventTime_ISO = eventTime;

    /// If we are offine then we only want to bind a few parameters
    if (!statement->bindIntParameter(boundParam++, id) || !statement->bindStringParameter(boundParam++, alertToken) ||
        !statement->bindStringParameter(boundParam++, scheduledTime_ISO) ||
        !statement->bindStringParameter(boundParam, eventTime_ISO)) {
        ACSDK_ERROR(LX("storeOfflineAlertHelperFailed").m("Could not bind parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("storeOfflineAlertHelperFailed").m("Could not perform step."));
        return false;
    }

    ACSDK_DEBUG9(LX("Successfully stored offline alert to " + offlineAlertsTableName));
    return true;
}

bool SQLiteAlertStorage::isOfflineTableV1Legacy() {
    if (!m_db.tableExists(OFFLINE_ALERTS_TABLE_NAME)) {
        ACSDK_DEBUG5(LX("isOfflineTableV1Legacy").m("table does not exist " + OFFLINE_ALERTS_TABLE_NAME));
        return false;
    }

    auto statement = m_db.createStatement("PRAGMA table_info(" + OFFLINE_ALERTS_TABLE_NAME + ");");
    if (!statement || !statement->step()) {
        ACSDK_ERROR(LX("isOfflineTableV1LegacyFailedFailed").m("null statement or could not perform step"));
        return false;
    }

    std::string tableInfoColumnName = "name";
    std::string targetColumnName = "event_time_iso_8601";
    while (SQLITE_ROW == statement->getStepResult()) {
        int columnCount = statement->getColumnCount();
        for (int i = 0; i < columnCount; i++) {
            if (statement->getColumnName(i) == tableInfoColumnName) {
                auto columnName = statement->getColumnText(i);
                if (targetColumnName == columnName) {
                    ACSDK_DEBUG5(LX("isOfflineTableV1Legacy").m(targetColumnName + " column exists."));
                    return false;
                }
            }
        }
        statement->step();
    }
    return true;
}

/**
 * Loads asset data into the map.
 *
 * @param db The database object to read data from.
 * @param alertAssetsMap The map that will receive the assets.
 * @return @c true if data was loaded successfully
 */
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

/**
 * Reads the assets order from the database and stores this data on the given map.
 *
 * @param db The database object to read data from.
 * @param alertAssetOrderItemsMap The map that will receive data.
 * @return @c true if data was loaded successfully from the database
 */
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

bool SQLiteAlertStorage::loadHelper(
    int dbVersion,
    std::vector<std::shared_ptr<Alert>>* alertContainer,
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager) {
    if (!alertContainer) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Alert container parameter is nullptr."));
        return false;
    }

    if (dbVersion != ALERTS_DATABASE_VERSION_THREE) {
        ACSDK_ERROR(LX("loadHelperFailed").d("Invalid version", dbVersion));
        return false;
    }

    // Loads the assets map from the database
    std::map<int, std::vector<Alert::Asset>> alertAssetsMap;
    if (!loadAlertAssets(&m_db, &alertAssetsMap)) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not load alert assets."));
        return false;
    }

    // Loads the asset order item map from the database
    std::map<int, std::set<AssetOrderItem, AssetOrderItemCompare>> alertAssetOrderItemsMap;
    if (!loadAlertAssetPlayOrderItems(&m_db, &alertAssetOrderItemsMap)) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not load alert asset play order items."));
        return false;
    }

    std::string alertsTableName = ALERTS_V3_TABLE_NAME;
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
    std::string originalTime;
    std::string label;

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadHelperFailed").m("Could not perform step."));
        return false;
    }

    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement->getColumnName(i);

            if (DATABASE_COLUMN_ID_NAME == columnName) {
                id = statement->getColumnInt(i);
            } else if (DATABASE_COLUMN_TOKEN_NAME == columnName) {
                token = statement->getColumnText(i);
            } else if (DATABASE_COLUMN_TYPE_NAME == columnName) {
                type = statement->getColumnInt(i);
            } else if (DATABASE_COLUMN_STATE_NAME == columnName) {
                state = statement->getColumnInt(i);
            } else if (DATABASE_COLUMN_SCHEDULED_TIME_ISO_8601_NAME == columnName) {
                scheduledTime_ISO_8601 = statement->getColumnText(i);
            } else if (DATABASE_COLUMN_ASSET_LOOP_COUNT_NAME == columnName) {
                loopCount = statement->getColumnInt(i);
            } else if (DATABASE_COLUMN_ASSET_LOOP_PAUSE_MILLISECONDS_NAME == columnName) {
                loopPauseInMilliseconds = statement->getColumnInt(i);
            } else if (DATABASE_COLUMN_BACKGROUND_ASSET_NAME == columnName) {
                backgroundAssetId = statement->getColumnText(i);
            } else if (DATABASE_COLUMN_ORIGINAL_TIME_NAME == columnName) {
                originalTime = statement->getColumnText(i);
            } else if (DATABASE_COLUMN_LABEL_NAME == columnName) {
                label = statement->getColumnText(i);
            }
        }

        std::shared_ptr<Alert> alert;
        if (ALERT_EVENT_TYPE_ALARM == type) {
            alert = std::make_shared<Alarm>(
                m_alertsAudioFactory->alarmDefault(), m_alertsAudioFactory->alarmShort(), settingsManager);
        } else if (ALERT_EVENT_TYPE_TIMER == type) {
            alert = std::make_shared<Timer>(
                m_alertsAudioFactory->timerDefault(), m_alertsAudioFactory->timerShort(), settingsManager);
        } else if (ALERT_EVENT_TYPE_REMINDER == type) {
            alert = std::make_shared<Reminder>(
                m_alertsAudioFactory->reminderDefault(), m_alertsAudioFactory->reminderShort(), settingsManager);
        } else {
            ACSDK_ERROR(
                LX("loadHelperFailed").m("Could not instantiate an alert object.").d("type read from database", type));
            return false;
        }

        Alert::DynamicData dynamicData;
        Alert::StaticData staticData;
        alert->getAlertData(&staticData, &dynamicData);

        staticData.dbId = id;
        staticData.token = token;
        dynamicData.timePoint.setTime_ISO_8601(scheduledTime_ISO_8601);
        dynamicData.loopCount = loopCount;
        dynamicData.assetConfiguration.loopPause = std::chrono::milliseconds{loopPauseInMilliseconds};
        dynamicData.assetConfiguration.backgroundAssetId = backgroundAssetId;
        dynamicData.originalTime = originalTime;
        dynamicData.label = label;

        // alertAssetsMap is an alert id to asset map
        if (alertAssetsMap.find(id) != alertAssetsMap.end()) {
            for (auto& mapEntry : alertAssetsMap[id]) {
                dynamicData.assetConfiguration.assets[mapEntry.id] = mapEntry;
            }
        }

        // alertAssetOrderItemsMap is an alert id to asset order items map
        if (alertAssetOrderItemsMap.find(id) != alertAssetOrderItemsMap.end()) {
            for (auto& mapEntry : alertAssetOrderItemsMap[id]) {
                dynamicData.assetConfiguration.assetPlayOrderItems.push_back(mapEntry.name);
            }
        }

        if (!dbFieldToAlertState(state, &dynamicData.state)) {
            ACSDK_ERROR(LX("loadHelperFailed").m("Could not convert alert state."));
            return false;
        }

        if (!alert->setAlertData(&staticData, &dynamicData)) {
            ACSDK_ERROR(LX("loadHelperFailed").m("Could not set alert data."));
            return false;
        }

        alertContainer->push_back(alert);

        statement->step();
    }

    statement->finalize();

    return true;
}

bool SQLiteAlertStorage::load(
    std::vector<std::shared_ptr<Alert>>* alertContainer,
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager) {
    return loadHelper(ALERTS_DATABASE_VERSION_THREE, alertContainer, settingsManager);
}

bool SQLiteAlertStorage::loadOfflineAlerts(
    rapidjson::Value* alertContainer,
    rapidjson::Document::AllocatorType& allocator) {
    return loadOfflineAlertsHelper(OFFLINE_ALERTS_DATABASE_VERSION_TWO, alertContainer, allocator);
}

bool SQLiteAlertStorage::loadOfflineAlertsHelper(
    const int dbVersion,
    rapidjson::Value* alertContainer,
    rapidjson::Document::AllocatorType& allocator) {
    if (!alertContainer) {
        ACSDK_ERROR(LX("loadOfflineAlertsHelperFailed").m("nullAlertContainer"));
        return false;
    }

    if (dbVersion != OFFLINE_ALERTS_DATABASE_VERSION_ONE && dbVersion != OFFLINE_ALERTS_DATABASE_VERSION_TWO) {
        ACSDK_ERROR(LX("loadOfflineAlertsHelperFailed").d("Invalid db version", dbVersion));
        return false;
    }

    std::string offlineAlertsTableName = OFFLINE_ALERTS_TABLE_NAME;
    if (OFFLINE_ALERTS_DATABASE_VERSION_TWO == dbVersion) {
        offlineAlertsTableName = OFFLINE_ALERTS_V2_TABLE_NAME;
    }

    const std::string sqlString = "SELECT * FROM " + offlineAlertsTableName + ";";
    ACSDK_DEBUG9(LX("Loading offline alerts"));

    auto statement = m_db.createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("loadOfflineAlertsFailed").m("Could not create statement."));
        return false;
    }

    int id = 0;
    std::string token = "";
    std::string scheduledTime_ISO_8601 = "";
    std::string eventTime_ISO_8601 = "";

    if (!statement->step()) {
        ACSDK_ERROR(LX("loadOfflineAlertsFailed").m("Could not perform step."));
        return false;
    }

    while (SQLITE_ROW == statement->getStepResult()) {
        int numberColumns = statement->getColumnCount();

        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement->getColumnName(i);

            if ("id" == columnName) {
                id = statement->getColumnInt(i);
            } else if ("token" == columnName) {
                token = statement->getColumnText(i);
            } else if ("scheduled_time_iso_8601" == columnName) {
                scheduledTime_ISO_8601 = statement->getColumnText(i);
            } else if ("event_time_iso_8601" == columnName) {
                eventTime_ISO_8601 = statement->getColumnText(i);
            }
        }

        rapidjson::Value alertJson;
        alertJson.SetObject();
        alertJson.AddMember(StringRef(OFFLINE_STOPPED_ALERT_TOKEN_KEY), token, allocator);
        alertJson.AddMember(StringRef(OFFLINE_STOPPED_ALERT_SCHEDULED_TIME_KEY), scheduledTime_ISO_8601, allocator);
        alertJson.AddMember(StringRef(OFFLINE_STOPPED_ALERT_EVENT_TIME_KEY), eventTime_ISO_8601, allocator);
        alertJson.AddMember(StringRef(OFFLINE_STOPPED_ALERT_ID_KEY), id, allocator);

        alertContainer->PushBack(alertJson, allocator);

        statement->step();
    }

    return true;
}

bool SQLiteAlertStorage::modify(std::shared_ptr<Alert> alert) {
    if (!alert) {
        ACSDK_ERROR(LX("modifyFailed").m("Alert parameter is nullptr."));
        return false;
    }

    if (!alertExists(ALERTS_DATABASE_VERSION_THREE, alert->getToken())) {
        ACSDK_ERROR(LX("modifyFailed").m("Cannot modify alert").d("token", alert->getToken()));
        return false;
    }

    return modifyAlert(ALERTS_DATABASE_VERSION_THREE, alert);
}

bool SQLiteAlertStorage::modifyAlert(const int dbVersion, std::shared_ptr<Alert> alert) {
    if (dbVersion != ALERTS_DATABASE_VERSION_THREE) {
        ACSDK_ERROR(LX("modifyAlertFailed").d("UnsupportedDbVersion", dbVersion));
        return false;
    }

    std::string tableName = ALERTS_V3_TABLE_NAME;

    const std::string sqlString =
        "UPDATE " + tableName + " SET " + "state=?, scheduled_time_unix=?, scheduled_time_iso_8601=? " + "WHERE id=?;";

    int alertState = ALERT_STATE_SET;
    if (!alertStateToDbField(alert->getState(), &alertState)) {
        ACSDK_ERROR(LX("modifyFailed").m("Cannot convert state.").d("dbVersion", dbVersion));
        return false;
    }

    auto statement = m_db.createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX("modifyFailed").m("Could not create statement.").d("dbVersion", dbVersion));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindIntParameter(boundParam++, alertState) ||
        !statement->bindInt64Parameter(boundParam++, alert->getScheduledTime_Unix()) ||
        !statement->bindStringParameter(boundParam++, alert->getScheduledTime_ISO_8601()) ||
        !statement->bindIntParameter(boundParam++, alert->getId())) {
        ACSDK_ERROR(LX("modifyFailed").m("Could not bind a parameter.").d("dbVersion", dbVersion));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("modifyFailed").m("Could not perform step.").d("dbVersion", dbVersion));
        return false;
    }
    return true;
}

template <typename Task, typename... Args>
bool SQLiteAlertStorage::retryDataMigration(Task task, Args&&... args) {
    auto boundTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);
    size_t attempt = 0;
    m_waitRetryEvent.reset();
    while (attempt < RETRY_TIME_MAXIMUM) {
        /// migration succeeded.
        if (boundTask()) {
            break;
        }
        // wait before retry.
        if (m_waitRetryEvent.wait(m_retryTimer.calculateTimeToRetry(static_cast<int>(attempt)))) {
            break;
        }
        attempt++;
        ACSDK_DEBUG5(LX("retryDataMigration").d("attempt", attempt));
    }
    return attempt < RETRY_TIME_MAXIMUM;
}

/**
 * A utility function to delete alert records from the database for a given alert id.
 * This function will clean up records in the alerts table.
 *
 * @param dbVersion The version of the alerts table.
 * @param db The database object.
 * @param alertId The alert id of the alert to be deleted.
 * @return Whether the delete operation was successful.
 */
static bool eraseAlert(int dbVersion, SQLiteDatabase* db, int alertId) {
    if (dbVersion != ALERTS_DATABASE_VERSION_THREE) {
        ACSDK_ERROR(LX("eraseAlertFailed").d("UnsupportedDbVersion", dbVersion));
        return false;
    }

    std::string tableName = ALERTS_V3_TABLE_NAME;
    const std::string sqlString = "DELETE FROM " + tableName + " WHERE id=?;";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseAlertFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindIntParameter(boundParam, alertId)) {
        ACSDK_ERROR(LX("eraseAlertFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseAlertFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

/**
 * A utility function to delete alert records from the offline database for a given alert id.
 * This function will clean up records in the offline alerts table.
 *
 * @param db The database object.
 * @param token The AVS token to identify the alert.
 * @return Whether the delete operation was successful.
 */
static bool eraseOfflineAlert(const int dbVersion, SQLiteDatabase* db, const std::string& token) {
    if (dbVersion != OFFLINE_ALERTS_DATABASE_VERSION_TWO) {
        ACSDK_ERROR(LX("eraseOfflineAlertFailed").d("Invalid db version", dbVersion));
        return false;
    }

    std::string offlineAlertsTableName = OFFLINE_ALERTS_V2_TABLE_NAME;

    const std::string sqlString = "DELETE FROM " + offlineAlertsTableName + " WHERE token=?;";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("eraseOfflineAlertFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, token)) {
        ACSDK_ERROR(LX("eraseOfflineAlertFailed").m("Could not bind a parameter."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("eraseOfflineAlertFailed").m("Could not perform step."));
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

    if (!eraseAlert(ALERTS_DATABASE_VERSION_THREE, db, alertId)) {
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

    if (!alertExists(ALERTS_DATABASE_VERSION_THREE, alert->getToken())) {
        ACSDK_ERROR(LX("eraseFailed").m("Cannot delete alert - not in database.").d("token", alert->getToken()));
        return false;
    }

    return eraseAlertByAlertId(&m_db, alert->getId());
}

bool SQLiteAlertStorage::eraseOffline(const std::string& token, int id) {
    return eraseOfflineHelper(OFFLINE_ALERTS_DATABASE_VERSION_TWO, token);
}

bool SQLiteAlertStorage::eraseOfflineHelper(const int dbVersion, const std::string& token) {
    if (dbVersion != OFFLINE_ALERTS_DATABASE_VERSION_TWO) {
        ACSDK_ERROR(LX("eraseOfflineHelperFailed").d("Invalid db version", dbVersion));
        return false;
    }

    if (!offlineAlertExists(dbVersion, token)) {
        ACSDK_WARN(LX("eraseOfflineHelper").m("Offline alert does not exist.").d("token", token));
        return true;
    }

    if (!eraseOfflineAlert(dbVersion, &m_db, token)) {
        ACSDK_ERROR(LX("eraseOfflineHelperFailed").m("Could not erase offlineAlerts table items."));
        return false;
    }

    return true;
}

bool SQLiteAlertStorage::bulkErase(const std::list<std::shared_ptr<Alert>>& alertList) {
    if (alertList.empty()) {
        return true;
    }

    auto transaction = m_db.beginTransaction();
    if (!transaction) {
        ACSDK_ERROR(LX("bulkEraseFailed").d("reason", "Failed to begin transaction."));
        return false;
    }

    for (auto& alert : alertList) {
        if (!erase(alert)) {
            ACSDK_ERROR(LX("bulkEraseFailed").d("reason", "Failed to erase alert"));
            if (!transaction->rollback()) {
                ACSDK_ERROR(LX("bulkEraseFailed").d("reason", "Failed to rollback alerts storage changes"));
            }
            return false;
        }
    }

    if (!transaction->commit()) {
        ACSDK_ERROR(LX("bulkEraseFailed").d("reason", "Failed to commit alerts storage changes"));
        return false;
    }
    return true;
}

bool SQLiteAlertStorage::clearDatabase() {
    m_waitRetryEvent.wakeUp();
    std::vector<std::string> tablesToClear = {ALERTS_V3_TABLE_NAME,
                                              ALERT_ASSETS_TABLE_NAME,
                                              ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME,
                                              OFFLINE_ALERTS_V2_TABLE_NAME};

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

    if (!getNumberTableRows(db, ALERTS_V3_TABLE_NAME, &numberAlerts)) {
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
    load(&alerts, nullptr);
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
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
