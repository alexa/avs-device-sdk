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

#include "System/LocaleHandler.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <Settings/DeviceSettingsManager.h>
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
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::logger;
using namespace settings;

/// String to identify log entries originating from this file.
static const std::string TAG("LocaleHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the namespace for AVS enpointing.
static const std::string LOCALE_NAMESPACE = "System";

/// This string holds the name of the directive that's being sent for setting the locale.
static const std::string SET_LOCALE_DIRECTIVE = "SetLocales";

/// This string holds the name of the event to report the locale.
static const std::string LOCALE_REPORT_EVENT = "LocalesReport";

/// This string holds the name of the event to change the locale.
static const std::string LOCALE_CHANGED_EVENT = "LocalesChanged";

/// This string holds the key for the locale in the payload.
static const std::string LOCALE_PAYLOAD_KEY = "locales";

std::unique_ptr<LocaleHandler> LocaleHandler::create(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<LocalesSetting> localeSetting) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    if (!localeSetting) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLocaleSetting"));
        return nullptr;
    }

    return std::unique_ptr<LocaleHandler>(new LocaleHandler(exceptionSender, localeSetting));
}

LocaleHandler::LocaleHandler(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<LocalesSetting> localeSetting) :
        CapabilityAgent(LOCALE_NAMESPACE, exceptionSender),
        m_localeSetting{localeSetting} {
}

settings::SettingEventMetadata LocaleHandler::getLocaleEventsMetadata() {
    return settings::SettingEventMetadata{
        LOCALE_NAMESPACE, LOCALE_CHANGED_EVENT, LOCALE_REPORT_EVENT, LOCALE_PAYLOAD_KEY};
}

DirectiveHandlerConfiguration LocaleHandler::getConfiguration() const {
    return DirectiveHandlerConfiguration{{NamespaceAndName{LOCALE_NAMESPACE, SET_LOCALE_DIRECTIVE},
                                          BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}

void LocaleHandler::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirective"));
        return;
    }
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void LocaleHandler::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // intentional no-op
}

void LocaleHandler::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    if (!info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }
    m_executor.submit([this, info]() { executeHandleDirective(info); });
}

void LocaleHandler::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // intentional no-op
}

void LocaleHandler::executeHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("executeHandleDirectiveImmediately"));
    auto directiveName = info->directive->getName();

    if (SET_LOCALE_DIRECTIVE == directiveName) {
        handleSetLocale(info);
    } else {
        ACSDK_ERROR(LX("executeHandleDirectiveFailed")
                        .d("reason", "unknownDirective")
                        .d("namespace", info->directive->getNamespace())
                        .d("name", info->directive->getName()));

        const std::string errorMessage =
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();
        sendProcessingDirectiveException(info, errorMessage);
    }
}

void LocaleHandler::handleSetLocale(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    auto locales = retrieveStringArray<DeviceLocales>(info->directive->getPayload(), LOCALE_PAYLOAD_KEY);
    if (locales.empty()) {
        std::string errorMessage = "locale not specified for SetLocale";
        ACSDK_ERROR(LX("handleSetLocaleFailed").d("reason", "localeMissing"));
        sendProcessingDirectiveException(info, errorMessage);
        return;
    }

    if (!m_localeSetting->setAvsChange(locales)) {
        ACSDK_ERROR(LX("handleSetLocaleFailed").d("reason", "setRequestFailed"));
        sendProcessingDirectiveException(info, "cannot apply locale change");
        return;
    }

    if (info->result) {
        info->result->setCompleted();
    }

    if (info->directive) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void LocaleHandler::sendProcessingDirectiveException(
    std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
    const std::string& errorMessage) {
    auto unparsedDirective = info->directive->getUnparsedDirective();
    m_exceptionEncounteredSender->sendExceptionEncountered(
        unparsedDirective, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, errorMessage);

    if (info->result) {
        info->result->setFailed(errorMessage);
    }

    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
