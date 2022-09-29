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

#include "SynchronizeStateSender/PostConnectSynchronizeStateSender.h"

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/RetryTimer.h>

#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

namespace alexaClientSDK {
namespace synchronizeStateSender {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
#define TAG "PostConnectSynchronizeStateSender"

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Metric Activity Name Prefix for PostConnectSynchronizeStateSender metric source
#define METRIC_ACTIVITY_NAME_PREFIX "POSTCONNECT_SYNCHRONIZE_STATE_SENDER-"

/// Metric activity name for PostConnectSynchronizeStateSender#retrySynchronizeStateEvent().
#define RETRY_SYNCHRONIZE_ACTIVITY_NAME METRIC_ACTIVITY_NAME_PREFIX "retrySynchronizeStateEvent"

/// Prefix for post-connect data point with status value.
#define POST_CONNECT_STATUS_PREFIX "STATUS-"

/// The namespace for the SynchronizeState event.
static const std::string SYNCHRONIZE_STATE_NAMESPACE = "System";

/// The name of the SynchronizeState event.
static const std::string SYNCHRONIZE_STATE_NAME = "SynchronizeState";

/// Table with the retry times on subsequent retries.
static const std::vector<int> RETRY_TABLE = {
    500,     // Retry 1:  0.5s
    1000,    // Retry 1: 1s
    2000,    // Retry 2: 2s
    4000,    // Retry 3  4s
    8000,    // Retry 4: 8s
    16000,   // Retry 5: 16s
    32000,   // Retry 6: 32s
    64000,   // Retry 7: 64s
    128000,  // Retry 8: 128s
    256000   // Retry 9: 256s
};

#ifdef ACSDK_ENABLE_METRICS_RECORDING
/**
 * Submits metric event if not null. If metric event is null, produces an error message.
 *
 * @param metricRecorder Metric recorder interface. If nullptr, metric is not submitted.
 * @param metricEvent  Metric event to submit. If nullptr, an error is added to the log.
 */
static void submitEvent(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    std::shared_ptr<MetricEvent>&& metricEvent) {
    if (metricEvent == nullptr) {
        ACSDK_ERROR(LX("Error creating metric."));
        return;
    }
    recordMetric(metricRecorder, std::move(metricEvent));
}

/**
 * Handles a Metric event by creating and recording it. Failure to create or record the event results
 * in an early return.
 *
 * @param metricRecorder The @c MetricRecorderInterface which records Metric events.
 * @param activityName The activity name of the Metric event.
 * @param eventName The data point name of the Metric event.
 * @param reason Additional information on cause of Metric event.
 */
static void submitMetric(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder,
    const char* activityName,
    const char* eventName,
    const char* reason) {
    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(activityName)
                           .addDataPoint(DataPointCounterBuilder{}.setName(eventName).increment(1).build())
                           .addDataPoint(DataPointStringBuilder{}.setName("REASON").setValue(reason).build())
                           .build();

    submitEvent(metricRecorder, std::move(metricEvent));
}
#endif

/// The instance of the @c RetryTimer used to calculate retry backoff times.
static avsCommon::utils::RetryTimer RETRY_TIMER{RETRY_TABLE};

/// Timeout value for ContextManager to return the context.
std::chrono::milliseconds CONTEXT_FETCH_TIMEOUT = std::chrono::milliseconds(2000);

std::shared_ptr<PostConnectSynchronizeStateSender> PostConnectSynchronizeStateSender::create(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    ACSDK_DEBUG5(LX(__func__));

    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
    } else {
        return std::shared_ptr<PostConnectSynchronizeStateSender>(
            new PostConnectSynchronizeStateSender(contextManager, metricRecorder));
    }
    return nullptr;
}

PostConnectSynchronizeStateSender::PostConnectSynchronizeStateSender(
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) :
        m_contextManager{contextManager},
        m_metricRecorder{metricRecorder},
        m_isStopping{false} {
    ACSDK_INFO(LX("init").p("this", this));
}

PostConnectSynchronizeStateSender::~PostConnectSynchronizeStateSender() {
    ACSDK_INFO(LX("destroyed").p("this", this));
}

unsigned int PostConnectSynchronizeStateSender::getOperationPriority() {
    return SYNCHRONIZE_STATE_PRIORITY;
}

void PostConnectSynchronizeStateSender::onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    ACSDK_ERROR(LX(__func__).d("reason", error));
    m_wakeTrigger.notifyAll();
}

