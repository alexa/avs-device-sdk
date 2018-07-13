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

#include "SQLiteStorage/SQLiteUtils.h"
#include "SQLiteStorage/SQLiteStatement.h"

#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include <fstream>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

using namespace avsCommon::utils::file;
using namespace avsCommon::utils::logger;
using namespace avsCommon::utils::string;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * A utility function to open or create a SQLite database, depending on the flags being passed in.
 * The possible flags defined by SQLite for this operation are as follows:
 *
 * SQLITE_OPEN_READONLY
 * SQLITE_OPEN_READWRITE
 * SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
 *
 * The meanings of these flags are as one might expect, however for further details please refer to the online
 * documentation here:
 *
 * https://sqlite.org/c3ref/open.html
 *
 * @param filePath The path, including file name, to where the database is to be opened or created.
 * @param sqliteFlags Flags which will be passed to the SQLite call.  These flags determine the method of opening.
 * @param[out] dbHandle A double-pointer to a sqlite3 handle which will be updated to point to a valid handle.
 * @return Whether the operation was successful.
 */
static sqlite3* openSQLiteDatabaseHelper(const std::string& filePath, int sqliteFlags) {
    sqlite3* dbHandle = nullptr;

    int rcode = sqlite3_open_v2(
        filePath.c_str(),  // file path
        &dbHandle,         // the db handle
        sqliteFlags,       // flags
        nullptr);          // optional vfs name (C-string)

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("openSQLiteDatabaseHelperFailed")
                        .m("Could not open database.")
                        .d("rcode", rcode)
                        .d("file path", filePath)
                        .d("error message", sqlite3_errmsg(dbHandle)));
        sqlite3_close(dbHandle);
        dbHandle = nullptr;
    }

    return dbHandle;
}

sqlite3* createSQLiteDatabase(const std::string& filePath) {
    if (fileExists(filePath)) {
        ACSDK_ERROR(LX("createSQLiteDatabaseFailed").m("File already exists.").d("file", filePath));
        return nullptr;
    }

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    sqlite3* dbHandle = openSQLiteDatabaseHelper(filePath, flags);

    if (!dbHandle) {
        ACSDK_ERROR(LX("createSQLiteDatabaseFailed").m("Could not create database."));
    }

    return dbHandle;
}

sqlite3* openSQLiteDatabase(const std::string& filePath) {
    if (!fileExists(filePath)) {
        ACSDK_ERROR(LX("openSQLiteDatabaseFailed").m("File could not be found.").d("file", filePath));
        return nullptr;
    }

    int flags = SQLITE_OPEN_READWRITE;
    sqlite3* dbHandle = openSQLiteDatabaseHelper(filePath, flags);

    if (!dbHandle) {
        ACSDK_ERROR(LX("openSQLiteDatabaseFailed").m("Could not open database."));
    }

    return dbHandle;
}

bool closeSQLiteDatabase(sqlite3* dbHandle) {
    if (!dbHandle) {
        ACSDK_ERROR(LX("closeSQLiteDatabaseFailed").m("dbHandle is nullptr."));
    }

    int rcode = sqlite3_close(dbHandle);

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("closeSQLiteDatabaseFailed").d("rcode", rcode).d("error message", sqlite3_errmsg(dbHandle)));
        return false;
    }

    return true;
}

bool performQuery(sqlite3* dbHandle, const std::string& sqlString) {
    if (!dbHandle) {
        ACSDK_ERROR(LX("performQueryFailed").m("dbHandle was nullptr."));
        return false;
    }

    int rcode = sqlite3_exec(
        dbHandle,           // handle to the open database
        sqlString.c_str(),  // the SQL query
        nullptr,            // optional callback function
        nullptr,            // first argument to optional callback function
        nullptr);           // optional pointer to error message (char**)

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("performQueryFailed")
                        .m("Could not execute SQL:" + sqlString)
                        .d("rcode", rcode)
                        .d("error message", sqlite3_errmsg(dbHandle)));
        return false;
    }

    return true;
}

bool getNumberTableRows(SQLiteDatabase* db, const std::string& tableName, int* numberRows) {
    if (!db) {
        ACSDK_ERROR(LX("getNumberTableRowsFailed").m("db was nullptr."));
        return false;
    }

    if (!numberRows) {
        ACSDK_ERROR(LX("getNumberTableRowsFailed").m("numberRows was nullptr."));
        return false;
    }

    std::string sqlString = "SELECT COUNT(*) FROM " + tableName + ";";
    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("getNumberTableRowsFailed").m("Could not create statement."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("getNumberTableRowsFailed").m("Could not step to next row."));
        return false;
    }

    const int RESULT_COLUMN_POSITION = 0;
    std::string rowValue = statement->getColumnText(RESULT_COLUMN_POSITION);

    if (!stringToInt(rowValue.c_str(), numberRows)) {
        ACSDK_ERROR(LX("getNumberTableRowsFailed").d("Could not convert string to integer", rowValue));
        return false;
    }

    return true;
}

