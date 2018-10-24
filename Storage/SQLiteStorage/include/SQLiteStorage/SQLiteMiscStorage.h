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

#ifndef ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEMISCSTORAGE_H_
#define ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEMISCSTORAGE_H_

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
    static std::unique_ptr<SQLiteMiscStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

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
    bool getKeyValueTypes(
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
    std::string checkKeyType(const std::string& componentName, const std::string& tableName, KeyType keyType);

    /**
     * Helper method that will check value column type.
     *
     * @param componentName The component name.
     * @param tableName The table name to check.
     * @param valueType The ValueType that needs to be matched.
     * @return an error message if the checks fail, else a blank string
     */
    std::string checkValueType(const std::string& componentName, const std::string& tableName, ValueType valueType);

    /**
     * Helper method that will check key and value column type.
     *
     * @param componentName The component name.
     * @param tableName The table name to check.
     * @param keyType The KeyType that needs to be matched.
     * @param valueType The ValueType that needs to be matched.
     * @return an error message if the checks fail, else a blank string
     */
    std::string checkKeyValueType(
        const std::string& componentName,
        const std::string& tableName,
        KeyType keyType,
        ValueType valueType);

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;
};

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_STORAGE_SQLITESTORAGE_INCLUDE_SQLITESTORAGE_SQLITEMISCSTORAGE_H_
