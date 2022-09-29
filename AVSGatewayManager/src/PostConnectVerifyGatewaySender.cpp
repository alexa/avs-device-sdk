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

#include <vector>

#include "AVSGatewayManager/PostConnectVerifyGatewaySender.h"

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/RetryTimer.h>

namespace alexaClientSDK {
namespace avsGatewayManager {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
#define TAG "PostConnectVerifyGatewaySender"

/// Activity name for post-connect metric.
/// This name is dependent on TAG value.
#define POST_CONNECT_ACTIVITY_NAME TAG "-sendVerifyGateway"

/// Prefix for post-connect data point with status value.
#define POST_CONNECT_STATUS_PREFIX "STATUS-"

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The namespace for the verify gateway API.
static const std::string VERIFY_GATEWAY_NAMESPACE = "Alexa.ApiGateway";

/// The name of the event to verify gateway.
static const std::string VERIFY_GATEWAY_NAME = "VerifyGateway";

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

std::shared_ptr<PostConnectVerifyGatewaySender> PostConnectVerifyGatewaySender::create(
    std::function<void(const std::shared_ptr<PostConnectVerifyGatewaySender>& verifyGatewaySender)>
        gatewayVerifiedCallback,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) {
    if (!gatewayVerifiedCallback) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalid gatewayVerifiedCallback"));
    } else {
        return std::shared_ptr<PostConnectVerifyGatewaySender>(
            new PostConnectVerifyGatewaySender(gatewayVerifiedCallback, metricRecorder));
    }

    return nullptr;
}

PostConnectVerifyGatewaySender::PostConnectVerifyGatewaySender(
    std::function<void(const std::shared_ptr<PostConnectVerifyGatewaySender>& verifyGatewaySender)>
        gatewayVerifiedCallback,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) :
        m_gatewayVerifiedCallback{std::move(gatewayVerifiedCallback)},
        m_metricRecorder{std::move(metricRecorder)},
        m_isStopping{false} {
    ACSDK_INFO(LX("init").p("this", this));
}

PostConnectVerifyGatewaySender::~PostConnectVerifyGatewaySender() {
    ACSDK_INFO(LX("destroyed").p("this", this));
}

unsigned int PostConnectVerifyGatewaySender::getOperationPriority() {
    return VERIFY_GATEWAY_PRIORITY;
}

bool PostConnectVerifyGatewaySender::performOperation(const std::shared_ptr<MessageSenderInterface>& messageSender) {
    ACSDK_INFO(LX(__func__));
    if (!messageSender) {
        ACSDK_ERROR(LX("performOperationFailed").d("reason", "nullPostConnectSender"));
        return false;
    }
    int retryAttempt = 0;
    while (!isStopping()) {
        auto status = sendVerifyGateway(messageSender);
        switch (status) {
            /// Notify AVSGatewayManager and proceed to next post connect operation.
            case VerifyGatewayReturnCode::GATEWAY_VERIFIED:
                m_gatewayVerifiedCallback(shared_from_this());
                return true;
            case VerifyGatewayReturnCode::CHANGING_GATEWAY:
                return true;
            /// Stop the post connect sequence.
            case VerifyGatewayReturnCode::FATAL_ERROR:
                return false;
            /// initiate retry attempt.
            case VerifyGatewayReturnCode::RETRIABLE_ERROR:
                break;
        }

        if (m_wakeEvent.wait(RETRY_TIMER.calculateTimeToRetry(retryAttempt++))) {
            ACSDK_DEBUG5(LX(__func__).m("Wait aborted"));
            m_wakeEvent.reset();
        }
    }

    return false;
}

bool PostConnectVerifyGatewaySender::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

void PostConnectVerifyGatewaySender::wakeOperation() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_wakeEvent.wakeUp();
}

void PostConnectVerifyGatewaySender::abortOperation() {
    ACSDK_INFO(LX(__func__));
    std::shared_ptr<WaitableMessageRequest> requestCopy;
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

    m_wakeEvent.wakeUp();
}

PostConnectVerifyGatewaySender::VerifyGatewayReturnCode PostConnectVerifyGatewaySender::sendVerifyGateway(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) {
    ACSDK_DEBUG5(LX(__func__));
    std::unique_lock<std::mutex> lock{m_mutex};
    auto event = buildJsonEventString(VERIFY_GATEWAY_NAMESPACE, VERIFY_GATEWAY_NAME);
    m_postConnectRequest.reset();
    m_postConnectRequest = std::make_shared<WaitableMessageRequest>(event.second);
    lock.unlock();

    messageSender->sendMessage(m_postConnectRequest);

    /// Wait for the response.
    auto status = m_postConnectRequest->waitForCompletion();

#ifdef ACSDK_ENABLE_METRICS_RECORDING
    if (m_metricRecorder) {
        std::stringstream eventNameBuilder;
        eventNameBuilder << POST_CONNECT_STATUS_PREFIX << status;
        auto metricEvent =
            MetricEventBuilder{}
                .setActivityName(POST_CONNECT_ACTIVITY_NAME)
                .addDataPoint(DataPointCounterBuilder{}.setName(eventNameBuilder.str()).increment(1).build())
                .build();
        if (metricEvent) {
            m_metricRecorder->recordMetric(std::move(metricEvent));
        }
    }
#endif

    switch (status) {
        /// 200 Response with a set gateway directive.
        case MessageRequestObserverInterface::Status::SUCCESS:
            return VerifyGatewayReturnCode::CHANGING_GATEWAY;
        /// 204 Response indicating gateway has been verified.
        case MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT:
            return VerifyGatewayReturnCode::GATEWAY_VERIFIED;
        /// 4xx Response indicating failure.
        case MessageRequestObserverInterface::Status::CANCELED:
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
        case MessageRequestObserverInterface::Status::PROTOCOL_ERROR:
        case MessageRequestObserverInterface::Status::REFUSED:
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
            return VerifyGatewayReturnCode::FATAL_ERROR;
        default:
            return VerifyGatewayReturnCode::RETRIABLE_ERROR;
    }

    return VerifyGatewayReturnCode::FATAL_ERROR;
}

}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
