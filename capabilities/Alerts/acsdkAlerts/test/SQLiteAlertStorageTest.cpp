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

#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/MockMetricRecorder.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include <AVSCommon/SDKInterfaces/Audio/MockAlertsAudioFactory.h>

#include "acsdkAlerts/Alert.h"
#include "acsdkAlerts/Storage/SQLiteAlertStorage.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace test {

using namespace acsdkAlerts::storage;
using namespace alexaClientSDK::storage::sqliteStorage;
using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces::audio::test;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::file;
using namespace avsCommon::utils::metrics::test;
using namespace avsCommon::utils::string;
using namespace rapidjson;
using namespace ::testing;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteAlertStorageTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of database file for testing.
static const std::string TEST_DATABASE_FILE_NAME = "SQLiteAlertStorageTest.db";

// clang-format off
static const std::string VALID_ALERTS_DB_CONFIG_JSON = R"(
    {
        "alertsCapabilityAgent": {
            "databaseFilePath": ")" + TEST_DATABASE_FILE_NAME + R"("
        }
    }
)";
// clang-format on

// clang-format off
static const std::string INVALID_ALERTS_DB_CONFIG_JSON = R"(
    {
        "alertsCapabilityAgent": {
            "databaseFilePath": ""
        }
    }
)";
// clang-format on

/// The name of the alerts (v2) table.
static const std::string ALERTS_V2_TABLE_NAME = "alerts_v2";

/// The name of the alerts (v3) table.
static const std::string ALERTS_V3_TABLE_NAME = "alerts_v3";

/// The SQL string to create the alerts table.
// clang-format off
static const std::string CREATE_ALERTS_V2_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        ALERTS_V2_TABLE_NAME + " (" +
        "id INT PRIMARY KEY NOT NULL," +
        "token TEXT NOT NULL," +
        "type INT NOT NULL," +
        "state INT NOT NULL," +
        "scheduled_time_unix INT NOT NULL," +
        "scheduled_time_iso_8601 TEXT NOT NULL," +
        "asset_loop_count INT NOT NULL," +
        "asset_loop_pause_milliseconds INT NOT NULL," +
        "background_asset TEXT NOT NULL);";
// clang-format on

// clang-format off
static const std::string CREATE_ALERTS_V3_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        ALERTS_V3_TABLE_NAME + " (" +
        "id INT PRIMARY KEY NOT NULL," +
        "token TEXT NOT NULL," +
        "type INT NOT NULL," +
        "state INT NOT NULL," +
        "scheduled_time_unix INT NOT NULL," +
        "scheduled_time_iso_8601 TEXT NOT NULL," +
        "asset_loop_count INT NOT NULL," +
        "asset_loop_pause_milliseconds INT NOT NULL," +
        "background_asset TEXT NOT NULL," +
        "original_time TEXT NOT NULL," +
        "label TEXT NOT NULL," +
        "created_time_iso_8601 TEXT NOT NULL);";
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

/// The SQL string to create the offline alerts table.
// clang-format off
static const std::string CREATE_OFFLINE_ALERTS_V1_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
        OFFLINE_ALERTS_TABLE_NAME + " (" +
        "id INT PRIMARY KEY NOT NULL," +
        "token TEXT NOT NULL," +
        "scheduled_time_iso_8601 TEXT NOT NULL);";
// clang-format on

// clang-format off
static const std::string CREATE_OFFLINE_ALERTS_V2_TABLE_SQL_STRING = std::string("CREATE TABLE ") +
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

/// Constants for Alerts.
/// Type of the alert for testing.
static const std::string TEST_ALERT_TYPE_ALARM = "ALARM";
static const std::string TEST_ALERT_TYPE_TIMER = "TIMER";
static const std::string TEST_ALERT_TYPE_REMINDER = "REMINDER";

/// Schduled time string in ISO 8601 format.
static const std::string SCHEDULED_TIME_ISO_STRING = "2008-08-08T08:00:00+0000";
static const std::string SCHEDULED_TIME_ISO_STRING_ALARM = "2020-08-08T08:00:00+0000";
static const std::string SCHEDULED_TIME_ISO_STRING_TIMER = "2020-08-09T08:00:00+0000";
static const std::string SCHEDULED_TIME_ISO_STRING_REMINDER = "2020-08-10T08:00:00+0000";

/// Alerts token.
static const std::string TOKEN_ALARM = "token-alarm";
static const std::string TOKEN_TIMER = "token-timer";
static const std::string TOKEN_REMINDER = "token-reminder";

/// Original time.
static const std::string ORIGINAL_TIME_ALARM = "16:00:00.000";
static const std::string ORIGINAL_TIME_REMINDER = "18:00:00.000";

