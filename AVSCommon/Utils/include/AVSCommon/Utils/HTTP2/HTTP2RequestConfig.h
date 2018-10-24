/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTCONFIG_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTCONFIG_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "HTTP2RequestType.h"
#include "HTTP2RequestSourceInterface.h"
#include "HTTP2ResponseSinkInterface.h"
#include "HTTP2RequestType.h"

#ifdef ACSDK_EMIT_SENSITIVE_LOGS
#define ACSDK_EMIT_CURL_LOGS
#endif

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Class for configuring an HTTP2 request.
 */
class HTTP2RequestConfig {
public:
    /**
     * Constructor.
     *
     * @param type The type of request.
     * @param url The URL to receive the request.
     * @param idPrefix Prefix used when creating the request's ID.
     */
    HTTP2RequestConfig(HTTP2RequestType type, const std::string& url, const std::string& idPrefix);

    /**
     * Specify the maximum time, in milliseconds, for the connection phase to the server to take.
     *
     * @param timeout The max amount of time, in milliseconds, for the connection phase to the server to take.
     */
    void setConnectionTimeout(std::chrono::milliseconds timeout);

    /**
     * Specify the maximum time the request is allowed to take.
     *
     * @param timeout The max amount of time, in milliseconds, that the request is allowed to take.
     */
    void setTransferTimeout(std::chrono::milliseconds timeout);

    /**
     * Specify the maximum time, in milliseconds, to wait between any read or write operations for this request.
     *
     * @param timeout The max amount of time, in milliseconds, between any read or write operations for this request.
     */
    void setActivityTimeout(std::chrono::milliseconds timeout);

    /**
     * Specify the priority of this request.
     *
     * @param The priority of the request. Higher values (max 255) specify higher priority.
     * By default requests are assigned a priority of 16.
     * @see https://tools.ietf.org/html/rfc7540#section-5.3
     */
    void setPriority(uint8_t priority);

    /**
     * Specify the object to provide the data for this HTTP2 request.
     *
     * @param source The object to provide the data for this HTTP2 POST request.
     */
    void setRequestSource(std::shared_ptr<HTTP2RequestSourceInterface> source);

    /**
     * Specify the object to receive the response to this HTTP2 request.
     *
     * @param sink The object to receive the response to this HTTP2 request.
     */
    void setResponseSink(std::shared_ptr<HTTP2ResponseSinkInterface> sink);

    /**
     * If this request expects that transfer will happen intermittently, set this property. (It is false by default.)
     */
    void setIntermittentTransferExpected();

    /**
     * Set stream identification prefix to use for file names if extended curl logging is enabled.
     * Request ID will be appended to it.
     * @param logicalStreamIdPrefix logical stream Id prefix to use in CURL log file names.
     */
    void setLogicalStreamIdPrefix(std::string logicalStreamIdPrefix);

    /**
     * Get the type of the request (like GET or POST)
     *
     * @return  the request type
     */
    HTTP2RequestType getRequestType() const;

    /**
     * Get the URL which is to receive the request.
     *
     * @return The URL to receive the request.
     */
    std::string getUrl() const;

    /**
     * Get the maximum time, in milliseconds, for the connection phase to the server to take.
     *
     * @return The max amount of time, in milliseconds, for the connection phase to the server to take.
     */
    std::chrono::milliseconds getConnectionTimeout() const;

    /**
     * Get the maximum time the request is allowed to take.
     *
     * @return The max amount of time, in milliseconds, that the request is allowed to take.
     */
    std::chrono::milliseconds getTransferTimeout() const;

    /**
     * Get the maximum time, in milliseconds, to wait between any read or write operations for this request.
     *
     * @param timeout The max amount of time, in milliseconds, between any read or write operations for this request.
     */
    std::chrono::milliseconds getActivityTimeout() const;

    /**
     * Get the priority of this request.
     *
     * @return The priority of the request. Higher values (max 255) specify higher priority.
     * By default requests are assigned a priority of 16.
     * @see https://tools.ietf.org/html/rfc7540#section-5.3
     */
    uint8_t getPriority() const;

    /**
     * Get the object to provide the data for this HTTP2 request.
     *
     * @return The object to provide the data for this HTTP2 POST request.
     */
    std::shared_ptr<HTTP2RequestSourceInterface> getSource() const;

    /**
     * Get the object to receive the response to this HTTP2 request.
     *
     * @return The object to receive the response to this HTTP2 request.
     */
    std::shared_ptr<HTTP2ResponseSinkInterface> getSink() const;

    /**
     * Whether this request expects that transfer will happen intermittently.
     * @return Whether this request expects that transfer will happen intermittently.
     */
    bool isIntermittentTransferExpected() const;