void PostConnectSynchronizeStateSender::onContextAvailable(const std::string& jsonContext) {
    ACSDK_DEBUG5(LX(__func__));
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_contextString = jsonContext;
    }
    m_wakeTrigger.notifyAll();
}

bool PostConnectSynchronizeStateSender::fetchContext() {
    ACSDK_DEBUG5(LX(__func__));
    std::unique_lock<std::mutex> lock{m_mutex};
    m_contextString = "";
    m_contextManager->getContext(shared_from_this());

    auto pred = [this] { return !m_contextString.empty() || m_isStopping; };

    if (!m_wakeTrigger.waitFor(lock, CONTEXT_FETCH_TIMEOUT, pred)) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "context fetch timeout"));
        return false;
    }

    if (m_contextString.empty()) {
        ACSDK_ERROR(LX(__func__).m("invalid context received."));
        return false;
    }

    if (m_isStopping) {
        ACSDK_DEBUG5(LX(__func__).m("Stopped while context fetch in progress"));
        return false;
    }

    return true;
}

bool PostConnectSynchronizeStateSender::performOperation(const std::shared_ptr<MessageSenderInterface>& messageSender) {
    ACSDK_INFO(LX(__func__));
    if (!messageSender) {
        ACSDK_ERROR(LX("performOperationFailed").d("reason", "nullPostConnectSender"));
        return false;
    }

    int retryAttempt = 0;
    while (!isStopping()) {
        if (fetchContext()) {
            /// Context fetch successful, proceed to send SynchronizeState event.
            std::unique_lock<std::mutex> lock{m_mutex};
            if (m_isStopping) {
                return false;
            }

            auto event =
                buildJsonEventString(SYNCHRONIZE_STATE_NAMESPACE, SYNCHRONIZE_STATE_NAME, "", "{}", m_contextString);
            m_postConnectRequest = std::make_shared<WaitableMessageRequest>(event.second);
            lock.unlock();

            messageSender->sendMessage(m_postConnectRequest);

            auto status = m_postConnectRequest->waitForCompletion();
            ACSDK_DEBUG5(LX(__func__).d("SynchronizeState event status", status));

#ifdef ACSDK_ENABLE_METRICS_RECORDING
            if (m_metricRecorder) {
                std::stringstream eventNameBuilder;
                eventNameBuilder << POST_CONNECT_STATUS_PREFIX << status;
                auto metricEvent =
                    MetricEventBuilder{}
                        .setActivityName(RETRY_SYNCHRONIZE_ACTIVITY_NAME)
                        .addDataPoint(DataPointCounterBuilder{}.setName(eventNameBuilder.str()).increment(1).build())
                        .build();
                submitEvent(m_metricRecorder, std::move(metricEvent));
            }
#endif

            if (status == MessageRequestObserverInterface::Status::SUCCESS ||
                status == MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT) {
                return true;
            } else if (status == MessageRequestObserverInterface::Status::CANCELED) {
                return false;
            }

#ifdef ACSDK_ENABLE_METRICS_RECORDING
            submitMetric(
                m_metricRecorder,
                RETRY_SYNCHRONIZE_ACTIVITY_NAME,
                "retrySynchronizeStateEvent",
                "NON_SUCCESS_RESPONSE");
#endif
        } else {
#ifdef ACSDK_ENABLE_METRICS_RECORDING
            submitMetric(
                m_metricRecorder,
                RETRY_SYNCHRONIZE_ACTIVITY_NAME,
                "retrySynchronizeStateEvent",
                "CONTEXT_FETCH_TIMEDOUT");
#endif
        }

        /// Retry with backoff.
        std::unique_lock<std::mutex> lock{m_mutex};
        auto timeout = RETRY_TIMER.calculateTimeToRetry(retryAttempt++);
        if (m_wakeTrigger.waitFor(lock, timeout, [this] { return m_isStopping; })) {
            return false;
        }
    }

    return false;
}

bool PostConnectSynchronizeStateSender::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

void PostConnectSynchronizeStateSender::abortOperation() {
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<WaitableMessageRequest> requestCopy;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isStopping) {
            /// Already stopping, return.
            return;
        }
        m_isStopping = true;
        requestCopy = m_postConnectRequest;
    }

    /// Call shutdown outside the lock.
    if (requestCopy) {
        requestCopy->shutdown();
    }

    m_wakeTrigger.notifyAll();
}

}  // namespace synchronizeStateSender
}  // namespace alexaClientSDK
