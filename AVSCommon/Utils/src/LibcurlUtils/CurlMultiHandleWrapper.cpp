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

#include <AVSCommon/Utils/LibcurlUtils/CurlMultiHandleWrapper.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/// String to identify log entries originating from this file.
#define TAG "CurlMultiHandleWrapper"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<CurlMultiHandleWrapper> CurlMultiHandleWrapper::create() {
    auto handle = curl_multi_init();
    if (!handle) {
        ACSDK_ERROR(LX("createFailed").d("reason", "curlMultiInitFailed"));
        return nullptr;
    }
    return std::unique_ptr<CurlMultiHandleWrapper>(new CurlMultiHandleWrapper(handle));
}

CurlMultiHandleWrapper::~CurlMultiHandleWrapper() {
    bool shouldCleanup = true;
    for (auto streamHandle : m_streamHandles) {
        auto result = curl_multi_remove_handle(m_handle, streamHandle);
        if (result != CURLM_OK) {
            ACSDK_ERROR(LX("curlMultiRemoveHandleFailed").d("error", curl_multi_strerror(result)));
            shouldCleanup = false;
        }
    }
    m_streamHandles.clear();
    if (shouldCleanup) {
        curl_multi_cleanup(m_handle);
    } else {
        // If removal of any stream handles failed, skip curl_multi_cleanup() because it will hang.
        ACSDK_ERROR(LX("multiHandleLeaked").d("reason", "curlMultiRemoveHandleFailed"));
    }
    m_handle = nullptr;
}

CURLM* CurlMultiHandleWrapper::getCurlHandle() {
    return m_handle;
}

CURLMcode CurlMultiHandleWrapper::addHandle(CURL* handle) {
    auto result = curl_multi_add_handle(m_handle, handle);
    if (CURLM_OK == result) {
        m_streamHandles.insert(handle);
    } else {
        ACSDK_ERROR(LX("curlMultiAddHandleFailed").d("error", curl_multi_strerror(result)));
    }
    return result;
}

CURLMcode CurlMultiHandleWrapper::removeHandle(CURL* handle) {
    auto result = curl_multi_remove_handle(m_handle, handle);
    if (CURLM_OK == result) {
        m_streamHandles.erase(handle);
    } else {
        ACSDK_ERROR(LX("curlMultiRemoveHandleFailed").d("error", curl_multi_strerror(result)));
    }
    return result;
}

CURLMcode CurlMultiHandleWrapper::perform(int* runningHandles) {
    auto result = curl_multi_perform(m_handle, runningHandles);
    if (result != CURLM_OK && result != CURLM_CALL_MULTI_PERFORM) {
        ACSDK_ERROR(LX("curlMultiPerformFailed").d("error", curl_multi_strerror(result)));
    }
    return result;
}

CURLMcode CurlMultiHandleWrapper::poll(std::chrono::milliseconds timeout, int* countHandlesUpdated) {
#if CURL_AT_LEAST_VERSION(7, 68, 0)
    auto result = curl_multi_poll(m_handle, nullptr, 0, timeout.count(), countHandlesUpdated);
#else
    auto result = curl_multi_wait(m_handle, nullptr, 0, static_cast<int>(timeout.count()), countHandlesUpdated);
#endif
    if (result != CURLM_OK) {
        ACSDK_ERROR(LX("curlMultiWaitFailed").d("error", curl_multi_strerror(result)));
    }
    return result;
}

bool CurlMultiHandleWrapper::wakeup() {
    bool returnVal = true;
#if CURL_AT_LEAST_VERSION(7, 68, 0)
    auto result = curl_multi_wakeup(m_handle);
    if (result != CURLM_OK) {
        ACSDK_ERROR(LX("curlMultiWakeupFailed").d("error", curl_multi_strerror(result)));
        returnVal = false;
    }
#else
    // no-op
#endif
    return returnVal;
}

CURLMsg* CurlMultiHandleWrapper::infoRead(int* messagesInQueue) {
    return curl_multi_info_read(m_handle, messagesInQueue);
}

CurlMultiHandleWrapper::CurlMultiHandleWrapper(CURLM* handle) : m_handle{handle} {
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
