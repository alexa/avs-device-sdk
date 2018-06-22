/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CONNECTIONSTATUSOBSERVER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CONNECTIONSTATUSOBSERVER_H_

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>

namespace alexaClientSDK {
namespace integration {

/**
 * The class implements ConnectionStatusObserverInterface for testing.
 */
class ConnectionStatusObserver : public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface {
public:
    /**
     * ConnectionStatusObserver constructor.
     */
    ConnectionStatusObserver();

    void onConnectionStatusChanged(
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status connectionStatus,
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;
    /**
     * The utility function to get the connection status.
     * @return Status The @c connectionStatus for the connection.
     */
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status getConnectionStatus() const;

    /**
     * Function to allow waiting for an expected status when a connection or disconnection is done.
     * @param connectionStatus The expected connection status for which the waiting is done.
     * @param duration The maximum time waiting for the expected connectionStatus.
     * @return true if expected connectionStatus is received within @c duration else false.
     */
    bool waitFor(
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status connectionStatus,
        const std::chrono::seconds duration = std::chrono::seconds(15));

    /**
     * Function to check if the connection is broken due to Server side Disconnect.
     * @return true if the disconnect happens due to SERVER_SIDE_DISCONNECT else false.
     */
    bool checkForServerSideDisconnect();

private:
    /// Mutex used internally to enforce thread safety and serialize read/write access to @c m_statusChanges.
    mutable std::mutex m_mutex;
    /// The cv used when waiting for a particular status of a connection
    std::condition_variable m_wakeTrigger;
    /// The queue of values of the pair (Connection status, ChangedReason) throughout the connection.
    std::deque<std::pair<ConnectionStatusObserverInterface::Status, ConnectionStatusObserverInterface::ChangedReason>>
        m_statusChanges;
};

}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CONNECTIONSTATUSOBSERVER_H_