bool getTableMaxIntValue(SQLiteDatabase* db, const std::string& tableName, const std::string& columnName, int* maxInt) {
    if (!db) {
        ACSDK_ERROR(LX("getTableMaxIntValue").m("db was nullptr."));
        return false;
    }

    if (!maxInt) {
        ACSDK_ERROR(LX("getMaxIntFailed").m("maxInt was nullptr."));
        return false;
    }

    std::string sqlString =
        "SELECT " + columnName + " FROM " + tableName + " ORDER BY " + columnName + " DESC LIMIT 1;";

    auto statement = db->createStatement(sqlString);

    if (!statement) {
        ACSDK_ERROR(LX("getTableMaxIntValueFailed").m("Could not create statement."));
        return false;
    }

    if (!statement->step()) {
        ACSDK_ERROR(LX("getTableMaxIntValueFailed").m("Could not step to next row."));
        return false;
    }

    int stepResult = statement->getStepResult();

    if (stepResult != SQLITE_ROW && stepResult != SQLITE_DONE) {
        ACSDK_ERROR(LX("getTableMaxIntValueFailed").m("Step did not evaluate to either row or completion."));
        return false;
    }

    // No entries were found in database - set to zero as the current max id.
    if (SQLITE_DONE == stepResult) {
        *maxInt = 0;
    }

    // Entries were found - let's get the value.
    if (SQLITE_ROW == stepResult) {
        const int RESULT_COLUMN_POSITION = 0;
        std::string rowValue = statement->getColumnText(RESULT_COLUMN_POSITION);

        if (!stringToInt(rowValue.c_str(), maxInt)) {
            ACSDK_ERROR(LX("getTableMaxIntValueFailed").d("Could not convert string to integer", rowValue));
            return false;
        }
    }

    return true;
}

bool tableExists(sqlite3* dbHandle, const std::string& tableName) {
    if (!dbHandle) {
        ACSDK_ERROR(LX("tableExistsFailed").m("dbHandle was nullptr."));
        return false;
    }

    std::string sqlString = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + tableName + "';";

    SQLiteStatement statement(dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("tableExistsFailed").m("Could not create statement."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("tableExistsFailed").m("Could not step to next row."));
        return false;
    }

    int stepResult = statement.getStepResult();

    if (stepResult != SQLITE_ROW && stepResult != SQLITE_DONE) {
        ACSDK_ERROR(LX("tableExistsFailed").m("Step did not evaluate to either row or completion."));
        return false;
    }

    // No entries were found in database - the table does not exist.
    if (SQLITE_DONE == stepResult) {
        return false;
    }

    // Entries were found - let's get the value.
    const int RESULT_COLUMN_POSITION = 0;
    std::string rowValue = statement.getColumnText(RESULT_COLUMN_POSITION);

    int count = 0;
    if (!stringToInt(rowValue.c_str(), &count)) {
        ACSDK_ERROR(LX("tableExistsFailed").d("Could not convert string to integer", rowValue));
        return false;
    }

    return (1 == count);
}

bool clearTable(sqlite3* dbHandle, const std::string& tableName) {
    if (!dbHandle) {
        ACSDK_ERROR(LX("clearTableFailed").m("dbHandle was nullptr."));
        return false;
    }

    std::string sqlString = "DELETE FROM " + tableName + ";";

    SQLiteStatement statement(dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("clearTableFailed").m("Could not create statement."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("clearTableFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

bool dropTable(sqlite3* dbHandle, const std::string& tableName) {
    if (!dbHandle) {
        ACSDK_ERROR(LX("dropTableFailed").m("dbHandle was nullptr."));
        return false;
    }

    std::string sqlString = "DROP TABLE IF EXISTS " + tableName + ";";

    SQLiteStatement statement(dbHandle, sqlString);

    if (!statement.isValid()) {
        ACSDK_ERROR(LX("dropTableFailed").m("Could not create statement."));
        return false;
    }

    if (!statement.step()) {
        ACSDK_ERROR(LX("dropTableFailed").m("Could not perform step."));
        return false;
    }

    return true;
}

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK
