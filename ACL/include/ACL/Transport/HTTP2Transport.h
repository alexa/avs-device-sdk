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
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

#include "AVSCommon/SDKInterfaces/AuthDelegateInterface.h"
#include "AVSCommon/Utils/LibcurlUtils/CurlMultiHandleWrapper.h"
#include "ACL/Transport/HTTP2Stream.h"
#include "ACL/Transport/HTTP2StreamPool.h"
#include "ACL/Transport/MessageConsumerInterface.h"
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
        : public TransportInterface
        , public PostConnectObserverInterface
        , public PostConnectSendMessageInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface
        , public std::enable_shared_from_this<HTTP2Transport> {
public:
    /**
     * A function that creates a HTTP2Transport object.
     *
     * @param authDelegate The AuthDelegate implementation.
     * @param avsEndpoint The URL for the AVS endpoint of this object.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @param attachmentManager The attachment manager that manages the attachments.
     * @param transportObserver The observer to this class.
     * @param postConnectFactory The object used to create @c PostConnectInterface instances.
     * @return A shared pointer to a HTTP2Transport object.
     */
    static std::shared_ptr<HTTP2Transport> create(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        const std::string& avsEndpoint,
        std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        std::shared_ptr<TransportObserverInterface> transportObserver,
        std::shared_ptr<PostConnectFactoryInterface> postConnectFactory);

    /**
     * @inheritDoc
     * Initializes and prepares the multi-handle and downchannel for connection.
     * Also starts the main network thread.
     */
    bool connect() override;

    void disconnect() override;

    bool isConnected() override;

    void onPostConnected() override;

    void sendPostConnectMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;
    void send(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;

    /// @name AuthObserverInterface methods
    /// @{
    void onAuthStateChange(State newState, Error error) override;
    /// @}

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

private:
    /**
     * HTTP2Transport Constructor.
     *
     * @param authDelegate The AuthDelegate implementation.
     * @param avsEndpoint The URL for the AVS endpoint of this object.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @param attachmentManager The attachment manager that manages the attachments.
     * @param postConnect The object used to create PostConnectInterface instances.
     * @param observer The observer to this class.
     */
    HTTP2Transport(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        const std::string& avsEndpoint,
        std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
        std::shared_ptr<TransportObserverInterface> observer);

    /**
     * Notify registered observers on a transport disconnect.
     */
    void notifyObserversOnServerSideDisconnect();

    /**
     * Notify registered observers on a server side disconnect.
     */
    void notifyObserversOnDisconnect(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason);

    /**
     * Notify registered observers on a transport connect.
     */
    void notifyObserversOnConnected();

    void doShutdown() override;

    /// Type defining an entry in the Event queue
    typedef std::pair<CURL*, std::shared_ptr<HTTP2Stream>> ActiveTransferEntry;

    /**
     * Main network loop. Will establish a connection then repeatedly call curl_multi_perform in order to
     * receive data from the AVS backend.
     */
    void networkLoop();

    /**
     * Establishes a connection to AVS.
     *
     * @return Whether the connection was successful or if we're already connected.
     */
    bool establishConnection();

    /**
     * Sets up the downchannel stream. If a downchannel stream already exists, it is torn down and reset.
     *
     * @return Whether the operation was successful.
     */
    bool setupDownchannelStream();

    /**
     * Checks if an active stream is finished and reports the response code the observer.
     */
    void cleanupFinishedStreams();

    /**
     * Check for streams that have not progressed within their timeout and remove them.
     */
    void cleanupStalledStreams();

    /**
     * Checks all the currently executing message requests to see if they have HTTP response codes.
     *
     * @return If all the currently executing message requests have HTTP response codes.
     */
    bool canProcessOutgoingMessage();

    /**
     * Send the next @c MessageRequest if any are queued.
     */
    void processNextOutgoingMessage();

    /**
     * Attempts to create a stream that will send a ping to the backend. If a ping stream is in flight, we do not
     * attempt to create a new one (returning true in this case).
     *
     * @return Whether setting up the HTTP2Stream was successful. Returns true if there is a ping in flight already.
     */
    bool sendPing();

    /**
     * Cleans up the ping stream when the ping request is complete, also identifies if there was an error sending
     * the ping.
     */
    void handlePingResponse();

    /**
     * Set whether or not the network thread is stopping. If transitioning to true, this method wakes up the
     * connection retry loop so that it can break out.
     *
     * @param reason Reason why this transport is stopping.
     */
    void setIsStopping(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason);

    /**
     * Set whether or not the network thread is stopping. If transitioning to true, this method wakes up the
     * connection retry loop so that it can break out.
     * @note This method must be called while @c m_mutex is acquired.
     *
     * @param reason Reason why this transport is stopping.
     */
    void setIsStoppingLocked(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason);

    /**
     * Get whether or not the @c m_networkLoop is stopping.
     *
     * @return Whether or not the network thread stopping.
     */
    bool isStopping();

    /**
     * Get whether or not a viable connection is available (returns @c false if @c m_isStopping to discourage
     * doomed sends).
     * @note This method must be called while @c m_mutex is acquired.
     * @return Whether or not a viable connection is available.
     */
    bool isConnectedLocked() const;

    /**
     * Set the state to connected, unless already stopping. Notifies observer of any change to connection state.
     */
    void setIsConnectedTrueUnlessStopping();

    /**
     * Set the state to not connected. Notifies observer of any change to connection state.
     */
    void setIsConnectedFalse();

    /**
     * Queue a @c MessageRequest for processing (to the back of the queue).
     *
     * @param request The MessageRequest to queue for sending.
     * @param ignoreConnectionStatus set to @c false to block messages to AVS, @c true
     * when invoked through the @c PostConnectSendMessage interface to allow
     * post-connect messages to AVS in the unconnected state.
     * @return Whether the request was enqueued.
     */
    bool enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> request, bool ignoreConnectionStatus = false);

    /**
     * De-queue a @c MessageRequest from (the front of) the queue of @c MessageRequest instances to process.
     *
     * @return The next @c MessageRequest to process (or @c nullptr).
     */
    std::shared_ptr<avsCommon::avs::MessageRequest> dequeueRequest();

    /**
     * Clear the queue of @c MessageRequest instances, but first call @c onSendCompleted(NOT_CONNECTED) for any
     * requests in the queue.
     */
    void clearQueuedRequests();

    /**
     * Release the down channel stream.
     *
     * @param removeFromMulti Whether to remove the stream from @c m_multi as part of releasing the stream.
     * @return Whether the operation was successful.
     */
    bool releaseDownchannelStream(bool removeFromMulti);

    /**
     * Release the ping stream.
     *
     * @param removeFromMulti Whether to remove the stream from @c m_multi as part of releasing the stream.
     * @return Whether the operation was successful.
     */
    bool releasePingStream(bool removeFromMulti = true);

    /**
     * Release all the event streams.
     */
    void releaseAllEventStreams();

    /**
     * Release an event stream.
     *
     * @param stream The event stream to release.
     * @param removeFromMulti Whether to remove the stream from @c m_multi as part of releasing the stream.
     * @return Whether the operation was successful.
     */
    bool releaseEventStream(std::shared_ptr<HTTP2Stream> stream, bool removeFromMulti = true);

    /**
     * Release a stream
     *
     * @param stream The stream to release.
     * @param removeFromMulti Whether to remove the stream from @c m_multi as part of releasing the stream.
     * @param name Name of the stream to release (for logging).
     * @return Whether the operation was successful.
     */
    bool releaseStream(std::shared_ptr<HTTP2Stream> stream, bool removeFromMulti, const std::string& name);

    /**
     * Return whether a specified stream is an 'event' stream.
     *
     * @param stream The stream to check.
     * @return Whether a specified stream is an 'event' stream.
     */
    bool isEventStream(std::shared_ptr<HTTP2Stream> stream);

    /// Mutex to protect access to the m_observers variable.
    std::mutex m_observerMutex;

    /// Observers of this class, to be notified on changes in connection and received attachments.
    std::unordered_set<std::shared_ptr<TransportObserverInterface>> m_observers;

    /// Observer of this class, to be passed received messages from AVS.
    std::shared_ptr<MessageConsumerInterface> m_messageConsumer;

    /// Auth delegate implementation.
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// The URL of the AVS server we will connect to.
    std::string m_avsEndpoint;

    /// Representation of the downchannel stream.
    std::shared_ptr<HTTP2Stream> m_downchannelStream;

    /// Representation of the ping stream.
    std::shared_ptr<HTTP2Stream> m_pingStream;

    /// Represents a CURL multi handle.
    std::unique_ptr<avsCommon::utils::libcurlUtils::CurlMultiHandleWrapper> m_multi;

    /// The list of streams that either do not have HTTP response headers, or have outstanding response data.
    std::map<CURL*, std::shared_ptr<HTTP2Stream>> m_activeStreams;

    /// Main thread for this class.
    std::thread m_networkThread;

    /// An abstracted HTTP/2 stream pool to ensure that we efficiently and correctly manage our active streams.
    HTTP2StreamPool m_streamPool;

    /// Serializes access to various members.
    std::mutex m_mutex;

    /// Reason the connection was lost. Serialized by @c m_mutex.
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason m_disconnectReason;

    /// Keeps track of whether the main network loop is running. Serialized by @c m_mutex.
    bool m_isNetworkThreadRunning;

    //// Whether or not requests for an auth token are expected to provide a valid token.
    bool m_isAuthRefreshed;

    /// Keeps track of whether we're connected to AVS. Serialized by @c m_mutex.
    bool m_isConnected;

    /// Whether or not the @c networkLoop is stopping. Serialized by @c m_mutex.
    bool m_isStopping;

    /// Whether or not the onDisconnected() notification has been sent. Serialized by @c m_mutex.
    bool m_disconnectedSent;

    /// Queue of @c MessageRequest instances to send. Serialized by @c m_mutex.
    std::deque<std::shared_ptr<avsCommon::avs::MessageRequest>> m_requestQueue;

    /// Used to wake the main network thread in connection retry back-off situation.
    std::condition_variable m_wakeRetryTrigger;

    /// Factory for creating @c PostConnectInterface instances.
    std::shared_ptr<PostConnectFactoryInterface> m_postConnectFactory;

    /// PostConnect object is used to perform activities required once a connection is established.
    std::shared_ptr<PostConnectInterface> m_postConnect;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORT_H_
