/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTPCONTENTFETCHER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTPCONTENTFETCHER_H_

#include <atomic>
#include <future>
#include <string>
#include <thread>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * A class used to retrieve content from remote URLs. Note that this object will only write to the Attachment while it
 * remains alive. If the object goes out of scope, writing to the Attachment will abort.
 */
class LibCurlHttpContentFetcher : public avsCommon::sdkInterfaces::HTTPContentFetcherInterface {
public:
    LibCurlHttpContentFetcher(const std::string& url);

    /// @name HTTPContentFetcherInterface methods
    /// @{
    State getState() override;
    std::string getUrl() const override;
    Header getHeader(std::atomic<bool>* shouldShutdown) override;
    bool getBody(std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) override;
    void shutdown() override;
    /// @}

    /**
     * @copydoc
     * In this implementation, the function may only be called once. Subsequent calls will return @c nullptr.
     */
    std::unique_ptr<avsCommon::utils::HTTPContent> getContent(
        FetchOptions option,
        std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> writer = nullptr,
        const std::vector<std::string>& customHeaders = std::vector<std::string>()) override;

    /*
     * Destructor.
     */
    ~LibCurlHttpContentFetcher() override;

private:
    /// The callback to parse HTTP headers.
    static size_t headerCallback(char* data, size_t size, size_t nmemb, void* userData);

    /// The callback to parse HTTP bodies.
    static size_t bodyCallback(char* data, size_t size, size_t nmemb, void* userData);

    /// A no-op callback to not parse HTTP bodies.
    static size_t noopCallback(char* data, size_t size, size_t nmemb, void* userData);

    /// The content fetching state
    State m_state;

    /**
     * Helper method to get custom HTTP headers list.
     *
     * @param customHeaders Custom HTTP headers to add.
     * @return @c curl_slist of custom headers if customHeaders are not empty, otherwise NULL.
     */
    curl_slist* getCustomHeaderList(std::vector<std::string> customHeaders);

    /// The URL to fetch from.
    const std::string m_url;

    /// A libcurl wrapper.
    CurlEasyHandleWrapper m_curlWrapper;

    /// A promise for header loading
    std::promise<bool> m_headerPromise;

    /// A future for header loading
    std::shared_future<bool> m_headerFuture;

    /// The fetched header
    HTTPContentFetcherInterface::Header m_header;

    /**
     * The writer used to write the HTTP body to, if desired by the caller of @c getContent().
     */
    std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> m_streamWriter;

    /// Number of bytes that has been received in the ongoing request.
    ssize_t m_currentContentReceivedLength;

    /// Number of bytes that has been received since the first request.
    ssize_t m_totalContentReceivedLength;

    /// Flag to indicate that the data-fetch operation has completed.
    std::atomic<bool> m_done;

    /// Flag to indicate that the @c LibCurlHttpContentFetcher is being shutdown.
    std::atomic<bool> m_isShutdown;

    /**
     * Internal thread that does the curl_easy_perform. The reason for using a thread is that curl_easy_perform may
     * block forever if the URL specified is a live stream.
     */
    std::thread m_thread;

    /// Flag to indicate that a call to @c getContent() has been made. Subsequent calls will not be accepted.
    std::atomic_flag m_hasObjectBeenUsed;

    /// A mutex to ensure that all state transitions on the m_state variable are thead-safe.
    std::mutex m_stateMutex;

    /**
     * A mutex to ensure that concurrent calls to getBody are thread-safe. The @c getBody() function should be called
     * only once in order to ensure that the content fetcher receives a single attachment writer at the right state.
     */
    std::mutex m_getBodyMutex;

    /**
     * Writes an error in the logs warning us that an attempt to perform an invalid state transition was made.
     *
     * @param currentState The current state.
     * @param newState The state to which the transition attempt was made.
     */
    void reportInvalidStateTransitionAttempt(State currentState, State newState);

    /**
     * Performs the state transition, guaranteeing it to be atomic in regard with the header and body future and the
     * header successful field. If the transition is invalid, refrains from performing the transition and logs an error
     * message.
     *
     * @param newState The new state.
     * @param value The value of the the futures and the header successful field.
     */
    void stateTransition(State newState, bool value);

    /**
     * Checks if the content fetcher is still waiting for the @c getBody method to be called.
     * @return @c true if it is still waiting for the method call.
     */
    bool waitingForBodyRequest();
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLHTTPCONTENTFETCHER_H_
