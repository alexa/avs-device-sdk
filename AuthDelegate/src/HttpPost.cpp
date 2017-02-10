/*
 * HttpPost.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "AuthDelegate/Config.h"
#include "AuthDelegate/HttpPost.h"

namespace alexaClientSDK {
namespace authDelegate {

std::unique_ptr<HttpPost> HttpPost::create() {
    std::unique_ptr<HttpPost> httpPost(new HttpPost());
    if (httpPost->init()){
        return httpPost;
    }
    return nullptr;
}

HttpPost::HttpPost() : m_curl{nullptr} {
}

bool HttpPost::init() {
    m_curl = curl_easy_init();
    if (!m_curl) {
        std::cerr << "curl_easy_init() failed." << std::endl;
        return false;
    }
    if (!setopt(CURLOPT_WRITEFUNCTION, staticWriteCallbackLocked)) {
        return false;
    }
    return true;
}

HttpPost::~HttpPost() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

HttpPost::ResponseCode HttpPost::doPost(
        const std::string& url,
        const std::string& data,
        std::chrono::seconds timeout,
        std::string& body) {
    std::lock_guard<std::mutex> lock(m_mutex);

    body.clear();

    if (!setopt(CURLOPT_TIMEOUT, static_cast<long>(timeout.count())) ||
            !setopt(CURLOPT_URL, url.c_str()) ||
            !setopt(CURLOPT_POSTFIELDS, data.c_str()) ||
            !setopt(CURLOPT_WRITEDATA, &body)) {
        return ResponseCode::UNDEFINED;
    }

    if (!m_curl) {
        std::cerr << "HttpPost::perform() m_curl == nullptr" << std::endl;
        return ResponseCode::UNDEFINED;
    }

    auto result = curl_easy_perform(m_curl);

    if (result != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(result) << std::endl;
        body.clear();
        return ResponseCode::UNDEFINED;
    }

    long responseCode = 0;
    result = curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
    if (result != CURLE_OK) {
        std::cerr << "curl_easy_getinfo(CURLINFO_RESPONSE_CODE) failed: " << curl_easy_strerror(result) << std::endl;
        body.clear();
        return ResponseCode::UNDEFINED;
    }

    if (static_cast<int>(ResponseCode::SUCCESS_OK) == responseCode) {
        std::cout << "HttpPost::perform() success (OK)" << std::endl;
        return ResponseCode::SUCCESS_OK;
    } else if (static_cast<int>(ResponseCode::CLIENT_ERROR_BAD_REQUEST) == responseCode) {
        std::cout << "HttpPost::perform() client error (BAD_REQUEST)" << std::endl;
        return ResponseCode::CLIENT_ERROR_BAD_REQUEST;
    }

    std::cerr << "curl_easy_getinfo(CURLINFO_RESPONSE_CODE) returned: " << responseCode << std::endl;
#ifdef DEBUG
    // TODO Integrate with Logging library: ACSDK-57
    // This may be private information so any logging of values must be at debug level
    if (!body.empty()) {
        std::cerr << "body: '" << body << "'" << std::endl;
    }
#endif

    body.clear();
    return ResponseCode::UNDEFINED;
}

template<typename ParamType>
bool HttpPost::setopt(CURLoption option, ParamType param) {
    if (!m_curl) {

#ifdef DEBUG
        // TODO Integrate with Logging library: ACSDK-57
        // This may be private information so any logging of values must be at debug level
        std::cerr << "!m_curl in HttpPost::setopt(option=" << option << ")" << std::endl;
#endif

        return false;
    }
    auto result = curl_easy_setopt(m_curl, option, param);
    if (result != CURLE_OK) {
        std::cerr << "curl_easy_setopt() failed: " << curl_easy_strerror(result) << std::endl;
        return false;
    }
    return true;
}

size_t HttpPost::staticWriteCallbackLocked(char* ptr, size_t size, size_t nmemb, void* userdata) {
    if (!userdata) {
        std::cerr << "staticWriteCallback() userdata == nullptr." << std::endl;
        return 0;
    }

    size_t count = size * nmemb;
    auto body = static_cast<std::string*>(userdata);
    body->append(ptr, count);
    return count;
}

} // namespace Auth
} // namespace AlexaClientSDK
