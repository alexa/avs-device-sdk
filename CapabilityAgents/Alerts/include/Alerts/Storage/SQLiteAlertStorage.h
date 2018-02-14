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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_STORAGE_SQLITEALERTSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_STORAGE_SQLITEALERTSTORAGE_H_

#include "Alerts/Storage/AlertStorageInterface.h"

#include <sqlite3.h>

#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace storage {

/**
 * An implementation that allows us to store alerts using SQLite.
 *
 * TODO: ACSDK-390 Investigate adding an abstraction layer between this class and the AlertStorageInterface,
 * where the middle layer is expressed purely in SQL.
 */
class SQLiteAlertStorage : public AlertStorageInterface {
public:
    /**
     * Constructor.
     */
    SQLiteAlertStorage(
        const std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface>& alertsAudioFactory);

    bool createDatabase(const std::string& filePath) override;

    bool open(const std::string& filePath) override;

    bool isOpen() override;

    void close() override;

    bool alertExists(const std::string& token) override;

    bool store(std::shared_ptr<Alert> alert) override;

    bool load(std::vector<std::shared_ptr<Alert>>* alertContainer) override;

    bool modify(std::shared_ptr<Alert> alert) override;

    bool erase(std::shared_ptr<Alert> alert) override;

    bool erase(const std::vector<int>& alertDbIds) override;

    bool clearDatabase() override;

    void printStats(StatLevel level) override;

private:
    /**
     * Utility function to migrate an existing V1 Alerts database file to the V2 format.
     *
     * The expectation for V2 is that a table with the name 'alerts_v2' exists.
     *
     * If this table does not exist, then this function will create it, and the additional tables that V2 expects,
     * and then load all alerts from the V1 table and save them into the V2 table.
     *
     * @param dbHandle A SQLite handle to an open database.
     * @return Whether the migration was successful.  Returns true by default if the db is already V2.
     */
    bool migrateAlertsDbFromV1ToV2();

    /**
     * A utility function to help us load alerts from different versions of the alerts table.  Currently, versions
     * 1 and 2 are supported.
     *
     * @param dbVersion The version of the database we wish to load from.
     * @param[out] alertContainer The container where alerts should be stored.
     * @return Whether the alerts were loaded ok.
     */
    bool loadHelper(int dbVersion, std::vector<std::shared_ptr<Alert>>* alertContainer);

    /// The sqlite database handle.
    sqlite3* m_dbHandle;

    /// A member that stores a factory that produces audio streams for alerts.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AlertsAudioFactoryInterface> m_alertsAudioFactory;
};

}  // namespace storage
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_STORAGE_SQLITEALERTSTORAGE_H_
