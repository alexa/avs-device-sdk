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

#include <chrono>

#include "Settings/SettingEventSender.h"
#include "Settings/SettingEventRequestObserver.h"

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace settings {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;

/// String to identify log entries originating from this file.
static const std::string TAG("SettingEventSender");

/**
 * A timeout for an HTTP response, for cases
 * @c MessageRequest failed to call its observers upon completion.
 * Hence, the timeout here is longer than @c MessageRequestHandler activity timeout.
 */
static const std::chrono::seconds HTTP_RESPONSE_TIMEOUT(40);

/// Table with the retry times on subsequent retries.
static const std::vector<int> DEFAULT_RETRY_TABLE = {
    1000,   // Retry 1: 1s
    2000,   // Retry 2: 2s
    4000,   // Retry 3  4s
    10000,  // Retry 4: 10s
    30000,  // Retry 5: 30s
};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

const std::vector<int>& SettingEventSender::getDefaultRetryTable() {
    return DEFAULT_RETRY_TABLE;
}

std::unique_ptr<SettingEventSender> SettingEventSender::create(
    const SettingEventMetadata& metadata,
    std::shared_ptr<MessageSenderInterface> messageSender,
    const std::vector<int>& retryTable) {
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    return std::unique_ptr<SettingEventSender>(new SettingEventSender(metadata, messageSender, retryTable));
}

SettingEventSender::SettingEventSender(
    const SettingEventMetadata& metadata,
    std::shared_ptr<MessageSenderInterface> messageSender,
    const std::vector<int>& retryTable) :
        // Parenthesis used for initializing @c m_metadata to work-around the issue on aggregate initialization of the
        // same type. see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#1467.
        m_metadata(metadata),
        m_messageSender{messageSender},
        m_retryTimer{retryTable},
        m_maxRetries{retryTable.size()} {
}

SettingEventSender::~SettingEventSender() {
    cancel();
}

std::string SettingEventSender::buildEventJson(const std::string& eventName, const std::string& value) const {
    ACSDK_DEBUG5(LX(__func__).d("eventName", eventName).sensitive("value", value));
    const std::string EMPTY_DIALOG_REQUEST_ID;
    JsonGenerator payload;
    if (!payload.addRawJsonMember(m_metadata.settingName, value, true)) {
        ACSDK_ERROR(LX("createEventJson").d("reason", "failedToAddValueToPayload"));
        return "";
    }

    return buildJsonEventString(m_metadata.eventNamespace, eventName, EMPTY_DIALOG_REQUEST_ID, payload.toString())
        .second;
}

std::shared_future<bool> SettingEventSender::sendEvent(const std::string& jsonEventString) {
    std::promise<bool> retPromise;

    if (jsonEventString.empty()) {
        ACSDK_ERROR(LX("sendEventFailed").d("reason", "emptyJsonString"));
        retPromise.set_value(false);
        return retPromise.get_future();
    }

    std::lock_guard<std::mutex> sendLock{m_sendMutex};
    m_waitCancelEvent.reset();
    std::size_t attempt = 0;

    while (attempt < m_maxRetries) {
        auto eventRequest = std::make_shared<avsCommon::avs::MessageRequest>(jsonEventString);
        auto observer = std::make_shared<SettingEventRequestObserver>();
        eventRequest->addObserver(observer);

        ACSDK_DEBUG7(LX(__func__).d("attempt", attempt).sensitive("sendingEvent", jsonEventString));

        m_messageSender->sendMessage(eventRequest);

        auto httpResponseFuture = observer->getResponseFuture();
        if (httpResponseFuture.wait_for(HTTP_RESPONSE_TIMEOUT) == std::future_status::ready) {
            auto httpResponse = httpResponseFuture.get();
            std::ostringstream oss;
            oss << httpResponse;

            ACSDK_DEBUG7(LX(__func__).d("status", oss.str()).sensitive("event", jsonEventString));

            switch (httpResponse) {
                case MessageRequestObserverInterface::Status::THROTTLED:
                case MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR:
                case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
                case MessageRequestObserverInterface::Status::TIMEDOUT:
                case MessageRequestObserverInterface::Status::INTERNAL_ERROR:
                    // retry
                    break;

                case MessageRequestObserverInterface::Status::SUCCESS:
                case MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED:
                case MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT:
                    retPromise.set_value(true);
                    return retPromise.get_future();

                case MessageRequestObserverInterface::Status::CANCELED:
                case MessageRequestObserverInterface::Status::BAD_REQUEST:
                case MessageRequestObserverInterface::Status::PENDING:
                case MessageRequestObserverInterface::Status::NOT_CONNECTED:
                case MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED:
                case MessageRequestObserverInterface::Status::PROTOCOL_ERROR:
                case MessageRequestObserverInterface::Status::REFUSED:
                case MessageRequestObserverInterface::Status::INVALID_AUTH:
                    retPromise.set_value(false);
                    return retPromise.get_future();
            }
        } else {
            ACSDK_ERROR(LX("sendEventFailed").d("reason", "noHTTPResponse"));
        }

        // Exponential back-off before resending event.
        // Can be canceled anytime.
        if (m_waitCancelEvent.wait(m_retryTimer.calculateTimeToRetry(static_cast<int>(attempt)))) {
            break;
        }

        attempt++;
    }

    retPromise.set_value(false);
    return retPromise.get_future();
}

std::shared_future<bool> SettingEventSender::sendChangedEvent(const std::string& value) {
    return sendEvent(buildEventJson(m_metadata.eventChangedName, value));
}

std::shared_future<bool> SettingEventSender::sendReportEvent(const std::string& value) {
    return sendEvent(buildEventJson(m_metadata.eventReportName, value));
}

std::shared_future<bool> SettingEventSender::sendStateReportEvent(const std::string& payload) {
    auto eventJson =
        buildJsonEventString(m_metadata.eventNamespace, m_metadata.eventReportName, std::string(), payload).second;
    return sendEvent(eventJson);
}

void SettingEventSender::cancel() {
    ACSDK_DEBUG9(LX(__func__));
    m_waitCancelEvent.wakeUp();
}

}  // namespace settings
}  // namespace alexaClientSDK
