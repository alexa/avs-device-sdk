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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSCONNECTIONMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSCONNECTIONMANAGERINTERFACE_H_

#include <memory>

#include "AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h"
#include "AVSCommon/SDKInterfaces/MessageObserverInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class reflects a connection to AVS and how it may be observed.
 */
class AVSConnectionManagerInterface {
public:
    /// Type alias for brevity.
    using ConnectionStatusObserverInterface = avsCommon::sdkInterfaces::ConnectionStatusObserverInterface;
    /**
     * Destructor.
     */
    virtual ~AVSConnectionManagerInterface() = default;

    /**
     * Enable the AVSConnectionManager object to make connections to AVS.  Once enabled, the object will attempt to
     * create a connection to AVS.  If the object is already connected, this function will do nothing.
     */
    virtual void enable() = 0;

    /**
     * Disable the AVSConnectionManager object.  If the object is currently connected to AVS, then calling this
     * function will cause the connection to be closed.  If the object is not connected, then calling this function
     * will do nothing.
     */
    virtual void disable() = 0;

    /**
     * Returns if the object is enabled for making connections to AVS.
     *
     * @return Whether this Connection object is enabled to make connections.
     */
    virtual bool isEnabled() = 0;

    /**
     * This function causes the object, if enabled, to create new connection to AVS.  If the object is already
     * connected, then that connection will be closed and a new one created.  If the object is not connected, but
     * perhaps in the process of waiting for its next connection attempt, then its waiting policy will be reset and
     * it will attempt to create a new connection immediately.
     * If the object is disabled, then this function will do nothing.
     */
    virtual void reconnect() = 0;

    /**
     * Returns whether the AVS connection is established. If the connection is pending, @c false will be returned.
     *
     * @return Whether the AVS connection is established.
     */
    virtual bool isConnected() const = 0;

    /**
     * Adds an observer to be notified of message receptions.
     *
     * @param observer The observer object to add.
     */
    virtual void addMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) = 0;

    /**
     * Removes an observer from being notified of message receptions.
     *
     * @param observer The observer object to remove.
     */
    virtual void removeMessageObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) = 0;

    /**
     * Adds an observer to be notified of connection status changes. The observer will be notified of the current
     * connection status before this function returns.
     *
     * @param observer The observer to add.
     */
    virtual void addConnectionStatusObserver(std::shared_ptr<ConnectionStatusObserverInterface> observer) = 0;

    /**
     * Removes an observer from being notified of connection status changes.
     *
     * @param observer The observer to remove.
     */
    virtual void removeConnectionStatusObserver(std::shared_ptr<ConnectionStatusObserverInterface> observer) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSCONNECTIONMANAGERINTERFACE_H_