    /**
     * Get the name used to identify the request.
     *
     * @return the name sued to identify the request.
     */
    std::string getId() const;

private:
    /// The type of request.
    const HTTP2RequestType m_type;

    /// The URL to receive the request.
    const std::string m_url;

    /// Default priority for streams. @see https://tools.ietf.org/html/rfc7540#section-5.3
    static const uint8_t DEFAULT_PRIORITY = 16;

    /// The max amount of time, in milliseconds, for the connection phase to the server to take.
    /// Value of std::chrono::milliseconds::zero() means "not set"
    std::chrono::milliseconds m_connectionTimeout;

    /// The max amount of time, in milliseconds, that the request is allowed to take.
    /// Value of std::chrono::milliseconds::zero() means "not set"
    std::chrono::milliseconds m_transferTimeout;

    /// The max amount of time, in milliseconds, between any read or write operations for this request.
    /// Value of std::chrono::milliseconds::zero() means "not set"
    std::chrono::milliseconds m_activityTimeout;

    /// The priority of the request. Higher values (max 255) specify higher priority.
    /// By default requests are assigned a priority of 16.
    /// @see https://tools.ietf.org/html/rfc7540#section-5.3
    uint8_t m_priority;

    /// The object to provide the data for this HTTP2 POST request.
    std::shared_ptr<HTTP2RequestSourceInterface> m_source;

    /// The object to receive the response to this HTTP2 request.
    std::shared_ptr<HTTP2ResponseSinkInterface> m_sink;

    /// Whether this requests expects intermittent transfers.
    /// If true, the transfer thread may be put to sleep even when this request isn't paused.
    bool m_isIntermittentTransferExpected;

    /// The ID to assign to the request.

    std::string m_id;
};

inline HTTP2RequestConfig::HTTP2RequestConfig(
    HTTP2RequestType type,
    const std::string& url,
    const std::string& idPrefix) :
        m_type(type),
        m_url(url),
        m_connectionTimeout{std::chrono::milliseconds::zero()},
        m_transferTimeout{std::chrono::milliseconds::zero()},
        m_activityTimeout{std::chrono::milliseconds::zero()},
        m_priority{DEFAULT_PRIORITY},
        m_isIntermittentTransferExpected{false} {
    static std::atomic<uint64_t> nextId{1};
    m_id = idPrefix + std::to_string(std::atomic_fetch_add(&nextId, uint64_t{2}));
};

inline void HTTP2RequestConfig::setConnectionTimeout(std::chrono::milliseconds timeout) {
    m_connectionTimeout = timeout;
}

inline void HTTP2RequestConfig::setTransferTimeout(std::chrono::milliseconds timeout) {
    m_transferTimeout = timeout;
}

inline void HTTP2RequestConfig::setActivityTimeout(std::chrono::milliseconds timeout) {
    m_activityTimeout = timeout;
}

inline void HTTP2RequestConfig::setPriority(uint8_t priority) {
    m_priority = priority;
}

inline void HTTP2RequestConfig::setRequestSource(std::shared_ptr<HTTP2RequestSourceInterface> source) {
    m_source = std::move(source);
}

inline void HTTP2RequestConfig::setResponseSink(std::shared_ptr<HTTP2ResponseSinkInterface> sink) {
    m_sink = std::move(sink);
}

inline void HTTP2RequestConfig::setIntermittentTransferExpected() {
    m_isIntermittentTransferExpected = true;
}

inline std::string HTTP2RequestConfig::getId() const {
    return m_id;
}

inline HTTP2RequestType HTTP2RequestConfig::getRequestType() const {
    return m_type;
}

inline std::string HTTP2RequestConfig::getUrl() const {
    return m_url;
};

inline std::chrono::milliseconds HTTP2RequestConfig::getConnectionTimeout() const {
    return m_connectionTimeout;
}

inline std::chrono::milliseconds HTTP2RequestConfig::getTransferTimeout() const {
    return m_transferTimeout;
}

inline std::chrono::milliseconds HTTP2RequestConfig::getActivityTimeout() const {
    return m_activityTimeout;
}

inline uint8_t HTTP2RequestConfig::getPriority() const {
    return m_priority;
}

inline std::shared_ptr<HTTP2RequestSourceInterface> HTTP2RequestConfig::getSource() const {
    return m_source;
}

inline std::shared_ptr<HTTP2ResponseSinkInterface> HTTP2RequestConfig::getSink() const {
    return m_sink;
}

inline bool HTTP2RequestConfig::isIntermittentTransferExpected() const {
    return m_isIntermittentTransferExpected;
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2REQUESTCONFIG_H_
