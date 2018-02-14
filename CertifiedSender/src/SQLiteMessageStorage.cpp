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

#include "CertifiedSender/SQLiteMessageStorage.h"

#include <SQLiteStorage/SQLiteUtils.h>
#include <SQLiteStorage/SQLiteStatement.h>

#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <fstream>

namespace alexaClientSDK {
namespace certifiedSender {

using namespace avsCommon::utils::file;
using namespace avsCommon::utils::logger;
using namespace alexaClientSDK::storage::sqliteStorage;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteMessageStorage");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The name of the alerts table.
static const std::string MESSAGES_TABLE_NAME = "messages";
/// The name of the 'id' field we will use as the primary key in our tables.
static const std::string DATABASE_COLUMN_ID_NAME = "id";
/// The name of the 'id' field we will use as the primary key in our tables.
static const std::string DATABASE_COLUMN_MESSAGE_TEXT_NAME = "message_text";
/// The SQL string to create the alerts table.
static const std::string CREATE_MESSAGES_TABLE_SQL_STRING = std::string("CREATE TABLE ") + MESSAGES_TABLE_NAME + " (" +
                                                            DATABASE_COLUMN_ID_NAME + " INT PRIMARY KEY NOT NULL," +
                                                            DATABASE_COLUMN_MESSAGE_TEXT_NAME + " TEXT NOT NULL);";

SQLiteMessageStorage::SQLiteMessageStorage() : m_dbHandle{nullptr} {
}

SQLiteMessageStorage::~SQLiteMessageStorage() {
    doClose();
}

bool SQLiteMessageStorage::createDatabase(const std::string& filePath) {
    if (m_dbHandle) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("Database handle is already open."));
        return false;
    }

    if (fileExists(filePath)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("File specified already exists.").d("file path", filePath));
        return false;
    }

    m_dbHandle = createSQLiteDatabase(filePath);
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("Database could not be created.").d("file path", filePath));
        return false;
    }

    if (!performQuery(m_dbHandle, CREATE_MESSAGES_TABLE_SQL_STRING)) {
        ACSDK_ERROR(LX("createDatabaseFailed").m("Table could not be created."));
        close();
        return false;
    }

    return true;
}

bool SQLiteMessageStorage::open(const std::string& filePath) {
    if (m_dbHandle) {
        ACSDK_ERROR(LX("openFailed").m("Database handle is already open."));
        return false;
    }

    if (!fileExists(filePath)) {
        ACSDK_ERROR(LX("openFailed").m("File specified does not exist.").d("file path", filePath));
        return false;
    }

    m_dbHandle = openSQLiteDatabase(filePath);
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("openFailed").m("Database could not be opened.").d("file path", filePath));
        return false;
    }

    return true;
}

bool SQLiteMessageStorage::isOpen() {
    return (nullptr != m_dbHandle);
}

void SQLiteMessageStorage::close() {
    doClose();
}

void SQLiteMessageStorage::doClose() {
    if (isOpen()) {
        if (!closeSQLiteDatabase(m_dbHandle)) {
            ACSDK_ERROR(LX("closeFailed").m("Could not close the database."));
        }
        m_dbHandle = nullptr;
    }
}

bool SQLiteMessageStorage::store(const std::string& message, int* id) {
    if (!id) {
        ACSDK_ERROR(LX("storeFailed").m("id parameter was nullptr."));
        return false;
    }
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("storeFailed").m("Database handle is not open."));
        return false;
    }

    std::string sqlString = std::string("INSERT INTO " + MESSAGES_TABLE_NAME + " (") + DATABASE_COLUMN_ID_NAME + ", " +
                            DATABASE_COLUMN_MESSAGE_TEXT_NAME + ") VALUES (" + "?, ?" + ");";

    int nextId = 0;
    if (!getTableMaxIntValue(m_dbHandle, MESSAGES_TABLE_NAME, DATABASE_COLUMN_ID_NAME, &nextId)) {
        ACSDK_ERROR(LX("storeFailed").m("Cannot generate message id."));
        return false;
    }
    nextId++;

    if (nextId <= 0) {
        ACSDK_ERROR(LX("storeFailed").m("Invalid computed row id.  Possible numerical overflow.").d("id", nextId));
        return false;
    }

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("storeFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement.bindIntParameter(boundParam++, nextId) || !statement.bindStringParameter(boundParam, message)) {
        ACSDK_ERROR(LX("storeFailed").m("Could not bind parameter."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("storeFailed").m("Could not perform step."));
        return false;
    }

    *id = nextId;

    return true;
}

bool SQLiteMessageStorage::load(std::queue<StoredMessage>* messageContainer) {
    if (!m_dbHandle) {
        ACSDK_ERROR(LX("loadFailed").m("Database handle is not open."));
        return false;
    }

    if (!messageContainer) {
        ACSDK_ERROR(LX("loadFailed").m("Alert container parameter is nullptr."));
        return false;
    }

    std::string sqlString = "SELECT * FROM " + MESSAGES_TABLE_NAME + " ORDER BY id;";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("loadFailed").m("Could not create statement."));
        return false;
    }

    // local values which we will use to capture what we read from the database.
    int id = 0;
    std::string message;

    if (!statement.step()) {
        ACSDK_ERROR(LX("loadFailed").m("Could not perform step."));
        return false;
    }

    while (SQLITE_ROW == statement.getStepResult()) {
        int numberColumns = statement.getColumnCount();

        // SQLite cannot guarantee the order of the columns in a given row, so this logic is required.
        for (int i = 0; i < numberColumns; i++) {
            std::string columnName = statement.getColumnName(i);

            if (DATABASE_COLUMN_ID_NAME == columnName) {
                id = statement.getColumnInt(i);
            } else if (DATABASE_COLUMN_MESSAGE_TEXT_NAME == columnName) {
                message = statement.getColumnText(i);
            }
        }

        StoredMessage storedMessage(id, message);

        messageContainer->push(storedMessage);

        statement.step();
    }

    statement.finalize();

    return true;
}

bool SQLiteMessageStorage::erase(int messageId) {
    std::string sqlString = "DELETE FROM " + MESSAGES_TABLE_NAME + " WHERE id=?;";

    SQLiteStatement statement(m_dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("eraseFailed").m("Could not create statement."));
        return false;
    }

    int boundParam = 1;
    if (!statement.bindIntParameter(boundParam, messageId)) {
        ACSDK_ERROR(LX("eraseFailed").m("Could not bind messageId."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("eraseFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

bool SQLiteMessageStorage::clearDatabase() {
    if (!clearTable(m_dbHandle, MESSAGES_TABLE_NAME)) {
        ACSDK_ERROR(LX("clearDatabaseFailed").m("could not clear messages table."));
        return false;
    }

    return true;
}

}  // namespace certifiedSender
}  // namespace alexaClientSDK
