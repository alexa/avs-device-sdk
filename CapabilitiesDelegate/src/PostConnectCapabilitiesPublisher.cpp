/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h"
#include "CapabilitiesDelegate/Utils/DiscoveryUtils.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/RetryTimer.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace capabilitiesDelegate::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectCapabilitiesPublisher");

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Table with the retry times on subsequent retries.
static const std::vector<int> RETRY_TABLE = {
    1000,    // Retry 1: 1s
    2000,    // Retry 2: 2s
    4000,    // Retry 3  4s
    8000,    // Retry 4: 8s
    16000,   // Retry 5: 16s
    32000,   // Retry 6: 32s
    64000,   // Retry 7: 64s
    128000,  // Retry 8: 128s
    256000,  // Retry 9: 256s
};

static avsCommon::utils::RetryTimer RETRY_TIMER{RETRY_TABLE};

/// Maximum number of endpoints per event.
static const int MAX_ENDPOINTS_PER_EVENT = 300;

/// The timeout for the Asynchronous response directive (Alexa.EventProcessed) to be received.
static const auto ASYNC_RESPONSE_TIMEOUT = std::chrono::seconds(2);

std::shared_ptr<PostConnectCapabilitiesPublisher> PostConnectCapabilitiesPublisher::create(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints,
    const std::shared_ptr<AuthDelegateInterface>& authDelegate) {
    if (addOrUpdateReportEndpoints.empty() && deleteReportEndpoints.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "endpoint map empty"));
    } else if (!authDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalid auth delegate"));
    } else {
        auto instance = std::shared_ptr<PostConnectCapabilitiesPublisher>(
            new PostConnectCapabilitiesPublisher(addOrUpdateReportEndpoints, deleteReportEndpoints, authDelegate));

        return instance;
    }
    return nullptr;
}

PostConnectCapabilitiesPublisher::PostConnectCapabilitiesPublisher(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints,
    const std::shared_ptr<AuthDelegateInterface>& authDelegate) :
        m_addOrUpdateReportEndpoints{addOrUpdateReportEndpoints},
        m_deleteReportEndpoints{deleteReportEndpoints},
        m_authDelegate{authDelegate},
        m_currentAuthState{AuthObserverInterface::State::UNINITIALIZED},
        m_isStopping{false},
        m_isPerformOperationInvoked{false} {
}

PostConnectCapabilitiesPublisher::~PostConnectCapabilitiesPublisher() {
    stop();
}

unsigned int PostConnectCapabilitiesPublisher::getOperationPriority() {
    return ENDPOINT_DISCOVERY_PRIORITY;
}

void PostConnectCapabilitiesPublisher::onAuthStateChange(
    AuthObserverInterface::State newState,
    AuthObserverInterface::Error newError) {
    ACSDK_DEBUG5(LX(__func__).d("state", newState).d("error", newError));
    std::lock_guard<std::mutex> lock(m_authStatusMutex);
    m_currentAuthState = newState;
    if ((AuthObserverInterface::State::REFRESHED == m_currentAuthState)) {
        m_authStatusReady.notify_one();
    }
}

std::string PostConnectCapabilitiesPublisher::getAuthToken() {
    {
        std::unique_lock<std::mutex> lock(m_authStatusMutex);
        m_authStatusReady.wait(lock, [this]() {
            return (isStopping() || (AuthObserverInterface::State::REFRESHED == m_currentAuthState));
        });

        if (isStopping()) {
            ACSDK_DEBUG0(LX("getAuthTokenFailed").d("reason", "shutdownWhileWaitingForToken"));
            return "";
        }
    }
    return m_authDelegate->getAuthToken();
}

