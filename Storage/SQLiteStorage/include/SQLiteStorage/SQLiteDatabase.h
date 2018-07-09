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

#ifndef ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEDATABASE_H_
#define ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEDATABASE_H_

#include <memory>
#include <string>
#include <vector>

#include <sqlite3.h>

#include <SQLiteStorage/SQLiteStatement.h>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

/**
 * A basic class for performing basic SQLite database operations.  This the boilerplate code used to manage the
 * SQLiteDatabase.  This database is not thread-safe, and must be protected before being used in a mutlithreaded
 * fashion.
 */
class SQLiteDatabase {
public:
    /**
     * Class to manage SQL transaction lifecycle.
     */
    class Transaction {
        /// Allow @c SQLiteDatabase to access private constructor of @c Transaction.
        friend SQLiteDatabase;

    public:
        /**
         * Destructor
         */
        ~Transaction();

        /**
         * Commits the current transaction.
         *
         * @return true if commit was successful, false otherwise
         */
        bool commit();

        /**
         * Rolls back the current transaction.
         *
         * @return true if rollback was successful, false otherwise
         */
        bool rollback();

    private:
        /**
         * Constructor
         *
         * @param dbHandle weak pointer to a @c SQLiteDatabase instance that will own the transaction.
         */
        Transaction(std::weak_ptr<SQLiteDatabase> dbHandle);

        /**
         * Pointer to a @c SQLiteDatabase instance that owns the transaction.
         */
        std::weak_ptr<SQLiteDatabase> m_database;

        /**
         * Flag indicating whether the transaction has already been completed.
         */
        bool m_transactionCompleted;
    };

    /**
     * Constructor.  The internal variables are initialized.
     *
     * @param filePath The location of the file that the SQLite DB will use as it's backing storage when initialize or
     * open are called.
     */
    SQLiteDatabase(const std::string& filePath);

    /**
     * Destructor.
     *
     * On destruction, the DB is checked to see if it is closed.  It must be closed before the SQLiteDatabase object is
     * destroyed as there is no handle to the DB to close it after this object is gone.
     */
    ~SQLiteDatabase();

    /**
     * The internal SQLite DB is created.
     *
     * @return true, if the (empty) DB is created successfully.  If there is already a DB at the path specified, this
     * function fails and returns false.  It also returns false if the database is already opened or if there is an
     * internal failure in creating the DB.
     */
    bool initialize();

    /**
     * The internal SQLite DB is opened.
     *
     * @return true, if the DB is opened successfully.  If there is no DB at the path specified, this function fails and
     * returns false.  It also returns false if the database is already opened or if there is an internal failure in
     * creating the DB.
     */
    bool open();

    /**
     * Run a SQL query on the database.
     *
     * @param sqlString A valid SQL query.
     * @return true if successful, false if there is a problem.
     */
    bool performQuery(const std::string& sqlString);

    /**
     * Check to see if a specified table exists.
     *
     * @param tableName The name of the table to check.
     * @return true if the table exists, flase if it doesn't or there is an error.
     */
    bool tableExists(const std::string& tableName);

    /**
     * Remove all the rows from the specified table.
     *
     * @param tableName The name of the table to clear.
     * @return true if successful, false if there was an error.
     */
    bool clearTable(const std::string& tableName);

    /**
     * If open, close the internal SQLite DB.  Do nothing if there is no DB open.
     */
    void close();

    /**
     * Create an SQLiteStatement object to execute the provided string.
     *
     * @param sqlString The SQL command to execute.
     * @return A unique_ptr to the SQLiteStatement that represents the sqlString.
     */
    std::unique_ptr<SQLiteStatement> createStatement(const std::string& sqlString);

    /**
     * Checks if the database is ready to be acted upon.
     *
     * @return true if database is ready to be acted upon, false otherwise.
     */
    bool isDatabaseReady();

    /**
     * Begins transaction.
     *
     * @return true if query succeeded, false otherwise
     */
    std::unique_ptr<Transaction> beginTransaction();

private:
    /**
     * Commits the transaction started with @c beginTransaction.
     *
     * @return true if transaction has been successfully committed, false otherwise.
     */
    bool commitTransaction();

    /**
     * Rolls back the transaction started with @c beginTransaction.
     * @return true if transaction has been successfully rolled back, false otherwise.
     */
    bool rollbackTransaction();

    /// The path to use when creating/opening the internal SQLite DB.
    const std::string m_storageFilePath;

    /// Flag indicating whether there is a transaction in progress.
    bool m_transactionIsInProgress;

    /// The sqlite database handle.
    sqlite3* m_dbHandle;

    /**
     * A shared_ptr to this that is used to manage viability of weak_ptrs to this.  This shared_ptr has a no-op deleter,
     * and does not manage the lifecycle of this instance.  Instead, ~SQLiteDatabase() resets this shared_ptr to signal
     * to any outstanding Transaction that this instance has been deleted.
     */
    std::shared_ptr<SQLiteDatabase> m_sharedThisPlaceholder;
};

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEDATABASE_H_
