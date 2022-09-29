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

#include "System/TimeZoneHandler.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <Settings/Setting.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingEventSender.h>
#include <Settings/SharedAVSSettingProtocol.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace settings;

/// String to identify log entries originating from this file.
#define TAG "TimeZoneHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the namespace for AVS enpointing.
static const std::string TIMEZONE_NAMESPACE = "System";

/// This string holds the name of the directive that's being sent for setting the timezone.
static const std::string SET_TIMEZONE_DIRECTIVE = "SetTimeZone";

/// This string holds the name of the event to report the timezone.
static const std::string TIMEZONE_REPORT_EVENT = "TimeZoneReport";

/// This string holds the name of the event to change the timezone.
static const std::string TIMEZONE_CHANGED_EVENT = "TimeZoneChanged";

/// This string holds the key for the timezone in the payload.
static const std::string TIMEZONE_PAYLOAD_KEY = "timeZone";

std::unique_ptr<TimeZoneHandler> TimeZoneHandler::create(
    std::shared_ptr<settings::TimeZoneSetting> timeZoneSetting,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
    if (!timeZoneSetting) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullTimeZoneSetting"));
        return nullptr;
    }

    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncountered"));
        return nullptr;
    }

    auto timezoneHandler =
        std::unique_ptr<TimeZoneHandler>(new TimeZoneHandler(timeZoneSetting, exceptionEncounteredSender));
    return timezoneHandler;
}
DirectiveHandlerConfiguration TimeZoneHandler::getConfiguration() const {
    return DirectiveHandlerConfiguration{{NamespaceAndName{TIMEZONE_NAMESPACE, SET_TIMEZONE_DIRECTIVE},
                                          BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}
void TimeZoneHandler::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "directive is nullptr."));
        return;
    }
    auto info = createDirectiveInfo(directive, nullptr);
    m_executor.execute([this, info]() { executeHandleDirectiveImmediately(info); });
}
void TimeZoneHandler::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // intentional no-op
}
void TimeZoneHandler::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "info is nullptr."));
        return;
    }
    m_executor.execute([this, info]() { executeHandleDirectiveImmediately(info); });
}
void TimeZoneHandler::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // intentional no-op
}

void TimeZoneHandler::executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("executeHandleDirectiveImmediately"));
    auto& directive = info->directive;

    rapidjson::Document payload;
    payload.Parse(directive->getPayload());

    if (payload.HasParseError()) {
        std::string errorMessage = "Unable to parse payload";
        ACSDK_ERROR(LX("executeHandleDirectiveImmediatelyFailed").m(errorMessage));
        sendProcessingDirectiveException(directive, errorMessage);
        return;
    }

    auto directiveName = directive->getName();

    if (SET_TIMEZONE_DIRECTIVE == directiveName) {
        handleSetTimeZone(directive, payload);
    }
}

bool TimeZoneHandler::handleSetTimeZone(
    const std::shared_ptr<AVSDirective>& directive,
    const rapidjson::Document& payload) {
    std::string timeZoneValue;
    if (!retrieveValue(payload, TIMEZONE_PAYLOAD_KEY, &timeZoneValue)) {
        std::string errorMessage = "timeZone not specified for SetTimeZone";
        ACSDK_ERROR(LX("handleSetTimeZoneFailed").m(errorMessage));
        sendProcessingDirectiveException(directive, errorMessage);
        return false;
    }

    return m_timeZoneSetting->setAvsChange(timeZoneValue);
}

void TimeZoneHandler::sendProcessingDirectiveException(
    const std::shared_ptr<AVSDirective>& directive,
    const std::string& errorMessage) {
    auto unparsedDirective = directive->getUnparsedDirective();

    ACSDK_ERROR(LX("sendProcessingDirectiveException")
                    .d("errorMessage", errorMessage)
                    .d("unparsedDirective", unparsedDirective));

    m_exceptionEncounteredSender->sendExceptionEncountered(
        unparsedDirective, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, errorMessage);
}

settings::SettingEventMetadata TimeZoneHandler::getTimeZoneMetadata() {
    return settings::SettingEventMetadata{
        TIMEZONE_NAMESPACE, TIMEZONE_CHANGED_EVENT, TIMEZONE_REPORT_EVENT, TIMEZONE_PAYLOAD_KEY};
}

TimeZoneHandler::TimeZoneHandler(
    std::shared_ptr<settings::TimeZoneSetting> timeZoneSetting,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent(TIMEZONE_NAMESPACE, exceptionEncounteredSender),
        m_timeZoneSetting{timeZoneSetting} {
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
