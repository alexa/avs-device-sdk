/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstdint>
#include <sstream>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <AVSCommon/Utils/Logger/ThreadMoniker.h>

#include "ACL/Transport/HTTP2Stream.h"
#include "ACL/Transport/HTTP2Transport.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::libcurlUtils;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2Stream");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// MIME boundary string prefix in HTTP header.
static const std::string BOUNDARY_PREFIX = "boundary=";
/// Size in chars of the MIME boundary string prefix
static const int BOUNDARY_PREFIX_SIZE = BOUNDARY_PREFIX.size();
/// MIME HTTP header value delimiter
static const std::string BOUNDARY_DELIMITER = ";";
/// The HTTP header to pass the LWA token into
static const std::string AUTHORIZATION_HEADER = "Authorization: Bearer ";
/// The POST field name for an attachment
static const std::string ATTACHMENT_FIELD_NAME = "audio";
/// The POST field name for message metadata
static const std::string METADATA_FIELD_NAME = "metadata";
/// The prefix for a stream contextId.
static const std::string STREAM_CONTEXT_ID_PREFIX_STRING = "ACL_LOGICAL_HTTP2_STREAM_ID_";
/// The prefix of request IDs passed back in the header of AVS replies.
static const std::string X_AMZN_REQUESTID_PREFIX = "x-amzn-requestid:";
/// Key under root configuration node for ACL configuration values.
static const std::string ACL_CONFIGURATION_KEY("acl");
/// Key under 'acl' configuration node for path/prefix of per-stream log file names.
static const std::string STREAM_LOG_PREFIX_KEY("streamLogPrefix");
/// Prefix for per-stream log file names.
static const std::string STREAM_LOG_NAME_PREFIX("stream-");
/// Suffix for per-stream log file names.
static const std::string STREAM_LOG_NAME_SUFFIX(".log");
/// Suffix for per-stream dump of incoming data.
static const std::string STREAM_IN_DUMP_SUFFIX("-in.bin");
/// Suffix for per-stream dump of outgoing data.
static const std::string STREAM_OUT_DUMP_SUFFIX("-out.bin");

#ifdef DEBUG
/// Carriage return
static const char CR = 0x0D;
#endif

#ifdef ACSDK_EMIT_CURL_LOGS

/**
 * Macro to simplify building a switch that translates from enum values to strings.
 *
 * @param name The name of the enum value to translate.
 */
#define ACSDK_TYPE_CASE(name) \
    case name:                \
        return #name;

/**
 * Return a string identifying a @c curl_infotype value.
 *
 * @param type The value to identify
 * @return A string identifying the specified @c curl_infotype value.
 */
static const char* curlInfoTypeToString(curl_infotype type) {
    switch (type) {
        ACSDK_TYPE_CASE(CURLINFO_TEXT)
        ACSDK_TYPE_CASE(CURLINFO_HEADER_OUT)
        ACSDK_TYPE_CASE(CURLINFO_DATA_OUT)
        ACSDK_TYPE_CASE(CURLINFO_SSL_DATA_OUT)
        ACSDK_TYPE_CASE(CURLINFO_HEADER_IN)
        ACSDK_TYPE_CASE(CURLINFO_DATA_IN)
        ACSDK_TYPE_CASE(CURLINFO_SSL_DATA_IN)
        ACSDK_TYPE_CASE(CURLINFO_END)
    }
    return ">>> unknown curl_infotype value <<<";
}

#undef ACSDK_TYPE_CASE

/**
 * Return a prefix suitable for the data associated with a @c curl_infotype value.
 *
 * @param type The type of data to prefix.
 * @return The prefix to use for the specified typw of data.
 */
static const char* curlInfoTypeToPrefix(curl_infotype type) {
    switch (type) {
        case CURLINFO_TEXT:
            return "* ";
        case CURLINFO_HEADER_OUT:
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_OUT:
            return "> ";
        case CURLINFO_HEADER_IN:
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
            return "< ";
        case CURLINFO_END:
            return "";
    }
    return ">>> unknown curl_infotype value <<<";
}

#endif  // ACSDK_EMIT_CURL_LOGS

