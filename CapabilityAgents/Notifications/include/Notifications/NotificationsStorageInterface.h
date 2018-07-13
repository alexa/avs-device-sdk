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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSSTORAGEINTERFACE_H_

#include <AVSCommon/AVS/IndicatorState.h>
#include "NotificationIndicator.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {

/**
 * This class implements an interface for interacting with a Notifications database.
 *
 * Users should be notified of their pending notifications in the order that SetIndicator directives are received.
 * NotificationIndicators are therefore stored in a queue and should be persisted across system shutdown.
 *
 * This storage is also responsible for maintaining IndicatorState as defined in IndicatorState.h.
 */
class NotificationsStorageInterface {
public:
    /// Some useful shorthand.
    using IndicatorState = avsCommon::avs::IndicatorState;

    virtual ~NotificationsStorageInterface() = default;

    /**
     * Creates a new database.
     * If a database is already being handled by this object or there is another internal error, then this function
     * returns false.
     *
     * @return @c true If the database is created ok, or @c false if a database is already being handled by this object
     * or there is a problem creating the database.
     */
    virtual bool createDatabase() = 0;

    /**
     * Open an existing database.  If this object is already managing an open database, or there is a problem opening
     * the database, this function returns false.
     *
     * @return @c true If the database is opened ok, @c false if this object is already managing an open database, or if
     * there is another internal reason the database could not be opened.
     */
    virtual bool open() = 0;

    /**
     * Close the currently open database, if one is open.
     */
    virtual void close() = 0;

    /**
     * Enqueues a single @c NotificationIndicator in the database.
     *
     * @param notificationIndicator The @c NotificationIndicator to enqueue.
     * @return Whether the @c NotificationIndicator was successfully enqueued.
     */
    virtual bool enqueue(const NotificationIndicator& notificationIndicator) = 0;

    /**
     * Dequeues the next @c NotificationIndicator in the database.
     *
     * @return Whether the dequeue operation was successful.
     */
    virtual bool dequeue() = 0;

    /**
     * Peeks at the next @c NotificationIndicator in the database.
     *
     * @param [out] notificationIndicator A pointer to receive the peeked @c NotificationIndicator
     * @return Whether the peek operation was successful.
     */
    virtual bool peek(NotificationIndicator* notificationIndicator) = 0;

    /**
     * Stores the current @c IndicatorState.
     *
     * @return Whether the store operation was successful.
     */
    virtual bool setIndicatorState(IndicatorState state) = 0;

    /**
     * Retrieves the currently stored @c IndicatorState.
     *
     * @param [out] state A pointer to receive the currently stored @c IndicatorState
     * @return Whether the get operation was successful.
     * @note The default IndicatorState for a new database is IndicatorState::OFF.
     */
    virtual bool getIndicatorState(IndicatorState* state) = 0;

    /**
     * Checks if there are any NotificationIndicator records in the database.
     *
     * @param [out] empty Whether there were any records in the database.
     * @return Whether the operation was successful.
     */
    virtual bool checkForEmptyQueue(bool* empty) = 0;

    /**
     * Clears the database of all @c NotificationIndicators.
     *
     * @return Whether the clear operation was successful.
     */
    virtual bool clearNotificationIndicators() = 0;

    /**
     * Gets the size of the queue (number of records in the queue table).
     *
     * @param [out] size A pointer to receive the calculated size.
     * @return Whether the size operation was successful.
     */
    virtual bool getQueueSize(int* size) = 0;
};

}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_NOTIFICATIONS_INCLUDE_NOTIFICATIONS_NOTIFICATIONSSTORAGEINTERFACE_H_