/// Label for the alerts.
static const std::string LABEL_TIMER = "coffee";
static const std::string LABEL_REMINDER = "walk the dog";

/**
 * Mock class for @c Alert.
 */
class MockAlert : public Alert {
public:
    MockAlert(const std::string& typeName) :
            Alert(defaultAudioFactory, shortAudioFactory, nullptr),
            m_alertType{typeName} {
    }

    std::string getTypeName() const override {
        return m_alertType;
    }

private:
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> defaultAudioFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream("default audio")),
            avsCommon::utils::MediaType::MPEG);
    }
    static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> shortAudioFactory() {
        return std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>(
            std::unique_ptr<std::stringstream>(new std::stringstream("short audio")),
            avsCommon::utils::MediaType::MPEG);
    }
    const std::string m_alertType;
};

class SQLiteAlertStorageTest : public Test {
public:
    /// Constructor,
    SQLiteAlertStorageTest();

    /// Setup for tests.
    void SetUp() override;

    /// Cleanup for test.
    void TearDown() override;

    /// Sets up alerts database.
    void setUpDatabase();

    /// Utility function to create the alert;
    std::shared_ptr<MockAlert> createAlert(const std::string& alertType);

    /// Utility function to check if the alert exists in a specific table.
    bool alertExists(SQLiteDatabase* db, const std::string& tableName, const std::string& token);

    /// Utility function to check if a table is empty.
    bool isTableEmpty(SQLiteDatabase* db, const std::string& tableName);

    /// The @c SQLiteAlertStorage instance to test.
    std::shared_ptr<SQLiteAlertStorage> m_alertStorage;

    /// The @c AlertsAudioFactoryInterface instance to provide audio resources.
    std::shared_ptr<MockAlertsAudioFactory> m_mockAlertsAudioFactory;

    /// The @c MetricRecorderInterface instance to record metrics.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_mockMetricRecorder;
};

void SQLiteAlertStorageTest::SetUp() {
    ASSERT_NE(m_alertStorage, nullptr);
}

void SQLiteAlertStorageTest::TearDown() {
    ConfigurationNode::uninitialize();
    if (m_alertStorage) {
        m_alertStorage->close();
    }
    m_alertStorage.reset();
    if (fileExists(TEST_DATABASE_FILE_NAME)) {
        removeFile(TEST_DATABASE_FILE_NAME);
    }
}

void SQLiteAlertStorageTest::setUpDatabase() {
    if (!m_alertStorage) {
        return;
    }
    ASSERT_TRUE(m_alertStorage->createDatabase());
}

SQLiteAlertStorageTest::SQLiteAlertStorageTest() {
    m_mockAlertsAudioFactory = std::make_shared<NiceMock<MockAlertsAudioFactory>>();
    m_mockMetricRecorder = std::make_shared<NiceMock<MockMetricRecorder>>();
    auto configJson = std::make_shared<std::istringstream>(VALID_ALERTS_DB_CONFIG_JSON);
    ConfigurationNode::initialize({configJson});
    m_alertStorage =
        SQLiteAlertStorage::create(ConfigurationNode::getRoot(), m_mockAlertsAudioFactory, m_mockMetricRecorder);
}

std::shared_ptr<MockAlert> SQLiteAlertStorageTest::createAlert(const std::string& alertType) {
    std::shared_ptr<MockAlert> alert = std::make_shared<MockAlert>("");
    if (TEST_ALERT_TYPE_ALARM == alertType) {
        alert = std::make_shared<MockAlert>(TEST_ALERT_TYPE_ALARM);
        Alert::StaticData staticDataAlarm;
        Alert::DynamicData dynamicDataAlarm;
        staticDataAlarm.token = TOKEN_ALARM;
        dynamicDataAlarm.timePoint.setTime_ISO_8601(SCHEDULED_TIME_ISO_STRING_ALARM);
        dynamicDataAlarm.loopCount = 1;
        dynamicDataAlarm.originalTime = ORIGINAL_TIME_ALARM;
        alert->setAlertData(&staticDataAlarm, &dynamicDataAlarm);
        return alert;
    }

    if (TEST_ALERT_TYPE_TIMER == alertType) {
        alert = std::make_shared<MockAlert>(TEST_ALERT_TYPE_TIMER);
        Alert::StaticData staticDataTimer;
        Alert::DynamicData dynamicDataTimer;
        staticDataTimer.token = TOKEN_TIMER;
        dynamicDataTimer.timePoint.setTime_ISO_8601(SCHEDULED_TIME_ISO_STRING_TIMER);
        dynamicDataTimer.loopCount = 2;
        dynamicDataTimer.label = LABEL_TIMER;
        alert->setAlertData(&staticDataTimer, &dynamicDataTimer);
        return alert;
    }

    if (TEST_ALERT_TYPE_REMINDER == alertType) {
        alert = std::make_shared<MockAlert>(TEST_ALERT_TYPE_REMINDER);
        Alert::StaticData staticDataReminder;
        Alert::DynamicData dynamicDataReminder;
        staticDataReminder.token = TOKEN_REMINDER;
        dynamicDataReminder.timePoint.setTime_ISO_8601(SCHEDULED_TIME_ISO_STRING_REMINDER);
        dynamicDataReminder.loopCount = 3;
        dynamicDataReminder.originalTime = ORIGINAL_TIME_REMINDER;
        dynamicDataReminder.label = LABEL_REMINDER;
        alert->setAlertData(&staticDataReminder, &dynamicDataReminder);
        return alert;
    }
    return alert;
}

