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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STORAGE_MISCSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STORAGE_MISCSTORAGEINTERFACE_H_

#include <string>
#include <unordered_map>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace storage {

/**
 * This class provides an interface to MiscStorage - a simple key/value database.
 * Since this database is supposed to be shared by various components of the SDK, there could be
 * conflicts in the table names across different SDK components. Hence, the APIs take the SDK component name
 * as well as the table name so that table names can be unique within a specific SDK component.
 */
class MiscStorageInterface {
public:
    /// The type of the key column in Misc DB
    enum class KeyType {
        /// Unknown type
        UNKNOWN_KEY,
        /// String key
        STRING_KEY
    };

    /// The type of the value column in Misc DB
    enum class ValueType {
        /// Unknown type
        UNKNOWN_VALUE,
        /// String value
        STRING_VALUE
    };

    /**
     * Destructor
     */
    virtual ~MiscStorageInterface() = default;

    /**
     * Creates a new database.
     * If a database is already being handled by this object or there is another internal error, then this function
     * returns false.
     *
     * @return @c true If the database is created ok, or @c false if a database is already being handled by this object
     * or there is a problem creating the database.
     */
    virtual bool createDatabase() = 0;

    /**
     * Open an existing database.  If this object is already managing an open database, or there is a problem opening
     * the database, this function returns false.
     *
     * @return @c true If the database is opened ok, @c false if this object is already managing an open database, or if
     * there is another internal reason the database could not be opened.
     */
    virtual bool open() = 0;

    /**
     * Returns true if this object is already managing an open database, false otherwise.
     *
     * @return True if this object is already managing an open database, false otherwise.
     */
    virtual bool isOpened() = 0;

    /**
     * Close the currently open database, if one is open.
     */
    virtual void close() = 0;

    /**
     * Create a simple key/value pair table. If there is a problem creating
     * the table, this function returns false.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param keyType The key type.
     * @param valueType The value type.
     * @return @c true If the table is created ok, @c false if the table couldn't be created.
     */
    virtual bool createTable(
        const std::string& componentName,
        const std::string& tableName,
        KeyType keyType,
        ValueType valueType) = 0;

    /**
     * Removes all the entries in the table. The table itself will continue to exist.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @return @c true If the table is cleared ok, @c false if the table couldn't be cleared.
     */
    virtual bool clearTable(const std::string& componentName, const std::string& tableName) = 0;

    /**
     * Deletes the table.
     * The table must be empty before you can delete the table.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @return @c true If the table is deleted ok, @c false if the table couldn't be deleted.
     */
    virtual bool deleteTable(const std::string& componentName, const std::string& tableName) = 0;

    /**
     * Gets the value associated with a key in the table.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param [out] value The value associated with the key in the table.
     * @return true If the value was found out ok, @c false if not.
     */
    virtual bool get(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        std::string* value) = 0;

    /**
     * Adds a value in the table.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param value The value of the table entry.
     * @return @c true If the value was added ok, @c false if not.
     */
    virtual bool add(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) = 0;

    /**
     * Updates a value in the table.
     *
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param value The value of the table entry.
     * @return @c true If the value was updated ok, @c false if not (including if the entry does not exist).
     */
    virtual bool update(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) = 0;

    /**
     * Puts a value in the table.
     * Basically, this will add the entry for the key if it doesn't already exist, or it will
     * update the entry for the key if it already exists.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param value The value of the table entry.
     * @return @c true If the value was put ok, @c false if not.
     */
    virtual bool put(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) = 0;

    /**
     * Removes a value from the table.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @return @c true If the value was removed ok, @c false if not.
     */
    virtual bool remove(const std::string& componentName, const std::string& tableName, const std::string& key) = 0;

    /**
     * Checks if a key exists in the table.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param key The key for the table entry.
     * @param [out] tableEntryExistsValue True if the key exists, @c false if not.
     * @return @c true if the table entry's existence was found out ok, else false..
     */
    virtual bool tableEntryExists(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        bool* tableEntryExistsValue) = 0;

    /**
     * Checks if a table exists in the DB.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param [out] tableExistsValue True if the table exists, @c false if not.
     * @return @c true if the table's existence was found out ok, else false.
     */
    virtual bool tableExists(
        const std::string& componentName,
        const std::string& tableName,
        bool* tableExistsValue) = 0;

    /**
     * Loads the table entries into a map.
     *
     * @param componentName The component name.
     * @param tableName The table name.
     * @param [out] valueContainer The container for the values in the table.
     * @return @c true If the values were loaded ok, @c false if not.
     */
    virtual bool load(
        const std::string& componentName,
        const std::string& tableName,
        std::unordered_map<std::string, std::string>* valueContainer) = 0;
};

}  // namespace storage
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STORAGE_MISCSTORAGEINTERFACE_H_