/**
 * Get @c std::chrono::steady_clock::now() in a form that can be wrapped in @c atomic.
 *
 * @return @c std::chrono::steady_clock::now() in a form that can be wrapped in @c atomic.
 */
static std::chrono::steady_clock::rep getNow() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

HTTP2Stream::HTTP2Stream(
    std::shared_ptr<MessageConsumerInterface> messageConsumer,
    std::shared_ptr<AttachmentManager> attachmentManager) :
        m_logicalStreamId{0},
        m_parser{messageConsumer, attachmentManager},
        m_isPaused{false},
        m_progressTimeout{std::chrono::steady_clock::duration::max().count()},
        m_timeOfLastTransfer{getNow()} {
}

bool HTTP2Stream::reset() {
    if (!m_transfer.reset()) {
        ACSDK_ERROR(LX("resetFailed").d("reason", "resetHandleFailed"));
        return false;
    }
    m_parser.reset();
    m_currentRequest.reset();
    m_isPaused = false;
    m_exceptionBeingProcessed.clear();
    m_progressTimeout = std::chrono::steady_clock::duration::max().count();
    m_timeOfLastTransfer = getNow();
    return true;
}

bool HTTP2Stream::setCommonOptions(const std::string& url, const std::string& authToken) {
#ifdef ACSDK_EMIT_CURL_LOGS

    if (!(setopt(CURLOPT_DEBUGDATA, "CURLOPT_DEBUGDATA", this) &&
          setopt(CURLOPT_DEBUGFUNCTION, "CURLOPT_DEBUGFUNCTION", debugFunction) &&
          setopt(CURLOPT_VERBOSE, "CURLOPT_VERBOSE", 1L))) {
        return false;
    }

#endif

    if (!m_transfer.setURL(url)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed").d("reason", "setURLFailed").d("url", url));
        return false;
    }

    std::ostringstream authHeader;
    authHeader << AUTHORIZATION_HEADER << authToken;
    if (!m_transfer.addHTTPHeader(authHeader.str())) {
        ACSDK_ERROR(
            LX("setCommonOptionsFailed").d("reason", "addHTTPHeaderFailed").sensitive("authHeader", authHeader.str()));
        return false;
    }

    if (!m_transfer.setWriteCallback(&HTTP2Stream::writeCallback, this)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed").d("reason", "setWriteCallbackFailed"));
        return false;
    }

    if (!m_transfer.setHeaderCallback(&HTTP2Stream::headerCallback, this)) {
        ACSDK_ERROR(LX("setCommonOptionsFailed").d("reason", "setHeaderCallbackFailed"));
        return false;
    }

    return setopt(CURLOPT_TCP_KEEPALIVE, "CURLOPT_TCP_KEEPALIVE", 1);
}

bool HTTP2Stream::initGet(const std::string& url, const std::string& authToken) {
    reset();
    initStreamLog();

    if (url.empty()) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "emptyURL"));
        return false;
    }

    if (authToken.empty()) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "emptyAuthToken"));
        return false;
    }

    if (!m_transfer.getCurlHandle()) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "curlEasyHandleUninitialized"));
        return false;
    }

    if (!m_transfer.setTransferType(avsCommon::utils::libcurlUtils::CurlEasyHandleWrapper::TransferType::kGET)) {
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        ACSDK_ERROR(LX("initGetFailed").d("reason", "setCommonOptionsFailed"));
        return false;
    }

    return true;
}

bool HTTP2Stream::initPost(
    const std::string& url,
    const std::string& authToken,
    std::shared_ptr<avsCommon::avs::MessageRequest> request) {
    reset();
    initStreamLog();

    std::string requestPayload = request->getJsonContent();

    if (url.empty()) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "emptyURL"));
        return false;
    }

    if (authToken.empty()) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "emptyAuthToken"));
        return false;
    }

    if (!request) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "nullMessageRequest"));
        return false;
    }

    if (!m_transfer.getCurlHandle()) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "curlEasyHandleUninitialized"));
        return false;
    }

    if (!m_transfer.setPostContent(METADATA_FIELD_NAME, requestPayload)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setPostContentFailed"));
        return false;
    }

    if (!m_transfer.setReadCallback(HTTP2Stream::readCallback, this)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setReadCallbackFailed"));
        return false;
    }

    if (request->getAttachmentReader()) {
        if (!m_transfer.setPostStream(ATTACHMENT_FIELD_NAME, this)) {
            ACSDK_ERROR(LX("initPostFailed").d("reason", "setPostStreamFailed"));
            return false;
        }
    }

    if (!m_transfer.setTransferType(avsCommon::utils::libcurlUtils::CurlEasyHandleWrapper::TransferType::kPOST)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setTransferTypeFailed"));
        return false;
    }

    if (!setCommonOptions(url, authToken)) {
        ACSDK_ERROR(LX("initPostFailed").d("reason", "setCommonOptionsFailed"));
        return false;
    }

    m_currentRequest = request;
    return true;
}