bool SQLiteAlertStorageTest::alertExists(SQLiteDatabase* db, const std::string& tableName, const std::string& token) {
    const std::string sqlString = "SELECT COUNT(*) FROM " + tableName + " WHERE token=?;";
    auto statement = db->createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Create statement failed."));
        return false;
    }
    int boundParam = 1;
    if (!statement->bindStringParameter(boundParam, token)) {
        ACSDK_ERROR(LX("alertExistsFailed").m("bindStringParameter failed."));
        return false;
    }
    if (!statement->step()) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Perform step failed."));
        return false;
    }
    int countValue = 0;
    if (!stringToInt(statement->getColumnText(0).c_str(), &countValue)) {
        ACSDK_ERROR(LX("alertExistsFailed").m("stringToInt failed"));
        return false;
    }
    return countValue > 0;
}

bool SQLiteAlertStorageTest::isTableEmpty(SQLiteDatabase* db, const std::string& tableName) {
    const std::string sqlString = "SELECT COUNT(*) FROM " + tableName + ";";
    auto statement = db->createStatement(sqlString);
    if (!statement) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Create statement failed."));
        return false;
    }
    if (!statement->step()) {
        ACSDK_ERROR(LX("alertExistsFailed").m("Perform step failed."));
        return false;
    }
    int countValue = 0;
    if (!stringToInt(statement->getColumnText(0).c_str(), &countValue)) {
        ACSDK_ERROR(LX("alertExistsFailed").m("stringToInt failed"));
        return false;
    }
    return !(countValue > 0);
}

/**
 * Test create with empty @c ConfigurationNode.
 */
TEST_F(SQLiteAlertStorageTest, test_emptyDbConfiguration) {
    ConfigurationNode::uninitialize();
    ConfigurationNode::initialize(std::vector<std::shared_ptr<std::istream>>());
    auto alertStorage =
        SQLiteAlertStorage::create(ConfigurationNode::getRoot(), m_mockAlertsAudioFactory, m_mockMetricRecorder);
    ASSERT_EQ(alertStorage, nullptr);
}

/**
 * Test create with invalid alerts database configuration.
 */
TEST_F(SQLiteAlertStorageTest, test_invalidDbConfiguration) {
    ConfigurationNode::uninitialize();
    auto configJson = std::make_shared<std::istringstream>(INVALID_ALERTS_DB_CONFIG_JSON);
    ConfigurationNode::initialize({configJson});
    auto alertStorage =
        SQLiteAlertStorage::create(ConfigurationNode::getRoot(), m_mockAlertsAudioFactory, m_mockMetricRecorder);
    ASSERT_EQ(alertStorage, nullptr);
}

/**
 * Test create with null @c AlertsAudioFactoryInterface.
 */
TEST_F(SQLiteAlertStorageTest, test_nullAlertsAudioFactory) {
    auto alertStorage = SQLiteAlertStorage::create(ConfigurationNode::getRoot(), nullptr, m_mockMetricRecorder);
    ASSERT_EQ(alertStorage, nullptr);
}

/**
 * Test create with null @c MetricRecorderInterface. It's okay if metric recorder instance is not provided.
 */
TEST_F(SQLiteAlertStorageTest, test_nullMetricRecorder) {
    auto alertStorage = SQLiteAlertStorage::create(ConfigurationNode::getRoot(), m_mockAlertsAudioFactory, nullptr);
    ASSERT_NE(alertStorage, nullptr);
}

/**
 * Test if open existing database succeeds.
 */
TEST_F(SQLiteAlertStorageTest, test_openExistingDatabaseSucceeds) {
    setUpDatabase();
    m_alertStorage->close();
    ASSERT_TRUE(m_alertStorage->open());
}

/**
 * Test if create existing database fails.
 */
