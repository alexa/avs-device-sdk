/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "CapabilitiesDelegate/DiscoveryEventSender.h"
#include "CapabilitiesDelegate/Utils/DiscoveryUtils.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/RetryTimer.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::metrics;
using namespace capabilitiesDelegate::utils;

/// String to identify log entries originating from this file.
#define TAG "DiscoveryEventSender"

/// Activity name for post-connect metrics after sending a report.
#define POST_CONNECT_ADD_OR_UPDATE_REPORT_ACTIVITY "PostConnect" TAG "-sendAddOrUpdateReport"

/// Activity name for post-connect metrics after sending a report.
#define POST_CONNECT_DELETE_REPORT_ACTIVITY "PostConnect" TAG "-sendDeleteReport"

/// Prefix for post-connect data point with status value.
#define POST_CONNECT_STATUS_PREFIX "STATUS-"

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
static constexpr int MAX_ENDPOINTS_PER_ADD_OR_UPDATE_REPORT_EVENT = 300;

/// The timeout for the Asynchronous response directive (Alexa.EventProcessed) to be received.
static const auto ASYNC_RESPONSE_TIMEOUT = std::chrono::seconds(2);

std::shared_ptr<DiscoveryEventSender> DiscoveryEventSender::create(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints,
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    bool waitForEventProcessed,
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    bool postConnect) {
    if (addOrUpdateReportEndpoints.empty() && deleteReportEndpoints.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "endpoint map empty"));
    } else if (!authDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalid auth delegate"));
    } else {
        auto instance = std::shared_ptr<DiscoveryEventSender>(new DiscoveryEventSender(
            addOrUpdateReportEndpoints,
            deleteReportEndpoints,
            authDelegate,
            waitForEventProcessed,
            metricRecorder,
            postConnect));

        return instance;
    }
    return nullptr;
}

DiscoveryEventSender::DiscoveryEventSender(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints,
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    bool waitForEventProcessed,
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    bool postConnect) :
        m_addOrUpdateReportEndpoints{addOrUpdateReportEndpoints},
        m_deleteReportEndpoints{deleteReportEndpoints},
        m_authDelegate{authDelegate},
        m_metricRecorder{metricRecorder},
        m_currentAuthState{AuthObserverInterface::State::UNINITIALIZED},
        m_isStopping{false},
        m_isSendDiscoveryEventsInvoked{false},
        m_waitForEventProcessed{waitForEventProcessed},
        m_postConnect{postConnect} {
}

DiscoveryEventSender::~DiscoveryEventSender() {
    stop();
}

bool DiscoveryEventSender::sendDiscoveryEvents(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) {
    ACSDK_DEBUG5(LX(__func__));

    {
        /// Ensure that sendDiscoveryEvents method cannot be called twice.
        std::lock_guard<std::mutex> lock{m_isSendDiscoveryEventsInvokedMutex};
        if (m_isSendDiscoveryEventsInvoked) {
            ACSDK_ERROR(LX("sendDiscoveryEventsFailed").m("sendDiscoveryEventsAlreadyInvoked"));
            return false;
        }
        m_isSendDiscoveryEventsInvoked = true;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("sendDiscoveryEventsFailed").d("reason", "invalid message sender"));
        return false;
    }

    m_authDelegate->addAuthObserver(shared_from_this());
    bool result = false;
    if (sendAddOrUpdateReportEvents(messageSender) && sendDeleteReportEvents(messageSender)) {
        result = true;
        reportDiscoveryStatus(MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED);
    }
    m_authDelegate->removeAuthObserver(shared_from_this());
    return result;
}

void DiscoveryEventSender::onAuthStateChange(
    AuthObserverInterface::State newState,
    AuthObserverInterface::Error newError) {
    ACSDK_DEBUG5(LX(__func__).d("state", newState).d("error", newError));
    std::lock_guard<std::mutex> lock(m_authStatusMutex);
    m_currentAuthState = newState;
    if ((AuthObserverInterface::State::REFRESHED == m_currentAuthState)) {
        m_authStatusReady.notifyOne();
    }
}

std::string DiscoveryEventSender::getAuthToken() {
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

void DiscoveryEventSender::stop() {
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<WaitableMessageRequest> requestCopy;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isStopping) {
            return;
        }
        m_isStopping = true;
        requestCopy = m_messageRequest;
    }

    if (requestCopy) {
        requestCopy->shutdown();
    }

    {
        std::lock_guard<std::mutex> lock{m_authStatusMutex};
        m_authStatusReady.notifyOne();
    }

    m_retryWait.wakeUp();
    m_eventProcessedWaitEvent.wakeUp();

    {
        /// Reset the observer.
        std::lock_guard<std::mutex> lock{m_observerMutex};
        m_observer.reset();
    }
}

bool DiscoveryEventSender::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

