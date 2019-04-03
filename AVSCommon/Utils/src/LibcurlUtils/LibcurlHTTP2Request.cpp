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

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include "AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2Request.h"
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace avsCommon::utils::http2;
using namespace std::chrono;

static constexpr long INVALID_RESPONSE_CODE = -1L;

/// String to identify log entries originating from this file.
static const std::string TAG("LibcurlHTTP2Request");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

size_t LibcurlHTTP2Request::writeCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullUserData"));
        return CURLE_WRITE_ERROR;
    }

    LibcurlHTTP2Request* stream = static_cast<LibcurlHTTP2Request*>(userData);
    ACSDK_DEBUG9(LX(__func__).d("id", stream->getId()).d("size", size).d("nmemb", nmemb).d("userData", userData));

    stream->setTimeOfLastTransfer();
    stream->reportResponseCode();

    auto length = size * nmemb;
    if (length != 0 && stream->m_sink) {
        auto result = stream->m_sink->onReceiveData(data, length);
        switch (result) {
            case HTTP2ReceiveDataStatus::SUCCESS:
                return length;
            case HTTP2ReceiveDataStatus::PAUSE:
                stream->m_isPaused = true;
                return CURL_WRITEFUNC_PAUSE;
            case HTTP2ReceiveDataStatus ::ABORT:
                return 0;
        }
    }

    return 0;
}

size_t LibcurlHTTP2Request::headerCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX("headerCallbackFailed").d("reason", "nullUserData"));
        return 0;
    }

    LibcurlHTTP2Request* stream = static_cast<LibcurlHTTP2Request*>(userData);
    ACSDK_DEBUG9(LX(__func__).d("id", stream->getId()).d("size", size).d("nmemb", nmemb).d("userData", userData));

    stream->setTimeOfLastTransfer();
    stream->reportResponseCode();

    auto length = size * nmemb;
    if (length != 0 && stream->m_sink) {
        std::string line(data, length);
        stream->m_sink->onReceiveHeaderLine(line);
    }

    return length;
}

size_t LibcurlHTTP2Request::readCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX("readCallback").d("reason", "nullUserData"));
        return CURLE_READ_ERROR;
    }

    LibcurlHTTP2Request* stream = static_cast<LibcurlHTTP2Request*>(userData);
    ACSDK_DEBUG9(LX(__func__).d("id", stream->getId()).d("size", size).d("nmemb", nmemb).d("userData", userData));

    stream->setTimeOfLastTransfer();

    auto length = size * nmemb;
    if (length != 0 && stream->m_source) {
        auto result = stream->m_source->onSendData(data, length);
        switch (result.status) {
            case HTTP2SendStatus::CONTINUE:
                return result.size;
            case HTTP2SendStatus::PAUSE:
                stream->m_isPaused = true;
                return CURL_READFUNC_PAUSE;
            case HTTP2SendStatus::COMPLETE:
                return 0;
            case HTTP2SendStatus::ABORT:
                return CURL_READFUNC_ABORT;
        }
    }

    return CURL_READFUNC_ABORT;
}

long LibcurlHTTP2Request::getResponseCode() {
    long responseCode = 0;
    CURLcode ret = curl_easy_getinfo(m_stream.getCurlHandle(), CURLINFO_RESPONSE_CODE, &responseCode);
    if (ret != CURLE_OK) {
        ACSDK_ERROR(LX("getResponseCodeFailed")
                        .d("reason", "curlFailure")
                        .d("method", "curl_easy_getinfo")
                        .d("info", "CURLINFO_RESPONSE_CODE")
                        .d("error", curl_easy_strerror(ret)));
        return INVALID_RESPONSE_CODE;
    }
    return responseCode;
}

CURL* LibcurlHTTP2Request::getCurlHandle() {
    return m_stream.getCurlHandle();
}

LibcurlHTTP2Request::LibcurlHTTP2Request(
    const alexaClientSDK::avsCommon::utils::http2::HTTP2RequestConfig& config,
    std::string id) :
        m_responseCodeReported{false},
        m_activityTimeout{milliseconds::zero()},
        m_timeOfLastTransfer{steady_clock::now()},
        m_stream{std::move(id)},
        m_isIntermittentTransferExpected{config.isIntermittentTransferExpected()},
        m_isPaused{false},
        m_isCancelled{false} {
    switch (config.getRequestType()) {
        case HTTP2RequestType::GET:
            m_stream.setTransferType(CurlEasyHandleWrapper::TransferType::kGET);
            break;
        case HTTP2RequestType::POST:
            curl_easy_setopt(m_stream.getCurlHandle(), CURLOPT_POST, 1L);
            m_stream.setReadCallback(LibcurlHTTP2Request::readCallback, this);
            break;
    }
    m_stream.setURL(config.getUrl());
    m_stream.setWriteCallback(LibcurlHTTP2Request::writeCallback, this);
    m_stream.setHeaderCallback(LibcurlHTTP2Request::headerCallback, this);
    m_stream.setopt(CURLOPT_TCP_KEEPALIVE, 1);
    m_stream.setopt(CURLOPT_STREAM_WEIGHT, config.getPriority());

    if (config.getSource()) {
        m_source = config.getSource();
        auto headers = m_source->getRequestHeaderLines();
        for (const auto& header : headers) {
            m_stream.addHTTPHeader(header);
        }
    }
    if (config.getSink()) {
        m_sink = config.getSink();
    }
    if (config.getConnectionTimeout() != std::chrono::milliseconds::zero()) {
        m_stream.setopt(CURLOPT_CONNECTTIMEOUT_MS, config.getConnectionTimeout().count());
    }
    if (config.getTransferTimeout() != std::chrono::milliseconds::zero()) {
        m_stream.setopt(CURLOPT_TIMEOUT_MS, config.getTransferTimeout().count());
    }
    if (config.getActivityTimeout() != std::chrono::milliseconds::zero()) {
        m_activityTimeout = config.getActivityTimeout();
    }
};

bool LibcurlHTTP2Request::hasProgressTimedOut() const {
    if (m_activityTimeout == milliseconds::zero()) {
        return false;  // no activity timeout checks
    }
    return duration_cast<milliseconds>(steady_clock::now() - m_timeOfLastTransfer) > m_activityTimeout;
}
bool LibcurlHTTP2Request::isIntermittentTransferExpected() const {
    return m_isIntermittentTransferExpected;
}

void LibcurlHTTP2Request::unPause() {
    m_isPaused = false;
    m_stream.pause(CURLPAUSE_CONT);
}

bool LibcurlHTTP2Request::isPaused() const {
    return m_isPaused;
}

bool LibcurlHTTP2Request::isCancelled() const {
    return m_isCancelled;
}

bool LibcurlHTTP2Request::cancel() {
    m_isCancelled = true;
    return true;
}

std::string LibcurlHTTP2Request::getId() const {
    return m_stream.getId();
}

void LibcurlHTTP2Request::reportCompletion(HTTP2ResponseFinishedStatus status) {
    if (m_sink) {
        m_sink->onResponseFinished(status);
    }
}

void LibcurlHTTP2Request::reportResponseCode() {
    if (m_responseCodeReported || !m_sink) {
        return;
    }
    if (long responseCode = getResponseCode()) {
        m_sink->onReceiveResponseCode(responseCode);
        m_responseCodeReported = true;
    }
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
