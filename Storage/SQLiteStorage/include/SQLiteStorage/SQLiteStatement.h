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

#ifndef ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITESTATEMENT_H_
#define ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITESTATEMENT_H_

#include <sqlite3.h>
#include <string>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

/**
 * A utility class to simplify interaction with a SQLite statement.  In particular, the resource management
 * operations which are common to many functions are captured in this class's constructor and destructor, as well
 * as making the underlying C-style interface more C++ friendly.
 *
 * The main operations of this class generally map to SQLite operations, so please refer to the online documentation
 * for SQLite for further guidance.  This is a good place to begin:
 *
 * https://sqlite.org/c3ref/intro.html
 */
class SQLiteStatement {
public:
    /**
     * Constructor.
     *
     * @param dbHandle A SQLite database handle.
     * @param sqlString The SQL which this statement object will perform.
     */
    SQLiteStatement(sqlite3* dbHandle, const std::string& sqlString);

    /**
     * Destructor.
     */
    ~SQLiteStatement();

    /**
     * Returns whether the statement has initialized itself correctly.
     *
     * @return Whether the statement has initialized itself correctly.
     */
    bool isValid();

    /**
     * Performs an iteration of the SQL query, which evaluates to either a single row of the results,
     * or completes the query.
     *
     * @return Whether the iteration was performed successfully.
     */
    bool step();

    /**
     * Resets a statement object to be re-executed with different bound parameters.
     *
     * @return Whether the reset was successful.
     */
    bool reset();

    /**
     * Binds an integer to an index within a query.
     * NOTE: The left-most index for SQLite bind operations begins at 1, not 0.
     *
     * @param index The SQL index (the left-most begins at 1, not 0) the value should be bound to.
     * @param value The value to be bound.
     * @return Whether the bind was successful.
     */
    bool bindIntParameter(int index, int value);

    /**
     * Binds a 64-bit integer to an index within a query.
     * NOTE: The left-most index for SQLite bind operations begins at 1, not 0.
     *
     * @param index The SQL index (the left-most begins at 1, not 0) the value should be bound to.
     * @param value The value to be bound.
     * @return Whether the bind was successful.
     */
    bool bindInt64Parameter(int index, int64_t value);

    /**
     * Binds a string to an index within a query.
     * NOTE: The left-most index for SQLite bind operations begins at 1, not 0.
     *
     * @param index The SQL index (the left-most begins at 1, not 0) the value should be bound to.
     * @param value The value to be bound.
     * @return Whether the bind was successful.
     */
    bool bindStringParameter(int index, const std::string& value);

    /**
     * Returns the SQLite result for the last step operation performed.
     *
     * @return The SQLite result for the last step operation performed.
     */
    int getStepResult() const;

    /**
     * Returns the number of columns in the current row being evaluated.
     *
     * @return The number of columns in the current row being evaluated.
     */
    int getColumnCount() const;

    /**
     * Returns the name of a particular column in the current row being evaluated.
     *
     * @param index The index of the column being queried.
     * @return The name of the column.  If the index is out of bounds, the name will be the empty string.
     */
    std::string getColumnName(int index) const;

    /**
     * Returns the text value of a particular column in the current row being evaluated.
     * NOTE: The left-most index for SQLite lookup operations begins at 0.
     *
     * If the function is called with an out-of-bounds index, the result is undefined.
     * If the function is called with a valid index, but a different type than specified in the table schema,
     * then SQLite will perform an implicit conversion as described by its online documentation here:
     *
     * https://sqlite.org/c3ref/column_blob.html
     *
     * If sufficient care is taken while using this class, then both errors above can be avoided.
     *
     * @param index The index of the column being queried (left-most column is 0).
     * @return The text value of the column.  See function comment with respect to error handling.
     */
    std::string getColumnText(int index) const;

    /**
     * Returns the integer value of a particular column in the current row being evaluated.
     * NOTE: The left-most index for SQLite lookup operations begins at 0.
     *
     * If the function is called with an out-of-bounds index, the result is undefined.
     * If the function is called with a valid index, but a different type than specified in the table schema,
     * then SQLite will perform an implicit conversion as described by its online documentation here:
     *
     * https://sqlite.org/c3ref/column_blob.html
     *
     * If sufficient care is taken while using this class, then both errors above can be avoided.
     *
     * @param index The index of the column being queried (left-most column is 0).
     * @return The integer value of the column.  See function comment with respect to error handling.
     */
    int getColumnInt(int index) const;

    /**
     * Returns the 64-bit integer value of a particular column in the current row being evaluated.
     * NOTE: The left-most index for SQLite lookup operations begins at 0.
     *
     * If the function is called with an out-of-bounds index, the result is undefined.
     * If the function is called with a valid index, but a different type than specified in the table schema,
     * then SQLite will perform an implicit conversion as described by its online documentation here:
     *
     * https://sqlite.org/c3ref/column_blob.html
     *
     * If sufficient care is taken while using this class, then both errors above can be avoided.
     *
     * @param index The index of the column being queried (left-most column is 0).
     * @return The 64-bit integer value of the column.  See function comment with respect to error handling.
     */
    int64_t getColumnInt64(int index) const;

    /**
     * Releases the SQLite resources.
     */
    void finalize();

private:
    /// Our internal SQLite statement handle.
    sqlite3_stmt* m_handle;

    /// The result of the last step operation.
    int m_stepResult;
};

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITESTATEMENT_H_