MessageRequestObserverInterface::Status DiscoveryEventSender::sendDiscoveryEvent(
    const std::shared_ptr<MessageSenderInterface>& messageSender,
    const std::string& eventString,
    bool waitForEventProcessed) {
    ACSDK_INFO(LX(__func__));
    ACSDK_DEBUG5(LX(__func__).sensitive("discoveryEvent", eventString));
    std::unique_lock<std::mutex> lock{m_mutex};
    m_eventProcessedWaitEvent.reset();
    m_messageRequest.reset();
    m_messageRequest = std::make_shared<WaitableMessageRequest>(eventString);
    lock.unlock();

    messageSender->sendMessage(m_messageRequest);
    auto status = m_messageRequest->waitForCompletion();

    ACSDK_DEBUG5(LX(__func__).d("Discovery event status", status));
    if (MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED == status && waitForEventProcessed &&
        m_waitForEventProcessed) {
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

bool DiscoveryEventSender::sendDiscoveryEventWithRetries(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
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

        auto status = sendDiscoveryEvent(messageSender, eventString, isAddOrUpdateReportEvent);

#ifdef ACSDK_ENABLE_METRICS_RECORDING
        if (m_postConnect && m_metricRecorder) {
            const auto activityName = isAddOrUpdateReportEvent ? POST_CONNECT_ADD_OR_UPDATE_REPORT_ACTIVITY
                                                               : POST_CONNECT_DELETE_REPORT_ACTIVITY;
            std::stringstream eventNameBuilder;
            eventNameBuilder << POST_CONNECT_STATUS_PREFIX << status;
            auto metricEvent =
                MetricEventBuilder{}
                    .setActivityName(activityName)
                    .addDataPoint(DataPointCounterBuilder{}.setName(eventNameBuilder.str()).increment(1).build())
                    .build();
            if (metricEvent) {
                m_metricRecorder->recordMetric(std::move(metricEvent));
            }
        }
#else
        (void)m_postConnect;
        (void)m_metricRecorder;
#endif

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

bool DiscoveryEventSender::sendDiscoveryEvents(
    const std::vector<std::string>& endpointConfigurations,
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
    bool isAddOrUpdateReportEvent) {
    size_t currentEventSize = 0;
    std::vector<std::string> currentEndpointConfigurationsBuffer;
    auto currentEndpointConfigIt = endpointConfigurations.begin();

    while (currentEndpointConfigIt != endpointConfigurations.end()) {
        size_t currentEndpointConfigSize = currentEndpointConfigIt->size();

        bool sendEvent = false;

        // Check for maximum allowed endpoints in Discovery.AddOrUpdateReport event.
        if (isAddOrUpdateReportEvent) {
            if (currentEndpointConfigurationsBuffer.size() == MAX_ENDPOINTS_PER_ADD_OR_UPDATE_REPORT_EVENT) {
                sendEvent = true;
            }
        }

        // Check for endpoint config size in payload
        if (currentEventSize + currentEndpointConfigSize > MAX_ENDPOINTS_SIZE_IN_PAYLOAD) {
            sendEvent = true;
        }

        if (sendEvent) {
            if (!sendDiscoveryEventWithRetries(
                    messageSender, currentEndpointConfigurationsBuffer, isAddOrUpdateReportEvent)) {
                return false;
            }

            // Reset buffer
            currentEventSize = 0;
            currentEndpointConfigurationsBuffer.clear();
        }

        currentEndpointConfigurationsBuffer.push_back(*currentEndpointConfigIt);
        currentEventSize += currentEndpointConfigSize;

        currentEndpointConfigIt++;
    }

    // Send the remaining endpoints.
    return sendDiscoveryEventWithRetries(messageSender, currentEndpointConfigurationsBuffer, isAddOrUpdateReportEvent);
}

bool DiscoveryEventSender::sendAddOrUpdateReportEvents(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) {
    ACSDK_INFO(LX(__func__).d("num endpoints", m_addOrUpdateReportEndpoints.size()));

    if (m_addOrUpdateReportEndpoints.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("endpoints list empty"));
        return true;
    }
    /// Collect all endpoint configurations
    std::vector<std::string> allEndpointConfigs;
    for (const auto& endpointIdToConfigPair : m_addOrUpdateReportEndpoints) {
        allEndpointConfigs.push_back(endpointIdToConfigPair.second);
    }

    return sendDiscoveryEvents(allEndpointConfigs, messageSender, true);
}

bool DiscoveryEventSender::sendDeleteReportEvents(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) {
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

    return sendDiscoveryEvents(allEndpointConfigs, messageSender, false);
}

void DiscoveryEventSender::onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) {
    ACSDK_DEBUG5(LX(__func__));
    if (m_currentEventCorrelationToken == eventCorrelationToken) {
        ACSDK_DEBUG5(LX(__func__).m("valid event correlation token received"));
        m_eventProcessedWaitEvent.wakeUp();
    } else {
        ACSDK_WARN(LX(__func__).m("invalid event correlation token received"));
    }
}

void DiscoveryEventSender::addDiscoveryStatusObserver(
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

void DiscoveryEventSender::removeDiscoveryStatusObserver(
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

void DiscoveryEventSender::reportDiscoveryStatus(MessageRequestObserverInterface::Status status) {
    ACSDK_INFO(LX(__func__).d("status", status));

    std::shared_ptr<DiscoveryStatusObserverInterface> observer;
    {
        std::lock_guard<std::mutex> lock{m_observerMutex};
        observer = m_observer;
    }

    if (!observer) {
        return;
    }

    if (status == MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED) {
        observer->onDiscoveryCompleted(m_addOrUpdateReportEndpoints, m_deleteReportEndpoints);
    } else {
        observer->onDiscoveryFailure(status);
    }
}

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