size_t HTTP2Stream::writeCallback(char* data, size_t size, size_t nmemb, void* user) {
    size_t numChars = size * nmemb;
    HTTP2Stream* stream = static_cast<HTTP2Stream*>(user);
    stream->m_timeOfLastTransfer = getNow();

    /**
     * If we get an HTTP 200 response code then we're getting a MIME multipart
     * payload. For all other response codes we're getting a JSON string without
     * multipart headers.
     */
    if (HTTPResponseCode::SUCCESS_OK == stream->getResponseCode()) {
        MimeParser::DataParsedStatus status = stream->m_parser.feed(data, numChars);

        if (MimeParser::DataParsedStatus::OK == status) {
            return numChars;
        } else if (MimeParser::DataParsedStatus::INCOMPLETE == status) {
            stream->m_isPaused = true;
            return CURL_WRITEFUNC_PAUSE;
        } else if (MimeParser::DataParsedStatus::ERROR == status) {
            return CURL_READFUNC_ABORT;
        }
    } else {
        stream->m_exceptionBeingProcessed.append(data, numChars);
    }
    return numChars;
}

size_t HTTP2Stream::headerCallback(char* data, size_t size, size_t nmemb, void* user) {
    if (!user) {
        ACSDK_ERROR(LX("headerCallbackFailed").d("reason", "nullUser"));
        return 0;
    }
    size_t headerLength = size * nmemb;
    std::string header(data, headerLength);
#ifdef DEBUG
    if (0 == header.find(X_AMZN_REQUESTID_PREFIX)) {
        auto end = header.find(CR);
        ACSDK_DEBUG(LX("receivedRequestId").d("value", header.substr(0, end)));
    }
#endif
    std::string boundary;
    HTTP2Stream* stream = static_cast<HTTP2Stream*>(user);
    stream->m_timeOfLastTransfer = getNow();
    if (HTTPResponseCode::SUCCESS_OK == stream->getResponseCode()) {
        if (header.find(BOUNDARY_PREFIX) != std::string::npos) {
            boundary = header.substr(header.find(BOUNDARY_PREFIX));
            boundary = boundary.substr(BOUNDARY_PREFIX_SIZE, boundary.find(BOUNDARY_DELIMITER) - BOUNDARY_PREFIX_SIZE);
            stream->m_parser.setBoundaryString(boundary);
        }
    }
    return headerLength;
}

size_t HTTP2Stream::readCallback(char* data, size_t size, size_t nmemb, void* userData) {
    if (!userData) {
        ACSDK_ERROR(LX("readCallbackFailed").d("reason", "nullUserData"));
        return 0;
    }

    HTTP2Stream* stream = static_cast<HTTP2Stream*>(userData);

    stream->m_timeOfLastTransfer = getNow();
    auto attachmentReader = stream->m_currentRequest->getAttachmentReader();

    // This is ok - it means there's no attachment to send.  Return 0 so libcurl can complete the stream to AVS.
    if (!attachmentReader) {
        return 0;
    }

    // Pass the data to libcurl.
    const size_t maxBytesToRead = size * nmemb;
    auto readStatus = AttachmentReader::ReadStatus::OK;
    auto bytesRead = attachmentReader->read(data, maxBytesToRead, &readStatus);

    switch (readStatus) {
        // The good cases.
        case AttachmentReader::ReadStatus::OK:
        case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
        case AttachmentReader::ReadStatus::OK_TIMEDOUT:
            break;

        // No more data to send - close the stream.
        case AttachmentReader::ReadStatus::CLOSED:
            return 0;

        // Handle any attachment read errors.
        case AttachmentReader::ReadStatus::ERROR_OVERRUN:
        case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
        case AttachmentReader::ReadStatus::ERROR_INTERNAL:
            return CURL_READFUNC_ABORT;
    }
    // The attachment has no more data right now, but is still readable.
    if (0 == bytesRead) {
        stream->m_isPaused = true;
        return CURL_READFUNC_PAUSE;
    }

    return bytesRead;
}

