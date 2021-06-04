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

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "acsdkDeviceSetup/DeviceSetupMessageRequest.h"

#include "acsdkDeviceSetup/DeviceSetup.h"

namespace alexaClientSDK {
namespace acsdkDeviceSetup {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"DeviceSetup"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// DeviceSetup capability constants
static const std::string DEVICESETUP_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
static const std::string DEVICESETUP_INTERFACE_NAME = "DeviceSetup";
static const std::string DEVICESETUP_CAPABILITY_INTERFACE_VERSION = "1.0";

/// The assistedSetup key.
static const std::string ASSISTED_SETUP_KEY = "assistedSetup";

/// Name of the SetupCompleted event.
static const std::string SETUP_COMPLETED_EVENT = "SetupCompleted";

std::shared_ptr<DeviceSetup> DeviceSetup::create(std::shared_ptr<MessageSenderInterface> messageSender) {
    ACSDK_DEBUG5(LX(__func__));

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    return std::shared_ptr<DeviceSetup>(new DeviceSetup(messageSender));
}

std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface> DeviceSetup::createDeviceSetupInterface(
    const std::shared_ptr<MessageSenderInterface>& messageSender) {
    ACSDK_DEBUG5(LX(__func__));

    return create(messageSender);
}

DeviceSetup::DeviceSetup(std::shared_ptr<MessageSenderInterface> messageSender) : m_messageSender{messageSender} {
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> DeviceSetup::getCapabilityConfigurations() {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, DEVICESETUP_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, DEVICESETUP_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, DEVICESETUP_CAPABILITY_INTERFACE_VERSION});

    return {std::make_shared<CapabilityConfiguration>(configMap)};
}

std::future<bool> DeviceSetup::sendDeviceSetupComplete(acsdkDeviceSetupInterfaces::AssistedSetup assistedSetup) {
    ACSDK_DEBUG5(LX(__func__).d(ASSISTED_SETUP_KEY.c_str(), assistedSetup));

    avsCommon::utils::json::JsonGenerator json;
    json.addMember(ASSISTED_SETUP_KEY, assistedSetupToString(assistedSetup));
    std::string payload = json.toString();

    std::promise<bool> sendMessagePromise;
    std::future<bool> sendMessageFuture = sendMessagePromise.get_future();

    if (payload.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyPayload"));
        sendMessagePromise.set_value(false);
    } else {
        auto msgIdAndJsonEvent =
            buildJsonEventString(DEVICESETUP_INTERFACE_NAME, SETUP_COMPLETED_EVENT, "", payload, "");
        auto request =
            std::make_shared<DeviceSetupMessageRequest>(msgIdAndJsonEvent.second, std::move(sendMessagePromise));

        m_messageSender->sendMessage(request);
    }

    return sendMessageFuture;
}

}  // namespace acsdkDeviceSetup
}  // namespace alexaClientSDK
