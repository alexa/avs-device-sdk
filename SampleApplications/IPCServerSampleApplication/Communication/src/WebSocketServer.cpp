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

#include <memory>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "Communication/WebSocketServer.h"

static const std::string TAG("WebSocketServer");

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

using namespace websocketpp::lib::placeholders;
using server = websocketpp::server<WebSocketConfig>;
using connection_hdl = websocketpp::connection_hdl;

WebSocketServer::WebSocketServer(const std::string& interface, const unsigned short port) {
    websocketpp::lib::error_code errorCode;
    m_webSocketServer.init_asio(errorCode);
    if (errorCode) {
        ACSDK_ERROR(
            LX("server::init_asio").d("errorCode", errorCode.value()).d("errorCategory", errorCode.category().name()));
        return;
    }

    m_webSocketServer.set_reuse_addr(true);
    m_webSocketServer.set_open_handler(bind(&WebSocketServer::onConnectionOpen, this, _1));
    m_webSocketServer.set_close_handler(bind(&WebSocketServer::onConnectionClose, this, _1));
    m_webSocketServer.set_message_handler(bind(&WebSocketServer::onMessage, this, _1, _2));

#ifdef ENABLE_WEBSOCKET_SSL
    m_webSocketServer.set_tls_init_handler(bind(&WebSocketServer::onTlsInit, this));
#endif

    m_webSocketServer.set_validate_handler(bind(&WebSocketServer::onValidate, this, _1));

    auto endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(interface), port);
    m_webSocketServer.listen(endpoint, errorCode);

    if (errorCode) {
        ACSDK_ERROR(
            LX("server::listen").d("errorCode", errorCode.value()).d("errorCategory", errorCode.category().name()));
        return;
    }

    m_initialised = true;
}

void WebSocketServer::setMessageListener(std::shared_ptr<MessageListenerInterface> messageListener) {
    m_messageListener = messageListener;
}

void WebSocketServer::setCertificateFile(
    const std::string& certificateAuthority,
    const std::string& certificate,
    const std::string& privateKey) {
    m_certificateAuthorityFile = certificateAuthority;
    m_certificateFile = certificate;
    m_privateKeyFile = privateKey;
}

bool WebSocketServer::start() {
    if (!m_initialised) {
        ACSDK_ERROR(LX("startFailed").d("reason", "server not initialised"));
        return false;
    }

#ifdef ENABLE_WEBSOCKET_SSL
    if (m_certificateAuthorityFile.empty() || m_certificateFile.empty() || m_privateKeyFile.empty()) {
        ACSDK_CRITICAL(LX("startFailed")
                           .d("reason", "SSL certificate configuration missing")
                           .d("m_certificateAuthorityFile", m_certificateAuthorityFile)
                           .d("m_certificateFile", m_certificateFile)
                           .d("m_privateKeyFile", m_privateKeyFile));
        return false;
    }
#endif

    websocketpp::lib::error_code errorCode;

    m_webSocketServer.start_accept(errorCode);
    if (errorCode) {
        ACSDK_ERROR(LX("server::start_accept")
                        .d("errorCode", errorCode.value())
                        .d("errorCategory", errorCode.category().name()));
        return false;
    }

    auto endpoint = m_webSocketServer.get_local_endpoint(errorCode);
    if (errorCode) {
        ACSDK_ERROR(LX("server::get_local_endpoint")
                        .d("errorCode", errorCode.value())
                        .d("errorCategory", errorCode.category().name()));
        return false;
    }

    ACSDK_INFO(LX("Listening for websocket connections").d("interface", endpoint.address()).d("port", endpoint.port()));

    m_webSocketServer.run();

    return true;
}

void WebSocketServer::stop() {
    websocketpp::lib::error_code errorCode;
    m_webSocketServer.stop_listening(errorCode);
    if (errorCode) {
        ACSDK_ERROR(LX("server::stop_listening")
                        .d("errorCode", errorCode.value())
                        .d("errorCategory", errorCode.category().name()));
    }

    m_webSocketServer.close(m_connection, websocketpp::close::status::going_away, "shutting down", errorCode);
    if (errorCode) {
        ACSDK_ERROR(
            LX("server::close").d("errorCode", errorCode.value()).d("errorCategory", errorCode.category().name()));
    }

    m_connection.reset();
}

