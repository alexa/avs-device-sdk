/*
 * HTTP2Transport.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_TRANSPORT_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_TRANSPORT_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

#include "ACL/AuthDelegateInterface.h"
#include "ACL/Transport/TransportInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"
#include "ACL/Transport/HTTP2Stream.h"
#include "ACL/Transport/HTTP2StreamPool.h"
#include "ACL/Transport/MessageConsumerInterface.h"


namespace alexaClientSDK {
namespace acl {

/**
 * Class to create and manage an HTTP/2 connection to AVS.
 */
class HTTP2Transport : public TransportInterface {
public:
    /**
     * Constructor.
     *
     * @param authDelegate The AuthDelegate implementation.
     * @param avsEndpoint The URL for the AVS endpoint of this object.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @param attachmentManager The attachment manager that manages the attachments.
     * @param observer The optional observer to this class.
     */
    HTTP2Transport(std::shared_ptr<AuthDelegateInterface> authDelegate, const std::string& avsEndpoint,
        MessageConsumerInterface* messageConsumerInterface,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        TransportObserverInterface* observer = nullptr);

    /**
     * Destructor.
     */
    ~HTTP2Transport();

    /**
     * @inheritDoc
     * Initializes and prepares the multi-handle and downchannel for connection.
     * Also starts the main network thread.
     */
    bool connect() override;

    void disconnect() override;

    bool isConnected() override;

    void send(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;

private:
    /**
     * Struct to hold a CURL multi handle and clean up the multi handle on delete.
     */
    struct MultiHandle {
        CURLM* handle;
        ~MultiHandle() {
            curl_multi_cleanup(handle);
        }
    };

    /// Type defining an entry in the Event queue
    typedef std::pair<CURL*, std::shared_ptr<HTTP2Stream>> ActiveTransferEntry;

    /**
     * Sets up the downchannel stream. If a downchannel stream already exists, it is torn down and reset.
     *
     * @return Whether the downchannel stream was successfully set up.
     */
    bool setupDownchannelStreamLocked();

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
     * @return Whether the request was enqueued.
     */
    bool enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> request);

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

    /// Observer of this class, to be notified on changes in connection and received attachments.
    TransportObserverInterface *m_observer;

    /// Observer of this class, to be passed received messages from AVS.
    MessageConsumerInterface *m_messageConsumer;

    /// Auth delegate implementation.
    std::shared_ptr<AuthDelegateInterface> m_authDelegate;

    /// The URL of the AVS server we will connect to.
    std::string m_avsEndpoint;

    /// Representation of the downchannel stream.
    std::shared_ptr<HTTP2Stream> m_downchannelStream;

    /// Representation of the ping stream.
    std::shared_ptr<HTTP2Stream> m_pingStream;

    /// Represents a CURL multi handle.
    std::unique_ptr<MultiHandle> m_multi;

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

    /// Keeps track of whether we're connected to AVS. Serialized by @c m_mutex.
    bool m_isConnected;

    /// Whether or not the @c networkLoop is stopping. Serialized by @c m_mutex.
    bool m_isStopping;

    /// Queue of @c MessageRequest instances to send. Serialized by @c m_mutex.
    std::deque<std::shared_ptr<avsCommon::avs::MessageRequest>> m_requestQueue;

    /// Used to wake the main network thread in connection retry back-off situation.
    std::condition_variable m_wakeRetryTrigger;
};

} // alexaClientSDK
} // acl

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_TRANSPORT_H_
