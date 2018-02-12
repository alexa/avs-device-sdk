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

#include <SQLiteStorage/SQLiteUtils.h>
#include <SQLiteStorage/SQLiteStatement.h>

#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <Notifications/SQLiteNotificationsStorage.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

using namespace avsCommon::utils::file;
using namespace avsCommon::utils::logger;
using namespace alexaClientSDK::storage::sqliteStorage;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteNotificationsStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const std::string NOTIFICATION_INDICATOR_TABLE_NAME = "notificationIndicators";

static const std::string DATABASE_COLUMN_PERSIST_VISUAL_INDICATOR_NAME = "persistVisualIndicator";

static const std::string DATABASE_COLUMN_PLAY_AUDIO_INDICATOR_NAME = "playAudioIndicator";

static const std::string DATABASE_COLUMN_ASSET_ID_NAME = "assetId";

static const std::string DATABASE_COLUMN_ASSET_URL_NAME = "assetUrl";

static const std::string CREATE_NOTIFICATION_INDICATOR_TABLE_SQL_STRING =
    std::string("CREATE TABLE ") + NOTIFICATION_INDICATOR_TABLE_NAME + " (" +
    DATABASE_COLUMN_PERSIST_VISUAL_INDICATOR_NAME + " INT NOT NULL," + DATABASE_COLUMN_PLAY_AUDIO_INDICATOR_NAME +
    " INT NOT NULL," + DATABASE_COLUMN_ASSET_ID_NAME + " TEXT NOT NULL," + DATABASE_COLUMN_ASSET_URL_NAME +
    " TEXT NOT NULL);";

/// The name of the table and the field that will hold the state of the indicator.
static const std::string INDICATOR_STATE_NAME = "indicatorState";

static const std::string CREATE_INDICATOR_STATE_TABLE_SQL_STRING =
    std::string("CREATE TABLE ") + INDICATOR_STATE_NAME + " (" + INDICATOR_STATE_NAME + " INT NOT NULL);";

SQLiteNotificationsStorage::SQLiteNotificationsStorage() : m_dbHandle{nullptr} {
}

bool SQLiteNotificationsStorage::createDatabase(const std::string& filePath) {
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

    // Create NotificationIndicator table
    if (!performQuery(m_dbHandle, CREATE_NOTIFICATION_INDICATOR_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create notification indicator table"));
        close();
        return false;
    }

    // Note: If the NotificationIndicator table creation succeeds and the IndicatorState table creation fails,
    // the database will be in an inconsistent state and will require deletion or another call to createDatabase().

    // Create IndicatorState table
    if (!performQuery(m_dbHandle, CREATE_INDICATOR_STATE_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to create indicator state table"));
        close();
        return false;
    }

    // The default IndicatorState should be OFF.
    if (!setIndicatorState(IndicatorState::OFF)) {
        ACSDK_ERROR(LX("createDatabaseFailed").d("reason", "failed to set default indicator state"));
        close();
        return false;
    }

    return true;
}

bool SQLiteNotificationsStorage::open(const std::string& filePath) {
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

    if (!tableExists(m_dbHandle, NOTIFICATION_INDICATOR_TABLE_NAME)) {
        ACSDK_ERROR(
            LX("openFailed").d("reason", "table doesn't exist").d("TableName", NOTIFICATION_INDICATOR_TABLE_NAME));
        return false;
    }

    if (!tableExists(m_dbHandle, INDICATOR_STATE_NAME)) {
        ACSDK_ERROR(LX("openFailed").d("reason", "table doesn't exist").d("TableName", INDICATOR_STATE_NAME));
        return false;
    }

    return true;
}

bool SQLiteNotificationsStorage::isOpen() const {
    return (nullptr != m_dbHandle);
}

void SQLiteNotificationsStorage::close() {
    if (m_dbHandle) {
        closeSQLiteDatabase(m_dbHandle);
        m_dbHandle = nullptr;
    }
}

bool SQLiteNotificationsStorage::enqueue(const NotificationIndicator& notificationIndicator) {
    // Inserted rows representations of a NotificationIndicator:
    // | id | persistVisualIndicator | playAudioIndicator | assetId | assetUrl |

    const std::string sqlString = "INSERT INTO " + NOTIFICATION_INDICATOR_TABLE_NAME + " (" +
                                  DATABASE_COLUMN_PERSIST_VISUAL_INDICATOR_NAME + "," +
                                  DATABASE_COLUMN_PLAY_AUDIO_INDICATOR_NAME + "," + DATABASE_COLUMN_ASSET_ID_NAME +
                                  "," + DATABASE_COLUMN_ASSET_URL_NAME + ") VALUES (" + "?, ?, ?, ?);";

    // lock here to bind the id generation and the enqueue operations
    std::lock_guard<std::mutex> lock{m_databaseMutex};

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("enqueueFailed").m("Database handle is not open"));
        return false;
    }

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("enqueueFailed").m("Could not create statement"));
        return false;
    }
    int boundParam = 1;
    if (!statement.bindIntParameter(boundParam++, notificationIndicator.persistVisualIndicator) ||
        !statement.bindIntParameter(boundParam++, notificationIndicator.playAudioIndicator) ||
        !statement.bindStringParameter(boundParam++, notificationIndicator.asset.assetId) ||
        !statement.bindStringParameter(boundParam++, notificationIndicator.asset.url)) {
        ACSDK_ERROR(LX("enqueueFailed").m("Could not bind parameter"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("enqueueFailed").m("Could not perform step"));
        return false;
    }

    statement.finalize();

    return true;
}

