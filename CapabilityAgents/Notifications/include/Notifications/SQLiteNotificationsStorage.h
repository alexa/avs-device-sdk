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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_SQLITENOTIFICATIONSSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_SQLITENOTIFICATIONSSTORAGE_H_

#include "Notifications/NotificationsStorageInterface.h"

#include <mutex>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * An implementation that allows us to store NotificationIndicators using SQLite.
 *
 */
class SQLiteNotificationsStorage : public NotificationsStorageInterface {
public:
    /**
     * Factory method for creating a storage object for Notifications based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteMessagetStorge object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteNotificationsStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Constructor.
     *
     * @param dbFilePath The location of the SQLite database file.
     */
    SQLiteNotificationsStorage(const std::string& databaseFilePath);

    ~SQLiteNotificationsStorage();

    bool createDatabase() override;

    bool open() override;

    void close() override;

    bool enqueue(const NotificationIndicator& notificationIndicator) override;

    bool dequeue() override;

    bool peek(NotificationIndicator* notificationIndicator) override;

    bool setIndicatorState(IndicatorState state) override;

    bool getIndicatorState(IndicatorState* state) override;

    bool checkForEmptyQueue(bool* empty) override;

    bool clearNotificationIndicators() override;

    bool getQueueSize(int* size) override;

private:
    /**
     * Utility function to get the next record in the database. This method is not thread-safe.
     * @param notificationIndicator A pointer to hold the next notificationIndicator in the database.
     * @return Whether the getNext operation was successful.
     */
    bool getNextNotificationIndicatorLocked(NotificationIndicator* notificationIndicator);

    /// A mutex to protect database access.
    std::mutex m_databaseMutex;

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_database;
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_SQLITENOTIFICATIONSSTORAGE_H_
