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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CURLEASYHANDLEWRAPPER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CURLEASYHANDLEWRAPPER_H_

#include <chrono>
#include <curl/curl.h>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * Class to allocate and configure a curl easy handle
 */
class CurlEasyHandleWrapper {
public:
    /**
     * Callbacks to libcurl typically follow the below pattern.
     * size_t callback(char* data, size_t size, size_t nmemb, void* user)
     * @param buffer A pointer to the data buffer to either read or write to
     * @param blockSize The size of a "block" of data (ala fwrite)
     * @param numBlocks The number of "blocks" to read or write
     * @param userData Some user data passed in with CURLOPT_XDATA (where X = READ, WRITE, or HEADER)
     */
    typedef size_t (*CurlCallback)(char* buffer, size_t blockSize, size_t numBlocks, void* userData);

    /**
     * Definitions for HTTP action types
     */
    enum class TransferType {
        /// HTTP GET
        kGET,
        /// HTTP POST
        kPOST
    };

    /**
     * Default constructor
     */
    CurlEasyHandleWrapper();

    /**
     * Default destructor
     */
    ~CurlEasyHandleWrapper();

    /**
     * Resets an allocated easy handle for re-use in another transfer
     * Calls curl_easy_reset on the curl easy handle
     * Frees and sets the following members to @c nullptr:
     * <ul>
     * <li>HTTP headers</li>
     * <li>POST headers</li>
     * <li>CURL post form</li>
     * </ul>
     * @return Whether the reset was successful
     */
    bool reset();

    /**
     * Used to get the underlying CURL easy handle. The handle returned
     * may be a nullptr
     *
     * @return The associated libcurl easy handle
     */
    CURL* getCurlHandle();

    /*
     * Adds an HTTP Header to the current easy handle
     *
     * @param header The HTTP header to add
     * @returns true if the addition was successful
     */
    bool addHTTPHeader(const std::string& header);

    /*
     * Adds a POST Header to the list of the headers to add to the future POST request
     *
     * @param header The HTTP header to add
     * @returns true if the addition was successful
     */
    bool addPostHeader(const std::string& header);

    /**
     * Set the request URL. No validation is done at this stage
     *
     * @param url The required URL
     * @returns true if setting was successful
     */
    bool setURL(const std::string& url);

    /**
     * Sets the HTTP action to perform.
     *
     * @param type The action to perform
     * @returns true if the setting was successful
     */
    bool setTransferType(TransferType type);

    /**
     * Adds a POST field to the current multipart form named @c fieldName with a string
     * value contained in payload
     *
     * @param fieldName The POST field name
     * @param payload The string to send
     * @return Whether the addition was successful
     */
    bool setPostContent(const std::string& fieldName, const std::string& payload);

    /**
     * Sets a timeout, in seconds, for how long the stream transfer is allowed to take.
     * If not set explicitly, there will be no timeout.
     *
     * @param timeoutSeconds The amount of time in seconds the stream can take. Set to @c 0 to disable a timeout.
     * @returns Whether setting the timeout was succesful
     */
    bool setTransferTimeout(const long timeoutSeconds);

    /**
     * Adds a POST field to the current multipart form named @c fieldName with a chunked
     * transfer encoded data stream. The readCallback set in setReadCallback will be called
     * when data is required.
     *
     * @param fieldName The POST field name.
     * @param userData User data passed into the read callback.
     * @return Whether the addition was successful.
     */
    bool setPostStream(const std::string& fieldName, void* userData);

    /**
     * Sets how long the stream should take, in seconds, to establish a connection.
     * If not set explicitly there is no timeout.
     *
     * @param timeoutSeconds The amount of time, in seconds, establishing a connection should take. Set to @c 0 to
     * disable a timeout.
     * @returns Whether setting the timeout was successful
     */
    bool setConnectionTimeout(const std::chrono::seconds timeoutSeconds);

    /**
     * Sets the callback to call when libcurl has response data to consume
     *
     * @param callback A function pointer to the callback
     * @param userData Any data to be passed to the callback
     * @return Whether the addition was successful
     */

    bool setWriteCallback(CurlCallback callback, void* userData);
    /**
     * Sets the callback to call when libcurl has HTTP header data available
     * NOTE: Each header line is provided individually
     *
     * @param callback A function pointer to the callback
     * @param userData Any data to be passed to the callback
     * @return Whether the addition was successful
     */

    bool setHeaderCallback(CurlCallback callback, void* userData);
    /**
     * Sets the callback to call when libcurl requires data to POST
     *
     * @param callback A function pointer to the callback
     * @param userData Any data to be passed to the callback
     * @return Whether the addition was successful
     */
    bool setReadCallback(CurlCallback callback, void* userData);

private:
    /**
     * Frees and sets the following attributes to NULL:
     * <ul>
     * <li>m_requestHeaders</li>
     * <li>m_postHeaders</li>
     * <li>m_post</li>
     * </ul>
     */
    void cleanupResources();

    /**
     * Sets common options to the curl easy handle common to all transfers
     *
     * @return Whether the setting was successful
     */
    bool setDefaultOptions();

    /// The associated libcurl easy handle
    CURL* m_handle;
    /// A list of headers needed to be added at the HTTP level
    curl_slist* m_requestHeaders;
    /// A list of headers needed to be added to a POST action
    curl_slist* m_postHeaders;
    /// The associated multipart post
    curl_httppost* m_post;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CURLEASYHANDLEWRAPPER_H_
