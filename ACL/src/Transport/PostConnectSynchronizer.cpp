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

#include <set>
#include <utility>

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/PostConnectSynchronizer.h"
#include "ACL/Transport/TransportDefines.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectSynchronizer");

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

/**
 * Method that creates post-connect object.
 */
std::shared_ptr<PostConnectSynchronizer> PostConnectSynchronizer::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "contextManagerNullData"));
        return nullptr;
    }

    return std::shared_ptr<PostConnectSynchronizer>{new PostConnectSynchronizer(contextManager)};
}

PostConnectSynchronizer::~PostConnectSynchronizer() {
    ACSDK_DEBUG0(LX("~PostConnectSynchronizer"));
    stop();
}

bool PostConnectSynchronizer::doPostConnect(std::shared_ptr<HTTP2Transport> transport) {
    ACSDK_DEBUG0(LX("doPostConnect"));

    if (!transport) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullTransport"));
        return false;
    }

    std::lock_guard<std::mutex> lock{m_mutex};

    if (m_state != State::IDLE) {
        ACSDK_ERROR(LX("doPostConnectFailed").d("reason", "notIdle"));
        return false;
    }

    if (!setStateLocked(State::RUNNING)) {
        ACSDK_ERROR(LX("doPostConnectFailed").d("reason", "setStateRunningFailed"));
        return false;
    }

    m_transport = transport;

    m_mainLoopThread = std::thread(&PostConnectSynchronizer::mainLoop, this);

    return true;
}

void PostConnectSynchronizer::onDisconnect() {
    ACSDK_DEBUG0(LX("onDisconnect"));
    stop();
}

void PostConnectSynchronizer::onContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG5(LX("onContextAvailable").sensitive("context", jsonContext));

    if (!setState(State::SENDING)) {
        // This is expected if m_state is STOPPING.
        ACSDK_DEBUG5(LX("onContextAvailableIgnored").d("reason", "setStateSendingFailed"));
        return;
    }

    auto msgIdAndJsonEvent = avsCommon::avs::buildJsonEventString(
        STATE_SYNCHRONIZER_NAMESPACE, STATE_SYNCHRONIZER_NAME, "", "{}", jsonContext);
    auto postConnectMessage = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);
    postConnectMessage->addObserver(shared_from_this());
    auto transport = getTransport();
    if (transport) {
        transport->sendPostConnectMessage(postConnectMessage);
    }
}

void PostConnectSynchronizer::onContextFailure(const ContextRequestError error) {
    ACSDK_ERROR(LX("onContextFailure").d("error", error));

    if (!setState(State::RUNNING)) {
        // This is expected if m_state is STOPPING.
        ACSDK_DEBUG5(LX("onContextFailureIgnored").d("reason", "setStateRunningFailed"));
    }
}

void PostConnectSynchronizer::onSendCompleted(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG5(LX("onSendCompleted").d("status", status));

    if (status == MessageRequestObserverInterface::Status::SUCCESS ||
        status == MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT) {
        // Capture transport before stop(), which resets m_transport.
        auto transport = getTransport();
        if (stop()) {
            if (transport) {
                transport->onPostConnected();
            }
        } else {
            // This is expected if m_state is STOPPING.
            ACSDK_DEBUG5(LX("onSendCompletedSuccessIgnored").d("reason", "stopFailed"));
        }
    } else {
        if (!setState(State::RUNNING)) {
            ACSDK_ERROR(LX("onSendCompletedFailureIgnored").d("reason", "setStateRunningFailed"));
        }
    }
}

void PostConnectSynchronizer::onExceptionReceived(const std::string& exceptionMessage) {
    // Exceptions are ignored here because the @c onSendCompleted() notification will also
    // be received with a failure status.  If both are processed, the first will trigger
    // a return to the RUNNING and then FETCHING states and the second will try and
    // abort the FETCHING state
    ACSDK_ERROR(LX("onExceptionReceivedIgnored").d("exception", exceptionMessage));
}

PostConnectSynchronizer::PostConnectSynchronizer(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) :
        m_state{State::IDLE},
        m_contextManager{contextManager} {
}

bool PostConnectSynchronizer::setState(State state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return setStateLocked(state);
}

