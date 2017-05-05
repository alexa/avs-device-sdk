/*
 * CurlEasyHandleWrapper.cpp
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

#include <AVSUtils/LibcurlUtils/LibcurlUtils.h>
#include <AVSUtils/Logging/Logger.h>

#include "ACL/Transport/CurlEasyHandleWrapper.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsUtils;

/// MIME Content-Type for JSON data
static std::string JSON_MIME_TYPE = "text/json";
/// MIME Content-Type for octet stream data
static std::string OCTET_MIME_TYPE = "application/octet-stream";
/// Response code for HTTP 204 (Success No Content response)
static const long HTTP_RESPONSE_SUCCESS_NO_CONTENT = 204;

CurlEasyHandleWrapper::CurlEasyHandleWrapper()
        : m_handle{curl_easy_init()},
          m_requestHeaders{nullptr},
          m_postHeaders{nullptr},
          m_post{nullptr} {
    setDefaultOptions();
};

CurlEasyHandleWrapper::~CurlEasyHandleWrapper() {
    cleanupResources();
    curl_easy_cleanup(m_handle);
};

bool CurlEasyHandleWrapper::reset() {
    cleanupResources();
    long responseCode = 0;
    if (curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &responseCode) != CURLE_OK) {
        Logger::log("could not get transfer response code");
        curl_easy_cleanup(m_handle);
        m_handle = curl_easy_init();
        //If we can't create a new handle don't try to set options
        if (!m_handle) {
            Logger::log("could not create new curl handle");
            return false;
        }
        setDefaultOptions();
        return false;
    }

    /*
     * It appears that re-using a curl easy handle after receiving an HTTP 204 (Success No Content)
     * causes the next transfer to timeout. As a workaround just cleanup the handle and create a new one
     * if we receive a 204.
     *
     * TODO: ACSDK-104 Find a way to re-use all handles, or re-evaluate the easy handle pooling scheme
     */
    if (HTTP_RESPONSE_SUCCESS_NO_CONTENT == responseCode) {
        curl_easy_cleanup(m_handle);
        m_handle = curl_easy_init();
        if (!m_handle) {
            Logger::log("could not create new curl handle");
            return false;
        }
    } else {
        curl_easy_reset(m_handle);
    }
    return setDefaultOptions();
}

CURL* CurlEasyHandleWrapper::getCurlHandle() {
    return m_handle;
}

bool CurlEasyHandleWrapper::addHTTPHeader(const std::string& header) {
    m_requestHeaders = curl_slist_append(m_requestHeaders, header.c_str());
    if (!m_requestHeaders) {
        Logger::log("Could not add header to HTTPheaders");
        // TODO: log headers in debug mode
        return false;
    }
    if (curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_requestHeaders) != CURLE_OK) {
        Logger::log("Could not add HTTPheaders to easy handle");
        // TODO: log headers in debug mode
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::addPostHeader(const std::string& header) {
    m_postHeaders = curl_slist_append(m_postHeaders, header.c_str());
    if (!m_requestHeaders) {
        Logger::log("Could not add header to POST headers");
        // TODO: log headers in debug mode
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setURL(const std::string& url) {
    if (curl_easy_setopt(m_handle, CURLOPT_URL, url.c_str()) != CURLE_OK) {
        Logger::log("Cannot set URL");
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setTransferType(TransferType type) {
    switch (type) {
        case TransferType::kGET:
            if (curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1L) != CURLE_OK) {
                Logger::log("Cannot set transfer to GET");
                return false;
            }
            break;
        case TransferType::kPOST:
            if (!m_post || curl_easy_setopt(m_handle, CURLOPT_HTTPPOST, m_post) != CURLE_OK) {
                Logger::log("Cannot set transfer to POST");
                return false;
            }
            break;
    }
    return true;
}

bool CurlEasyHandleWrapper::setPostContent(const std::string& fieldName, const std::string& payload) {
    curl_httppost *last = nullptr;
    CURLFORMcode ret = curl_formadd(&m_post, &last, CURLFORM_COPYNAME, fieldName.c_str(), CURLFORM_COPYCONTENTS, payload.c_str(),
                CURLFORM_CONTENTTYPE, JSON_MIME_TYPE.c_str(), CURLFORM_CONTENTHEADER, m_postHeaders, CURLFORM_END);
    if (ret) {
        Logger::log("Could not set string post content: " + fieldName);
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setTransferTimeout(const long timeoutSeconds) {
    CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, timeoutSeconds);
    if (ret != CURLE_OK) {
        Logger::log("Could not set transfer timeout returned: " + std::string(curl_easy_strerror(ret)));
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setPostStream(const std::string& fieldName, void *userData) {
    curl_httppost *last = m_post;
    CURLFORMcode ret = curl_formadd(&m_post, &last, CURLFORM_COPYNAME, fieldName.c_str(), CURLFORM_STREAM, userData,
                CURLFORM_CONTENTTYPE, OCTET_MIME_TYPE.c_str(), CURLFORM_END);
    if (ret) {
        Logger::log("Could not set string post content: " + fieldName +", errror code: " +std::to_string(ret));
        return false;
    }
    return true;
}

bool CurlEasyHandleWrapper::setConnectionTimeout(const std::chrono::seconds timeoutSeconds) {
    CURLcode ret = curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, timeoutSeconds.count());
    if (ret != CURLE_OK) {
        Logger::log("Could not set connection timeout returned: " + std::string(curl_easy_strerror(ret)));
        return false;
    }

    return true;
}

bool CurlEasyHandleWrapper::setWriteCallback(CurlCallback callback, void* userData) {
    if (curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, callback) != CURLE_OK) {
        Logger::log("Cannot set write callback");
        return false;
    }
    if (userData) {
        if (curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, userData) != CURLE_OK) {
            Logger::log("Cannot set user data for write callback");
            return false;
        }
    }
    return true;
}

bool CurlEasyHandleWrapper::setHeaderCallback(CurlCallback callback, void* userData) {
    if (curl_easy_setopt(m_handle, CURLOPT_HEADERFUNCTION, callback) != CURLE_OK) {
        Logger::log("Cannot set header callback");
        return false;
    }
    if (userData) {
        if (curl_easy_setopt(m_handle, CURLOPT_HEADERDATA, userData) != CURLE_OK) {
            Logger::log("Cannot set user data for header callback");
            return false;
        }
    }
    return true;
}

bool CurlEasyHandleWrapper::setReadCallback(CurlCallback callback, void* userData) {
    if (curl_easy_setopt(m_handle, CURLOPT_READFUNCTION, callback) != CURLE_OK) {
        Logger::log("Cannot set read callback");
        return false;
    }
    if (userData) {
        if (curl_easy_setopt(m_handle, CURLOPT_READDATA, userData) != CURLE_OK) {
            Logger::log("Cannot set user data for read callback");
            return false;
        }
    }
    return true;
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
    return libcurlUtils::prepareForTLS(m_handle);
}

} // acl
} // alexaClientSDK
