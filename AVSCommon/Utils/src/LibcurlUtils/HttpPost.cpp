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

#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("HttpPost");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<HttpPost> HttpPost::create() {
    std::unique_ptr<HttpPost> httpPost(new HttpPost());
    if (httpPost->init()) {
        return httpPost;
    }
    return nullptr;
}

HttpPost::HttpPost() : m_curl{nullptr}, m_requestHeaders{nullptr} {
}

bool HttpPost::init() {
    m_curl = curl_easy_init();
    if (!m_curl) {
        ACSDK_ERROR(LX("initFailed").d("reason", "curl_easy_initFailed"));
        return false;
    }
    if (!libcurlUtils::prepareForTLS(m_curl)) {
        return false;
    }
    if (!setopt(CURLOPT_WRITEFUNCTION, staticWriteCallbackLocked)) {
        return false;
    }
    /*
     * The documentation from libcurl recommends setting CURLOPT_NOSIGNAL to 1 for multi-threaded applications.
     * https://curl.haxx.se/libcurl/c/threadsafe.html
     */
    if (!setopt(CURLOPT_NOSIGNAL, 1)) {
        return false;
    }
    return true;
}

HttpPost::~HttpPost() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }

    if (m_requestHeaders) {
        curl_slist_free_all(m_requestHeaders);
        m_requestHeaders = nullptr;
    }
}

bool HttpPost::addHTTPHeader(const std::string& header) {
    m_requestHeaders = curl_slist_append(m_requestHeaders, header.c_str());
    if (!m_requestHeaders) {
        ACSDK_ERROR(LX("addHTTPHeaderFailed")
                        .d("reason", "curlFailure")
                        .d("method", "curl_slist_append")
                        .sensitive("header", header));
        return false;
    }
    if (!setopt(CURLOPT_HTTPHEADER, m_requestHeaders)) {
        return false;
    }
    return true;
}

long HttpPost::doPost(
    const std::string& url,
    const std::string& data,
    std::chrono::seconds timeout,
    std::string& body) {
    std::lock_guard<std::mutex> lock(m_mutex);

    body.clear();

    if (!setopt(CURLOPT_TIMEOUT, static_cast<long>(timeout.count())) || !setopt(CURLOPT_URL, url.c_str()) ||
        !setopt(CURLOPT_POSTFIELDS, data.c_str()) || !setopt(CURLOPT_WRITEDATA, &body)) {
        return HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
    }

    auto result = curl_easy_perform(m_curl);

    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("doPostFailed")
                        .d("reason", "curl_easy_performFailed")
                        .d("result", result)
                        .d("error", curl_easy_strerror(result)));
        body.clear();
        return HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
    }

    long responseCode = 0;
    result = curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("doPostFailed")
                        .d("reason", "curl_easy_getinfoFailed")
                        .d("property", "CURLINFO_RESPONSE_CODE")
                        .d("result", result)
                        .d("error", curl_easy_strerror(result)));
        body.clear();
        return HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
    } else {
        ACSDK_DEBUG(LX("doPostSucceeded").d("code", responseCode));
        return responseCode;
    }
}

template <typename ParamType>
bool HttpPost::setopt(CURLoption option, ParamType value) {
    auto result = curl_easy_setopt(m_curl, option, value);
    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("setoptFailed")
                        .d("reason", "nullCurlHandle")
                        .d("option", option)
                        .sensitive("value", value)
                        .d("result", result)
                        .d("error", curl_easy_strerror(result)));
        return false;
    }
    return true;
}

size_t HttpPost::staticWriteCallbackLocked(char* ptr, size_t size, size_t nmemb, void* userdata) {
    if (!userdata) {
        ACSDK_ERROR(LX("staticWriteCallbackFailed").d("reason", "nullUserData"));
        return 0;
    }

    size_t count = size * nmemb;
    auto body = static_cast<std::string*>(userdata);
    body->append(ptr, count);
    return count;
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