/**
 * A utility function to pop the next notificationIndicator from the database.
 *
 * @param dbHandle The database handle.
 * @return Whether the delete operation was successful.
 * @note This function should only be called when holding m_databaseMutex.
 */
static bool popNotificationIndicatorLocked(sqlite3* dbHandle) {
    // the next notificationIndicator in the queue corresponds to the minimum id
    const std::string minTableId =
        "(SELECT ROWID FROM " + NOTIFICATION_INDICATOR_TABLE_NAME + " order by ROWID limit 1)";

    const std::string sqlString =
        "DELETE FROM " + NOTIFICATION_INDICATOR_TABLE_NAME + " WHERE ROWID=" + minTableId + ";";

    SQLiteStatement statement(dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("popNotificationIndicatorLockedFailed").m("Could not create statement."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("popNotificationIndicatorLockedFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

bool SQLiteNotificationsStorage::dequeue() {
    std::lock_guard<std::mutex> lock{m_databaseMutex};

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("dequeueFailed").m("Database handle is not open"));
        return false;
    }

    // need to check if there are NotificationIndicators left to dequeue, but the indicator itself doesn't matter
    NotificationIndicator dummyIndicator;

    if (!getNextNotificationIndicatorLocked(&dummyIndicator)) {
        ACSDK_ERROR(LX("dequeueFailed").m("No records left in the database!"));
        return false;
    }

    if (!popNotificationIndicatorLocked(m_dbHandle)) {
        ACSDK_ERROR(LX("dequeueFailed").m("Could not pop notificationIndicator from table"));
        return false;
    }

    return true;
}

bool SQLiteNotificationsStorage::peek(NotificationIndicator* notificationIndicator) {
    std::lock_guard<std::mutex> lock{m_databaseMutex};
    if (!getNextNotificationIndicatorLocked(notificationIndicator)) {
        ACSDK_ERROR(LX("peekFailed").m("Could not retrieve the next notificationIndicator from the database"));
        return false;
    }
    return true;
}

bool SQLiteNotificationsStorage::setIndicatorState(IndicatorState state) {
    std::lock_guard<std::mutex> lock{m_databaseMutex};

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("setIndicatorStateFailed").m("Database handle is not open"));
        return false;
    }

    // first delete the old record, we only need to maintain one record of IndicatorState at a time.
    std::string sqlString = "DELETE FROM " + INDICATOR_STATE_NAME + " WHERE ROWID IN (SELECT ROWID FROM " +
                            INDICATOR_STATE_NAME + " limit 1);";

    SQLiteStatement deleteStatement(m_dbHandle, sqlString);

    if (!deleteStatement.isValid()) {
        ACSDK_ERROR(LX("setIndicatorStateFailed").m("Could not create deleteStatement"));
        return false;
    }

    if (!deleteStatement.step()) {
        ACSDK_ERROR(LX("setIndicatorStateFailed").m("Could not perform step"));
        return false;
    }

    deleteStatement.finalize();

    // we should only be storing one record in this table at any given time
    sqlString = "INSERT INTO " + INDICATOR_STATE_NAME + " (" + INDICATOR_STATE_NAME + ") VALUES (?);";

    SQLiteStatement insertStatement(m_dbHandle, sqlString);

    if (!insertStatement.isValid()) {
        ACSDK_ERROR(LX("setIndicatorStateFailed").m("Could not create insertStatement"));
        return false;
    }

    if (!insertStatement.bindIntParameter(1, indicatorStateToInt(state))) {
        ACSDK_ERROR(LX("setIndicatorStateFailed").m("Could not bind parameter"));
        return false;
    }

    if (!insertStatement.step()) {
        ACSDK_ERROR(LX("setIndicatorStateFailed").m("Could not perform step"));
        return false;
    }

    insertStatement.finalize();

    return true;
}

