/*
 * PostConnectSynchronizer.cpp
 *
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

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/TransportDefines.h"
#include "ACL/Transport/PostConnectSynchronizer.h"
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectSynchronize");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String to identify the AVS namespace of the event we send.
static const std::string STATE_SYNCHRONIZER_NAMESPACE = "System";

/// String to identify the AVS name of the event we send.
static const std::string STATE_SYNCHRONIZER_NAME = "SynchronizeState";

PostConnectSynchronizer::PostConnectSynchronizer() :
        m_contextFetchInProgress{false},
        m_isPostConnected{false},
        m_isStopping{false},
        m_postConnectThreadRunning{false} {
}

void PostConnectSynchronizer::doShutdown() {
    ACSDK_DEBUG(LX("PostConnectSynchronizer::doShutdown()."));
    std::thread localPostConnectThread;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isStopping) {
            return;
        }
        m_isStopping = true;
        m_wakeRetryTrigger.notify_one();
        m_transport.reset();

        std::swap(m_postConnectThread, localPostConnectThread);
    }
    {
        if (localPostConnectThread.joinable()) {
            localPostConnectThread.join();
        }
    }
    {
        std::lock_guard<std::mutex> lock{m_observerMutex};
        m_observers.clear();
    }
}

bool PostConnectSynchronizer::doPostConnect(std::shared_ptr<HTTP2Transport> transport) {
    if (!transport) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullTransport"));
        return false;
    }

    std::lock_guard<std::mutex> lock{m_mutex};

    /*
     * To handle cases where shutdown was invoked before the post-connect object is
     * called. We do not want to spawn the post-connect thread again which ends up
     * waiting forever.
     */
    if (m_isStopping) {
        return false;
    }

    // This function spawns a worker thread, so ensure this function is called
    // only when the the worker thread is not running.
    if (m_postConnectThreadRunning) {
        ACSDK_ERROR(LX("postConnectFailed").d("reason", "postConnectThreadAlreadyRunning"));
        return false;
    }

    m_transport = transport;

    // Register the post-connect object as a listener to the HTTP m_transport
    // observer interface to stop the thread when the m_transport is
    // disconnected.
    m_transport->addObserver(shared_from_this());

    m_postConnectThreadRunning = true;

    m_postConnectThread = std::thread(&PostConnectSynchronizer::postConnectLoop, this);

    return true;
}

void PostConnectSynchronizer::postConnectLoop() {
    int retryCount = 0;

    ACSDK_DEBUG9(LX("Entering postConnectLoop thread"));

    while (!isStopping() && !isPostConnected()) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_contextFetchInProgress) {
                PostConnectObject::m_contextManager->getContext(shared_from_this());
                m_contextFetchInProgress = true;
            }
        }

        auto retryBackoff = TransportDefines::RETRY_TIMER.calculateTimeToRetry(retryCount);
        retryCount++;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeRetryTrigger.wait_for(lock, retryBackoff, [this] { return m_isPostConnected || m_isStopping; });
    }

    ACSDK_DEBUG9(LX("Exiting postConnectLoop thread"));

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_postConnectThreadRunning = false;
    }
}

void PostConnectSynchronizer::onContextAvailable(const std::string& jsonContext) {
    if (isPostConnected() || isStopping()) {
        ACSDK_DEBUG(LX("onContextAvailableIgnored")
                        .d("reason", "PostConnectSynchronizerDone")
                        .d("m_isPostConnected", m_isPostConnected)
                        .d("m_isStopping", m_isStopping));
        return;
    }

    auto msgIdAndJsonEvent = avsCommon::avs::buildJsonEventString(
        STATE_SYNCHRONIZER_NAMESPACE, STATE_SYNCHRONIZER_NAME, "", "{}", jsonContext);

    auto postConnectMessage = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);
    postConnectMessage->addObserver(shared_from_this());

    /*
     * If the transport pointer held by the post-connect is still valid - not
     * shutdown yet then we send the message through the transport.
     */
    std::shared_ptr<HTTP2Transport> localTransport;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        swap(m_transport, localTransport);
    }

    if (localTransport) {
        ACSDK_DEBUG(LX("onContextAvailable : Send PostConnectMessage to transport"));
        localTransport->sendPostConnectMessage(postConnectMessage);
    }
}

void PostConnectSynchronizer::onContextFailure(const ContextRequestError error) {
    if (isPostConnected() || isStopping()) {
        ACSDK_DEBUG(LX("onContextFailureIgnored")
                        .d("reason", "PostConnectSynchronizerDone")
                        .d("m_isPostConnected", m_isPostConnected)
                        .d("m_isStopping", m_isStopping));
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_contextFetchInProgress = false;
    }

    ACSDK_ERROR(LX("contextRetrievalFailed").d("reason", "contextRequestErrorOccurred").d("error", error));
}

void PostConnectSynchronizer::onSendCompleted(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG(LX("onSendCompleted").d("status", status));
    if (status == MessageRequestObserverInterface::Status::SUCCESS ||
        status == MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT) {
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_isPostConnected = true;
            m_wakeRetryTrigger.notify_one();
        }
        notifyObservers();
    } else {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_contextFetchInProgress = false;
    }
}

void PostConnectSynchronizer::onExceptionReceived(const std::string& exceptionMessage) {
    ACSDK_ERROR(LX("onExceptionReceived").d("exception", exceptionMessage));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_contextFetchInProgress = false;
}

void PostConnectSynchronizer::addObserver(std::shared_ptr<PostConnectObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.insert(observer);
}

void PostConnectSynchronizer::removeObserver(std::shared_ptr<PostConnectObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.erase(observer);
}

void PostConnectSynchronizer::notifyObservers() {
    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onPostConnected();
    }
}

void PostConnectSynchronizer::onServerSideDisconnect(std::shared_ptr<TransportInterface> transport) {
    ACSDK_DEBUG(LX("onServerSideDisconnect()"));
    doShutdown();
}

void PostConnectSynchronizer::onConnected(std::shared_ptr<TransportInterface> transport) {
    ACSDK_DEBUG(LX("onConnected()"));
}

void PostConnectSynchronizer::onDisconnected(
    std::shared_ptr<TransportInterface> transport,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    ACSDK_DEBUG(LX("onDisconnected()"));
    doShutdown();
}

bool PostConnectSynchronizer::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

bool PostConnectSynchronizer::isPostConnected() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isPostConnected;
}

}  // namespace acl
}  // namespace alexaClientSDK
