/*
 * MessageRouter.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "AVSUtils/Threading/Executor.h"
#include "ACL/AuthDelegateInterface.h"

#include "ACL/MessageRequest.h"
#include "ACL/Transport/MessageRouterInterface.h"
#include "ACL/Transport/MessageRouterObserverInterface.h"
#include "ACL/Transport/TransportInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"
#include "ACL/Transport/MessageConsumerInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This an abstract base class which specifies the interface to manage an actual connection over some medium to AVS.
 *
 * Implementations of this class are required to be thread-safe.
 */
class MessageRouter: public MessageRouterInterface, public TransportObserverInterface, public MessageConsumerInterface {

public:
    /**
     * Constructor.
     * @param authDelegate An implementation of an AuthDelegate, which will provide valid access tokens with which
     * the MessageRouter can authorize the client to AVS.
     * @param avsEndpoint The endpoint to connect to AVS.
     * @param sendExecutor An Executor on which all incoming requests will be submitted to be sent to the server.
     * @param receiveExecutor An Executor on which all outgoing responses will be submitted to be sent to client
     *     callbacks.
     */
    MessageRouter(
            std::shared_ptr<AuthDelegateInterface> authDelegate,
            const std::string& avsEndpoint,
            std::shared_ptr<avsUtils::threading::Executor> sendExecutor,
            std::shared_ptr<avsUtils::threading::Executor> receiveExecutor);

    /**
     * Destructor.
     */
    virtual ~MessageRouter();

    void enable() override;

    void disable() override;

    ConnectionStatus getConnectionStatus() override;

    void send(std::shared_ptr<MessageRequest> request) override;

    void setAVSEndpoint(const std::string& avsEndpoint) override;

    void setObserver(std::shared_ptr<MessageRouterObserverInterface> observer) override;

    void onConnected() override;

    void onDisconnected(ConnectionChangedReason reason) override;

    void onServerSideDisconnect() override;

    void consumeMessage(std::shared_ptr<Message> message) override;

private:
    /**
     * Creates a new MessageRouter.
     *
     * @param authDelegate The AuthDelegateInterface to use for authentication and authorization with AVS.
     * @param avsEndpoint The URL for the AVS server we will connect to.
     * @param messageConsumerInterface The object which should be notified on messages which arrive from AVS.
     * @param transportObserverInterface A pointer to the transport observer the new transport should notify.
     * @return A new MessageRouter object.
     *
     * TODO: ACSDK-99 Replace this with an injected transport factory.
     */
    virtual std::shared_ptr<TransportInterface> createTransport(
            std::shared_ptr<AuthDelegateInterface> authDelegate,
            const std::string& avsEndpoint,
            MessageConsumerInterface* messageConsumerInterface,
            TransportObserverInterface* transportObserverInterface) = 0;

    /**
     * Notify the connection observer when the status has changed.
     * Architectural note:
     *  * A derived class cannot access the required observer method directly due a friend relationship at the base
     *    class level.  However this method bridges the gap, and allows the observer's public interface to remain
     *    unchanged.
     *
     * @param status The current status of the connection.
     * @param reason The reason the connection status changed.
     */
    void notifyObserverOnConnectionStatusChanged(const ConnectionStatus status, const ConnectionChangedReason reason);

    /**
     * Notify the message observer of an incoming message from AVS.
     * Architectural note:
     *  * A derived class cannot access the required observer method directly due a friend relationship at the base
     *    class level.  However this method bridges the gap, and allows the observer's public interface to remain
     *    unchanged.
     *
     * @param message The message that was received from AVS.
     */
    void notifyObserverOnReceive(std::shared_ptr<Message> message);

    /**
     * Creates a new transport, and begins the connection process. The new transport immediately becomes the active
     * transport. The connection mutex must be locked to call this method.
     */
    void createActiveTransportLocked();

    /**
     * Disconnects all transports. The connection mutex must be locked to call this method.
     *
     * @param reason The reason the last transport was disconnected
     */
    void disconnectAllTransportsLocked(const ConnectionChangedReason reason);

    /// The observer object.
    std::shared_ptr<MessageRouterObserverInterface> m_observer;

    /// The current AVS endpoint
    std::string m_avsEndpoint;

    /// The AuthDelegateInterface which provides a valid access token.
    std::shared_ptr<AuthDelegateInterface> m_authDelegate;

    /*
     * This mutex guards access to all connection related state, specifically the status and all transport interaction.
     *
     * TODO: ACSDK-98 Remove the requirement for this to be a recursive mutex.
     */
    std::recursive_mutex m_connectionMutex;

    /// The current connection status.
    ConnectionStatus m_connectionStatus;

    /// When the MessageRouter is enabled, any disconnect should automatically trigger a reconnect with AVS.
    bool m_isEnabled;

    /// A list of all transports which are not disconnected.
    std::vector<std::shared_ptr<TransportInterface>> m_transports;

    /// The current active transport to send messages on.
    std::shared_ptr<TransportInterface> m_activeTransport;

    /// Executor to execute sending any messages to AVS.
    std::shared_ptr<avsUtils::threading::Executor> m_sendExecutor;

    /// Executor to execute sending any callbacks to the client.
    std::shared_ptr<avsUtils::threading::Executor> m_receiveExecutor;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_H_
