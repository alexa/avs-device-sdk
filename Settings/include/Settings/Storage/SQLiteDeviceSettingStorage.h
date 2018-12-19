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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_STORAGE_SQLITEDEVICESETTINGSTORAGE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_STORAGE_SQLITEDEVICESETTINGSTORAGE_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

#include "Settings/Storage/DeviceSettingStorageInterface.h"
#include "Settings/SettingStatus.h"

namespace alexaClientSDK {
namespace settings {
namespace storage {

/*
 * An SQLite implementation of the @c DeviceSettingStorageInterface.
 */
class SQLiteDeviceSettingStorage : public DeviceSettingStorageInterface {
public:
    /**
     * Factory method for creating a storage object for settings based on an SQLite database.
     * Settings will be stored in the misc database.
     *
     * @param configurationRoot The global config object where the location of the misc database can be found.
     * @return Pointer to the SQLiteDeviceSettingStorage object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteDeviceSettingStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /*
     * Destructor.
     */
    ~SQLiteDeviceSettingStorage();

    /// @name SettingStorageInterface Functions
    /// @{
    bool open() override;
    void close() override;
    bool storeSetting(const std::string& key, const std::string& value, SettingStatus status) override;
    SettingStatusAndValue loadSetting(const std::string& key) override;
    bool deleteSetting(const std::string& key) override;
    bool updateSettingStatus(const std::string& key, SettingStatus status) override;
    /// @}

private:
    /*
     * Constructor.
     *
     * @param dbFilePath The file path of the SQLite database.
     */
    SQLiteDeviceSettingStorage(const std::string& dbFilePath);

    /*
     * Creates a database table for the settings.
     *
     * @return A boolean to specify whether the creation of the table was successful.
     */
    bool createSettingsTable();

    /// The underlying database object.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;

    /// Mutex for ensuring access to database is thread-safe.
    std::mutex m_mutex;
};

}  // namespace storage
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_STORAGE_SQLITEDEVICESETTINGSTORAGE_H_
