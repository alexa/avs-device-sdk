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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACE_H_

#include <memory>

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/HTTPContent.h>
#include <AVSCommon/Utils/SDKVersion.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows users to fetch content from remote location using HTTP(S).
 */
class HTTPContentFetcherInterface {
public:
    /// Represents what HTTP content to fetch.
    enum class FetchOptions {
        /// Retrieves the content type part of the HTTP header.
        CONTENT_TYPE,
        /// Retrieves the entire body of the remote location.
        ENTIRE_BODY
    };

    /// The state of payload fetching
    enum class State {
        /// Initialized but nothing was downloaded yet.
        INITIALIZED,
        /// Currently fetching the header
        FETCHING_HEADER,
        /// Done fetching the header. Ready to start fetching the body.
        HEADER_DONE,
        /// Currently fetching the body.
        FETCHING_BODY,
        /// Done fetching the body. At this point the content fetcher can only be used to read the headers.
        BODY_DONE,
        /// Some error happened at any stage and the content fetcher cannot be used anymore.
        ERROR
    };

    /**
     * A struct that represents the header that was retrieved from the HTTP connection. Objects that receive this
     * struct are responsible for checking if the successful field is true before reading the other fields.
     */
    struct Header {
        /// If @c false, there was an error retrieving the header. For instance, the content fetcher may have reached
        /// a timeout waiting for the server. If this field's value is @c false, all other field values should be
        /// ignored.
        bool successful;
        /// The HTTP status code received.
        avsCommon::utils::http::HTTPResponseCode responseCode;
        /// The value of the Content-Type HTTP header.
        std::string contentType;
        /// The value of the Content-Length HTTP header.
        ssize_t contentLength;

        Header() :
                successful(false),
                responseCode(avsCommon::utils::http::HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED),
                contentType(""),
                contentLength(0) {
        }
    };

    /**
     * Destructor.
     */
    virtual ~HTTPContentFetcherInterface() = default;

    /**
     * The current content fetching state. In particular, a caller of @c getBody, an asynchronous function, can use this
     * method to monitor the download progress.
     *
     * @return The current content fetching state.
     */
    virtual State getState() = 0;

    /**
     * Gets the URL associated with this content fetcher
     *
     * @return The content fetcher URL
     */
    virtual std::string getUrl() const = 0;

    /**
     * Waits until the header was fetched successfully. If any problem happened during header, returns @c false. After
     * the header was already fetched, this method can be called multiple times and will return immediately.
     *
     * @param shouldShutdown A pointer to allow for the caller to asynchronously cancel the wait, if needed. this
     * argument is optional. If it is a null pointer, the function will ignore it.
     * @return The header struct. It is the caller's responsibility to check the struct to see if the header was
     * retrieved successfully.
     */
    virtual Header getHeader(std::atomic<bool>* shouldShutdown) = 0;

    /**
     * Retrieves the body after the header was received. This method is asynchronous and the caller can monitor the
     * download progress using the @c getState method.
     *
     * @param writer The writer to write the payload.
     * @return @c true if the call is successful.
     */
    virtual bool getBody(std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer) = 0;

    /**
     * Shuts down the content fetcher.
     */
    virtual void shutdown() = 0;

    /**
     * This function retrieves content from a remote location. No thread safety is guaranteed.
     *
     * @param option Flag indicating desired content.
     * @param writer An optional writer parameter to be used when writing to an external stream.
     * @param customHeaders An optional list of headers to be attached to the request.
     * @return A new @c HTTPContent object or @c nullptr if a failure occurred.
     */
    virtual std::unique_ptr<avsCommon::utils::HTTPContent> getContent(
        FetchOptions option,
        std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> writer = nullptr,
        const std::vector<std::string>& customHeaders = std::vector<std::string>()) = 0;

    /**
     * Returns a string that represents the User-Agent to be used in HTTP requests.
     *
     * @return User-Agent string to be used in HTTP requests.
     */
    static std::string getUserAgent();

    /**
     * Produces the string representation of the state enum values.
     * @param state The state enum value.
     * @return Its string representation.
     */
    static std::string stateToString(State state);
};

inline std::string HTTPContentFetcherInterface::getUserAgent() {
    return "AvsDeviceSdk/" + utils::sdkVersion::getCurrentVersion();
}

inline std::string HTTPContentFetcherInterface::stateToString(HTTPContentFetcherInterface::State state) {
    switch (state) {
        case State::INITIALIZED:
            return "INITIALIZED";
        case State::FETCHING_HEADER:
            return "FETCHING_HEADER";
        case State::HEADER_DONE:
            return "HEADER_DONE";
        case State::FETCHING_BODY:
            return "FETCHING_BODY";
        case State::BODY_DONE:
            return "BODY_DONE";
        case State::ERROR:
            return "ERROR";
    }
    return "";
}

/**
 * Overwrites the << operator for @c HTTPContentFetcherInterface::State.
 *
 * @param os The output stream pointer.
 * @param code The HTTPContentFetcherInterface::State to write to the output stream.
 * @return The output stream pointer.
 */
inline std::ostream& operator<<(std::ostream& os, const HTTPContentFetcherInterface::State& state) {
    os << HTTPContentFetcherInterface::stateToString(state);
    return os;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACE_H_
