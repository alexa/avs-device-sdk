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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2STREAM_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2STREAM_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/Utils/LibcurlUtils/CurlEasyHandleWrapper.h>
#include <AVSCommon/Utils/Logger/LogStringFormatter.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

#include "ACL/Transport/MimeParser.h"
#include "ACL/Transport/MessageConsumerInterface.h"

/// Whether or not curl logs should be emitted.
#ifdef ACSDK_EMIT_SENSITIVE_LOGS

#define ACSDK_EMIT_CURL_LOGS
#include <fstream>

#endif

namespace alexaClientSDK {
namespace acl {

// forward declaration
class HTTP2Transport;

/**
 * Class that represents an HTTP/2 stream.
 */
class HTTP2Stream {
public:
    /**
     * Constructor.
     *
     * @param messageConsumer The MessageConsumerInterface which should receive messages from AVS.
     * @param attachmentManager The attachment manager.
     */
    HTTP2Stream(
        std::shared_ptr<MessageConsumerInterface> messageConsumer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

    /**
     * Initializes streams that are supposed to POST the given request.
     * Note the authToken that is passed in will get used at a later time. Ensure that the token will not
     * expire soon.
     *
     * @param url The request URL
     * @param authToken The LWA token
     * @param request The MessageRequest to post
     * @returns true if setup was successful
     */
    bool initPost(
        const std::string& url,
        const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> request);

    /**
     * Initializes streams that are supposed to perform an HTTP GET
     * Note the authToken that is passed in will get used at a later time. Ensure that the token will not
     * expire soon.
     *
     * @param url The request URL
     * @param authToken The LWA token
     * @returns Whether the setup was successful or not
     */
    bool initGet(const std::string& url, const std::string& authToken);

    /**
     * Sets up a stream for re-use
     *
     * @return Whether the reset was successful or not
     */
    bool reset();

    /**
     * Gets the CURL easy handle associated with this stream
     *
     * @returns The associated curl easy handle for the stream
     */
    CURL* getCurlHandle();

    /**
     * Returns the HTTP response code to the action this stream performs.
     *
     * @returns The HTTP response code if one has been received, 0 if not, and < 0 if there is an error
     */
    long getResponseCode();

    /**
     * Notify the current request observer that the transfer is complete with
     * the appropriate SendCompleteStatus code.
     */
    void notifyRequestObserver();

    /**
     * Notify the current request observer that the transfer is complete with
     * the specified status code.
     *
     * @param status The completion status.
     */
    void notifyRequestObserver(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status);

    /**
     * Callback that gets executed when data is received from the server
     * See CurlEasyHandleWrapper::CurlCallback for details
     */
    static size_t writeCallback(char* data, size_t size, size_t nmemb, void* userData);

    /**
     * Callback that gets executed when HTTP headers are received from the server
     * See CurlEasyHandleWrapper::CurlCallback for details
     */
    static size_t headerCallback(char* data, size_t size, size_t nmemb, void* userData);

    /**
     * Callback that gets executed when the server requires data.
     * See CurlEasyHandleWrapper::curlCallback for details.
     *
     * @param data A pointer to the data buffer to either read or write to.
     * @param size The size of a "block" of data (ala fwrite).
     * @param nmemb The number of "blocks" to read or write.
     * @param userData Some user data passed in with CURLOPT_READDATA.
     * @return The amount of bytes read.
     */
    static size_t readCallback(char* data, size_t size, size_t nmemb, void* userData);

    /**
     * Sets the max amount of time in seconds the stream can take. If not set explicitly there is no timeout.
     *
     * @param timeoutSeconds The timeout in seconds, set to @c 0 to disable an existing timeout.
     * @return Whether the addition was successful
     */
    bool setStreamTimeout(const long timeoutSeconds);

    /**
     * Sets how long, in seconds, the stream should take to establish a connection. If not explicitly set there is no
     * timeout.
     *
     * @param timeoutSeconds The connection timeout, in seconds. Set to @c 0 to disable an existing timeout.
     * @returns Whether setting the timeout was successful.
     */
    bool setConnectionTimeout(const std::chrono::seconds timeoutSeconds);

    /**
     * Un-pend all transfers for this stream
     */
    void unPause();

    /**
     * Return whether this stream has pending transfers.
     */
    bool isPaused() const;

    /**
     * Set the logical stream ID for this stream.
     *
     * @param logicalStreamId The new value for this streams logical stream ID.
     */
    void setLogicalStreamId(int logicalStreamId);

    /**
     * Get the logical ID of this stream.
     *
     * @return The logical ID of this stream.
     */
    unsigned int getLogicalStreamId() const;

    /**
     * Set the timeout for this stream to make progress sending or receiving.
     *
     * @tparam TickType Type to represent tick count.
     * @tparam TickPeriod @c std::ratio specifying ticks per second.
     * @param duration Max time the stream may make no progress before @c getHasProgressTimedOut() returns true.
     */
    template <class TickType, class TickPeriod = std::ratio<1>>
    void setProgressTimeout(std::chrono::duration<TickType, TickPeriod> duration);

    /**
     * Return whether or not the progress timeout has been reached.
     *
     * @return Whether or not the progress timeout has been reached.
     */
    bool hasProgressTimedOut() const;

    /**
     * Return a reference to the LogStringFormatter owned by this object.  This is to allow a callback that uses this
     * object to get access to a known good LogStringFormatter.
     *
     * @return A reference to a LogStringFormatter.
     */
    const avsCommon::utils::logger::LogStringFormatter& getLogFormatter() const;

private:
    /**
     * Configure the associated curl easy handle with options common to GET and POST
     *
     * @return Whether the setting was successful or not
     */
    bool setCommonOptions(const std::string& url, const std::string& authToken);

    /**
     * Helper function for calling @c curl_easy_setopt and checking the result.
     *
     * @param option The option parameter to pass through to curl_easy_setopt.
     * @param optionName The name of the option to be set (for logging)
     * @param param The param option to pass through to curl_easy_setopt.
     * @return @c true of the operation was successful.
     */
    template <typename ParamType>
    bool setopt(CURLoption option, const char* optionName, ParamType param);

    /**
     * Initialize capturing this streams activities in a log file.
     */
    void initStreamLog();

#ifdef ACSDK_EMIT_CURL_LOGS

    /**
     * Callback that is invoked when @c libcurl has debug information to provide.
     *
     * @param handle @c libcurl handle of the transfer being reported upon.
     * @param type The type of data being reported.
     * @param data Pointer to the data being provided.
     * @param size Size (in bytes) of the data being reported.
     * @param user User pointer used to pass which HTTP2Stream is being reported on.
     * @return Always returns zero.
     */
    static int debugFunction(CURL* handle, curl_infotype type, char* data, size_t size, void* user);

    /// File to log the stream I/O to
    std::unique_ptr<std::ofstream> m_streamLog;
    /// File to dump data streamed in
    std::unique_ptr<std::ofstream> m_streamInDump;
    /// File to dump data streamed out
    std::unique_ptr<std::ofstream> m_streamOutDump;

#endif  // ACSDK_EMIT_CURL_LOGS

    /**
     * The logical id for this particular object instance.  (see note for @c m_streamIdCounter in this class).
     * @note This is NOT the actual HTTP/2 stream id.  Instead, this is an id which this class generates which is
     * guaranteed to be different from the id of other objects of this class.  Also, we make an attempt to emulate
     * a real HTTP/2 stream id, by starting at '1' and incrementing by two for each new stream.
     */
    unsigned int m_logicalStreamId;
    /// The underlying curl easy handle.
    avsCommon::utils::libcurlUtils::CurlEasyHandleWrapper m_transfer;
    /// An DirectiveParser instance used to parse multipart MIME messages.
    MimeParser m_parser;
    /// The current request being sent on this HTTP/2 stream.
    std::shared_ptr<avsCommon::avs::MessageRequest> m_currentRequest;
    /// Whether this stream has any paused transfers.
    bool m_isPaused;
    /**
     * The exception message being received from AVS by this stream.  It may be built up over several calls if either
     * the write quanta are small, or if the message is long.
     */
    std::string m_exceptionBeingProcessed;
    /// Max time the stream may make no progress before @c hasProgressTimedOut() returns true.
    std::atomic<std::chrono::steady_clock::rep> m_progressTimeout;
    /// Last time something was transferred.
    std::atomic<std::chrono::steady_clock::rep> m_timeOfLastTransfer;
    /// Object to format log strings correctly.
    avsCommon::utils::logger::LogStringFormatter m_logFormatter;
};

template <class TickType, class TickPeriod>
void HTTP2Stream::setProgressTimeout(std::chrono::duration<TickType, TickPeriod> duration) {
    m_progressTimeout = std::chrono::duration_cast<std::chrono::steady_clock::duration>(duration).count();
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2STREAM_H_
