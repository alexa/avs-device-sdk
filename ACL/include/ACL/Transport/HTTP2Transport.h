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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORT_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORT_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/Utils/HTTP2/HTTP2ConnectionInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>

#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/PingHandler.h"
#include "ACL/Transport/PostConnectFactoryInterface.h"
#include "ACL/Transport/PostConnectObserverInterface.h"
#include "ACL/Transport/PostConnectSendMessageInterface.h"
#include "ACL/Transport/TransportInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Class to create and manage an HTTP/2 connection to AVS.
 */
class HTTP2Transport
        : public std::enable_shared_from_this<HTTP2Transport>
        , public TransportInterface
        , public PostConnectObserverInterface
        , public PostConnectSendMessageInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public ExchangeHandlerContextInterface {
public:
    /*
     *  Defines a set of HTTP2/2 connection settings.
     */
    struct Configuration {
        /*
         * Constructor. Initializes the configuration to default.
         */
        Configuration();

        /// The elapsed time without any activity before sending out a ping.
        std::chrono::seconds inactivityTimeout;
    };

    /**
     * A function that creates a HTTP2Transport object.
     *
     * @param authDelegate The AuthDelegate implementation.
     * @param avsEndpoint The URL for the AVS endpoint of this object.
     * @param http2Connection Instance of HTTP2ConnectionInterface with which to perform HTTP2 operations.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @param attachmentManager The attachment manager that manages the attachments.
     * @param transportObserver The observer of the new instance of TransportInterface.
     * @param postConnectFactory The object used to create @c PostConnectInterface instances.
     * @param configuration An optional configuration to specify HTTP2/2 connection settings.
     * @return A shared pointer to a HTTP2Transport object.
     */
    static std::shared_ptr<HTTP2Transport> create(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        const std::string& avsEndpoint,
        std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionInterface> http2Connection,
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        std::shared_ptr<TransportObserverInterface> transportObserver,
        std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
        Configuration configuration = Configuration());

    /**
     * Method to add a TransportObserverInterface instance.
     *
     * @param transportObserver The observer instance to add.
     */
    void addObserver(std::shared_ptr<TransportObserverInterface> transportObserver);

    /**
     * Method to remove a TransportObserverInterface instance.
     *
     * @param observer The observer instance to remove.
     */
    void removeObserver(std::shared_ptr<TransportObserverInterface> observer);

    /**
     * Get the HTTP2ConnectionInterface instance being used by this HTTP2Transport.
     *
     * @return The HTTP2ConnectionInterface instance being used by this HTTP2Transport.
     */
    std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionInterface> getHTTP2Connection();

    /// @name TransportInterface methods.
    /// @{
    bool connect() override;
    void disconnect() override;
    bool isConnected() override;
    void send(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;
    /// @}

    /// @name PostConnectSendMessageInterface methods.
    /// @{
    void sendPostConnectMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;
    /// @}

    /// @name PostConnectObserverInterface methods.
    /// @{
    void onPostConnected() override;
    /// @}

    /// @name AuthObserverInterface methods
    /// @{
    void onAuthStateChange(
        avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
        avsCommon::sdkInterfaces::AuthObserverInterface::Error error) override;
    /// @}

    /// @name RequiresShutdown methods.
    /// @{
    void doShutdown() override;
    /// @}

    /// @name { ExchangeHandlerContextInterface methods.
    /// @{
    void onDownchannelConnected() override;
    void onDownchannelFinished() override;
    void onMessageRequestSent() override;
    void onMessageRequestTimeout() override;
    void onMessageRequestAcknowledged() override;
    void onMessageRequestFinished() override;
    void onPingRequestAcknowledged(bool success) override;
    void onPingTimeout() override;
    void onActivity() override;
    void onForbidden(const std::string& authToken = "") override;
    std::shared_ptr<avsCommon::utils::http2::HTTP2RequestInterface> createAndSendRequest(
        const avsCommon::utils::http2::HTTP2RequestConfig& cfg) override;
    std::string getEndpoint() override;
    /// @}

private:
    /**
     * Enum to track the (internal) state of the HTTP2Transport
     */
    enum class State {
        /// Initial state, not doing anything.
        INIT,
        /// Waiting for authorization to complete.
        AUTHORIZING,
        /// Making a connection to AVS.
        CONNECTING,
        /// Waiting for a timeout before retrying to connect to AVS.
        WAITING_TO_RETRY_CONNECTING,
        /// Performing operations that require a connection, but which must be done before the connection
        /// becomes widely available.
        POST_CONNECTING,
        /// Connectsed to AVS and available for general use.
        CONNECTED,
        /// Handling the server disconnecting.
        SERVER_SIDE_DISCONNECT,
        /// Tearing down the connection.  Possibly waiting for some streams to complete.
        DISCONNECTING,
        /// The connection is completely shut down.
        SHUTDOWN
    };

    // Friend to allow access to enum class State.
    friend std::ostream& operator<<(std::ostream& stream, HTTP2Transport::State state);

    /**
     * HTTP2Transport Constructor.
     *
     * @param authDelegate The AuthDelegate implementation.
     * @param avsEndpoint The URL for the AVS endpoint of this object.
     * @param http2Connection Instance of HTTP2ConnectionInterface with which to perform HTTP2 operations.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @param attachmentManager The attachment manager that manages the attachments.
     * @param transportObserver The observer of the new instance of TransportInterface.
     * @param postConnect The object used to create PostConnectInterface instances.
     * @param configuration The HTTP2/2 connection settings.
     */
    HTTP2Transport(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        const std::string& avsEndpoint,
        std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionInterface> http2Connection,
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        std::shared_ptr<TransportObserverInterface> transportObserver,
        std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
        Configuration configuration);

    /**
     * Main loop for servicing the various states.
     */
    void mainLoop();

    /**
     * @c mainLoop() handler for the @c State::INIT.
     *
     * @return The current value of @c m_state.
     */
    State handleInit();

    /**
     * @c mainLoop() handler for the @c State::AUTHORIZING.
     *
     * @return The current value of @c m_state.
     */
    State handleAuthorizing();

    /**
     * @c mainLoop() handler for the @c State::CONNECTING.
     *
     * @return The current value of @c m_state.
     */
    State handleConnecting();

    /**
     * @c mainLoop() handler for the @c State::WAITING_TO_RETRY_CONNECTING.
     *
     * @return The current value of @c m_state.
     */
    State handleWaitingToRetryConnecting();

    /**
     * @c mainLoop() handler for the @c State::POST_CONNECTING.
     *
     * @return The current value of @c m_state.
     */
    State handlePostConnecting();

    /**
     * @c mainLoop() handler for the @c State::CONNECTED.
     *
     * @return The current value of @c m_state.
     */
    State handleConnected();

    /**
     * @c mainLoop() handler for the @c State::SERVER_SIDE_DISCONNECT.
     *
     * @return The current value of @c m_state.
     */
    State handleServerSideDisconnect();

    /**
     * @c mainLoop() handler for the @c State::DISCONNECTING.
     *
     * @return The current value of @c m_state.
     */
    State handleDisconnecting();

    /**
     * @c mainLoop() handler for the @c State::SHUTDOWN.
     *
     * @return The current value of @c m_state.
     */
    State handleShutdown();

    /**
     * Enqueue a MessageRequest for sending.
     *
     * @param request The MessageRequest to enqueue.
     * @param beforeConnected Whether or not to only allow enqueuing of messages before connected.
     */
    void enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> request, bool beforeConnected);

    /**
     * Handle sending @c MessageRequests and pings while in @c State::POST_CONNECTING or @c State::CONNECTED.
     *
     * @param whileState Continue sending @c MessageRequests and pings while in this state.
     * @return The current value of @c m_state.
     */
    State sendMessagesAndPings(State whileState);

    /**
     * Set the state to a new state.
     *
     * @note Must *not* be called while @c m_mutex is held by the calling thread.
     *
     * @param newState the new state to transition to.
     * @param changedReason The reason the connection status changed.
     * @return Whether the operation was successful.
     */
    bool setState(
        State newState,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason changedReason);

    /**
     * Set the state to a new state.
     *
     * @note Must be called while @c m_mutex is held by the calling thread.
     *
     * @param newState
     * @param reason The reason the connection status changed.
     * @return
     */
    bool setStateLocked(
        State newState,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason);

    /**
     * Notify observers that this @c HTTP2Transport has established a connection with AVS.
     */
    void notifyObserversOnConnected();

    /**
     * Notify observers that this @c HTTP2Transport is not connected to AVS.
     *
     * @param reason The reason the connection was lost.
     */
    void notifyObserversOnDisconnect(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason);

    /**
     * Notify observers that this @c HTTP2Transport's connection was terminated by AVS.
     */
    void notifyObserversOnServerSideDisconnect();

    /**
     * Get m_state in a thread-safe manner.
     *
     * @return The current value of m_state.
     */
    State getState();

    /// Mutex for accessing @c m_state and @c m_messageQueue
    std::mutex m_mutex;

    /// Condition variable use to wake the @c m_thread from various waits.
    std::condition_variable m_wakeEvent;

    /// The current state of this HTTP2Transport.
    State m_state;

    /// Auth delegate implementation.
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// The URL of the AVS server we will connect to.
    std::string m_avsEndpoint;

    /// The HTTP2ConnectionInterface with which to perform HTTP2 operations.
    std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionInterface> m_http2Connection;

    /// Observer of this class, to be passed received messages from AVS.
    std::shared_ptr<MessageConsumerInterface> m_messageConsumer;

    /// Object for creating and accessing attachments.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;

    /// Factory for creating @c PostConnectInterface instances.
    std::shared_ptr<PostConnectFactoryInterface> m_postConnectFactory;

    /// Mutex to protect access to the m_observers variable.
    std::mutex m_observerMutex;

    /// Observers of this class, to be notified on changes in connection and received attachments.
    std::unordered_set<std::shared_ptr<TransportObserverInterface>> m_observers;

    /// Thread for servicing the network connection.
    std::thread m_thread;

    /// PostConnect object is used to perform activities required once a connection is established.
    std::shared_ptr<PostConnectInterface> m_postConnect;

    /// Queue of @c MessageRequest instances to send. Serialized by @c m_mutex.
    std::deque<std::shared_ptr<avsCommon::avs::MessageRequest>> m_requestQueue;

    /// Number of times connecting has been retried.
    int m_connectRetryCount;

    /// Is a message handler awaiting a response?
    bool m_isMessageHandlerAwaitingResponse;

    /// The number of message handlers that are not finished with their request.
    int m_countOfUnfinishedMessageHandlers;

    /// The current ping handler (if any).
    std::shared_ptr<PingHandler> m_pingHandler;

    /// Time last activity on the connection was observed.
    std::chrono::time_point<std::chrono::steady_clock> m_timeOfLastActivity;

    /// A bool to specify whether a post connect message was already received
    std::atomic<bool> m_postConnected;

    /// The runtime HTTP2/2 connection settings.
    const Configuration m_configuration;

    /// The reason for disconnecting.
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason m_disconnectReason;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORT_H_
