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

#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>

#include <cstdlib>
#include <cstring>
#include <vector>

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/LibcurlUtils/CallbackData.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

using namespace alexaClientSDK::avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("HttpPut");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Read callback function used for CURLOPT_READFUNCTION option in libcurl
 */
static size_t readCallback(char* dataBuffer, size_t blockSize, size_t numBlocks, void* dataStream);

/**
 * Write callback function used for CURLOPT_WRITEFUNCTION option in libcurl
 */
static size_t writeCallback(char* dataBuffer, size_t blockSize, size_t numBlocks, void* dataStream);

std::unique_ptr<HttpPut> HttpPut::create() {
    std::unique_ptr<HttpPut> httpPut(new HttpPut());
    if (httpPut->m_curl.isValid()) {
        return httpPut;
    }
    return nullptr;
}

HTTPResponse HttpPut::doPut(const std::string& url, const std::vector<std::string>& headers, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::string errorEvent = "doPutFailed";
    const std::string errorReasonKey = "reason";
    HTTPResponse httpResponse;

    if (!m_curl.reset()) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToResetCurlHandle"));
        return httpResponse;
    }

    if (!m_curl.setURL(url)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetUrl"));
        return httpResponse;
    }

    if (!m_curl.setTransferType(CurlEasyHandleWrapper::TransferType::kPUT)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetHttpRequestType"));
        return httpResponse;
    }

    if (!m_curl.setopt(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(data.length()))) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetDataLength"));
        return httpResponse;
    }

    for (auto header : headers) {
        if (!m_curl.addHTTPHeader(header)) {
            ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToAddHttpHeader"));
            return httpResponse;
        }
    }

    CallbackData requestData(data.c_str());
    if (!m_curl.setReadCallback(readCallback, &requestData)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetReadCallback"));
        return httpResponse;
    }

    CallbackData responseData;
    if (!m_curl.setWriteCallback(writeCallback, &responseData)) {
        ACSDK_ERROR(LX(errorEvent).d(errorReasonKey, "unableToSetWriteCallback"));
        return httpResponse;
    }

    CURLcode curlResult = m_curl.perform();
    if (curlResult != CURLE_OK) {
        ACSDK_ERROR(
            LX(errorEvent).d(errorReasonKey, "curlFailedWithCode: " + std::string(curl_easy_strerror(curlResult))));
        return httpResponse;
    }

    size_t responseSize = responseData.getSize();
    if (responseSize > 0) {
        std::vector<char> responseBody(responseSize + 1, 0);
        responseData.getData(responseBody.data(), responseSize);
        httpResponse.body = std::string(responseBody.data());
    } else {
        httpResponse.body = "";
    }
    httpResponse.code = m_curl.getHTTPResponseCode();

    return httpResponse;
}

size_t readCallback(char* dataBuffer, size_t blockSize, size_t numBlocks, void* dataStream) {
    if (!dataStream) {
        ACSDK_ERROR(LX("readCallbackFailed").d("reason", "nullDataStream"));
        return 0;
    }

    CallbackData* callbackData = reinterpret_cast<CallbackData*>(dataStream);
    if (callbackData->getSize() == 0) {
        return 0;
    }
    size_t readSize = callbackData->getData(dataBuffer, blockSize * numBlocks);
    callbackData->clearData();
    return readSize;
}

size_t writeCallback(char* dataBuffer, size_t blockSize, size_t numBlocks, void* dataStream) {
    if (!dataStream) {
        ACSDK_ERROR(LX("writeCallbackFailed").d("reason", "nullDataStream"));
        return 0;
    }

    size_t realSize = blockSize * numBlocks;
    CallbackData* callbackData = reinterpret_cast<CallbackData*>(dataStream);

    return callbackData->appendData(dataBuffer, realSize);
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
