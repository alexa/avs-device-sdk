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

#ifndef ALEXA_CLIENT_SDK_ACSDKALERTS_INCLUDE_ACSDKALERTS_STORAGE_SQLITEALERTSTORAGE_H_
#define ALEXA_CLIENT_SDK_ACSDKALERTS_INCLUDE_ACSDKALERTS_STORAGE_SQLITEALERTSTORAGE_H_

#include <set>

#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/WaitEvent.h>
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
     * @param metricRecorder The @c MetricRecorderInterface used to record metrics.
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<AlertStorageInterface> createAlertStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>& audioFactory,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    /**
     * Factory method for creating a storage object for Alerts based on an SQLite database.
     *
     * @deprecated
     * @param configurationRoot The global config object.
     * @param alertsAudioFactory A factory that can produce default alert sounds.
     * @param metricRecorder The @c MetricRecorderInterface used to record metrics.
     * @return Pointer to the SQLiteAlertStorage object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteAlertStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot,
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

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
     * @param metricRecorder The @c MetricRecorderInterface used to record metrics.
     */
    SQLiteAlertStorage(
        const std::string& dbFilePath,
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * A utility function to help us load alerts from different versions of the alerts table.  Currently, versions
     * 2 and 3 are supported.
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
     * A utility function to store offline alerts for different versions of the offline alerts table. Currently V1 and
     * V2 are supported.
     *
     * @param dbVersion The version of the offline alerts table.
     * @param token The AVS token which uniquely identifies an alert.
     * @param scheduledTime The scheduled time of the alert.
     * @param eventTime The time when alert is stopped.
     * @return Whether the offline alert was stored successfully.
     */
    bool storeOfflineAlertHelper(
        const int dbVersion,
        const std::string& token,
        const std::string& scheduledTime,
        const std::string& eventTime);

    /**
     * A utility function to load offline alerts from different versions of the offline alerts table. Currently V1 and
     * V2 are supported.
     *
     * @param dbVersion The version of the offline alerts table.
     * @param[out] alertContainer The container where alerts should be stored.
     * @param allocator The rapidjson allocator.
     * @return Whether the offline alert was loaded successfully.
     */
    bool loadOfflineAlertsHelper(
        const int dbVersion,
        rapidjson::Value* alertContainer,
        rapidjson::Document::AllocatorType& allocator);

    /**
     * A utility function to erase offline alerts from different versions of the offline alerts table. Currently V1 and
     * V2 are supported.
     *
     * @param dbVersion The version of the offline alerts table.
     * @param token The AVS token which uniquely identifies an alert.
     * @return Whether the offline alert was erased successfully.
     */
    bool eraseOfflineHelper(const int dbVersion, const std::string& token);

    /**
     * Query whether an alert is currently stored in the alerts table with the given token.
     *
     * @param dbVersion The version of the alerts table.
     * @param token The AVS token which uniquely identifies an alert.
     * @return @c true If the alert is stored in the alerts database, @c false otherwise.
     */
    bool alertExists(const int dbVersion, const std::string& token);

    /**
     * Check whether offline alerts table includes column event_time_iso_8601.
     *
     * @return True if event_time_iso_8601 is not included, false otherwise.
     */
    bool isOfflineTableV1Legacy();

    /**
     * Query whether an alert is currently stored in the offline alerts table with the given token.
     *
     * @param dbVersion The version of the offline alerts table.
     * @param token The AVS token which uniquely identifies an alert.
     * @return @c true If the alert is stored in the offline alerts database, @c false otherwise.
     */
    bool offlineAlertExists(const int dbVersion, const std::string& token);

    /**
     * A utility function to migrate data from an existing offline alerts v1 table to v2.
     *
     * @return Whether migrating offline alerts data from v1 to v2 succeeds. If v2 table already exists or if there is
     * no existing v1 table, return true.
     */
    bool migrateOfflineAlertsDbFromV1ToV2();

    /**
     * A utility function to migrate data from an existing alerts v2 table to v3.
     *
     * @return Whether migrating alerts data from v2 to v3 succeeds. If v3 table already exists or if there is no
     * existing v2 table, return true.
     */
    bool migrateAlertsDbFromV2ToV3();

    /**
     * Modify an alert in the databse.
     *
     * @param dbVersion The version of the alerts table.
     * @param alert The alert to be modified.
     * @return Whether modifying the alert succeeds.
     */
    bool modifyAlert(const int dbVersion, std::shared_ptr<Alert> alert);

    /**
     * Retry data migration for db uplevel by using the @c RetryTimer with a list of expotential back-off retry times.
     *
     * @tparam Task The type of task to execute.
     * @tparam Args The argument types for the task to execute.
     * @param task A callable type representing a task.
     * @param args The arguments to call the task with.
     * @return Whether retrying data migration succeeds.
     */
    template <typename Task, typename... Args>
    bool retryDataMigration(Task task, Args&&... args);

    /// A member that stores a factory that produces audio streams for alerts.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> m_alertsAudioFactory;

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_db;

    /// The @c MetricRecorderInterface used to record metrics.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// The retry timer used to restart a task.
    avsCommon::utils::RetryTimer m_retryTimer;

    /// The wait event for a retry.
    avsCommon::utils::WaitEvent m_waitRetryEvent;
};

}  // namespace storage
}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKALERTS_INCLUDE_ACSDKALERTS_STORAGE_SQLITEALERTSTORAGE_H_
