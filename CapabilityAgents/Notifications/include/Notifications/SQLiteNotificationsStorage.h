/*
 * SQLiteNotificationsStorage.h
 *
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

#include <mutex>
#include <sqlite3.h>

#include "Notifications/NotificationsStorageInterface.h"

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
     * Constructor.
     * Initializes m_dbHandle to nullptr.
     */
    SQLiteNotificationsStorage();

    ~SQLiteNotificationsStorage();

    bool createDatabase(const std::string& filePath) override;

    bool open(const std::string& filePath) override;

    bool isOpen() const override;

    void close() override;

    bool enqueue(const NotificationIndicator& notificationIndicator) override;

    bool dequeue() override;

    bool peek(NotificationIndicator* notificationIndicator) override;

    bool setIndicatorState(IndicatorState state) override;

    bool getIndicatorState(IndicatorState* state) const override;

    bool checkForEmptyQueue(bool* empty) const override;

    bool clearNotificationIndicators() override;

    bool getQueueSize(int* size) const override;

private:
    /**
     * Utility function to get the next record in the database. This method is not thread-safe.
     * @param notificationIndicator A pointer to hold the next notificationIndicator in the database.
     * @return Whether the getNext operation was successful.
     */
    bool getNextNotificationIndicatorLocked(NotificationIndicator* notificationIndicator);

    /// The sqlite database handle.
    sqlite3* m_dbHandle;

    /// A mutex to protect database access.
    std::mutex m_databaseMutex;
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_SQLITENOTIFICATIONSSTORAGE_H_