TEST_F(SQLiteAlertStorageTest, test_createExistingDatabaseFails) {
    setUpDatabase();
    ASSERT_FALSE(m_alertStorage->createDatabase());
}

/**
 * Test if open succeeds when latest alerts table does not exist yet.
 */
TEST_F(SQLiteAlertStorageTest, test_openDatabaseWhenAlertsTableIsMissing) {
    setUpDatabase();
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase db(TEST_DATABASE_FILE_NAME);
    ASSERT_TRUE(db.open());
    const std::string dropTableSqlString = "DROP TABLE IF EXISTS " + ALERTS_V3_TABLE_NAME + ";";
    ASSERT_TRUE(db.performQuery(dropTableSqlString));
    ASSERT_FALSE(db.tableExists(ALERTS_V3_TABLE_NAME));
    m_alertStorage->close();

    /// missing table will be created on open.
    ASSERT_TRUE(m_alertStorage->open());
    ASSERT_TRUE(db.tableExists(ALERTS_V3_TABLE_NAME));
    db.close();
}

/**
 * Test if open succeeds when latest offline alerts table does not exist yet.
 */
TEST_F(SQLiteAlertStorageTest, test_openDatabaseWhenOfflineAlertsTableIsMissing) {
    setUpDatabase();
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase db(TEST_DATABASE_FILE_NAME);
    ASSERT_TRUE(db.open());
    const std::string dropTableSqlString = "DROP TABLE IF EXISTS " + OFFLINE_ALERTS_V2_TABLE_NAME + ";";
    ASSERT_TRUE(db.performQuery(dropTableSqlString));
    ASSERT_FALSE(db.tableExists(OFFLINE_ALERTS_V2_TABLE_NAME));
    m_alertStorage->close();

    /// missing table will be created on open.
    ASSERT_TRUE(m_alertStorage->open());
    ASSERT_TRUE(db.tableExists(OFFLINE_ALERTS_V2_TABLE_NAME));
    db.close();
}

/**
 * Test if open succeeds when 'alertAsset' and 'alertAssetPlayOrderItems' tables do not exist yet.
 */
TEST_F(SQLiteAlertStorageTest, test_openDatabaseWhenAssetTablesAreMissing) {
    setUpDatabase();
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase db(TEST_DATABASE_FILE_NAME);
    ASSERT_TRUE(db.open());
    const std::string dropAlertAssetsTable = "DROP TABLE IF EXISTS " + ALERT_ASSETS_TABLE_NAME + ";";
    const std::string dropAlertAssetPlayOrderItemsTable =
        "DROP TABLE IF EXISTS " + ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME + ";";
    ASSERT_TRUE(db.performQuery(dropAlertAssetsTable));
    ASSERT_TRUE(db.performQuery(dropAlertAssetPlayOrderItemsTable));
    ASSERT_FALSE(db.tableExists(ALERT_ASSETS_TABLE_NAME));
    ASSERT_FALSE(db.tableExists(ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME));
    m_alertStorage->close();

    /// missing tables will be created on open.
    ASSERT_TRUE(m_alertStorage->open());
    ASSERT_TRUE(db.tableExists(ALERT_ASSETS_TABLE_NAME));
    ASSERT_TRUE(db.tableExists(ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_NAME));
    db.close();
}

/**
 * Test data migration from alerts v2 table to v3.
 */
