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
     * Establishes a connection to AVS.
     *
     * @return If the connection was successful or if we're already connected.
     */
    bool establishDownchannel();

    /**
     * Main network loop. Will establish a connection then repeatedly call curl_multi_perform in order to
     * receive data from the AVS backend.
     */
    void networkLoop();

    /**
     * Checks if an active stream is finished and reports the response code the observer.
     * Must be locked before calling.
     */
    void cleanupFinishedStreams();

    /**
     * Checks all the currently executing message requests to see if they have HTTP response codes.
     * Must be locked before calling.
     *
     * @return If all the currently executing message requests have HTTP response codes.
     */
    bool canProcessOutgoingMessage();

    /**
     * Attempts to create a stream that will send a ping to the backend. If a ping stream is in flight, we do not
     * attempt to create a new one (returning true in this case).
     *
     * @return Whether setting up the HTTP2Stream was successful.  Returns true if there is a ping in flight already.
     */
    bool sendPing();

    /**
     * Cleans up the ping stream when the ping request is complete, also identifies if there was an error sending
     * the ping.
     */
    void handlePingResponse();

    /**
     * Sets up the downchannel stream. If a downchannel stream already exists, it is torn down and reset.
     *
     * @return Whether the downchannel stream was successfully set up.
     */
    bool setupDownchannelStream();

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
    /// Locks the incoming message queue.
    std::mutex m_mutex;
    /// Represents a CURL multi handle.
    std::unique_ptr<MultiHandle> m_multi;
    /// The list of streams that either do not have HTTP response headers, or have outstanding response data.
    std::map<CURL*, std::shared_ptr<HTTP2Stream>> m_activeStreams;
    /// Main thread for this class.
    std::thread m_networkThread;
    /// An abstracted HTTP/2 stream pool to ensure that we efficiently and correctly manage our active streams.
    HTTP2StreamPool m_streamPool;
    /// Keeps track of whether the main network loop is running.
    std::atomic_bool m_isNetworkThreadRunning;
    /// Keeps track of whether we're connected to AVS.
    std::atomic_bool m_isConnected;
    /**
     * Keeps track of if the message is ready to send. It is possible, through spurious wake ups or destruction
     * where the condition variable will be woken up, but a message should not be sent.
     */
    std::atomic_bool m_readyToSendMessage;
    /// Used to block sending MessageRequests until the connection is ready to do so.
    std::condition_variable m_readyToSendMessageCV;
    /// Mutex to lock for connection ready condition variable.
    std::mutex m_readyToSendMessageCVMutex;
    /// Used to block the main network thread in connection retry backoff situation.
    std::condition_variable m_connectionEstablishedTrigger;
};

} // alexaClientSDK
} // acl

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_TRANSPORT_H_
