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

#ifndef ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEMISCSTORAGE_H_
#define ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEMISCSTORAGE_H_

#include <mutex>

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

/**
 * A class that provides a SQLite implementation of MiscStorage database.
 */
class SQLiteMiscStorage : public avsCommon::sdkInterfaces::storage::MiscStorageInterface {
public:
    /**
     * Factory method for creating a storage object for a SQLite database.
     * Note that the actual database will not be created by this function.
     *
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> createMiscStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot);

    /**
     * Factory method for creating a storage object for a SQLite database.
     * Note that the actual database will not be created by this function.
     *
     * @deprecated
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteMiscStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Factory method for creating a storage object for a SQLite database.
     * Note that the actual database will not be created by this function.
     *
     * @deprecated
     * @param[in] databasePath Path to database
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteMiscStorage> create(const std::string& databasePath);

    /**
     * Destructor
     */
    ~SQLiteMiscStorage();

    /// @name MiscStorageInterface method overrides.
    /// @{
    bool createDatabase() override;
    bool open() override;
    bool isOpened() override;
    void close() override;
    bool createTable(
        const std::string& componentName,
        const std::string& tableName,
        KeyType keyType,
        ValueType valueType) override;
    bool clearTable(const std::string& componentName, const std::string& tableName) override;
    bool deleteTable(const std::string& componentName, const std::string& tableName) override;
    bool get(const std::string& componentName, const std::string& tableName, const std::string& key, std::string* value)
        override;
    bool add(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) override;
    bool update(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) override;
    bool put(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) override;
    bool remove(const std::string& componentName, const std::string& tableName, const std::string& key) override;
    bool tableEntryExists(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        bool* tableEntryExistsValue) override;
    bool tableExists(const std::string& componentName, const std::string& tableName, bool* tableExistsValue) override;
    bool load(
        const std::string& componentName,
        const std::string& tableName,
        std::unordered_map<std::string, std::string>* valueContainer) override;
    /// @}

    /**
     * @brief Provides reference to a database.
     *
     * This method provides a reference to inner database object for database maintenance operations. The access to the
     * database is not serialized against parallel access and should be used only when it is guaranteed there are no
     * other consumers of the objects.
     *
     * @return Reference to database.
     */
    SQLiteDatabase& getDatabase();

private:
    /**
     * Constructor.
     *
     * @param dbFilePath The location of the SQLite database file.
     */
    SQLiteMiscStorage(const std::string& dbFilePath);

    /**
     * Method that will get the key column type and value column type.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param [out] *keyType The key column type.
     * @param [out] *valueType The value column name.
     * @return true if the key and value column types were found, else false
     */
    bool getKeyValueTypesLocked(
        const std::string& componentName,
        const std::string& tableName,
        KeyType* keyType,
        ValueType* valueType);

    /**
     * Helper method that will check key column type.
     *
     * @param componentName The component name.
     * @param tableName The table name to check.
     * @param keyType The KeyType that needs to be matched.
     * @return an error message if the checks fail, else a blank string
     */
    std::string checkKeyTypeLocked(const std::string& componentName, const std::string& tableName, KeyType keyType);

    /**
     * Helper method that will check value column type.
     *
     * @param componentName The component name.
     * @param tableName The table name to check.
     * @param valueType The ValueType that needs to be matched.
     * @return an error message if the checks fail, else a blank string
     */
    std::string checkValueTypeLocked(
        const std::string& componentName,
        const std::string& tableName,
        ValueType valueType);

    /**
     * Helper method that will check key and value column type.
     *
     * @param componentName The component name.
     * @param tableName The table name to check.
     * @param keyType The KeyType that needs to be matched.
     * @param valueType The ValueType that needs to be matched.
     * @return an error message if the checks fail, else a blank string
     */
    std::string checkKeyValueTypeLocked(
        const std::string& componentName,
        const std::string& tableName,
        KeyType keyType,
        ValueType valueType);

