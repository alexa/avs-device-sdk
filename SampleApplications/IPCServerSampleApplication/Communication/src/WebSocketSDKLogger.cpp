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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "Communication/WebSocketSDKLogger.h"

#define TAG "WebSocket"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace communication {

void WebSocketSDKLogger::write(websocketpp::log::level channel, std::string const& msg) {
    write(channel, msg.c_str());
}

void WebSocketSDKLogger::write(websocketpp::log::level channel, char const* msg) {
    if (m_channelTypeHint == websocketpp::log::channel_type_hint::access) {
        logAccessMessage(channel, msg);
    } else {
        logErrorMessage(channel, msg);
    }
}

void WebSocketSDKLogger::logErrorMessage(websocketpp::log::level channel, char const* msg) const {
    switch (channel) {
        case websocketpp::log::elevel::devel:
        case websocketpp::log::elevel::library:
            ACSDK_DEBUG5(LX("ErrorLog").sensitive("message", msg));
            return;
        case websocketpp::log::elevel::info:
            ACSDK_INFO(LX("ErrorLog").sensitive("message", msg));
            return;
        case websocketpp::log::elevel::warn:
            ACSDK_WARN(LX("ErrorLog").sensitive("message", msg));
            return;
        case websocketpp::log::elevel::rerror:
            ACSDK_ERROR(LX("ErrorLog").sensitive("message", msg));
            return;
        case websocketpp::log::elevel::fatal:
            ACSDK_CRITICAL(LX("ErrorLog").sensitive("message", msg));
            return;
        default:
            ACSDK_INFO(LX("ErrorLog").sensitive("message", msg));
            return;
    }
}

void WebSocketSDKLogger::logAccessMessage(websocketpp::log::level, char const* msg) const {
    ACSDK_DEBUG9(LX("AccessLog").sensitive("message", msg));
}
}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK