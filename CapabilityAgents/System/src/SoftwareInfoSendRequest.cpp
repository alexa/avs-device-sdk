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

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/RetryTimer.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>

#include "System/SoftwareInfoSender.h"
#include "System/SoftwareInfoSendRequest.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::softwareInfo;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"SoftwareInfoSendRequest"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the "System" namespace.
static const std::string NAMESPACE_SYSTEM = "System";

/// This string holds the name of the "SoftwareInfo" event.
static const std::string NAME_SOFTWARE_INFO = "SoftwareInfo";

/// JSON value for the firmwareVersion field of the SoftwareInfo event.
static const char FIRMWARE_VERSION_STRING[] = "firmwareVersion";

/// Approximate amount of time to wait between retries.
static std::vector<int> RETRY_TABLE = {
    1000,     // Retry 1:   1.00s
    5000,     // Retry 2:   5.00s
    25000,    // Retry 3:  25.00s
    1250000,  // Retry 4: 125.00s
};

/// Object for calculating retry timeout values.
static avsCommon::utils::RetryTimer RETRY_TIMER(RETRY_TABLE);

std::shared_ptr<SoftwareInfoSendRequest> SoftwareInfoSendRequest::create(
    FirmwareVersion firmwareVersion,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<SoftwareInfoSenderObserverInterface> observer) {
    ACSDK_DEBUG5(LX("create").d("firmwareVersion", firmwareVersion));

    if (!isValidFirmwareVersion(firmwareVersion)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidFirmwareVersion").d("firmwareVersion", firmwareVersion));
        return nullptr;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "messageSenderNull"));
        return nullptr;
    }

    return std::shared_ptr<SoftwareInfoSendRequest>(
        new SoftwareInfoSendRequest(firmwareVersion, messageSender, observer));
}

void SoftwareInfoSendRequest::onSendCompleted(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG5(LX("onSendCompleted").d("status", status));

    std::lock_guard<std::mutex> lock(m_mutex);

    if (MessageRequestObserverInterface::Status::SUCCESS == status ||
        MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT == status) {
        if (m_observer) {
            m_observer->onFirmwareVersionAccepted(m_firmwareVersion);
            m_observer.reset();
        }
    } else {
        // At each retry, switch the Timer used to specify the time of the next retry.
        auto& timer = m_retryTimers[m_retryCounter % (sizeof(m_retryTimers) / sizeof(m_retryTimers[0]))];
        auto delay = RETRY_TIMER.calculateTimeToRetry(m_retryCounter++);
        ACSDK_INFO(
            LX("retrySendingSoftwareInfoQueued").d("retry", m_retryCounter).d("delayInMilliseconds", delay.count()));
        timer.stop();
        timer.start(
            delay,
            [](std::shared_ptr<SoftwareInfoSendRequest> softwareInfoSendRequest) { softwareInfoSendRequest->send(); },
            shared_from_this());
    }
}

void SoftwareInfoSendRequest::onExceptionReceived(const std::string& message) {
    ACSDK_DEBUG5(LX("onExceptionReceived").d("message", message));
}

void SoftwareInfoSendRequest::doShutdown() {
    ACSDK_DEBUG5(LX("doShutdown"));

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& timer : m_retryTimers) {
        timer.stop();
    }

    m_messageSender.reset();
    m_observer.reset();
}

SoftwareInfoSendRequest::SoftwareInfoSendRequest(
    FirmwareVersion firmwareVersion,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<SoftwareInfoSenderObserverInterface> observer) :
        RequiresShutdown("SoftwareInfoSendRequest"),
        m_firmwareVersion{firmwareVersion},
        m_messageSender{messageSender},
        m_observer{observer},
        m_retryCounter{0} {
}

void SoftwareInfoSendRequest::send() {
    ACSDK_DEBUG5(LX("send").d("firmwareVersion", m_firmwareVersion));

    std::string jsonContent;
    if (!buildJsonForSoftwareInfo(&jsonContent, m_firmwareVersion)) {
        ACSDK_ERROR(LX("sendFailed").d("reason", "buildJsonForSoftwareInfoFailed"));
        onSendCompleted(MessageRequestObserverInterface::Status::INTERNAL_ERROR);
        return;
    }
    auto request = std::make_shared<MessageRequest>(jsonContent);
    request->addObserver(shared_from_this());
    m_messageSender->sendMessage(request);
}

bool SoftwareInfoSendRequest::buildJsonForSoftwareInfo(std::string* jsonContent, FirmwareVersion firmwareVersion) {
    std::string versionString = std::to_string(firmwareVersion);
    rapidjson::Document payloadDataDocument(rapidjson::kObjectType);
    payloadDataDocument.AddMember(
        FIRMWARE_VERSION_STRING, rapidjson::StringRef(versionString), payloadDataDocument.GetAllocator());

    rapidjson::StringBuffer payloadJson;
    rapidjson::Writer<rapidjson::StringBuffer> payloadWriter(payloadJson);
    payloadDataDocument.Accept(payloadWriter);
    std::string payload = payloadJson.GetString();

    auto msgIdAndJsonEvent = buildJsonEventString(NAMESPACE_SYSTEM, NAME_SOFTWARE_INFO, "", payload, "");
    // msgId should not be empty
    if (msgIdAndJsonEvent.first.empty()) {
        ACSDK_ERROR(LX("buildJsonForSoftwareInfoFailed").d("reason", "msgIdEmpty"));
        return false;
    }

    // Json event should not be empty
    if (msgIdAndJsonEvent.second.empty()) {
        ACSDK_ERROR(LX("buildJsonForSoftwareInfoFailed").d("reason", "JsonEventEmpty"));
        return false;
    }

    *jsonContent = msgIdAndJsonEvent.second;
    return true;
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
