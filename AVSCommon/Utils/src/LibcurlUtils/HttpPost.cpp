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

#include <sstream>

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
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
    if (httpPost->m_curl.isValid()) {
        return httpPost;
    }
    return nullptr;
}

long HttpPost::doPost(
    const std::string& url,
    const std::string& data,
    std::chrono::seconds timeout,
    std::string& body) {
    auto response = doPost(url, {}, data, timeout);
    body = response.body;
    return response.code;
}

HTTPResponse HttpPost::doPost(
    const std::string& url,
    const std::vector<std::string> headerLines,
    const std::vector<std::pair<std::string, std::string>>& data,
    std::chrono::seconds timeout) {
    auto encodedData = buildPostData(data);
    return doPost(url, headerLines, encodedData, timeout);
}

HTTPResponse HttpPost::doPost(
    const std::string& url,
    const std::vector<std::string> headerLines,
    const std::string& data,
    std::chrono::seconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);

    HTTPResponse response;

    if (!m_curl.reset() || !m_curl.setTransferTimeout(static_cast<long>(timeout.count())) || !m_curl.setURL(url) ||
        !m_curl.setPostData(data) || !m_curl.setWriteCallback(staticWriteCallbackLocked, &response.body)) {
        return HTTPResponse();
    }

    for (auto line : headerLines) {
        if (!m_curl.addHTTPHeader(line)) {
            ACSDK_ERROR(LX("doPostFailed").d("reason", "unableToAddHttpHeader"));
            return HTTPResponse();
        }
    }

    auto curlHandle = m_curl.getCurlHandle();
    auto result = curl_easy_perform(curlHandle);

    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("doPostFailed")
                        .d("reason", "curl_easy_performFailed")
                        .d("result", result)
                        .d("error", curl_easy_strerror(result)));
        return HTTPResponse();
    }

    result = curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &response.code);
    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("doPostFailed")
                        .d("reason", "curl_easy_getinfoFailed")
                        .d("property", "CURLINFO_RESPONSE_CODE")
                        .d("result", result)
                        .d("error", curl_easy_strerror(result)));
        return HTTPResponse();
    } else {
        ACSDK_DEBUG5(LX("doPostSucceeded").d("code", response.code));
        return response;
    }
}

std::string HttpPost::buildPostData(const std::vector<std::pair<std::string, std::string>>& data) const {
    std::stringstream dataStream;

    for (auto ix = data.begin(); ix != data.end(); ix++) {
        if (ix != data.begin()) {
            dataStream << '&';
        }
        dataStream << m_curl.urlEncode(ix->first) << '=' << m_curl.urlEncode(ix->second);
    }

    return dataStream.str();
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
