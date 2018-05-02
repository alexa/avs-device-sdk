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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AVSCONNECTIONMANAGER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AVSCONNECTIONMANAGER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/AbstractAVSConnectionManager.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/AVSEndpointAssignerInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

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
class AVSConnectionManager
        : public avsCommon::avs::AbstractAVSConnectionManager
        , public avsCommon::sdkInterfaces::MessageSenderInterface
        , public avsCommon::sdkInterfaces::AVSEndpointAssignerInterface
        , public MessageRouterObserverInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * A factory function that creates an AVSConnectionManager object.
     *
     * @param messageRouter The entity which handles sending and receiving of AVS messages.
     * @param isEnabled The enablement setting.  If true, then the created object will attempt to connect to AVS.
     * @param connectionStatusObservers An optional set of observers which will be notified when the connection status
     *     changes. The observers cannot be a nullptr.
     * @param messageObservers An optional set of observer which will be sent messages that arrive from AVS.
     *      The observers cannot be a nullptr.
     * @return The created AVSConnectionManager object.
     */
    static std::shared_ptr<AVSConnectionManager> create(
        std::shared_ptr<MessageRouterInterface> messageRouter,
        bool isEnabled = true,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionStatusObservers =
                std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> messageObservers =
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>>());

    /// @name AVSConnectionManagerInterface method overrides.
    /// @{
    void enable() override;
    void disable() override;
    bool isEnabled() override;
    void reconnect() override;
    bool isConnected() const override;
    void addMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) override;
    void removeMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) override;
    /// @}

    void sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;

    /**
     * @note Set the URL endpoint for the AVS connection.  Calling this function with a new value will cause the
     * current active connection to be closed, and a new one opened to the new endpoint.
     */
    void setAVSEndpoint(const std::string& avsEndpoint) override;

private:
    /**
     * AVSConnectionManager constructor.
     *
     * @param messageRouter The entity which handles sending and receiving of AVS messages.
     * @param connectionStatusObservers An optional set of observers which will be notified when the connection status
     *     changes.
     * @param messageObserver An optional observer which will be sent messages that arrive from AVS.
     */
    AVSConnectionManager(
        std::shared_ptr<MessageRouterInterface> messageRouter,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
            connectionStatusObservers =
                std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>(),
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> messageObserver =
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>>());

    void doShutdown() override;

    void onConnectionStatusChanged(
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;

    void receive(const std::string& contextId, const std::string& message) override;

    /// Internal state to indicate if the Connection object is enabled for making an AVS connection.
    std::atomic<bool> m_isEnabled;

    /// Client-provided message listener, which will receive all messages sent from AVS.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface>> m_messageObservers;

    /// Mutex for message observers.
    std::mutex m_messageObserverMutex;

    /// Internal object that manages the actual connection to AVS.
    std::shared_ptr<MessageRouterInterface> m_messageRouter;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_AVSCONNECTIONMANAGER_H_
