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
#ifndef ACSDKBLUETOOTH_SQLITEBLUETOOTHSTORAGE_H_
#define ACSDKBLUETOOTH_SQLITEBLUETOOTHSTORAGE_H_

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "acsdkBluetooth/BluetoothStorageInterface.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {

/// A concrete implementation of @c BluetoothStorageInterface using SQLite.
class SQLiteBluetoothStorage : public BluetoothStorageInterface {
public:
    /**
     * Create an instance of a SQLiteBluetoothStorage object.
     *
     * @param configurationRoot A ConfigurationNode containing the location of the .db file.
     * Should take the form:
     * "bluetooth" : { "databaseFilePath" : "<filePath>" }
     */
    static std::unique_ptr<SQLiteBluetoothStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /// @name BluetoothStorageInterface
    /// @{
    bool createDatabase() override;
    bool open() override;
    void close() override;
    bool clear() override;
    bool getUuid(const std::string& mac, std::string* uuid) override;
    bool getCategory(const std::string& uuid, std::string* category) override;
    bool getMac(const std::string& uuid, std::string* mac) override;
    bool getMacToUuid(std::unordered_map<std::string, std::string>* macToUuid) override;
    bool getMacToCategory(std::unordered_map<std::string, std::string>* macToCategory) override;
    bool getUuidToMac(std::unordered_map<std::string, std::string>* uuidToMac) override;
    bool getUuidToCategory(std::unordered_map<std::string, std::string>* uuidToCategory) override;
    bool getOrderedMac(bool ascending, std::list<std::string>* macs) override;
    bool insertByMac(const std::string& mac, const std::string& uuid, bool overwrite = true) override;
    bool updateByCategory(const std::string& uuid, const std::string& category) override;
    bool remove(const std::string& mac) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param filepath The filepath of the sqlite file.
     */
    SQLiteBluetoothStorage(const std::string& filepath);

    /**
     * Utility that extracts a row from the database using a given statement. The lock must be obtained
     * before calling this function to ensure a consistent result. A step will be executed on the passed
     * in statement, so this function is not idempotent.
     *
     * @param statement The statement to execute.
     * @param[out] row The row obtained from the statement.
     * @return A bool indicating success.
     */
    bool getSingleRowLocked(
        std::unique_ptr<storage::sqliteStorage::SQLiteStatement>& statement,
        std::unordered_map<std::string, std::string>* row);

    /**
     * Utility that extracts from the database all key and value pairs specified. The pairs are returned
     * in a map, and the key is set by key param.
     * The lock must be obtained before calling this function.
     *
     * @param key Key for the mapping.
     * @param value Value for the mapping.
     * @param[out] mappings A mapping of key value pairs requested.
     * @return A bool indication of success.
     */
    bool getMappingsLocked(
        const std::string& key,
        const std::string& value,
        std::unordered_map<std::string, std::string>* mappings);

    /**
     * Utility that gets a data element from the database based on the constraint key. The constraint key must be
     * unique. The lock must be obtained before calling this function.
     *
     * @param constraintKey The key to filter against. Must be unique.
     * @param constraintVal The value to filter against.
     * @param resultKey The key of the data element.
     * @param[out] resultVal The resulting value that is extracted.
     * @return A bool indicating success.
     */
    bool getAssociatedDataLocked(
        const std::string& constraintKey,
        const std::string& constraintVal,
        const std::string& resultKey,
        std::string* resultVal);

    /**
     * Utility that updates a data element from the database based on the constraint key. The constraint key must be
     * unique. The lock must be obtained before calling this function.
     *
     * @param constraintKey The key to filter against. Must be unique.
     * @param constraintVal The value to filter against.
     * @param updateKey The key of the data element to update.
     * @param updateVal The resulting value that is to be updated to.
     * @return A bool indicating success.
     */
    bool updateValueLocked(
        const std::string& constraintKey,
        const std::string& constraintVal,
        const std::string& updateKey,
        const std::string& updateVal);

    /**
     * Utility that inserts a new data element to the database given the elements. The lock must be obtained before
     * calling this function.
     *
     * @param operation Operation given to database operation.
     * @param uuid uuid of device to add.
     * @param mac mac of device to add.
     * @param category category of device to add.
     * @return A bool indicating success.
     */
    bool insertEntryLocked(
        const std::string& operation,
        const std::string& uuid,
        const std::string& mac,
        const std::string& category);

    /**
     * Utility that checks if database has been migrated. The lock must be obtained before calling this function.
     *
     * @return true if migrated, otherwise false.
     */
    bool isDatabaseMigratedLocked();

    /**
     * Utility that migrates the database by adding in the device category column to the existing database and
     * set the category entry to OTHER. The lock must be obtained before calling this function.
     *
     * @return A bool indicating success.
     */
    bool migrateDatabaseLocked();

    /// A mutex to protect database access.
    std::mutex m_databaseMutex;

    /// The underlying SQLite database.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_SQLITEBLUETOOTHSTORAGE_H_
