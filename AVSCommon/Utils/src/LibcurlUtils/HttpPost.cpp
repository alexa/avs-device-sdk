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
#define TAG "HttpPost"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<HttpPostInterface> HttpPost::createHttpPostInterface() {
    return std::unique_ptr<HttpPostInterface>(new HttpPost());
}

std::unique_ptr<HttpPost> HttpPost::create() {
    return std::unique_ptr<HttpPost>(new HttpPost());
}

long HttpPost::doPost(
    const std::string& url,
    const std::string& data,
    std::chrono::seconds timeout,
    std::string& body) {
    CurlEasyHandleWrapper curl;
    auto response = doPostInternal(curl, url, {}, data, timeout);
    body = response.body;
    return response.code;
}

HTTPResponse HttpPost::doPost(
    const std::string& url,
    const std::vector<std::string> headerLines,
    const std::vector<std::pair<std::string, std::string>>& data,
    std::chrono::seconds timeout) {
    CurlEasyHandleWrapper curl;
    auto encodedData = buildPostData(curl, data);
    return doPostInternal(curl, url, headerLines, encodedData, timeout);
}

HTTPResponse HttpPost::doPost(
    const std::string& url,
    const std::vector<std::string> headerLines,
    const std::string& data,
    std::chrono::seconds timeout) {
    CurlEasyHandleWrapper curl;
    return doPostInternal(curl, url, headerLines, data, timeout);
}

HTTPResponse HttpPost::doPostInternal(
    CurlEasyHandleWrapper& curl,
    const std::string& url,
    const std::vector<std::string> headerLines,
    const std::string& data,
    std::chrono::seconds timeout) {
    HTTPResponse response;

    if (!curl.isValid() || !curl.setTransferTimeout(static_cast<long>(timeout.count())) || !curl.setURL(url) ||
        !curl.setPostData(data) || !curl.setWriteCallback(staticWriteCallbackLocked, &response.body)) {
        return HTTPResponse();
    }

    for (auto line : headerLines) {
        if (!curl.addHTTPHeader(line)) {
            ACSDK_ERROR(LX("doPostFailed").d("reason", "unableToAddHttpHeader"));
            return HTTPResponse();
        }
    }

    auto curlHandle = curl.getCurlHandle();
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

std::string HttpPost::buildPostData(
    CurlEasyHandleWrapper& curl,
    const std::vector<std::pair<std::string, std::string>>& data) const {
    std::stringstream dataStream;

    for (auto ix = data.begin(); ix != data.end(); ix++) {
        if (ix != data.begin()) {
            dataStream << '&';
        }
        dataStream << curl.urlEncode(ix->first) << '=' << curl.urlEncode(ix->second);
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
