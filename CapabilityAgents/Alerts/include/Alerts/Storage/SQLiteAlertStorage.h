/*
 * SQLiteAlertStorage.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_STORAGE_SQLITE_ALERT_STORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_STORAGE_SQLITE_ALERT_STORAGE_H_

#include "Alerts/Storage/AlertStorageInterface.h"

#include <sqlite3.h>

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
    SQLiteAlertStorage();

    bool createDatabase(const std::string & filePath) override;

    bool open(const std::string & filePath) override;

    bool isOpen() override;

    void close() override;

    bool alertExists(const std::string & token) override;

    bool store(std::shared_ptr<Alert> alert) override;

    bool load(std::vector<std::shared_ptr<Alert>>* alertContainer) override;

    bool modify(std::shared_ptr<Alert> alert) override;

    bool erase(std::shared_ptr<Alert> alert) override;

    bool erase(const std::vector<int> & alertDbIds) override;

    bool clearDatabase() override;

    void printStats(StatLevel level) override;

private:
    /// The sqlite database handle.
    sqlite3* m_dbHandle;
};

} // namespace storage
} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_STORAGE_SQLITE_ALERT_STORAGE_H_
