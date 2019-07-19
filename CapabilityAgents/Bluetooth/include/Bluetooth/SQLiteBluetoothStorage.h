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
#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_SQLITEBLUETOOTHSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_SQLITEBLUETOOTHSTORAGE_H_

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include "Bluetooth/BluetoothStorageInterface.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {

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
    bool getMac(const std::string& uuid, std::string* mac) override;
    bool getMacToUuid(std::unordered_map<std::string, std::string>* macToUuid) override;
    bool getUuidToMac(std::unordered_map<std::string, std::string>* uuidToMac) override;
    bool getOrderedMac(bool ascending, std::list<std::string>* macs) override;
    bool insertByMac(const std::string& mac, const std::string& uuid, bool overwrite = true) override;
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
     * Utility that extracts from the database all uuid and mac pairs. The pairs are returned in a map,
     * and the key is dependent on the @c keyPreference param.
     * The lock must be obtained before calling this function.
     *
     * @param keyPreference A preference of whether the uuid or the mac should be the key.
     * @param[out] mappings A mapping of uuid and mac keyed by keyPreference.
     * @return A bool indicationg success.
     */
    bool getMappingsLocked(const std::string& keyPreference, std::unordered_map<std::string, std::string>* mappings);

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

    /// A mutex to protect database access.
    std::mutex m_databaseMutex;

    /// The underlying SQLite database.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;
};

}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_SQLITEBLUETOOTHSTORAGE_H_
