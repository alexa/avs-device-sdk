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

#ifndef ACSDKALERTS_STORAGE_SQLITEALERTSTORAGE_H_
#define ACSDKALERTS_STORAGE_SQLITEALERTSTORAGE_H_

#include <set>

#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

#include "acsdkAlerts/Storage/AlertStorageInterface.h"

namespace alexaClientSDK {
namespace acsdkAlerts {
namespace storage {

/**
 * An implementation that allows us to store alerts using SQLite. This class is not thread safe.
 *
 * TODO: ACSDK-390 Investigate adding an abstraction layer between this class and the AlertStorageInterface,
 * where the middle layer is expressed purely in SQL.
 */
class SQLiteAlertStorage : public AlertStorageInterface {
public:
    /**
     * Factory method for creating a storage object for Alerts based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @param audioFactory A factory that can produce default alert sounds.
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<AlertStorageInterface> createAlertStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>& audioFactory);

    /**
     * Factory method for creating a storage object for Alerts based on an SQLite database.
     *
     * @deprecated
     * @param configurationRoot The global config object.
     * @param alertsAudioFactory A factory that can produce default alert sounds.
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteAlertStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory);

    /**
     * On destruction, close the underlying database.
     */
    ~SQLiteAlertStorage();

    bool createDatabase() override;

    bool open() override;

    void close() override;

    bool store(std::shared_ptr<Alert> alert) override;

    bool storeOfflineAlert(const std::string& token, const std::string& scheduledTime, const std::string& eventTime)
        override;

    bool load(
        std::vector<std::shared_ptr<Alert>>* alertContainer,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager) override;

    bool loadOfflineAlerts(rapidjson::Value* alertContainer, rapidjson::Document::AllocatorType& allocator) override;

    bool modify(std::shared_ptr<Alert> alert) override;

    bool erase(std::shared_ptr<Alert> alert) override;

    bool eraseOffline(const std::string& token, int id) override;

    bool bulkErase(const std::list<std::shared_ptr<Alert>>& alertList) override;

    bool clearDatabase() override;

    /**
     * An enum class to help debug database contents.  This type is used in the printStats function below.
     */
    enum class StatLevel {
        /// Print only a single line, providing a count of rows from each table.
        ONE_LINE,
        /// Print all details of the Alerts table, summarizing the other tables.
        ALERTS_SUMMARY,
        /// Print all details of all records.
        EVERYTHING
    };

    /**
     * A utility function to print the contents of the database to the SDK logger output.
     * This function is provided for debug use only.
     */
    void printStats(StatLevel level = StatLevel::ONE_LINE);

private:
    /**
     * Constructor.
     *
     * @param dbFilePath The location of the SQLite database file.
     * @param alertsAudioFactory A factory that can produce default alert sounds.
     */
    SQLiteAlertStorage(
        const std::string& dbFilePath,
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory);

    /**
     * A utility function to help us load alerts from different versions of the alerts table.  Currently, versions
     * 1 and 2 are supported.
     *
     * @param dbVersion The version of the database we wish to load from.
     * @param[out] alertContainer The container where alerts should be stored.
     * @param settingsManager A settingsManager object that manages alarm volume ramp setting.
     * @return Whether the alerts were loaded ok.
     */
    bool loadHelper(
        int dbVersion,
        std::vector<std::shared_ptr<Alert>>* alertContainer,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager);

    /**
     * Query whether an alert is currently stored in the alerts table with the given token.
     *
     * @param token The AVS token which uniquely identifies an alert.
     * @return @c true If the alert is stored in the alerts database, @c false otherwise.
     */
    bool alertExists(const std::string& token);

    /**
     * Query whether an alert is currently stored in the offline alerts table with the given token.
     *
     * @param token The AVS token which uniquely identifies an alert.
     * @return @c true If the alert is stored in the offline alerts database, @c false otherwise.
     */
    bool offlineAlertExists(const std::string& token);

    /// A member that stores a factory that produces audio streams for alerts.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> m_alertsAudioFactory;

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;
};

}  // namespace storage
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ACSDKALERTS_STORAGE_SQLITEALERTSTORAGE_H_
