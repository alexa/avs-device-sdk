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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ABSTRACTCONNECTION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ABSTRACTCONNECTION_H_

#include <memory>
#include <mutex>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * This class reflects a connection to AVS and how it may be observed.
 */
class AbstractConnection {
public:
    /// Type alias for brevity.
    using ConnectionStatusObserverInterface = avsCommon::sdkInterfaces::ConnectionStatusObserverInterface;

    /**
     * Constructor.
     */
    AbstractConnection(
        std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> observers =
            std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>>());

    /**
     * Destructor.
     */
    virtual ~AbstractConnection() = default;

    /**
     * Returns whether the AVS connection is established. If the connection is pending, @c false will be returned.
     *
     * @return Whether the AVS connection is established.
     */
    virtual bool isConnected() const = 0;

    /**
     * Adds an observer to be notified of connection status changes. The observer will be notified of the current
     * connection status before this function returns.
     *
     * @param observer The observer to add.
     */
    void addConnectionStatusObserver(std::shared_ptr<ConnectionStatusObserverInterface> observer);

    /**
     * Removes an observer from being notified of connection status changes.
     *
     * @param observer The observer to remove.
     */
    void removeConnectionStatusObserver(std::shared_ptr<ConnectionStatusObserverInterface> observer);

protected:
    /**
     * Utility function to update our local status variables.
     *
     * @param status The Connection Status.
     * @param reason The reason the Connection Status changed.
     */
    void updateConnectionStatus(
        ConnectionStatusObserverInterface::Status status,
        ConnectionStatusObserverInterface::ChangedReason reason);

    /**
     * Utility function to notify all observers of the current Connection Status and Reason.
     */
    void notifyObservers();

    /**
     * Removes all observers registered for Connection status notifications.
     */
    void clearObservers();

    /// Mutex to protect access to data members.
    std::mutex m_mutex;

    /// The current connection status.  @c m_mutex must be acquired before access.
    ConnectionStatusObserverInterface::Status m_connectionStatus;

    /// The reason we changed to the current connection status.  @c m_mutex must be acquired before access.
    ConnectionStatusObserverInterface::ChangedReason m_connectionChangedReason;

    /// Set of observers to notify when the connection status changes.  @c m_mutex must be acquired before access.
    std::unordered_set<std::shared_ptr<ConnectionStatusObserverInterface>> m_connectionStatusObservers;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ABSTRACTCONNECTION_H_