TEST_F(SQLiteAlertStorageTest, test_migrateAlertFromV2ToV3) {
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase db(TEST_DATABASE_FILE_NAME);
    ASSERT_TRUE(db.initialize());
    /// create alerts v2 table.
    ASSERT_TRUE(db.performQuery(CREATE_ALERTS_V2_TABLE_SQL_STRING));
    ASSERT_TRUE(db.performQuery(CREATE_ALERT_ASSETS_TABLE_SQL_STRING));
    ASSERT_TRUE(db.performQuery(CREATE_OFFLINE_ALERTS_V2_TABLE_SQL_STRING));
    ASSERT_TRUE(db.performQuery(CREATE_ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_SQL_STRING));
    const std::string storeToAlertV2 =
        "INSERT INTO " + ALERTS_V2_TABLE_NAME + " (" +
        "id, token, type, state, scheduled_time_unix, scheduled_time_iso_8601, asset_loop_count, " +
        "asset_loop_pause_milliseconds, background_asset" + ") VALUES (" +
        "?, ?, ?, ?, ?, ?, ?, "
        "?, ?"
        ");";
    auto storeStatement = db.createStatement(storeToAlertV2);
    ASSERT_NE(storeStatement, nullptr);
    int id = 1;
    std::string token = "token-abc";
    int type = 1;
    int state = 1;
    int64_t scheduledTime_Unix = 1218207600;
    std::string scheduledTime_ISO_8601 = SCHEDULED_TIME_ISO_STRING;
    int loopCount = 3;
    int loopPauseInMilliseconds = 0;
    std::string backgroundAssetId = "assetId";
    int boundParam = 1;
    bool bindResult = storeStatement->bindIntParameter(boundParam++, id) &&
                      storeStatement->bindStringParameter(boundParam++, token) &&
                      storeStatement->bindIntParameter(boundParam++, type) &&
                      storeStatement->bindIntParameter(boundParam++, state) &&
                      storeStatement->bindInt64Parameter(boundParam++, scheduledTime_Unix) &&
                      storeStatement->bindStringParameter(boundParam++, scheduledTime_ISO_8601) &&
                      storeStatement->bindIntParameter(boundParam++, loopCount) &&
                      storeStatement->bindIntParameter(boundParam++, loopPauseInMilliseconds) &&
                      storeStatement->bindStringParameter(boundParam, backgroundAssetId);
    ASSERT_TRUE(bindResult);
    ASSERT_TRUE(storeStatement->step());
    storeStatement->finalize();
    ASSERT_FALSE(db.tableExists(ALERTS_V3_TABLE_NAME));

    /// data migration happens on open.
    ASSERT_TRUE(m_alertStorage->open());
    ASSERT_TRUE(db.tableExists(ALERTS_V3_TABLE_NAME));

    /// verify if the alert has been migrated successfully from table v2 to v3.
    const std::string loadAlertsV3 = "SELECT * FROM " + ALERTS_V3_TABLE_NAME + ";";
    auto loadStatement = db.createStatement(loadAlertsV3);
    ASSERT_NE(loadStatement, nullptr);
    ASSERT_TRUE(loadStatement->step());
    while (SQLITE_ROW == loadStatement->getStepResult()) {
        int numberOfColumns = loadStatement->getColumnCount();
        for (int i = 0; i < numberOfColumns; i++) {
            std::string columnName = loadStatement->getColumnName(i);

            if ("id" == columnName) {
                ASSERT_EQ(id, loadStatement->getColumnInt(i));
            } else if ("token" == columnName) {
                ASSERT_EQ(token, loadStatement->getColumnText(i));
            } else if ("type" == columnName) {
                ASSERT_EQ(type, loadStatement->getColumnInt(i));
            } else if ("state" == columnName) {
                ASSERT_EQ(state, loadStatement->getColumnInt(i));
            } else if ("scheduled_time_unix" == columnName) {
                ASSERT_EQ(scheduledTime_Unix, loadStatement->getColumnInt64(i));
            } else if ("scheduled_time_iso_8601" == columnName) {
                ASSERT_EQ(scheduledTime_ISO_8601, loadStatement->getColumnText(i));
            } else if ("asset_loop_count" == columnName) {
                ASSERT_EQ(loopCount, loadStatement->getColumnInt(i));
            } else if ("asset_loop_pause_milliseconds" == columnName) {
                ASSERT_EQ(loopPauseInMilliseconds, loadStatement->getColumnInt(i));
            } else if ("background_asset" == columnName) {
                ASSERT_EQ(backgroundAssetId, loadStatement->getColumnText(i));
            }
        }
        loadStatement->step();
    }
    loadStatement->finalize();
    db.close();
}

/**
 * Test data migration from offline alerts v1 table to v2.
 */