bool SQLiteNotificationsStorage::getIndicatorState(IndicatorState* state) const {
    if (!state) {
        ACSDK_ERROR(LX("getIndicatorStateFailed").m("State parameter was nullptr"));
        return false;
    }

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("getIndicatorStateFailed").m("Database handle is not open"));
        return false;
    }

    std::string sqlString = "SELECT * FROM " + INDICATOR_STATE_NAME;

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("getIndicatorStateFailed").m("Could not create statement"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("getIndicatorStateFailed").m("Could not perform step"));
        return false;
    }

    int stepResult = statement.getStepResult();

    if (stepResult != SQLITE_ROW) {
        ACSDK_ERROR(LX("getIndicatorStateFailed").m("No records left in table"));
        return false;
    }

    *state = avsCommon::avs::intToIndicatorState(statement.getColumnInt(0));

    if (IndicatorState::UNDEFINED == *state) {
        ACSDK_ERROR(LX("getIndicatorStateFailed").m("Unknown indicator state retrieved from table"));
        return false;
    }

    statement.finalize();

    return true;
}

bool SQLiteNotificationsStorage::checkForEmptyQueue(bool* empty) const {
    if (!empty) {
        ACSDK_ERROR(LX("checkForEmptyQueueFailed").m("empty parameter was nullptr"));
        return false;
    }

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("checkForEmptyQueueFailed").m("Database handle is not open"));
        return false;
    }

    std::string sqlString = "SELECT * FROM " + NOTIFICATION_INDICATOR_TABLE_NAME;

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("checkForEmptyQueueFailed").m("Could not create statement"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("checkForEmptyQueueFailed").m("Could not perform step"));
        return false;
    }

    int stepResult = statement.getStepResult();

    if (stepResult != SQLITE_ROW) {
        *empty = true;
    } else {
        *empty = false;
    }

    return true;
}

bool SQLiteNotificationsStorage::clearNotificationIndicators() {
    const std::string sqlString = "DELETE FROM " + NOTIFICATION_INDICATOR_TABLE_NAME;

    std::lock_guard<std::mutex> lock{m_databaseMutex};

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("clearNotificationIndicatorsFailed").m("Database handle is not open"));
        return false;
    }

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("clearNotificationIndicatorsFailed").m("Could not create statement."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("clearNotificationIndicatorsFailed").m("Could not perform step."));
        return false;
    }
    return true;
}

SQLiteNotificationsStorage::~SQLiteNotificationsStorage() {
    close();
}

bool SQLiteNotificationsStorage::getNextNotificationIndicatorLocked(NotificationIndicator* notificationIndicator) {
    // the minimum id is the next NotificationIndicator in the queue
    std::string sqlString = "SELECT * FROM " + NOTIFICATION_INDICATOR_TABLE_NAME + " ORDER BY ROWID ASC LIMIT 1;";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("getNextNotificationIndicatorLockedFailed").m("Could not create statement"));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("getNextNotificationIndicatorLockedFailed").m("Could not perform step"));
        return false;
    }

    int stepResult = statement.getStepResult();

    if (stepResult != SQLITE_ROW) {
        ACSDK_ERROR(LX("getNextNotificationIndicatorLockedFailed").m("No records left in table"));
        return false;
    }

    int numberColumns = statement.getColumnCount();

    bool persistVisualIndicator = false;
    bool playAudioIndicator = false;
    std::string assetId = "";
    std::string assetUrl = "";

    // loop through columns to get all pertinent data for the out parameter
    for (int i = 0; i < numberColumns; i++) {
        std::string columnName = statement.getColumnName(i);

        if (DATABASE_COLUMN_PERSIST_VISUAL_INDICATOR_NAME == columnName) {
            persistVisualIndicator = statement.getColumnInt(i);

        } else if (DATABASE_COLUMN_PLAY_AUDIO_INDICATOR_NAME == columnName) {
            playAudioIndicator = statement.getColumnInt(i);

        } else if (DATABASE_COLUMN_ASSET_ID_NAME == columnName) {
            assetId = statement.getColumnText(i);

        } else if (DATABASE_COLUMN_ASSET_URL_NAME == columnName) {
            assetUrl = statement.getColumnText(i);
        }
    }

    // load up the out NotificationIndicator
    *notificationIndicator = NotificationIndicator(persistVisualIndicator, playAudioIndicator, assetId, assetUrl);
    return true;
}

bool SQLiteNotificationsStorage::getQueueSize(int* size) const {
    if (!size) {
        ACSDK_ERROR(LX("getQueueSizeFailed").m("size parameter was nullptr"));
        return false;
    }

    if (!m_dbHandle) {
        ACSDK_ERROR(LX("getQueueSizeFailed").m("Database handle is not open"));
        return false;
    }

    if (!getNumberTableRows(m_dbHandle, NOTIFICATION_INDICATOR_TABLE_NAME, size)) {
        ACSDK_ERROR(LX("getQueueSizeFailed").m("Failed to count rows in table"));
        return false;
    }
    return true;
}

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
