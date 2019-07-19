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

#include "System/UserInactivityMonitor.h"

#include <functional>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::utils::timing;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("UserInactivityMonitor");

/// Number of seconds in one hour.
static const int SECONDS_IN_HOUR = 3600;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String to identify the AVS namespace of the event we send.
static const std::string USER_INACTIVITY_MONITOR_NAMESPACE = "System";

/// String to identify the AVS name of the event we send.
static const std::string INACTIVITY_EVENT_NAME = "UserInactivityReport";

/// String to identify the key of the payload associated to the inactivity.
static const std::string INACTIVITY_EVENT_PAYLOAD_KEY = "inactiveTimeInSeconds";

/// String to identify the AVS name of the directive we receive.
static const std::string RESET_DIRECTIVE_NAME = "ResetUserInactivity";

void UserInactivityMonitor::removeDirectiveGracefully(
    std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
    bool isFailure,
    const std::string& report) {
    if (info) {
        if (info->result) {
            if (isFailure) {
                info->result->setFailed(report);
            } else {
                info->result->setCompleted();
            }
            if (info->directive) {
                CapabilityAgent::removeDirective(info->directive->getMessageId());
            }
        }
    }
}

std::shared_ptr<UserInactivityMonitor> UserInactivityMonitor::create(
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    const std::chrono::milliseconds& sendPeriod) {
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }
    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    }
    return std::shared_ptr<UserInactivityMonitor>(
        new UserInactivityMonitor(messageSender, exceptionEncounteredSender, sendPeriod));
}

UserInactivityMonitor::UserInactivityMonitor(
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    const std::chrono::milliseconds& sendPeriod) :
        CapabilityAgent{USER_INACTIVITY_MONITOR_NAMESPACE, exceptionEncounteredSender},
        RequiresShutdown{"UserInactivityMonitor"},
        m_messageSender{messageSender},
        m_lastTimeActive{std::chrono::steady_clock::now()},
        m_sendPeriod{sendPeriod} {
    startTimer();
}

void UserInactivityMonitor::sendInactivityReport() {
    std::chrono::time_point<std::chrono::steady_clock> lastTimeActive;
    m_recentUpdateBlocked = false;
    {
        std::lock_guard<std::mutex> timeLock(m_mutex);
        lastTimeActive = m_lastTimeActive;
    }
    if (m_recentUpdateBlocked) {
        std::lock_guard<std::mutex> timeLock(m_mutex);
        m_lastTimeActive = std::chrono::steady_clock::now();
        lastTimeActive = m_lastTimeActive;

        // Recent update was blocked means user was active, so we don't need to send an event.
        return;
    }
    Document inactivityPayload(kObjectType);
    SizeType payloadKeySize = INACTIVITY_EVENT_PAYLOAD_KEY.length();
    const Pointer::Token payloadKey[] = {{INACTIVITY_EVENT_PAYLOAD_KEY.c_str(), payloadKeySize, kPointerInvalidIndex}};

    // AVS requires inactivity time to be a multiple of 3600.
    auto inactiveSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastTimeActive);

    // Rounded hours.
    auto inactiveHours = ((inactiveSeconds.count() + SECONDS_IN_HOUR / 2) / SECONDS_IN_HOUR);

    Pointer(payloadKey, 1).Set(inactivityPayload, static_cast<int64_t>(inactiveHours * SECONDS_IN_HOUR));

    std::string inactivityPayloadString;
    jsonUtils::convertToValue(inactivityPayload, &inactivityPayloadString);

    auto inactivityEvent = buildJsonEventString(INACTIVITY_EVENT_NAME, "", inactivityPayloadString);
    m_messageSender->sendMessage(std::make_shared<MessageRequest>(inactivityEvent.second));

    notifyObservers();
}

void UserInactivityMonitor::startTimer() {
    m_eventTimer.start(
        m_sendPeriod,
        Timer::PeriodType::ABSOLUTE,
        Timer::FOREVER,
        std::bind(&UserInactivityMonitor::sendInactivityReport, this));
}

DirectiveHandlerConfiguration UserInactivityMonitor::getConfiguration() const {
    return DirectiveHandlerConfiguration{{NamespaceAndName{USER_INACTIVITY_MONITOR_NAMESPACE, RESET_DIRECTIVE_NAME},
                                          BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}

void UserInactivityMonitor::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<CapabilityAgent::DirectiveInfo>(directive, nullptr));
}

void UserInactivityMonitor::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
}

void UserInactivityMonitor::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    onUserActive();
    removeDirectiveGracefully(info);
}

void UserInactivityMonitor::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info->directive) {
        removeDirectiveGracefully(info, true, "nullDirective");
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullDirectiveInDirectiveInfo"));
        return;
    }
    removeDirective(info->directive->getMessageId());
}

void UserInactivityMonitor::onUserActive() {
    ACSDK_DEBUG5(LX(__func__));
    std::unique_lock<std::mutex> timeLock(m_mutex, std::defer_lock);

    m_eventTimer.stop();
    startTimer();

    if (timeLock.try_lock()) {
        m_lastTimeActive = std::chrono::steady_clock::now();
    } else {
        m_recentUpdateBlocked = true;
    }
}

std::chrono::seconds UserInactivityMonitor::timeSinceUserActivity() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto lastTimeActiveCopy = m_lastTimeActive;
    lock.unlock();

    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastTimeActiveCopy);
}

void UserInactivityMonitor::addObserver(std::shared_ptr<UserInactivityMonitorObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_inactivityObservers.insert(observer);
}

void UserInactivityMonitor::removeObserver(std::shared_ptr<UserInactivityMonitorObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_inactivityObservers.erase(observer);
}

void UserInactivityMonitor::notifyObservers() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto observers = m_inactivityObservers;
    lock.unlock();

    for (auto observer : observers) {
        observer->onUserInactivityReportSent();
    }
}

void UserInactivityMonitor::doShutdown() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_inactivityObservers.clear();
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
