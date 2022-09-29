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

#ifndef ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_WEBSOCKETSDKLOGGER_H_
#define ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_WEBSOCKETSDKLOGGER_H_

#include <websocketpp/common/cpp11.hpp>
#include <websocketpp/logger/levels.hpp>
#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace communication {
/// Wrapper around the Alexa Client SDK logger for use by websocketspp
class WebSocketSDKLogger {
public:
    using level = websocketpp::log::level;

    /// Construct the logger
    /**
     * @param hint A channel type specific hint for how to construct the logger
     */
    WebSocketSDKLogger(websocketpp::log::channel_type_hint::value hint = websocketpp::log::channel_type_hint::access) :
            m_channelTypeHint{hint} {
    }

    /// Construct the logger
    /**
     * @param channels
     * @param hint The type of messages this logger will process, access or error
     */
    WebSocketSDKLogger(level channels, websocketpp::log::channel_type_hint::value hint) : m_channelTypeHint{hint} {
    }

    /// Dynamically enable the given list of channels
    /**
     *  This logger depends on the SDK logger for log level selection, channel selection is ignored
     *
     * @param channels
     */
    void set_channels(level channels) {
    }

    /// Dynamically disable the given list of channels
    /**
     *  This logger depends on the SDK logger for log level selection, channel selection is ignored
     *
     * @param channels
     */
    void clear_channels(level channels) {
    }

    /// Tests whether a log level is statically enabled.
    /**
     *  This logger depends on the SDK logger for log level selection
     *  so always return true and ignore arguments
     *
     *  @param channel
     */
    bool static_test(level channel) const {
        return true;
    }

    /// Tests whether a log level is dynamically enabled.
    /**
     *  This logger depends on the SDK logger for log level selection
     *  so always return true and ignore arguments
     *
     *  @param channel
     */
    bool dynamic_test(level channel) const {
        return true;
    }

    /// Write a string message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, std::string const& msg);

    /// Write a cstring message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, char const* msg);

private:
    /// retrieve the syslog priority code given a WebSocket++ error channel
    /**
     * @param channel The level to look up
     * @return The syslog level associated with `channel`
     */
    void logErrorMessage(level channel, char const* msg) const;

    /// retrieve the syslog priority code given a WebSocket++ access channel
    /**
     * @param channel The level to look up
     * @return The syslog level associated with `channel`
     */
    void logAccessMessage(level, char const* msg) const;

    websocketpp::log::channel_type_hint::value m_channelTypeHint;
};
}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_WEBSOCKETSDKLOGGER_H_
