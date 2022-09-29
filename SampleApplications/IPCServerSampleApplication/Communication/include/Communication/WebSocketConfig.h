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

/*
 * Based on code supplied by the websocketpp library, original licence below
 *
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_WEBSOCKETCONFIG_H_
#define ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_WEBSOCKETCONFIG_H_

#include <websocketpp/transport/asio/endpoint.hpp>
// Always include the no_tls config, even if ssl is enabled
#include <websocketpp/config/asio_no_tls.hpp>
#ifdef ENABLE_WEBSOCKET_SSL
#include <websocketpp/transport/asio/security/tls.hpp>
#endif

#include "WebSocketSDKLogger.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace communication {
/**
 * Contains the configuration for WebSocketsPP.
 *
 * WebSocketsPP takes this configuration as a template parameter,
 * and uses these typedef's to configure the various parts of the library
 */
struct WebSocketConfig : public websocketpp::config::asio {
    /// The type of this struct
    typedef WebSocketConfig type;

    /// The base class which this config is extending
    typedef asio base;

    /// Concurrency policy used by websocketspp
    typedef base::concurrency_type concurrency_type;

    /// The type of request policy (a http request parser)
    typedef base::request_type request_type;

    /// The type of response policy (http)
    typedef base::response_type response_type;

    /// Type used to store messages
    typedef base::message_type message_type;

    /// Connection message manager policy
    typedef base::con_msg_manager_type con_msg_manager_type;

    /// Endpoint message manager policy
    typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

    /// Logger to use for access logs
    typedef WebSocketSDKLogger alog_type;

    /// Logger to use for error logs
    typedef WebSocketSDKLogger elog_type;

    /// Random number generator type
    typedef base::rng_type rng_type;

    /**
     * Specifies the transport configuration which will be used by websocketspp
     *
     * In this case contains a redefinition of the above types plus the type of transport in use
     */
    struct transport_config : public base::transport_config {
        /// Concurrency policy used by websocketspp
        typedef type::concurrency_type concurrency_type;

        /// Logger to use for access logs
        typedef type::alog_type alog_type;

        /// Logger to use for error logs
        typedef type::elog_type elog_type;

        /// The type of request policy (a http request parser)
        typedef type::request_type request_type;

        /// The type of response policy (http)
        typedef type::response_type response_type;

#ifdef ENABLE_WEBSOCKET_SSL
        typedef websocketpp::transport::asio::tls_socket::endpoint socket_type;
#else
        /// The socket type to use (basic, tls, etc)
        typedef websocketpp::transport::asio::basic_socket::endpoint socket_type;
#endif
    };

    /// Typedef for the endpoint encapsulating the transport configuration defined above
    typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;
};
}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_WEBSOCKETCONFIG_H_
