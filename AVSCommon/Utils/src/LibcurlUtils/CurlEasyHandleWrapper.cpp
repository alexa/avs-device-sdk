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

#include <iostream>

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include "AVSCommon/Utils/Configuration/ConfigurationNode.h"
#include <AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#ifdef ACSDK_EMIT_CURL_LOGS

#include <AVSCommon/Utils/Logger/ThreadMoniker.h>

#endif

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace avsCommon::utils::http;

/// String to identify log entries originating from this file.
static const std::string TAG("CurlEasyHandleWrapper");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// MIME Content-Type for JSON data
static std::string JSON_MIME_TYPE = "application/json";
/// MIME Content-Type for octet stream data
static std::string OCTET_MIME_TYPE = "application/octet-stream";
/// Key for looking up the @c LibCurlUtil @c ConfigurationNode.
static const std::string LIBCURLUTILS_CONFIG_KEY = "libcurlUtils";
/// Key for looking up a configuration value for @c CURLOPT_INTERFACE
static const std::string INTERFACE_CONFIG_KEY = "CURLOPT_INTERFACE";
/// Counter used to geneate unique IDs.
std::atomic<uint64_t> CurlEasyHandleWrapper::m_idGenerator{1};

#ifdef ACSDK_EMIT_CURL_LOGS
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

CurlEasyHandleWrapper::CurlEasyHandleWrapper(std::string id) :
        m_handle{curl_easy_init()},
        m_requestHeaders{nullptr},
        m_postHeaders{nullptr},
        m_post{nullptr},
        m_lastPost{nullptr} {
    if (m_handle == nullptr) {
        ACSDK_ERROR(LX("CurlEasyHandleWrapperFailed").d("reason", "curl_easy_init failed"));
    } else {
        if (id.empty()) {
            m_id = std::to_string(m_idGenerator++);
        } else {
            m_id = std::move(id);
        }
        setDefaultOptions();
    }
};

CurlEasyHandleWrapper::~CurlEasyHandleWrapper() {
    cleanupResources();
    if (m_handle != nullptr) {
        curl_easy_cleanup(m_handle);
    }
};

bool CurlEasyHandleWrapper::reset(std::string id) {
    if (id.empty()) {
        m_id = std::to_string(m_idGenerator++);
    } else {
        m_id = std::move(id);
    }
    cleanupResources();
    long responseCode = 0;
    CURLcode ret = curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &responseCode);
    if (ret != CURLE_OK) {
        ACSDK_ERROR(LX("resetFailed")
                        .d("reason", "curlFailure")
                        .d("method", "curl_easy_getinfo")
                        .d("info", "CURLINFO_RESPONSE_CODE")
                        .d("error", curl_easy_strerror(ret)));
        curl_easy_cleanup(m_handle);
        m_handle = curl_easy_init();
        // If we can't create a new handle don't try to set options
        if (!m_handle) {
            ACSDK_ERROR(LX("resetFailed").d("reason", "curlFailure").d("method", "curl_easy_init"));
            return false;
        }
        return setDefaultOptions();
    }

    /*
     * It appears that re-using a curl easy handle after receiving an HTTP 204 (Success No Content)
     * causes the next transfer to timeout. As a workaround just cleanup the handle and create a new one
     * if we receive a 204.
     *
     * This may be related to an older curl version. This workaround is confirmed unneeded for curl 7.55.1
     */
    if (HTTPResponseCode::SUCCESS_NO_CONTENT == responseCode) {
        ACSDK_DEBUG(LX("reset").d("responseCode", "HTTP_RESPONSE_SUCCESS_NO_CONTENT"));
        curl_easy_cleanup(m_handle);
        m_handle = curl_easy_init();
        if (!m_handle) {
            ACSDK_ERROR(LX("resetFailed").d("reason", "curlFailure").d("method", "curl_easy_init"));
            return false;
        }
    } else {
        curl_easy_reset(m_handle);
    }
    return setDefaultOptions();
}

bool CurlEasyHandleWrapper::isValid() {
    return m_handle != nullptr;
}

std::string CurlEasyHandleWrapper::getId() const {
    return m_id;
}

CURL* CurlEasyHandleWrapper::getCurlHandle() {
    return m_handle;
}

bool CurlEasyHandleWrapper::addHTTPHeader(const std::string& header) {
    m_requestHeaders = curl_slist_append(m_requestHeaders, header.c_str());
    if (!m_requestHeaders) {
        ACSDK_ERROR(LX("addHTTPHeaderFailed").d("reason", "curlFailure").d("method", "curl_slist_append"));
        ACSDK_DEBUG(LX("addHTTPHeaderFailed").sensitive("header", header));
        return false;
    }
    return setopt(CURLOPT_HTTPHEADER, m_requestHeaders);
}

bool CurlEasyHandleWrapper::addPostHeader(const std::string& header) {
    m_postHeaders = curl_slist_append(m_postHeaders, header.c_str());
    if (!m_postHeaders) {
        ACSDK_ERROR(LX("addPostHeaderFailed").d("reason", "curlFailure").d("method", "curl_slist_append"));
        ACSDK_DEBUG(LX("addPostHeaderFailed").sensitive("header", header));
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setURL(const std::string& url) {
    return setopt(CURLOPT_URL, url.c_str());
}

bool CurlEasyHandleWrapper::setTransferType(TransferType type) {
    bool ret = false;
    switch (type) {
        case TransferType::kGET:
            ret = setopt(CURLOPT_HTTPGET, 1L);
            break;
        case TransferType::kPOST:
            ret = setopt(CURLOPT_HTTPPOST, m_post);
            break;
        case TransferType::kPUT:
            ret = setopt(CURLOPT_UPLOAD, 1L);
            break;
    }
    return ret;
}

bool CurlEasyHandleWrapper::setTransferTimeout(const long timeoutSeconds) {
    return setopt(CURLOPT_TIMEOUT, timeoutSeconds);
}

bool CurlEasyHandleWrapper::setPostData(const std::string& data) {
    return setopt(CURLOPT_POSTFIELDS, data.c_str());
}

bool CurlEasyHandleWrapper::setConnectionTimeout(const std::chrono::seconds timeoutSeconds) {
    return setopt(CURLOPT_CONNECTTIMEOUT, timeoutSeconds.count());
}

bool CurlEasyHandleWrapper::setWriteCallback(CurlCallback callback, void* userData) {
    return setopt(CURLOPT_WRITEFUNCTION, callback) && (!userData || setopt(CURLOPT_WRITEDATA, userData));
}

bool CurlEasyHandleWrapper::setHeaderCallback(CurlCallback callback, void* userData) {
    return setopt(CURLOPT_HEADERFUNCTION, callback) && (!userData || setopt(CURLOPT_HEADERDATA, userData));
}

bool CurlEasyHandleWrapper::setReadCallback(CurlCallback callback, void* userData) {
    return setopt(CURLOPT_READFUNCTION, callback) && (!userData || setopt(CURLOPT_READDATA, userData));
}

std::string CurlEasyHandleWrapper::urlEncode(const std::string& in) const {
    std::string result;
    auto temp = curl_easy_escape(m_handle, in.c_str(), 0);
    if (temp) {
        result = temp;
        curl_free(temp);
    }
    return result;
}

void CurlEasyHandleWrapper::cleanupResources() {
    if (m_requestHeaders) {
        curl_slist_free_all(m_requestHeaders);
        m_requestHeaders = nullptr;
    }

    if (m_postHeaders) {
        curl_slist_free_all(m_postHeaders);
        m_postHeaders = nullptr;
    }

    if (m_post) {
        curl_formfree(m_post);
        m_post = nullptr;
        m_lastPost = nullptr;
    }
}

bool CurlEasyHandleWrapper::setDefaultOptions() {
#ifdef ACSDK_EMIT_CURL_LOGS
    if (!(setopt(CURLOPT_DEBUGDATA, this) && setopt(CURLOPT_DEBUGFUNCTION, debugFunction) &&
          setopt(CURLOPT_VERBOSE, 1L))) {
        return false;
    }
    initStreamLog();
#endif

    // Using a do-while(false) loop here so we can break from the loop and do some cleanup in case of failures.
    do {
        if (!prepareForTLS(m_handle)) {
            ACSDK_ERROR(LX("setDefaultOptions").d("reason", "prepareForTLS failed"));
            break;
        }

        /*
         * The documentation from libcurl recommends setting CURLOPT_NOSIGNAL to 1 for multi-threaded applications.
         * https://curl.haxx.se/libcurl/c/threadsafe.html
         */
        if (!setopt(CURLOPT_NOSIGNAL, 1)) {
            break;
        }

        auto config = configuration::ConfigurationNode::getRoot()[LIBCURLUTILS_CONFIG_KEY];
        std::string interfaceName;
        if (config.getString(INTERFACE_CONFIG_KEY, &interfaceName) &&
            !setopt(CURLOPT_INTERFACE, interfaceName.c_str())) {
            break;
        }

        // Everything is good, return true here.
        return true;
    } while (false);

    // Cleanup after breaking do-while(false) loop.
    curl_easy_cleanup(m_handle);
    m_handle = nullptr;
    return false;
}

long CurlEasyHandleWrapper::getHTTPResponseCode() {
    if (isValid()) {
        long http_code = 0;
        auto status = curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &http_code);
        if (status != CURLE_OK) {
            ACSDK_ERROR(LX("getHTTPResponseCodeFailed").d("reason", "curlEasyGetinfoFailed"));
            http_code = 0;
        }
        return http_code;
    }

    ACSDK_ERROR(LX("getHTTPResponseCodeFailed").d("reason", "notValid"));
    return 0;
}

CURLcode CurlEasyHandleWrapper::perform() {
    if (isValid()) {
        return curl_easy_perform(m_handle);
    }

    ACSDK_ERROR(LX("performFailed").d("reason", "notValid"));
    return CURLcode::CURLE_FAILED_INIT;
}

CURLcode CurlEasyHandleWrapper::pause(int mask) {
    if (isValid()) {
        return curl_easy_pause(m_handle, mask);
    }

    ACSDK_ERROR(LX("pauseFailed").d("reason", "notValid"));
    return CURLcode::CURLE_FAILED_INIT;
}

#ifdef ACSDK_EMIT_CURL_LOGS
void CurlEasyHandleWrapper::initStreamLog() {
    std::string streamLogPrefix;
    configuration::ConfigurationNode::getRoot()[LIBCURLUTILS_CONFIG_KEY].getString(
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
        sessionId = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        ACSDK_INFO(LX("initStreamLog").d("sessionId", sessionId));
    }

    auto basePath = streamLogPrefix + STREAM_LOG_NAME_PREFIX + sessionId + "-" + m_id;

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

int CurlEasyHandleWrapper::debugFunction(CURL* handle, curl_infotype type, char* data, size_t size, void* user) {
    CurlEasyHandleWrapper* stream = static_cast<CurlEasyHandleWrapper*>(user);
    if (!stream) {
        return 0;
    }
    if (stream->m_streamLog) {
        auto logFormatter = stream->m_logFormatter;
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
            ACSDK_DEBUG9(LX("libcurl").d("id", stream->m_id).sensitive("text", text));
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
#endif  // ACSDK_EMIT_CURL_LOGS

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