long HTTP2Stream::getResponseCode() {
    long responseCode = 0;
    CURLcode ret = curl_easy_getinfo(m_transfer.getCurlHandle(), CURLINFO_RESPONSE_CODE, &responseCode);
    if (ret != CURLE_OK) {
        ACSDK_ERROR(LX("getResponseCodeFailed")
                        .d("reason", "curlFailure")
                        .d("method", "curl_easy_getinfo")
                        .d("info", "CURLINFO_RESPONSE_CODE")
                        .d("error", curl_easy_strerror(ret)));
        return -1;
    }
    return responseCode;
}

CURL* HTTP2Stream::getCurlHandle() {
    return m_transfer.getCurlHandle();
}

void HTTP2Stream::notifyRequestObserver() {
    if (m_exceptionBeingProcessed.length() > 0) {
        m_currentRequest->exceptionReceived(m_exceptionBeingProcessed);
        m_exceptionBeingProcessed = "";
    }

    long responseCode = getResponseCode();

    switch (responseCode) {
        case HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR);
            break;
        case HTTPResponseCode::SUCCESS_OK:
            m_currentRequest->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS);
            break;
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT);
            break;
        case HTTPResponseCode::BAD_REQUEST:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::BAD_REQUEST);
            break;
        case HTTPResponseCode::SERVER_INTERNAL_ERROR:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
            break;
        default:
            m_currentRequest->sendCompleted(
                avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR);
    }
}

void HTTP2Stream::notifyRequestObserver(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) {
    m_currentRequest->sendCompleted(status);
}

bool HTTP2Stream::setStreamTimeout(const long timeoutSeconds) {
    if (!m_transfer.setTransferTimeout(timeoutSeconds)) {
        return false;
    }
    return true;
}

bool HTTP2Stream::setConnectionTimeout(const std::chrono::seconds timeoutSeconds) {
    return m_transfer.setConnectionTimeout(timeoutSeconds);
}

void HTTP2Stream::unPause() {
    m_isPaused = false;
    // Call curl_easy_pause() *after* resetting m_pendingBits because curl_easy_pause may call
    // readCallback() and/or writeCallback() which can modify m_pendingBits.
    curl_easy_pause(getCurlHandle(), CURLPAUSE_CONT);
}

bool HTTP2Stream::isPaused() const {
    return m_isPaused;
}

void HTTP2Stream::setLogicalStreamId(int logicalStreamId) {
    m_logicalStreamId = logicalStreamId;
    m_parser.setAttachmentContextId(STREAM_CONTEXT_ID_PREFIX_STRING + std::to_string(m_logicalStreamId));
}

unsigned int HTTP2Stream::getLogicalStreamId() const {
    return m_logicalStreamId;
}

bool HTTP2Stream::hasProgressTimedOut() const {
    return (getNow() - m_timeOfLastTransfer) > m_progressTimeout;
}

const avsCommon::utils::logger::LogStringFormatter& HTTP2Stream::getLogFormatter() const {
    return m_logFormatter;
}

template <typename ParamType>
bool HTTP2Stream::setopt(CURLoption option, const char* optionName, ParamType value) {
    auto result = curl_easy_setopt(m_transfer.getCurlHandle(), option, value);
    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("setoptFailed")
                        .d("option", optionName)
                        .sensitive("value", value)
                        .d("error", curl_easy_strerror(result)));
        return false;
    }
    return true;
}

#ifdef ACSDK_EMIT_CURL_LOGS