bool PostConnectSynchronizer::setStateLocked(State state) {
    static std::set<std::pair<State, State>> allowsStateTransitions = {{State::IDLE, State::RUNNING},
                                                                       {State::RUNNING, State::FETCHING},
                                                                       {State::RUNNING, State::STOPPING},
                                                                       {State::FETCHING, State::RUNNING},
                                                                       {State::FETCHING, State::SENDING},
                                                                       {State::FETCHING, State::STOPPING},
                                                                       {State::SENDING, State::RUNNING},
                                                                       {State::SENDING, State::STOPPING},
                                                                       {State::STOPPING, State::STOPPED}};

    if (allowsStateTransitions.count({m_state, state}) == 0) {
        ACSDK_ERROR(LX("stateTransitionNotAllowed").d("from", m_state).d("to", state));
        return false;
    }

    ACSDK_DEBUG5(LX("setState").d("from", m_state).d("to", state));
    m_state = state;
    if (State::RUNNING == m_state || State::STOPPING == m_state) {
        m_wakeTrigger.notify_all();
    }
    return true;
}

void PostConnectSynchronizer::mainLoop() {
    ACSDK_DEBUG5(LX("mainLoop"));

    int tryCount = -1;
    auto timeout = std::chrono::milliseconds(0);

    std::unique_lock<std::mutex> lock(m_mutex);

    while (State::RUNNING == m_state) {
        if (tryCount++ > 0) {
            timeout = TransportDefines::RETRY_TIMER.calculateTimeToRetry(tryCount - 1);
            ACSDK_WARN(LX("waitingToRetryPostConnectOperation").d("timeoutMs", timeout.count()));
            if (m_wakeTrigger.wait_for(lock, timeout, [this] { return State::STOPPING == m_state; })) {
                break;
            }
        }

        setStateLocked(State::FETCHING);
        m_contextManager->getContext(shared_from_this());

        m_wakeTrigger.wait(lock, [this] { return State::RUNNING == m_state || State::STOPPING == m_state; });
    }

    ACSDK_DEBUG5(LX("mainLoopReturning"));
}

bool PostConnectSynchronizer::stop() {
    ACSDK_DEBUG5(LX("stop"));

    std::unique_lock<std::mutex> lock(m_mutex);

    if (State::STOPPED == m_state) {
        ACSDK_DEBUG5(LX("stopIgnored").d("reason", "alreadyStopped"));
        return true;
    }
    if (State::STOPPING == m_state) {
        ACSDK_DEBUG5(LX("stopAlreadyInProgress").d("state", m_state));
        m_wakeTrigger.wait(lock, [this]() { return State::STOPPED == m_state; });
        return true;
    }
    if (!setStateLocked(State::STOPPING)) {
        ACSDK_ERROR(LX("stopFailed").d("reason", "setStateStoppingFailed"));
        return false;
    }

    lock.unlock();

    if (m_mainLoopThread.joinable()) {
        m_mainLoopThread.join();
    }

    setTransport(nullptr);

    if (!setState(State::STOPPED)) {
        ACSDK_ERROR(LX("stopFailed").d("reason", "setStateStoppedFailed"));
        return false;
    }

    m_wakeTrigger.notify_all();

    return true;
}

std::shared_ptr<HTTP2Transport> PostConnectSynchronizer::getTransport() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_transport;
}

void PostConnectSynchronizer::setTransport(std::shared_ptr<HTTP2Transport> transport) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_transport = transport;
}

std::ostream& operator<<(std::ostream& stream, const PostConnectSynchronizer::State state) {
    switch (state) {
        case PostConnectSynchronizer::State::IDLE:
            return stream << "IDLE";
        case PostConnectSynchronizer::State::RUNNING:
            return stream << "RUNNING";
        case PostConnectSynchronizer::State::FETCHING:
            return stream << "FETCHING";
        case PostConnectSynchronizer::State::SENDING:
            return stream << "SENDING";
        case PostConnectSynchronizer::State::STOPPING:
            return stream << "STOPPING";
        case PostConnectSynchronizer::State::STOPPED:
            return stream << "STOPPED";
    }
    return stream << "Unknown State: " << static_cast<int>(state);
}

}  // namespace acl
}  // namespace alexaClientSDK