void WebSocketServer::writeMessage(const std::string& payload) {
    websocketpp::lib::error_code errorCode;
    ACSDK_DEBUG9(LX("writeMessageBegin"));

    m_webSocketServer.send(m_connection, payload, websocketpp::frame::opcode::text, errorCode);
    if (errorCode) {
        ACSDK_ERROR(
            LX("server::send").d("errorCode", errorCode.value()).d("errorCategory", errorCode.category().name()));
    }

    ACSDK_DEBUG9(LX("writeMessageComplete"));
}

void WebSocketServer::onConnectionOpen(connection_hdl connectionHdl) {
    m_connection = connectionHdl;

    websocketpp::lib::error_code errorCode;
    auto client = m_webSocketServer.get_con_from_hdl(connectionHdl, errorCode);
    if (errorCode) {
        ACSDK_ERROR(
            LX("onConnectionOpen").d("errorCode", errorCode.value()).d("errorCategory", errorCode.category().name()));
        return;
    }
    ACSDK_INFO(LX("onConnectionOpen").sensitive("remoteHost", client->get_remote_endpoint()));

    m_observer->onConnectionOpened();
}

bool WebSocketServer::isReady() {
    return !m_connection.expired();
}

void WebSocketServer::setObserver(const std::shared_ptr<MessagingServerObserverInterface>& observer) {
    m_observer = observer;
}

void WebSocketServer::onConnectionClose(connection_hdl connectionHdl) {
    m_connection.reset();

    ACSDK_INFO(LX("onConnectionClose"));

    m_observer->onConnectionClosed();
}

void WebSocketServer::onMessage(connection_hdl connectionHdl, server::message_ptr messagePtr) {
    if (m_messageListener) {
        m_messageListener->onMessage(messagePtr->get_payload());
    } else {
        ACSDK_WARN(LX("onMessageFailed")
                       .d("reason", "messageListener is null")
                       .sensitive("message:", messagePtr->get_payload()));
    }
}

#ifdef ENABLE_WEBSOCKET_SSL
std::shared_ptr<asio::ssl::context> WebSocketServer::onTlsInit() {
    auto ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);
    asio::error_code errorCode;

    // Use do-while(false) to be able to break from it in case if error happened to not swallow it with the next one
    // No explicit check required as connection will preserve error happened and also fail on incomplete configuration.
    do {
        ctx->set_options(
            asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::no_sslv3 |
                asio::ssl::context::no_tlsv1 | asio::ssl::context::no_tlsv1_1 | asio::ssl::context::single_dh_use,
            errorCode);
        if (errorCode) {
            ACSDK_ERROR(LX("onTlsInit::set_options")
                            .d("errorCode", errorCode.value())
                            .d("errorCategory", errorCode.category().name()));
            break;
        }

        ctx->load_verify_file(m_certificateAuthorityFile, errorCode);
        if (errorCode) {
            ACSDK_ERROR(LX("onTlsInit::load_verify_file")
                            .d("errorCode", errorCode.value())
                            .d("errorCategory", errorCode.category().name()));
            break;
        }

        ctx->use_certificate_chain_file(m_certificateFile, errorCode);
        if (errorCode) {
            ACSDK_ERROR(LX("onTlsInit::use_certificate_chain_file")
                            .d("errorCode", errorCode.value())
                            .d("errorCategory", errorCode.category().name()));
            break;
        }

        ctx->use_private_key_file(m_privateKeyFile, asio::ssl::context::pem, errorCode);
        if (errorCode) {
            ACSDK_ERROR(LX("onTlsInit::use_private_key_file")
                            .d("errorCode", errorCode.value())
                            .d("errorCategory", errorCode.category().name()));
            break;
        }

        ctx->set_verify_mode(
            asio::ssl::context::verify_peer | asio::ssl::context::verify_fail_if_no_peer_cert, errorCode);
        if (errorCode) {
            ACSDK_ERROR(LX("onTlsInit::set_verify_mode")
                            .d("errorCode", errorCode.value())
                            .d("errorCategory", errorCode.category().name()));
            break;
        }
    } while (false);

    return ctx;
}
#endif  // ENABLE_WEBSOCKET_SSL

bool WebSocketServer::onValidate(connection_hdl connectionHdl) {
    // As we currently don't support more than one connection in general and in GUIClient in particular reject all
    // connections if we already have one.
    bool result = m_connection.expired();
    if (!result) {
        ACSDK_WARN(LX("onValidate").m("connection already open"));
        asio::error_code errorCode;
        auto conn = m_webSocketServer.get_con_from_hdl(connectionHdl, errorCode);
        if (!errorCode) {
            conn->set_status(websocketpp::http::status_code::conflict);
        }
    }
    return result;
}

}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
