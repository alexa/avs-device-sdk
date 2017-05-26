/*
 * HTTP2Stream.h
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

#ifndef ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_STREAM_H_
#define ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_STREAM_H_

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/MessageRequest.h>

#include "ACL/Transport/CurlEasyHandleWrapper.h"
#include "ACL/Transport/MimeParser.h"
#include "ACL/Transport/MessageConsumerInterface.h"

namespace alexaClientSDK {
namespace acl {

// forward declaration
class HTTP2Transport;

/**
 * Class that represents an HTTP/2 stream.
 */
class HTTP2Stream {
public:
    enum HTTPResponseCodes{
        /// No HTTP response received.
        NO_RESPONSE_RECEIVED = 0,
        /// HTTP Success with reponse payload.
        SUCCESS_OK = 200,
        /// HTTP Succcess with no response payload.
        SUCCESS_NO_CONTENT = 204
    };

    /**
     * Constructor.
     *
     * @param messageConsumer The MessageConsumerInterface which should receive messages from AVS.
     * @param attachmentManager The attachment manager.
     */
    HTTP2Stream(MessageConsumerInterface *messageConsumer,
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
    bool initPost(const std::string& url, const std::string& authToken,
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
     * the appropriate SendCompleteStatus code
     */
    void notifyRequestObserver();

    /**
     * Callback that gets executed when data is received from the server
     * See CurlEasyHandleWrapper::CurlCallback for details
     */
    static size_t writeCallback(char *data, size_t size, size_t nmemb, void *userData);

    /**
     * Callback that gets executed when HTTP headers are received from the server
     * See CurlEasyHandleWrapper::CurlCallback for details
     */
    static size_t headerCallback(char *data, size_t size, size_t nmemb, void *userData);

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
    static size_t readCallback(char *data, size_t size, size_t nmemb, void *userData);

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
     * Update the paused status of the stream.  This will take actual effect at the next point of usage with respect
     * to the underlying network library implementation.
     *
     * @param isPaused The new value to set the paused state.
     */
    void setPaused(bool isPaused);

    /**
     * Queries whether this stream is paused.
     *
     * @return Whether the stream is paused or not.
     */
    bool isPaused() const;

private:
    /**
     * Configure the associated curl easy handle with options common to GET and POST
     *
     * @return Whether the setting was successful or not
     */
    bool setCommonOptions(const std::string& url, const std::string& authToken);

    /**
     * A static counter to ensure each newly created stream has a different id.  The notion of a streamId is needed
     * to provide a per-HTTP/2-stream context for any given attachment received from AVS.  AVS can only guarantee that
     * the identifying 'contentId' for an attachment is unique within a HTTP/2 stream, hence this variable.
     */
    static unsigned int m_streamIdCounter;
    /**
     * The logical id for this particular object instance.  (see note for @c m_streamIdCounter in this class).
     * @note This is NOT the actual HTTP/2 stream id.  Instead, this is an id which this class generates which is
     * guaranteed to be different from the id of other objects of this class.  Also, we make an attempt to emulate
     * a real HTTP/2 stream id, by starting at '1' and incrementing by two for each new stream.
     */
    unsigned int m_logicalStreamId;
    /// The underlying curl easy handle.
    CurlEasyHandleWrapper m_transfer;
    /// An DirectiveParser instance used to parse multipart MIME messages.
    MimeParser m_parser;
    /// The current request being sent on this HTTP/2 stream.
    std::shared_ptr<avsCommon::avs::MessageRequest> m_currentRequest;
    /// A local variable to track if this stream is paused with respect to libcurl management.
    bool m_isPaused;
    /**
     * The exception message being received from AVS by this stream.  It may be built up over several calls if either
     * the write quantums are small, or if the message is long.
     */
    std::string m_exceptionBeingProcessed;
};

} // acl
} // alexaClientSDK

#endif // ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2_STREAM_H_