    /**
     * Creates a new database.
     * If a database is already being handled by this object or there is another internal error, then this function
     * returns false.
     *
     * @note Must be run with @c mutex locked.
     *
     * @return @c true If the database is created ok, or @c false if a database is already being handled by this object
     * or there is a problem creating the database.
     */
    bool createDatabaseLocked();

    /**
     * Open an existing database.  If this object is already managing an open database, or there is a problem opening
     * the database, this function returns false.
     *
     * @note Must be run with @c mutex locked.
     *
     * @return @c true If the database is opened ok, @c false if this object is already managing an open database, or if
     * there is another internal reason the database could not be opened.
     */
    bool openLocked();

    /**
     * Returns true if this object is already managing an open database, false otherwise.
     *
     * @note Must be run with @c mutex locked.
     *
     * @return True if this object is already managing an open database, false otherwise.
     */
    bool isOpenedLocked();

    /**
     * Close the currently open database, if one is open.
     *
     * @note Must be run with @c mutex locked.
     */
    void closeLocked();

    /**
     * Create a simple key/value pair table. If there is a problem creating
     * the table, this function returns false.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param keyType The key type.
     * @param valueType The value type.
     * @return @c true If the table is created ok, @c false if the table couldn't be created.
     */
    bool createTableLocked(
        const std::string& componentName,
        const std::string& tableName,
        KeyType keyType,
        ValueType valueType);

    /**
     * Removes all the entries in the table. The table itself will continue to exist.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @return @c true If the table is cleared ok, @c false if the table couldn't be cleared.
     */
    bool clearTableLocked(const std::string& componentName, const std::string& tableName);

    /**
     * Deletes the table.
     * The table must be empty before you can delete the table.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @return @c true If the table is deleted ok, @c false if the table couldn't be deleted.
     */
    bool deleteTableLocked(const std::string& componentName, const std::string& tableName);

    /**
     * Gets the value associated with a key in the table.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param [out] value The value associated with the key in the table.
     * @return true If the value was found out ok, @c false if not.
     */
    bool getLocked(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        std::string* value);

    /**
     * Adds a value in the table.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param value The value of the table entry.
     * @note @c value should be a string literal.
     * @return @c true If the value was added ok, @c false if not.
     */
    bool addLocked(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value);

    /**
     * Updates a value in the table.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param value The value of the table entry.
     * @note @c value should be a literal string.
     * @return @c true If the value was updated ok, @c false if not (including if the entry does not exist).
     */
    bool updateLocked(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value);

    /**
     * Puts a value in the table.
     * Basically, this will add the entry for the key if it doesn't already exist, or it will
     * update the entry for the key if it already exists.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param value The value of the table entry.
     * @note @c value should be a literal string.
     * @return @c true If the value was put ok, @c false if not.
     */
    bool putLocked(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value);

    /**
     * Removes a value from the table.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @return @c true If the value was removed ok, @c false if not.
     */
    bool removeLocked(const std::string& componentName, const std::string& tableName, const std::string& key);

    /**
     * Checks if a key exists in the table.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param [out] tableEntryExistsValue True if the key exists, @c false if not.
     * @return @c true if the table entry's existence was found out ok, else false..
     */
    bool tableEntryExistsLocked(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        bool* tableEntryExistsValue);

    /**
     * Checks if a table exists in the DB.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param [out] tableExistsValue True if the table exists, @c false if not.
     * @return @c true if the table's existence was found out ok, else false.
     */
    bool tableExistsLocked(const std::string& componentName, const std::string& tableName, bool* tableExistsValue);

    /**
     * Loads the table entries into a map.
     *
     * @note Must be run with @c mutex locked.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param [out] valueContainer The container for the values in the table.
     * @return @c true If the values were loaded ok, @c false if not.
     */
    bool loadLocked(
        const std::string& componentName,
        const std::string& tableName,
        std::unordered_map<std::string, std::string>* valueContainer);

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;

    /// This is the mutex to serialize access to @c m_db.
    std::mutex m_mutex;
};

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEMISCSTORAGE_H_