MessageRequestObserverInterface::Status PostConnectCapabilitiesPublisher::sendDiscoveryEvent(
    const std::shared_ptr<PostConnectSendMessageInterface>& postConnectSender,
    const std::string& eventString,
    bool waitForEventProcessed) {
    ACSDK_DEBUG5(LX(__func__));
    std::unique_lock<std::mutex> lock{m_mutex};
    m_eventProcessedWaitEvent.reset();
    m_postConnectRequest.reset();
    m_postConnectRequest = std::make_shared<PostConnectMessageRequest>(eventString);
    lock.unlock();

    postConnectSender->sendPostConnectMessage(m_postConnectRequest);
    auto status = m_postConnectRequest->waitForCompletion();

    ACSDK_DEBUG5(LX(__func__).d("Discovery event status", status));
    if (MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED == status && waitForEventProcessed) {
        ACSDK_DEBUG5(LX(__func__).m("waiting for Event Processed directive"));
        if (!m_eventProcessedWaitEvent.wait(ASYNC_RESPONSE_TIMEOUT)) {
            ACSDK_ERROR(LX("sendDiscoveryEventFailed").d("reason", "Timeout on waiting for Event Processed Directive"));
            status = MessageRequestObserverInterface::Status::TIMEDOUT;
        } else if (isStopping()) {
            status = MessageRequestObserverInterface::Status::CANCELED;
        }
    }

    return status;
}

bool PostConnectCapabilitiesPublisher::sendDiscoveryEventWithRetries(
    const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface>& postConnectSender,
    const std::vector<std::string>& endpointConfigurations,
    bool isAddOrUpdateReportEvent) {
    ACSDK_DEBUG5(LX(__func__));
    m_retryWait.reset();
    int retryAttempt = 0;
    while (!isStopping()) {
        std::string eventString, authToken;
        authToken = getAuthToken();
        if (authToken.empty()) {
            ACSDK_ERROR(LX("sendDiscoveryEventWithRetriesFailed").d("reason", "empty auth token"));
            return false;
        }

        if (isAddOrUpdateReportEvent) {
            auto eventAndEventCorrelationTokenPair = getAddOrUpdateReportEventJson(endpointConfigurations, authToken);
            eventString = eventAndEventCorrelationTokenPair.first;
            m_currentEventCorrelationToken = eventAndEventCorrelationTokenPair.second;
        } else {
            eventString = getDeleteReportEventJson(endpointConfigurations, authToken);
        }

        auto status = sendDiscoveryEvent(postConnectSender, eventString, isAddOrUpdateReportEvent);
        switch (status) {
            case MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED:
                /// Successful response, proceed to send next event if available.
                return true;
            case MessageRequestObserverInterface::Status::INVALID_AUTH:
                m_authDelegate->onAuthFailure(authToken);
                /// FALL-THROUGH
            case MessageRequestObserverInterface::Status::BAD_REQUEST:
                reportDiscoveryStatus(status);
                return false;
            default:
                /// Retriable error
                reportDiscoveryStatus(status);
                break;
        }

        if (m_retryWait.wait(RETRY_TIMER.calculateTimeToRetry(retryAttempt++))) {
            ACSDK_DEBUG5((LX(__func__).m("aborting operation")));
            return false;
        }
    }
    return false;
}

bool PostConnectCapabilitiesPublisher::sendDiscoveryEvents(
    const std::vector<std::string>& endpointConfigurations,
    const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface>& postConnectSender,
    bool isAddOrUpdateReportEvent) {
    int numFullEndpoints = endpointConfigurations.size() / MAX_ENDPOINTS_PER_EVENT;

    /// Send events with MAX_ENDPOINTS_PER_EVENT.
    auto it = endpointConfigurations.begin();
    for (int num = 0; num < numFullEndpoints; ++num) {
        std::vector<std::string> endpointConfigs(it, it + MAX_ENDPOINTS_PER_EVENT);
        if (!sendDiscoveryEventWithRetries(postConnectSender, endpointConfigs, isAddOrUpdateReportEvent)) {
            return false;
        }
        it += MAX_ENDPOINTS_PER_EVENT;
    }

    /// Send events with the remaining endpoints.
    std::vector<std::string> endpointConfigs(it, endpointConfigurations.end());
    return sendDiscoveryEventWithRetries(postConnectSender, endpointConfigs, isAddOrUpdateReportEvent);
}

bool PostConnectCapabilitiesPublisher::sendAddOrUpdateReportEvents(
    const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface>& postConnectSender) {
    ACSDK_DEBUG5(LX(__func__).d("num endpoints", m_addOrUpdateReportEndpoints.size()));

    if (m_addOrUpdateReportEndpoints.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("endpoints list empty"));
        return true;
    }
    /// Collect all endpoint configurations
    std::vector<std::string> allEndpointConfigs;
    for (const auto& endpointIdToConfigPair : m_addOrUpdateReportEndpoints) {
        allEndpointConfigs.push_back(endpointIdToConfigPair.second);
    }

    return sendDiscoveryEvents(allEndpointConfigs, postConnectSender, true);
}

