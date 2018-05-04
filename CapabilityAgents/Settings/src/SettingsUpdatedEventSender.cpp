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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "Settings/SettingsUpdatedEventSender.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

using namespace avsCommon::avs;
using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace rapidjson;
/// String to identify log entries originating from this file.
static const std::string TAG{"SettingsUpdatedEventSender"};

/// The namespace for this event
static const std::string NAMESPACE = "Settings";

/// JSON value for a SettingsUpdated event's name.
static const std::string SETTINGS_UPDATED_EVENT_NAME = "SettingsUpdated";

/// JSON value for the settings field of the SettingsUpdated event.
static const char SETTINGS_STRING[] = "settings";

/// JSON value for the key field of the settings.
static const char SETTING_KEY[] = "key";

/// JSON value for the value field of the settings.
static const char SETTING_VALUE[] = "value";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<SettingsUpdatedEventSender> SettingsUpdatedEventSender::create(
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender) {
    if (!certifiedMessageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "messageSenderNullReference"));
        return nullptr;
    }

    return std::unique_ptr<SettingsUpdatedEventSender>(new SettingsUpdatedEventSender(certifiedMessageSender));
}

void SettingsUpdatedEventSender::onSettingChanged(const std::unordered_map<std::string, std::string>& mapOfSettings) {
    // Constructing Json for Payload
    rapidjson::Document payloadDataDocument(rapidjson::kObjectType);
    rapidjson::Value settingsArray(rapidjson::kArrayType);

    for (auto& it : mapOfSettings) {
        rapidjson::Value settingsObj(rapidjson::kObjectType);
        settingsObj.AddMember(SETTING_KEY, it.first, payloadDataDocument.GetAllocator());
        settingsObj.AddMember(SETTING_VALUE, it.second, payloadDataDocument.GetAllocator());
        settingsArray.PushBack(settingsObj, payloadDataDocument.GetAllocator());
    }

    payloadDataDocument.AddMember(SETTINGS_STRING, settingsArray, payloadDataDocument.GetAllocator());
    rapidjson::StringBuffer payloadJson;
    rapidjson::Writer<rapidjson::StringBuffer> payloadWriter(payloadJson);
    payloadDataDocument.Accept(payloadWriter);
    std::string payload = payloadJson.GetString();

    // Payload should not be empty.
    if (payload.empty()) {
        ACSDK_ERROR(LX("onSettingChangedFailed").d("reason", "payloadEmpty"));
        return;
    }

    auto msgIdAndJsonEvent = buildJsonEventString(NAMESPACE, SETTINGS_UPDATED_EVENT_NAME, "", payload, "");

    // msgId should not be empty
    if (msgIdAndJsonEvent.first.empty()) {
        ACSDK_ERROR(LX("onSettingChangedFailed").d("reason", "msgIdEmpty"));
        return;
    }

    // Json event should not be empty
    if (msgIdAndJsonEvent.second.empty()) {
        ACSDK_ERROR(LX("onSettingChangedFailed").d("reason", "JsonEventEmpty"));
        return;
    }

    m_certifiedSender->sendJSONMessage(msgIdAndJsonEvent.second);
}

SettingsUpdatedEventSender::SettingsUpdatedEventSender(
    std::shared_ptr<certifiedSender::CertifiedSender> certifiedMessageSender) :
        m_certifiedSender{certifiedMessageSender} {
}

}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