TEST_F(SQLiteAlertStorageTest, test_migrateOfflineAlertFromV1ToV2) {
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase db(TEST_DATABASE_FILE_NAME);
    ASSERT_TRUE(db.initialize());
    /// create offline alerts v1 table.
    ASSERT_TRUE(db.performQuery(CREATE_ALERTS_V3_TABLE_SQL_STRING));
    ASSERT_TRUE(db.performQuery(CREATE_OFFLINE_ALERTS_V1_TABLE_SQL_STRING));
    ASSERT_TRUE(db.performQuery(CREATE_ALERT_ASSETS_TABLE_SQL_STRING));
    ASSERT_TRUE(db.performQuery(CREATE_ALERT_ASSET_PLAY_ORDER_ITEMS_TABLE_SQL_STRING));
    const std::string storeToOfflineAlertV1 = "INSERT INTO " + OFFLINE_ALERTS_TABLE_NAME + " (" +
                                              "id, token, scheduled_time_iso_8601"
                                              ") VALUES (" +
                                              "?, ?, ?"
                                              ");";

    auto storeStatement = db.createStatement(storeToOfflineAlertV1);
    ASSERT_NE(storeStatement, nullptr);
    int id = 1;
    std::string token = "token-offline";
    std::string scheduledTime_ISO_8601 = SCHEDULED_TIME_ISO_STRING;
    int boundParam = 1;
    bool bindResult = storeStatement->bindIntParameter(boundParam++, id) &&
                      storeStatement->bindStringParameter(boundParam++, token) &&
                      storeStatement->bindStringParameter(boundParam, scheduledTime_ISO_8601);
    ASSERT_TRUE(bindResult);
    ASSERT_TRUE(storeStatement->step());
    storeStatement->finalize();
    ASSERT_FALSE(db.tableExists(OFFLINE_ALERTS_V2_TABLE_NAME));

    /// data migration happens on open.
    ASSERT_TRUE(m_alertStorage->open());
    ASSERT_TRUE(db.tableExists(OFFLINE_ALERTS_V2_TABLE_NAME));

    /// verify if the offline alert has been migrated successfully from table v1 to v2.
    const std::string loadOfflineAlertsV2 = "SELECT * FROM " + OFFLINE_ALERTS_V2_TABLE_NAME + ";";
    auto loadStatement = db.createStatement(loadOfflineAlertsV2);
    ASSERT_NE(loadStatement, nullptr);
    ASSERT_TRUE(loadStatement->step());
    while (SQLITE_ROW == loadStatement->getStepResult()) {
        int numberOfColumns = loadStatement->getColumnCount();
        for (int i = 0; i < numberOfColumns; i++) {
            std::string columnName = loadStatement->getColumnName(i);

            if ("id" == columnName) {
                ASSERT_EQ(id, loadStatement->getColumnInt(i));
            } else if ("token" == columnName) {
                ASSERT_EQ(token, loadStatement->getColumnText(i));
            } else if ("scheduled_time_iso_8601" == columnName) {
                ASSERT_EQ(scheduledTime_ISO_8601, loadStatement->getColumnText(i));
            }
        }
        loadStatement->step();
    }
    loadStatement->finalize();
    db.close();
}

/**
 * Test store and load alerts.
 */
TEST_F(SQLiteAlertStorageTest, test_storeAndLoadAlerts) {
    setUpDatabase();
    auto alarm = createAlert(TEST_ALERT_TYPE_ALARM);
    auto timer = createAlert(TEST_ALERT_TYPE_TIMER);
    auto reminder = createAlert(TEST_ALERT_TYPE_REMINDER);
    /// store alerts
    ASSERT_TRUE(m_alertStorage->store(alarm));
    ASSERT_TRUE(m_alertStorage->store(timer));
    ASSERT_TRUE(m_alertStorage->store(reminder));
    /// load alerts
    std::vector<std::shared_ptr<Alert>> alerts;
    m_alertStorage->load(&alerts, nullptr);
    /// verify
    ASSERT_EQ(static_cast<int>(alerts.size()), 3);
    for (auto& alert : alerts) {
        if (TEST_ALERT_TYPE_ALARM == alert->getTypeName()) {
            Alert::StaticData staticData;
            Alert::DynamicData dynamicData;
            alert->getAlertData(&staticData, &dynamicData);
            ASSERT_EQ(dynamicData.timePoint.getTime_ISO_8601(), SCHEDULED_TIME_ISO_STRING_ALARM);
            ASSERT_EQ(dynamicData.loopCount, 1);
            ASSERT_EQ(dynamicData.originalTime, ORIGINAL_TIME_ALARM);
            ASSERT_EQ(staticData.token, TOKEN_ALARM);
        } else if (TEST_ALERT_TYPE_TIMER == alert->getTypeName()) {
            Alert::StaticData staticData;
            Alert::DynamicData dynamicData;
            alert->getAlertData(&staticData, &dynamicData);
            ASSERT_EQ(dynamicData.timePoint.getTime_ISO_8601(), SCHEDULED_TIME_ISO_STRING_TIMER);
            ASSERT_EQ(dynamicData.loopCount, 2);
            ASSERT_EQ(dynamicData.label, LABEL_TIMER);
            ASSERT_EQ(staticData.token, TOKEN_TIMER);
        } else if (TEST_ALERT_TYPE_REMINDER == alert->getTypeName()) {
            Alert::StaticData staticData;
            Alert::DynamicData dynamicData;
            alert->getAlertData(&staticData, &dynamicData);
            ASSERT_EQ(dynamicData.timePoint.getTime_ISO_8601(), SCHEDULED_TIME_ISO_STRING_REMINDER);
            ASSERT_EQ(dynamicData.loopCount, 3);
            ASSERT_EQ(dynamicData.label, LABEL_REMINDER);
            ASSERT_EQ(dynamicData.originalTime, ORIGINAL_TIME_REMINDER);
            ASSERT_EQ(staticData.token, TOKEN_REMINDER);
        }
    }
}

/**
 * Test modify an alert.
 */
