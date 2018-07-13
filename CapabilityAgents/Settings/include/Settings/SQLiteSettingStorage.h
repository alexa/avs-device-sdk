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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SQLITESETTINGSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SQLITESETTINGSTORAGE_H_

#include "Settings/SettingsStorageInterface.h"

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

/**
 * An implementation that allows us to store settings using SQLite.
 *
 * This class is not thread-safe.
 */
class SQLiteSettingStorage : public SettingsStorageInterface {
public:
    /**
     * Factory method for creating a storage object for Settings based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteSettingStorage object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteSettingStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Constructor.
     *
     * @param dbFilePath The location of the SQLite database file.
     */
    SQLiteSettingStorage(const std::string& databaseFilePath);

    bool createDatabase() override;

    bool open() override;

    void close() override;

    bool settingExists(const std::string& key) override;

    bool store(const std::string& key, const std::string& value) override;

    bool load(std::unordered_map<std::string, std::string>* mapOfSettings) override;

    bool modify(const std::string& key, const std::string& value) override;

    bool erase(const std::string& key) override;

    bool clearDatabase() override;

    ~SQLiteSettingStorage();

private:
    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_database;
};

}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SQLITESETTINGSTORAGE_H_
