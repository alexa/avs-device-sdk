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

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "acsdkExternalMediaPlayer/AuthorizedSender.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json::jsonUtils;

/// String to identify log entries originating from this file.
static const std::string TAG("AuthorizedSender");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The event key.
static const std::string EVENT_KEY = "event";

/// The header key.
static const std::string HEADER_KEY = "header";

/// The payload key.
static const std::string PAYLOAD_KEY = "payload";

/// The playerId key.
static const std::string PLAYER_ID_KEY = "playerId";

std::shared_ptr<AuthorizedSender> AuthorizedSender::create(std::shared_ptr<MessageSenderInterface> messageSender) {
    ACSDK_DEBUG5(LX(__func__));

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    return std::shared_ptr<AuthorizedSender>(new AuthorizedSender(messageSender));
}

AuthorizedSender::AuthorizedSender(std::shared_ptr<MessageSenderInterface> messageSender) :
        m_messageSender{messageSender} {
}

AuthorizedSender::~AuthorizedSender() {
}

void AuthorizedSender::sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    ACSDK_DEBUG5(LX(__func__));

    rapidjson::Document document;
    rapidjson::ParseResult result = document.Parse(request->getJsonContent());
    if (!result) {
        ACSDK_ERROR(LX("parseMessageFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
        return;
    }

    rapidjson::Value::ConstMemberIterator event;
    if (!findNode(document, EVENT_KEY, &event)) {
        request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
        return;
    }

    rapidjson::Value::ConstMemberIterator header;

    if (!findNode(event->value, HEADER_KEY, &header)) {
        request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
        return;
    }

    rapidjson::Value::ConstMemberIterator payload;
    if (!findNode(event->value, PAYLOAD_KEY, &payload)) {
        request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
        return;
    }

    std::string playerId;
    if (!retrieveValue(payload->value, PLAYER_ID_KEY, &playerId)) {
        request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
        return;
    }

    // Serialize calls to sendMessage to ensure authorizedPlayer updates
    // mid-message sending are handled properly.
    {
        std::lock_guard<std::mutex> lock(m_updatePlayersMutex);

        if (m_authorizedPlayerIds.count(playerId) == 0) {
            ACSDK_ERROR(LX("sendMessageFailed").d("reason", "unauthorizedPlayer").d("playerId", playerId));
            request->sendCompleted(MessageRequestObserverInterface::Status::BAD_REQUEST);
            return;
        }
        m_messageSender->sendMessage(request);
    }
}

void AuthorizedSender::updateAuthorizedPlayers(const std::unordered_set<std::string>& playerIds) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_updatePlayersMutex);
    m_authorizedPlayerIds = playerIds;
}

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
