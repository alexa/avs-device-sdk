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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_STORAGE_DEVICESETTINGSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_STORAGE_DEVICESETTINGSTORAGEINTERFACE_H_

#include <string>

#include "Settings/SettingStatus.h"

namespace alexaClientSDK {
namespace settings {
namespace storage {

/**
 * An Interface class which defines APIs for interacting with a database used for storing, loading, and modifying
 * settings.
 * @note: This class does not provide any setting level sanity testing. It is expected that the logic and @c
 * SettingStatus transitions are managed by the caller.  This class simply manages the persistence of settings data and
 * status.
 */
class DeviceSettingStorageInterface {
public:
    /**
     * The type holding the status and string value of a setting.
     * If the status is @c NOT_AVAILABLE the value will contain the reason or error message why the setting value can
     * not be retrieved.
     */
    using SettingStatusAndValue = std::pair<SettingStatus, std::string>;

    /**
     * Destructor.
     */
    virtual ~DeviceSettingStorageInterface() = default;

    /**
     * Open an existing database.
     *
     * @return @c true If the database is opened ok or is already open @c false if this object is already managing an
     * open database, or if there is another internal reason the database could not be opened.
     */
    virtual bool open() = 0;

    /**
     * Close the currently open database, if one is open.
     */
    virtual void close() = 0;

    /**
     * Stores a single setting in the database.
     *
     * @param key The string identifier for the setting.
     * @param value The string value of the setting.
     * @param status The status of the setting.
     * @return Whether the setting was successfully stored.
     */
    virtual bool storeSetting(const std::string& key, const std::string& value, SettingStatus status) = 0;

    /**
     * Retrieves the setting status and value from the database.
     *
     * @param key The string identifier for the setting.
     * @return The status and value of the setting. The value will contain the reason or error message if the status is
     * @c NOT_AVAILABLE.
     */
    virtual SettingStatusAndValue loadSetting(const std::string& key) = 0;

    /**
     * Removes the entry for a particular setting in the database.
     *
     * @param key The string identifier for the setting.
     * @return true if deletion is successful, false otherwise
     */
    virtual bool deleteSetting(const std::string& key) = 0;

    /**
     * Update the status of a setting  in the database. The update will fail if the setting does not exist.
     *
     * @param key The string identifier for the setting.
     * @param status The status of the setting.
     * @return Whether the status was successfully updated.
     */
    virtual bool updateSettingStatus(const std::string& key, SettingStatus status) = 0;
};

}  // namespace storage
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_STORAGE_DEVICESETTINGSTORAGEINTERFACE_H_