bool PostConnectCapabilitiesPublisher::sendDeleteReportEvents(
    const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface>& postConnectSender) {
    ACSDK_DEBUG5(LX(__func__).d("num endpoints", m_deleteReportEndpoints.size()));

    if (m_deleteReportEndpoints.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("endpoints list empty"));
        return true;
    }

    /// Collect all endpoint configurations
    std::vector<std::string> allEndpointConfigs;
    for (const auto& endpointIdToConfigPair : m_deleteReportEndpoints) {
        allEndpointConfigs.push_back(endpointIdToConfigPair.second);
    }

    return sendDiscoveryEvents(allEndpointConfigs, postConnectSender, false);
}

bool PostConnectCapabilitiesPublisher::performOperation(
    const std::shared_ptr<PostConnectSendMessageInterface>& postConnectMessageSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!postConnectMessageSender) {
        ACSDK_ERROR(LX("performOperationFailed").d("reason", "nullPostConnectSender"));
        return false;
    }

    {
        /// Ensure that performOperation method cannot be called twice in a row.
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isPerformOperationInvoked) {
            ACSDK_ERROR(LX("performOperationFailed").d("reason", "performOperation should only be called once."));
            return false;
        }
        m_isPerformOperationInvoked = true;
    }

    /// Add the auth observer to request the auth token.
    m_authDelegate->addAuthObserver(shared_from_this());

    bool result = false;
    if (sendAddOrUpdateReportEvents(postConnectMessageSender) && sendDeleteReportEvents(postConnectMessageSender)) {
        result = true;
        reportDiscoveryStatus(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
    }

    /// Remove the auth observer as we no longer need it.
    m_authDelegate->removeAuthObserver(shared_from_this());

    return result;
}

void PostConnectCapabilitiesPublisher::stop() {
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<PostConnectMessageRequest> requestCopy;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isStopping) {
            return;
        }
        m_isStopping = true;
        requestCopy = m_postConnectRequest;
    }
    if (requestCopy) {
        requestCopy->shutdown();
    }

    {
        std::lock_guard<std::mutex> lock{m_authStatusMutex};
        m_authStatusReady.notify_one();
    }
    m_retryWait.wakeUp();
    m_eventProcessedWaitEvent.wakeUp();

    {
        /// Reset the observer.
        std::lock_guard<std::mutex> lock{m_observerMutex};
        m_observer.reset();
    }
}

bool PostConnectCapabilitiesPublisher::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

void PostConnectCapabilitiesPublisher::abortOperation() {
    ACSDK_DEBUG5(LX(__func__));
    stop();
}

void PostConnectCapabilitiesPublisher::onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) {
    ACSDK_DEBUG5(LX(__func__));
    if (m_currentEventCorrelationToken == eventCorrelationToken) {
        ACSDK_DEBUG5(LX(__func__).m("valid event correlation token received"));
        m_eventProcessedWaitEvent.wakeUp();
    } else {
        ACSDK_WARN(LX(__func__).m("invalid event correlation token received"));
    }
}

void PostConnectCapabilitiesPublisher::addDiscoveryStatusObserver(
    const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
    ACSDK_DEBUG5(LX(__func__));
    if (!observer) {
        ACSDK_ERROR(LX("addDiscoveryObserverFailed").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_observer.reset();
    m_observer = observer;
}

void PostConnectCapabilitiesPublisher::removeDiscoveryStatusObserver(
    const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) {
    ACSDK_DEBUG5(LX(__func__));
    if (!observer) {
        ACSDK_ERROR(LX("removeDiscoveryObserverFailed").d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::mutex> lock(m_observerMutex);
    if (m_observer == observer) {
        m_observer.reset();
    } else {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "observer not found"));
    }
}

void PostConnectCapabilitiesPublisher::reportDiscoveryStatus(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_observerMutex};
    if (!m_observer) {
        return;
    }
    if (status == MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED) {
        m_observer->onDiscoveryCompleted(m_addOrUpdateReportEndpoints, m_deleteReportEndpoints);
    } else {
        m_observer->onDiscoveryFailure(status);
    }
}

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
