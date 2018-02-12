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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

#include "ACL/Transport/PostConnectObject.h"
#include "ACL/Transport/PostConnectSendMessageInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace acl {

class HTTP2Transport;

/**
 * Class that posts the StateSynchronizer message to the AVS.
 */
class PostConnectSynchronizer
        : public PostConnectObject
        , public avsCommon::sdkInterfaces::ContextRequesterInterface
        , public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public TransportObserverInterface
        , public std::enable_shared_from_this<PostConnectSynchronizer> {
public:
    /**
     * PostConnectSynchronizer Constructor.
     */
    PostConnectSynchronizer();

    bool doPostConnect(std::shared_ptr<HTTP2Transport> transport) override;

    void onSendCompleted(MessageRequestObserverInterface::Status status) override;
    void onExceptionReceived(const std::string& exceptionMessage) override;

    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;

    void onServerSideDisconnect(std::shared_ptr<TransportInterface> transport) override;
    void onConnected(std::shared_ptr<TransportInterface> transport) override;
    void onDisconnected(
        std::shared_ptr<TransportInterface> transport,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;

    void addObserver(std::shared_ptr<PostConnectObserverInterface> observer) override;
    void removeObserver(std::shared_ptr<PostConnectObserverInterface> observer) override;

private:
    void doShutdown() override;

    void notifyObservers() override;

    /**
     * Loop to fetch the context and send a post-connect message to AVS.
     */
    void postConnectLoop();

    /**
     * Thread safe method which returns if the post-connect object.
     * is being stopped.
     */
    bool isStopping();

    /**
     * Thread safe method which returns if the post-connect object.
     * is post-connected.
     */
    bool isPostConnected();

    /**
     * Set when a context fetch is in progress and reset when the context fetch fails or when the send
     * of SynchronizeState event fails needing another context fetch.
     */
    bool m_contextFetchInProgress;

    /// Set after the postConnection is completed.
    bool m_isPostConnected;

    /**
     * Set when the post-connect object is stopped.  This can happen for the following reasons tranport
     * disconnects, transport receives a server side disconnect or the AVSConnectManageer is shutdown.
     */
    bool m_isStopping;

    /// bool to identfy if the post-connect loop is still running.
    bool m_postConnectThreadRunning;

    /// Thread which runs the state synchronization operation.
    std::thread m_postConnectThread;

    /// Mutex to protect the m_observers variable.
    std::mutex m_observerMutex;

    /// Unordered set of registered observers.
    std::unordered_set<std::shared_ptr<PostConnectObserverInterface>> m_observers;

    /// Serializes access to members.
    std::mutex m_mutex;

    /// Condition variable on which m_postConnectThread waits.
    std::condition_variable m_wakeRetryTrigger;

    /// The HTTP2Transport object to which the StateSynchronizer message is sent.
    std::shared_ptr<HTTP2Transport> m_transport;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZER_H_
