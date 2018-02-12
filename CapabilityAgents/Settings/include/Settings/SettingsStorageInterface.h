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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGSSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGSSTORAGEINTERFACE_H_

#include <unordered_map>
#include <memory>
#include <fstream>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

/**
 * An Interface class which defines APIs for interacting with a general database, as well as for storing, loading,
 * and modifying Settings.
 */
class SettingsStorageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~SettingsStorageInterface() = default;

    /**
     * Creates a new database with the given filepath.
     * If the file specified already exists, or if a database is already being handled by this object, then
     * this function returns false.
     *
     * @param filePath The path to the file which will be used to contain the database.
     * @return @c true If the database is created ok, or @c false if either the file exists or a database is already
     * being handled by this object.
     */
    virtual bool createDatabase(const std::string& filePath) = 0;

    /**
     * Open a database with the given filepath.  If this object is already managing an open database, or the file
     * does not exist, or there is a problem opening the database, this function returns false.
     *
     * @param filePath The path to the file which will be used to contain the database.
     * @return @c true If the database is opened ok, @c false if either the file does not exist, if this object is
     * already managing an open database, or if there is another internal reason the database could not be opened.
     */
    virtual bool open(const std::string& filePath) = 0;

    /**
     * Query if this object is currently managing an open database.
     *
     * @return @c true If a database is being currently managed by this object, @c false otherwise.
     */
    virtual bool isOpen() = 0;

    /**
     * Close the currently open database, if one is open.
     */
    virtual void close() = 0;

    /**
     * Query whether a Setting is currently stored with the given token.
     *
     * @param key The name of the setting which uniquely identifies a setting.
     * @return @c true If the setting is stored in the database, @c false otherwise.
     */
    virtual bool settingExists(const std::string& key) = 0;

    /**
     * Stores a single @c Setting in the database.
     *
     * @param key The name of the setting.
     * @param value The value of the setting to which it has to be set.
     * @return Whether the setting was successfully stored.
     */
    virtual bool store(const std::string& key, const std::string& value) = 0;

    /**
     * Loads all settings in the database.
     *
     * @param[out] mapOfSettings The map where settings should be stored.
     * @return Whether the settings were successfully loaded.
     */
    virtual bool load(std::unordered_map<std::string, std::string>* mapOfSettings) = 0;

    /**
     * Updates a database record of the Setting parameter.
     * The field which is updated by this operation is the value of the particular setting.
     *
     * @param key The name of the setting to be modified.
     * @param value The value of the setting to which it has to be set.
     * @return Whether the setting was successfully modified.
     */
    virtual bool modify(const std::string& key, const std::string& value) = 0;

    /**
     * Erases a single setting from the database.
     *
     * @param key The name of the setting to be deleted.
     * @return Whether the setting was successfully erased.
     */
    virtual bool erase(const std::string& key) = 0;

    /**
     * A utility function to clear the database of all records.  Note that the database will still exist, as will
     * the tables.  Only the rows will be erased.
     *
     * @return Whether the database was successfully cleared.
     */
    virtual bool clearDatabase() = 0;
};

}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGSSTORAGEINTERFACE_H_
