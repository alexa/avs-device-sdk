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

#include <AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("CurlEasyHandleWrapper");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// MIME Content-Type for JSON data
static std::string JSON_MIME_TYPE = "text/json";
/// MIME Content-Type for octet stream data
static std::string OCTET_MIME_TYPE = "application/octet-stream";

CurlEasyHandleWrapper::CurlEasyHandleWrapper() :
        m_handle{curl_easy_init()},
        m_requestHeaders{nullptr},
        m_postHeaders{nullptr},
        m_post{nullptr} {
    if (m_handle == nullptr) {
        ACSDK_ERROR(LX("CurlEasyHandleWrapperFailed").d("reason", "curl_easy_init failed"));
    } else {
        setDefaultOptions();
    }
};

CurlEasyHandleWrapper::~CurlEasyHandleWrapper() {
    cleanupResources();
    if (m_handle != nullptr) {
        curl_easy_cleanup(m_handle);
    }
};

bool CurlEasyHandleWrapper::reset() {
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
    }
    return ret;
}

bool CurlEasyHandleWrapper::setPostContent(const std::string& fieldName, const std::string& payload) {
    curl_httppost* last = nullptr;
    CURLFORMcode ret = curl_formadd(
        &m_post,
        &last,
        CURLFORM_COPYNAME,
        fieldName.c_str(),
        CURLFORM_COPYCONTENTS,
        payload.c_str(),
        CURLFORM_CONTENTTYPE,
        JSON_MIME_TYPE.c_str(),
        CURLFORM_CONTENTHEADER,
        m_postHeaders,
        CURLFORM_END);
    if (ret) {
        ACSDK_ERROR(LX("setPostContentFailed")
                        .d("reason", "curlFailure")
                        .d("method", "curl_formadd")
                        .d("fieldName", fieldName)
                        .sensitive("content", payload)
                        .d("curlFormCode", ret));

        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setTransferTimeout(const long timeoutSeconds) {
    return setopt(CURLOPT_TIMEOUT, timeoutSeconds);
}

bool CurlEasyHandleWrapper::setPostStream(const std::string& fieldName, void* userData) {
    curl_httppost* last = m_post;
    CURLFORMcode ret = curl_formadd(
        &m_post,
        &last,
        CURLFORM_COPYNAME,
        fieldName.c_str(),
        CURLFORM_STREAM,
        userData,
        CURLFORM_CONTENTTYPE,
        OCTET_MIME_TYPE.c_str(),
        CURLFORM_END);
    if (ret) {
        ACSDK_ERROR(LX("setPostStreamFailed")
                        .d("reason", "curlFailure")
                        .d("method", "curl_formadd")
                        .d("fieldName", fieldName)
                        .d("curlFormCode", ret));
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setPostData(const std::string& data) {
    return setopt(CURLOPT_POSTFIELDS, data.c_str());
}

bool CurlEasyHandleWrapper::setConnectionTimeout(const std::chrono::seconds timeoutSeconds) {
    return setopt(CURLOPT_CONNECTTIMEOUT, timeoutSeconds.count());
}

bool CurlEasyHandleWrapper::setWriteCallback(CurlCallback callback, void* userData) {
    return setopt(CURLOPT_WRITEFUNCTION, callback) && (userData == nullptr || setopt(CURLOPT_WRITEDATA, userData));
}

bool CurlEasyHandleWrapper::setHeaderCallback(CurlCallback callback, void* userData) {
    return setopt(CURLOPT_HEADERFUNCTION, callback) && (userData == nullptr || setopt(CURLOPT_HEADERDATA, userData));
}

bool CurlEasyHandleWrapper::setReadCallback(CurlCallback callback, void* userData) {
    return setopt(CURLOPT_READFUNCTION, callback) && (userData == nullptr || setopt(CURLOPT_READDATA, userData));
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
    }
}

bool CurlEasyHandleWrapper::setDefaultOptions() {
    if (prepareForTLS(m_handle)) {
        /*
         * The documentation from libcurl recommends setting CURLOPT_NOSIGNAL to 1 for multi-threaded applications.
         * https://curl.haxx.se/libcurl/c/threadsafe.html
         */
        return setopt(CURLOPT_NOSIGNAL, 1);
    }
    ACSDK_ERROR(LX("setDefaultOptions").d("reason", "prepareForTLS failed"));
    curl_easy_cleanup(m_handle);
    m_handle = nullptr;
    return false;
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
