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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_STORAGE_SQLITECAPABILITIESDELEGATESTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_STORAGE_SQLITECAPABILITIESDELEGATESTORAGE_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "CapabilitiesDelegate/Storage/CapabilitiesDelegateStorageInterface.h"
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace storage {

/**
 * The SQLite implementation of the @c CapabilitiesDelegateStorageInterface.
 */
class SQLiteCapabilitiesDelegateStorage : public CapabilitiesDelegateStorageInterface {
public:
    /**
     * Creates an instance of the @c SQLiteCapabilitiesDelegate.
     * @param configurationRoot The @c ConfigurationNode used to get database file configuration.
     * @return A pointer to the @c SQLiteCapabilitiesDelegate.
     */
    static std::unique_ptr<SQLiteCapabilitiesDelegateStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /// @name CapabilitiesDelegateStorageInterface method overrides.
    /// @{
    bool createDatabase() override;
    bool open() override;
    void close() override;
    bool store(const std::string& endpointId, const std::string& endpointConfig) override;
    bool store(const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) override;
    bool load(std::unordered_map<std::string, std::string>* endpointConfigMap) override;
    bool load(const std::string& endpointId, std::string* endpointConfig) override;
    bool erase(const std::string& endpointId) override;
    bool erase(const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) override;
    bool clearDatabase() override;
    /// @}

    /**
     * Destructor.
     */
    ~SQLiteCapabilitiesDelegateStorage();

private:
    /**
     * Constructor.
     *
     * @param dbFilePath The location of the file that the SQLite DB will use as it's backing storage.
     */
    SQLiteCapabilitiesDelegateStorage(const std::string& dbFilePath);

    /**
     * Method that stores the given endpoint configuration with the endpointId as key.
     *
     * @param endpointId The endpointId used as key to store.
     * @param endpointConfig The value to be stored int he database.
     * @return True if successful, else false.
     */
    bool storeLocked(const std::string& endpointId, const std::string& endpointConfig);

    /**
     * Method that erases the given key and the corresponding value from the database.
     *
     * @param endpointId The endpoint Id key that needs to be erased.
     * @return
     */
    bool eraseLocked(const std::string& endpointId);

    /// Mutex to synchronize access to database.
    std::mutex m_mutex;

    /// The SQLiteDatabase instance used to store the values.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_database;
};

}  // namespace storage
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_STORAGE_SQLITECAPABILITIESDELEGATESTORAGE_H_
