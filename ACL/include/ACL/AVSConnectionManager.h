/*
 * AVSConnectionManager.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AVS_CONNECTION_MANAGER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AVS_CONNECTION_MANAGER_H_

#include <atomic>
#include <memory>
#include <string>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>

#include "ACL/Values.h"
#include "ACL/AuthDelegateInterface.h"
#include "ACL/ConnectionStatusObserverInterface.h"
#include "ACL/Transport/MessageRouterInterface.h"
#include "ACL/Transport/MessageRouterObserverInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This class is designed to manage a connection with AVS seamlessly for the client, internally taking care of the
 * following idiosyncracies:
 *
 *  - connection retry upon failure (for example, network timeout)
 *  - allowing an underlying backoff strategy for subsequent reconnects
 *  - ping management
 *  - periodically reconnecting when AVS performs a server-initiated disconnect
 *  - allowing a client to fully enable or disable connection management
 *  - allowing a client to reset the internal logic of the AVSConnectionManager, which may have utility in the
 *    situation where the client code has particular knowledge that the network is immediately available.
 *
 * The api provided by this class is designed around the above feature set.  As such, there is no explicit 'connect'
 * function, but rather if an object of this class is enabled, then that object will attempt to make a connection to
 * AVS on the client's behalf.
 *
 * If the client wishes the object to not connect to AVS, or to disconnect from AVS at any time, the 'disable' function
 * should be called.
 *
 * If the client wishes the object to either break the current connection to AVS and create a new one, or that the
 * object should try a reconnect immediately from a non-connected (but perhaps in an internally waiting state),
 * then the 'reconnect' function should be called.
 *
 * Finally, the client may send outgoing messages to AVS via the non-blocking 'send' function.
 *
 * Credentials to authenticate the client with AVS must be provided by an implementation of the AuthDelegate class.
 *
 * This SDK also provides observer interfaces for classes that may be notified when the connection status
 * changes, and when messages are received from AVS.  These observer objects are optional.
 */
class AVSConnectionManager :
    public avsCommon::sdkInterfaces::MessageSenderInterface,
    public MessageRouterObserverInterface {
public:
    /**
     * A factory function that creates an AVSConnectionManager object.
     *
     * @param messageRouter The entity which handles sending and receiving of AVS messages.
     * @param isEnabled The enablement setting.  If true, then the created object will attempt to connect to AVS.
     * @param connectionStatusObserver An optional observer which will be notified when the connection status changes.
     * @param messageObserver An optional observer which will be sent messages that arrive from AVS.
     * @return The created AVSConnectionManager object.
     */
    static std::shared_ptr<AVSConnectionManager>
            create(std::shared_ptr<MessageRouterInterface> messageRouter,
                   bool isEnabled = true,
                   std::shared_ptr<ConnectionStatusObserverInterface> connectionStatusObserver = nullptr,
                   std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> messageObserver = nullptr);

    /**
     * Enable the AVSConnectionManager object to make connections to AVS.  Once enabled, the object will attempt to
     * create a connection to AVS.  If the object is already connected, this function will do nothing.
     */
    void enable();

    /**
     * Disable the AVSConnectionManager object.  If the object is currently connected to AVS, then calling this
     * function will cause the connection to be closed.  If the object is not connected, then calling this function
     * will do nothing.
     */
    void disable();

    /**
     * Returns if the object is enabled for making connections to AVS.
     *
     * @return Whether this Connection object is enabled to make connections.
     */
    bool isEnabled();

    /**
     * This function causes the object, if enabled, to create new connection to AVS.  If the object is already
     * connected, then that connection will be closed and a new one created.  If the object is not connected, but
     * perhaps in the process of waiting for its next connection attempt, then its waiting policy will be reset and
     * it will attempt to create a new connection immediately.
     * If the object is disabled, then this function will do nothing.
     */
    void reconnect();

    /**
     * Set the URL endpoint for the AVS connection.  Calling this function with a new value will cause the
     * current active connection to be closed, and a new one opened to the new endpoint.
     * @param avsEndpoint The URL for the new AVS endpoint.
     */
    void setAVSEndpoint(const std::string& avsEndpoint);

    void sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;

private:

    /**
     * AVSConnectionManager constructor.
     *
     * @param messageRouter The entity which handles sending and receiving of AVS messages.
     * @param connectionStatusObserver An optional observer which will be notified when the connection status changes.
     * @param messageObserver An optional observer which will be sent messages that arrive from AVS.
     */
    AVSConnectionManager(std::shared_ptr<MessageRouterInterface> messageRouter,
                         std::shared_ptr<ConnectionStatusObserverInterface> connectionStatusObserver = nullptr,
                         std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> messageObserver = nullptr);

    void onConnectionStatusChanged(const ConnectionStatus status,
                                   const ConnectionChangedReason reason) override;

    void receive(const std::string & contextId, const std::string & message) override;

    /// Internal state to indicate if the Connection object is enabled for making an AVS connection.
    std::atomic<bool> m_isEnabled;

    /// Client-provided connection status observer.
    std::shared_ptr<ConnectionStatusObserverInterface> m_connectionStatusObserver;

    /// Client-provided message listener, which will receive all messages sent from AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> m_messageObserver;

    /// Internal object that manages the actual connection to AVS.
    std::shared_ptr<MessageRouterInterface> m_messageRouter;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AVS_CONNECTION_MANAGER_H_