void HTTP2Stream::initStreamLog() {
    std::string streamLogPrefix;
    configuration::ConfigurationNode::getRoot()[ACL_CONFIGURATION_KEY].getString(
        STREAM_LOG_PREFIX_KEY, &streamLogPrefix);
    if (streamLogPrefix.empty()) {
        return;
    }

    if (m_streamLog) {
        m_streamLog->close();
        m_streamLog.reset();
    }

    if (m_streamInDump) {
        m_streamInDump->close();
        m_streamInDump.reset();
    }

    if (m_streamOutDump) {
        m_streamOutDump->close();
        m_streamOutDump.reset();
    }

    // Include a 'session id' (just a time stamp) in the log file name to avoid overwriting previous sessions.
    static std::string sessionId;
    if (sessionId.empty()) {
        sessionId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        ACSDK_INFO(LX("initStreamLog").d("sessionId", sessionId));
    }

    auto basePath = streamLogPrefix + STREAM_LOG_NAME_PREFIX + sessionId + "-" + std::to_string(m_logicalStreamId);

    auto streamLogPath = basePath + STREAM_LOG_NAME_SUFFIX;
    m_streamLog.reset(new std::ofstream(streamLogPath));
    if (!m_streamLog->good()) {
        m_streamLog.reset();
        ACSDK_ERROR(LX("initStreamLogFailed").d("reason", "fileOpenFailed").d("streamLogPath", streamLogPath));
    }

    auto streamInDumpPath = basePath + STREAM_IN_DUMP_SUFFIX;
    m_streamInDump.reset(new std::ofstream(streamInDumpPath, std::ios_base::out | std::ios_base::binary));
    if (!m_streamInDump->good()) {
        m_streamInDump.reset();
        ACSDK_ERROR(LX("initStreamLogFailed").d("reason", "fileOpenFailed").d("streamInDumpPath", streamInDumpPath));
    }

    auto streamOutDumpPath = basePath + STREAM_OUT_DUMP_SUFFIX;
    m_streamOutDump.reset(new std::ofstream(streamOutDumpPath, std::ios_base::out | std::ios_base::binary));
    if (!m_streamOutDump->good()) {
        m_streamOutDump.reset();
        ACSDK_ERROR(LX("initStreamLogFailed").d("reason", "fileOpenFailed").d("streamOutDumpPath", streamOutDumpPath));
    }
}

int HTTP2Stream::debugFunction(CURL* handle, curl_infotype type, char* data, size_t size, void* user) {
    HTTP2Stream* stream = static_cast<HTTP2Stream*>(user);
    if (!stream) {
        return 0;
    }
    if (stream->m_streamLog) {
        auto logFormatter = stream->getLogFormatter();
        (*stream->m_streamLog) << logFormatter.format(
                                      logger::Level::INFO,
                                      std::chrono::system_clock::now(),
                                      logger::ThreadMoniker::getThisThreadMoniker().c_str(),
                                      curlInfoTypeToString(type))
                               << std::endl;
        if (CURLINFO_TEXT == type) {
            (*stream->m_streamLog) << curlInfoTypeToPrefix(type) << data;
        } else {
            logger::dumpBytesToStream(
                *stream->m_streamLog, curlInfoTypeToPrefix(type), 0x20, reinterpret_cast<unsigned char*>(data), size);
        }
        stream->m_streamLog->flush();
    }
    switch (type) {
        case CURLINFO_TEXT: {
            std::string text(data, size);
            auto index = text.rfind("\n");
            if (index != std::string::npos) {
                text.resize(index);
            }
            ACSDK_DEBUG0(LX("libcurl").d("streamId", stream->getLogicalStreamId()).sensitive("text", text));
        } break;
        case CURLINFO_HEADER_IN:
        case CURLINFO_DATA_IN:
            if (stream->m_streamInDump) {
                stream->m_streamInDump->write(data, size);
                stream->m_streamInDump->flush();
            }
            break;
        case CURLINFO_HEADER_OUT:
        case CURLINFO_DATA_OUT:
            if (stream->m_streamOutDump) {
                stream->m_streamOutDump->write(data, size);
                stream->m_streamOutDump->flush();
            }
            break;
        default:
            break;
    }

    return 0;
}

#else  // ACSDK_EMIT_CURL_LOGS

void HTTP2Stream::initStreamLog() {
}

#endif  // ACSDK_EMIT_CURL_LOGS

}  // namespace acl
}  // namespace alexaClientSDK