TEST_F(SQLiteAlertStorageTest, test_modifyAlerts) {
    setUpDatabase();
    auto alarm = createAlert(TEST_ALERT_TYPE_ALARM);
    ASSERT_TRUE(m_alertStorage->store(alarm));
    std::vector<std::shared_ptr<Alert>> alerts;
    m_alertStorage->load(&alerts, nullptr);
    ASSERT_EQ(static_cast<int>(alerts.size()), 1);

    auto alert = alerts.back();
    Alert::DynamicData dynamicData;
    Alert::StaticData staticData;
    alert->getAlertData(&staticData, &dynamicData);
    ASSERT_EQ(dynamicData.timePoint.getTime_ISO_8601(), SCHEDULED_TIME_ISO_STRING_ALARM);
    /// update schedule time.
    dynamicData.timePoint.setTime_ISO_8601(SCHEDULED_TIME_ISO_STRING);
    alert->setAlertData(&staticData, &dynamicData);
    ASSERT_TRUE(m_alertStorage->modify(alert));

    /// verify the value after modification.
    alerts.clear();
    m_alertStorage->load(&alerts, nullptr);
    ASSERT_EQ(static_cast<int>(alerts.size()), 1);
    alert = alerts.back();
    alert->getAlertData(&staticData, &dynamicData);
    ASSERT_EQ(dynamicData.timePoint.getTime_ISO_8601(), SCHEDULED_TIME_ISO_STRING);

    /// modify should fail if the alert does not exist.
    alert = std::make_shared<MockAlert>(TEST_ALERT_TYPE_ALARM);
    staticData.token = "token-invalid";
    alert->setAlertData(&staticData, nullptr);
    ASSERT_FALSE(m_alertStorage->modify(alert));
}

/**
 * Test erase an alert.
 */
TEST_F(SQLiteAlertStorageTest, test_eraseAlert) {
    setUpDatabase();
    auto alarm = createAlert(TEST_ALERT_TYPE_ALARM);
    ASSERT_TRUE(m_alertStorage->store(alarm));
    std::vector<std::shared_ptr<Alert>> alerts;
    m_alertStorage->load(&alerts, nullptr);
    ASSERT_EQ(static_cast<int>(alerts.size()), 1);
    alerts.clear();

    /// start to erase
    ASSERT_TRUE(m_alertStorage->erase(alarm));
    m_alertStorage->load(&alerts, nullptr);
    ASSERT_TRUE(alerts.empty());
}

/**
 * Test bulkErase alerts.
 */
TEST_F(SQLiteAlertStorageTest, test_bulkEraseAlert) {
    setUpDatabase();
    auto alarm = createAlert(TEST_ALERT_TYPE_ALARM);
    auto timer = createAlert(TEST_ALERT_TYPE_TIMER);
    auto reminder = createAlert(TEST_ALERT_TYPE_REMINDER);

    ASSERT_TRUE(m_alertStorage->store(alarm));
    ASSERT_TRUE(m_alertStorage->store(timer));
    ASSERT_TRUE(m_alertStorage->store(reminder));
    std::vector<std::shared_ptr<Alert>> alerts;
    m_alertStorage->load(&alerts, nullptr);
    ASSERT_EQ(static_cast<int>(alerts.size()), 3);
    alerts.clear();

    /// start to bulkErase
    ASSERT_TRUE(m_alertStorage->bulkErase({alarm, timer, reminder}));
    m_alertStorage->load(&alerts, nullptr);
    ASSERT_TRUE(alerts.empty());
}

/**
 * Test store and load offline alerts.
 */
