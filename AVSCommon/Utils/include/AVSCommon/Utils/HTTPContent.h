/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTPCONTENT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTPCONTENT_H_

#include <chrono>
#include <future>
#include <memory>
#include <string>

#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * This class encapsulates content received from HTTP request, specifically the status code, the content-type, and the
 * actual content of the response.
 */
class HTTPContent {
public:
    /**
     * Constructor.
     *
     * @param httpStatusCode The future for the HTTP status code.
     * @param httpContentType The future for the HTTP content type.
     * @param stream The Attachment from which to read the HTTP content from or @c nullptr if no data was fetched.
     */
    HTTPContent(
        std::future<long> httpStatusCode,
        std::future<std::string> httpContentType,
        std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> stream);

    /**
     * This function checks if the status code is HTTP success or not.  This function blocks until status code is
     * set.
     *
     * @return @c true if status code is 2xx, else @c false.
     */
    bool isStatusCodeSuccess();

    /**
     * This function returns the status code.  This function blocks until status code is set.
     *
     * @return The value of the status code.
     */
    long getStatusCode();

    /**
     * This function checks if the status code is ready to be read.
     *
     * @param timeout The timeout to wait for to see if status code is ready.
     * @return @c true if status code is ready, else @c false.
     */
    bool isReady(const std::chrono::milliseconds timeout) const;

    /**
     * This function returns the content type.  This function blocks until content type is set.
     *
     * @return The value of content type.
     */
    std::string getContentType();

    /**
     * This function returns the attachment from which to read the HTTP content.
     *
     * @return An @c InProcessAttachment or @c nullptr if no data was fetched.
     */
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> getDataStream();

private:
    /// A @c long representing the HTTP status code.
    mutable std::shared_future<long> m_statusCode;

    /// A @c string representing the content-type of the HTTP content.
    std::shared_future<std::string> m_contentType;

    /// An @c Attachment from which to read the HTTP content from or @c nullptr if no data was fetched.
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> m_dataStream;
};

inline HTTPContent::HTTPContent(
    std::future<long> httpStatusCode,
    std::future<std::string> httpContentType,
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> stream) :
        m_statusCode{std::move(httpStatusCode)},
        m_contentType{std::move(httpContentType)},
        m_dataStream{stream} {};

inline long HTTPContent::getStatusCode() {
    auto statusCodeFuture = m_statusCode;
    return statusCodeFuture.get();
}

inline bool HTTPContent::isStatusCodeSuccess() {
    auto statusCode = getStatusCode();
    return (statusCode >= 200) && (statusCode < 300);
}

inline bool HTTPContent::isReady(const std::chrono::milliseconds timeout) const {
    auto statusCodeFuture = m_statusCode;
    auto status = statusCodeFuture.wait_for(timeout);
    return std::future_status::ready == status;
}

inline std::string HTTPContent::getContentType() {
    auto contentTypeFuture = m_contentType;
    return contentTypeFuture.get();
}

inline std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> HTTPContent::getDataStream() {
    return m_dataStream;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTPCONTENT_H_
