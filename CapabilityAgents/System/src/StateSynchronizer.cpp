/*
 * StateSynchronizer.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "System/StateSynchronizer.h"

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
static const std::string TAG("StateSynchronizer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// String to identify the AVS namespace of the event we send.
static const std::string STATE_SYNCHRONIZER_NAMESPACE = "System";

/// String to identify the AVS name of the event we send.
static const std::string STATE_SYNCHRONIZER_NAME = "SynchronizeState";

std::shared_ptr<StateSynchronizer> StateSynchronizer::create(
        std::shared_ptr<ContextManagerInterface> contextManager,
        std::shared_ptr<MessageSenderInterface> messageSender) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }
    return std::shared_ptr<StateSynchronizer>(new StateSynchronizer(contextManager, messageSender));
}

void StateSynchronizer::onConnectionStatusChanged(
        const ConnectionStatusObserverInterface::Status status,
        const ConnectionStatusObserverInterface::ChangedReason reason) {
    if (ConnectionStatusObserverInterface::Status::CONNECTED == status) {
        m_contextManager->getContext(shared_from_this());
    }
}

void StateSynchronizer::onContextAvailable(const std::string& jsonContext) {
    auto msgIdAndJsonEvent = buildJsonEventString(
            STATE_SYNCHRONIZER_NAMESPACE,
            STATE_SYNCHRONIZER_NAME,
            "",
            "{}",
            jsonContext);
    m_messageSender->sendMessage(std::make_shared<MessageRequest>(msgIdAndJsonEvent.second));
}

void StateSynchronizer::onContextFailure(const ContextRequestError error) {
    ACSDK_ERROR(LX("contextRetrievalFailed").d("reason", "contextRequestErrorOccurred").d("error", error));
}

StateSynchronizer::StateSynchronizer(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender) :
    m_messageSender{messageSender},
    m_contextManager{contextManager}
{
}

} // namespace system
} // namespace capabilityAgents
} // namespace alexaClientSDK