TEST_F(SQLiteAlertStorageTest, test_storeAndLoadOfflineAlerts) {
    setUpDatabase();
    const std::string offlineToken1 = "token-offline1";
    const std::string offlineToken2 = "token-offline2";
    const std::string offlineToken3 = "token-offline3";
    /// store offline alerts.
    ASSERT_TRUE(
        m_alertStorage->storeOfflineAlert(offlineToken1, SCHEDULED_TIME_ISO_STRING, SCHEDULED_TIME_ISO_STRING_ALARM));
    ASSERT_TRUE(m_alertStorage->storeOfflineAlert(
        offlineToken2, SCHEDULED_TIME_ISO_STRING_ALARM, SCHEDULED_TIME_ISO_STRING_TIMER));
    ASSERT_TRUE(m_alertStorage->storeOfflineAlert(
        offlineToken3, SCHEDULED_TIME_ISO_STRING_ALARM, SCHEDULED_TIME_ISO_STRING_REMINDER));

    rapidjson::Document offlineAlerts(rapidjson::kObjectType);
    auto& allocator = offlineAlerts.GetAllocator();
    rapidjson::Value alertContainer(rapidjson::kArrayType);

    /// load offline alerts.
    ASSERT_TRUE(m_alertStorage->loadOfflineAlerts(&alertContainer, allocator));
    ASSERT_EQ(static_cast<int>(alertContainer.GetArray().Size()), 3);
    for (const auto& alert : alertContainer.GetArray()) {
        std::string token = "";
        std::string scheduledTime_ISO_8601 = "";
        std::string event_time_iso_8601 = "";
        ASSERT_TRUE(avsCommon::utils::json::jsonUtils::retrieveValue(alert, "token", &token));
        ASSERT_TRUE(avsCommon::utils::json::jsonUtils::retrieveValue(alert, "scheduledTime", &scheduledTime_ISO_8601));
        ASSERT_TRUE(avsCommon::utils::json::jsonUtils::retrieveValue(alert, "eventTime", &event_time_iso_8601));
        if (offlineToken1 == token) {
            ASSERT_EQ(scheduledTime_ISO_8601, SCHEDULED_TIME_ISO_STRING);
            ASSERT_EQ(event_time_iso_8601, SCHEDULED_TIME_ISO_STRING_ALARM);
            continue;
        }
        if (offlineToken2 == token) {
            ASSERT_EQ(scheduledTime_ISO_8601, SCHEDULED_TIME_ISO_STRING_ALARM);
            ASSERT_EQ(event_time_iso_8601, SCHEDULED_TIME_ISO_STRING_TIMER);
            continue;
        }
        if (offlineToken3 == token) {
            ASSERT_EQ(scheduledTime_ISO_8601, SCHEDULED_TIME_ISO_STRING_ALARM);
            ASSERT_EQ(event_time_iso_8601, SCHEDULED_TIME_ISO_STRING_REMINDER);
            continue;
        }
    }
}

/**
 * Test erase offline alerts.
 */
TEST_F(SQLiteAlertStorageTest, test_eraseOfflineAlerts) {
    setUpDatabase();
    /// store offline alerts.
    ASSERT_TRUE(m_alertStorage->storeOfflineAlert(
        "token-offline1", SCHEDULED_TIME_ISO_STRING, SCHEDULED_TIME_ISO_STRING_ALARM));
    ASSERT_TRUE(m_alertStorage->storeOfflineAlert(
        "token-offline2", SCHEDULED_TIME_ISO_STRING_ALARM, SCHEDULED_TIME_ISO_STRING_TIMER));

    /// erase the offline alert.
    ASSERT_TRUE(m_alertStorage->eraseOffline("token-offline1", 1));

    /// load offline alerts.
    rapidjson::Document offlineAlerts(rapidjson::kObjectType);
    auto& allocator = offlineAlerts.GetAllocator();
    rapidjson::Value alertContainer(rapidjson::kArrayType);
    ASSERT_TRUE(m_alertStorage->loadOfflineAlerts(&alertContainer, allocator));
    ASSERT_EQ(static_cast<int>(alertContainer.GetArray().Size()), 1);

    /// erase the offline alert.
    alertContainer.Clear();
    ASSERT_TRUE(m_alertStorage->eraseOffline("token-offline2", 2));
    ASSERT_TRUE(m_alertStorage->loadOfflineAlerts(&alertContainer, allocator));
    ASSERT_TRUE(alertContainer.GetArray().Empty());
}

/**
 * Test clear databse.
 */
TEST_F(SQLiteAlertStorageTest, test_clearDatabase) {
    setUpDatabase();
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase db(TEST_DATABASE_FILE_NAME);
    ASSERT_TRUE(db.open());

    /// store alerts.
    const std::string offlineAlertToken = "token-offline";
    ASSERT_TRUE(m_alertStorage->storeOfflineAlert(
        offlineAlertToken, SCHEDULED_TIME_ISO_STRING, SCHEDULED_TIME_ISO_STRING_ALARM));
    auto alarm = createAlert(TEST_ALERT_TYPE_ALARM);
    ASSERT_TRUE(m_alertStorage->store(alarm));

    /// check alerts exist.
    ASSERT_TRUE(alertExists(&db, OFFLINE_ALERTS_V2_TABLE_NAME, offlineAlertToken));
    ASSERT_TRUE(alertExists(&db, ALERTS_V3_TABLE_NAME, TOKEN_ALARM));
    /// clear database.
    ASSERT_TRUE(m_alertStorage->clearDatabase());
    ASSERT_TRUE(isTableEmpty(&db, ALERTS_V3_TABLE_NAME));
    ASSERT_TRUE(isTableEmpty(&db, OFFLINE_ALERTS_V2_TABLE_NAME));
    db.close();
}

}  // namespace test
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
