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

#include "SQLiteStorage/SQLiteDatabase.h"

#include <AVSCommon/Utils/File/FileUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <utility>
#include "SQLiteStorage/SQLiteUtils.h"

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteDatabase");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

SQLiteDatabase::SQLiteDatabase(const std::string& storageFilePath) :
        m_storageFilePath{storageFilePath},
        m_transactionIsInProgress{false},
        m_dbHandle{nullptr} {
    m_sharedThisPlaceholder = std::shared_ptr<SQLiteDatabase>(this, [](SQLiteDatabase*) {});
}

SQLiteDatabase::~SQLiteDatabase() {
    if (m_dbHandle) {
        ACSDK_WARN(
            LX(__func__).m("DB wasn't closed before destruction of SQLiteDatabase").d("file path", m_storageFilePath));

        // TODO: The DB should just be closed by the destructor, but currently some of the tests call
        // SQLiteDatabase::close().  It doesn't happen outside of the test code.  This JIRA is filed to take care of it
        // later: ACSDK-1094.
        close();
    }

    if (m_transactionIsInProgress) {
        ACSDK_ERROR(LX(__func__).d("reason", "There is an incomplete transaction. Rolling it back."));
        rollbackTransaction();
    }

    // Reset the shared_ptr so transaction's weak_ptr will invalidate.
    m_sharedThisPlaceholder.reset();
}

bool SQLiteDatabase::initialize() {
    if (m_dbHandle) {
        ACSDK_ERROR(LX(__func__).m("Database is already open."));
        return false;
    }

    if (avsCommon::utils::file::fileExists(m_storageFilePath)) {
        ACSDK_ERROR(LX(__func__).m("File specified already exists.").d("file path", m_storageFilePath));
        return false;
    }

    m_dbHandle = createSQLiteDatabase(m_storageFilePath);
    if (!m_dbHandle) {
        ACSDK_ERROR(LX(__func__).m("Database could not be created.").d("file path", m_storageFilePath));
        return false;
    }

    return true;
}

bool SQLiteDatabase::open() {
    if (m_dbHandle) {
        ACSDK_ERROR(LX(__func__).m("Database is already open."));
        return false;
    }

    if (!avsCommon::utils::file::fileExists(m_storageFilePath)) {
        ACSDK_DEBUG0(LX(__func__).m("File specified does not exist.").d("file path", m_storageFilePath));
        return false;
    }

    m_dbHandle = openSQLiteDatabase(m_storageFilePath);
    if (!m_dbHandle) {
        ACSDK_ERROR(LX(__func__).m("Database could not be opened.").d("file path", m_storageFilePath));
        return false;
    }

    return true;
}

bool SQLiteDatabase::isDatabaseReady() {
    return (m_dbHandle != nullptr);
}

bool SQLiteDatabase::performQuery(const std::string& sqlString) {
    if (!alexaClientSDK::storage::sqliteStorage::performQuery(m_dbHandle, sqlString)) {
        ACSDK_ERROR(LX("performQueryFailed").d("SQL string", sqlString));
        return false;
    }

    return true;
}

bool SQLiteDatabase::tableExists(const std::string& tableName) {
    if (!alexaClientSDK::storage::sqliteStorage::tableExists(m_dbHandle, tableName)) {
        ACSDK_DEBUG0(
            LX(__func__).d("reason", "table doesn't exist or there was an error checking").d("table", tableName));
        return false;
    }
    return true;
}

bool SQLiteDatabase::clearTable(const std::string& tableName) {
    if (!alexaClientSDK::storage::sqliteStorage::clearTable(m_dbHandle, tableName)) {
        ACSDK_ERROR(LX(__func__).d("could not clear table", tableName));
        return false;
    }

    return true;
}

void SQLiteDatabase::close() {
    if (m_dbHandle) {
        closeSQLiteDatabase(m_dbHandle);
        m_dbHandle = nullptr;
    }
}

std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteStatement> SQLiteDatabase::createStatement(
    const std::string& sqlString) {
    std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteStatement> statement(
        new SQLiteStatement(m_dbHandle, sqlString));
    if (!statement->isValid()) {
        ACSDK_ERROR(LX("createStatementFailed").d("sqlString", sqlString));
        statement = nullptr;
    }

    return statement;
}

std::unique_ptr<SQLiteDatabase::Transaction> SQLiteDatabase::beginTransaction() {
    if (m_transactionIsInProgress) {
        ACSDK_ERROR(LX("beginTransactionFailed").d("reason", "Only one transaction at a time is allowed"));
        return nullptr;
    }

    const std::string sqlString = "BEGIN TRANSACTION;";
    if (!performQuery(sqlString)) {
        ACSDK_ERROR(LX("beginTransactionFailed").d("reason", "Query failed"));
        return nullptr;
    }

    m_transactionIsInProgress = true;
    return std::unique_ptr<Transaction>(new Transaction(m_sharedThisPlaceholder));
}

bool SQLiteDatabase::commitTransaction() {
    if (!m_transactionIsInProgress) {
        ACSDK_ERROR(LX("commitTransactionFailed").d("reason", "No transaction in progress"));
        return false;
    }
    const std::string sqlString = "COMMIT TRANSACTION;";
    if (!performQuery(sqlString)) {
        ACSDK_ERROR(LX("commitTransactionFailed").d("reason", "Query failed"));
        return false;
    }

    m_transactionIsInProgress = false;
    return true;
}

bool SQLiteDatabase::rollbackTransaction() {
    if (!m_transactionIsInProgress) {
        ACSDK_ERROR(LX("rollbackTransactionFailed").d("reason", "No transaction in progress"));
        return false;
    }
    const std::string sqlString = "ROLLBACK TRANSACTION;";
    if (!performQuery(sqlString)) {
        ACSDK_ERROR(LX("rollbackTransactionFailed").d("reason", "Query failed"));
        return false;
    }
    m_transactionIsInProgress = false;
    return true;
}

bool SQLiteDatabase::Transaction::commit() {
    if (m_transactionCompleted) {
        ACSDK_ERROR(LX("commitFailed").d("reason", "Transaction has already been completed"));
        return false;
    }

    auto database = m_database.lock();
    if (!database) {
        ACSDK_ERROR(LX("commitFailed").d("reason", "Database has already been finalized"));
        return false;
    }
    m_transactionCompleted = true;
    return database->commitTransaction();
}

bool SQLiteDatabase::Transaction::rollback() {
    if (m_transactionCompleted) {
        ACSDK_ERROR(LX("rollbackFailed").d("reason", "Transaction has already been completed"));
        return false;
    }

    bool isExpired = m_database.expired();
    auto database = m_database.lock();
    if (!database && !isExpired) {
        ACSDK_ERROR(LX("rollbackFailed").d("reason", "Database has already been finalized"));
        return false;
    }
    m_transactionCompleted = true;
    return database->rollbackTransaction();
}

SQLiteDatabase::Transaction::Transaction(std::weak_ptr<SQLiteDatabase> database) :
        m_database{std::move(database)},
        m_transactionCompleted{false} {
}

SQLiteDatabase::Transaction::~Transaction() {
    if (m_transactionCompleted) {
        return;
    }

    ACSDK_ERROR(LX(__func__).m("Transaction was not completed manually, rolling it back automatically"));

    rollback();
}
}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK
