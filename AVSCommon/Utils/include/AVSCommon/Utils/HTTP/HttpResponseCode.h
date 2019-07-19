/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP_HTTPRESPONSECODE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP_HTTPRESPONSECODE_H_

#include <iostream>
#include <string>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http {

enum HTTPResponseCode {
    /// No HTTP response received.
    HTTP_RESPONSE_CODE_UNDEFINED = 0,

    /// HTTP Success with reponse payload.
    SUCCESS_OK = 200,
    /// HTTP Succcess with no response payload.
    SUCCESS_NO_CONTENT = 204,

    /// Multiple redirection choices
    REDIRECTION_MULTIPLE_CHOICES = 300,
    /// Content moved permanently
    REDIRECTION_MOVED_PERMANENTLY = 301,
    /// Content found on another URI
    REDIRECTION_FOUND = 302,
    /// See another - Should issue a GET to the other URI and assume that data was received on this request
    REDIRECTION_SEE_ANOTHER = 303,
    /// Content can be found on another URI, but the redirect should not be cached - cannot change HTTP method
    REDIRECTION_TEMPORARY_REDIRECT = 307,
    /// Content can be found on another URI, and the redirect should be cached - cannot change HTTP method
    REDIRECTION_PERMANENT_REDIRECT = 308,

    /// HTTP code for invalid request by user.
    CLIENT_ERROR_BAD_REQUEST = 400,
    /// HTTP code for forbidden request by user.
    CLIENT_ERROR_FORBIDDEN = 403,

    /// HTTP code for internal error by server which didn't fulfill the request.
    SERVER_ERROR_INTERNAL = 500,

    /// First success code
    SUCCESS_START_CODE = SUCCESS_OK,
    /// Last success code
    SUCCESS_END_CODE = 299,

    /// First code in redirection range.
    REDIRECTION_START_CODE = REDIRECTION_MULTIPLE_CHOICES,
    /// Last code in redirection range.
    REDIRECTION_END_CODE = REDIRECTION_PERMANENT_REDIRECT
};

/**
 * Checks if the status code is on the HTTP success range.
 *
 * @param code The status code to check
 * @return @c true if it is on the success range
 */
inline bool isStatusCodeSuccess(HTTPResponseCode code) {
    return (HTTPResponseCode::SUCCESS_START_CODE <= code) && (code <= HTTPResponseCode::SUCCESS_END_CODE);
}

/**
 * Checks if the status code is on the HTTP redirect range.
 *
 * @param code The status code to check
 * @return @c true if it is on the redirect range
 */
inline bool isRedirect(HTTPResponseCode code) {
    // Not all 3xx codes are redirects and we should avoid proxy codes for security reasons. Therefore the safest way
    // to check redirection is verifying code by code.
    return HTTPResponseCode::REDIRECTION_MULTIPLE_CHOICES == code ||
           HTTPResponseCode::REDIRECTION_MOVED_PERMANENTLY == code || HTTPResponseCode::REDIRECTION_FOUND == code ||
           HTTPResponseCode::REDIRECTION_SEE_ANOTHER == code ||
           HTTPResponseCode::REDIRECTION_TEMPORARY_REDIRECT == code ||
           HTTPResponseCode::REDIRECTION_PERMANENT_REDIRECT == code;
}

/**
 * Converts a response code in integer format to an @ HTTPResponseCode enumeration value. Gives an @c
 * HTTP_RESPONSE_CODE_UNDEFINED if the response code is not mapped by any enumeration value.
 *
 * @param code The response code.
 * @return The enumeration value.
 */
inline HTTPResponseCode intToHTTPResponseCode(int code) {
    switch (code) {
        case 200:
            return HTTPResponseCode::SUCCESS_OK;
        case 204:
            return HTTPResponseCode::SUCCESS_NO_CONTENT;
        case 300:
            return HTTPResponseCode::REDIRECTION_MULTIPLE_CHOICES;
        case 301:
            return HTTPResponseCode::REDIRECTION_MOVED_PERMANENTLY;
        case 302:
            return HTTPResponseCode::REDIRECTION_FOUND;
        case 303:
            return HTTPResponseCode::REDIRECTION_SEE_ANOTHER;
        case 307:
            return HTTPResponseCode::REDIRECTION_TEMPORARY_REDIRECT;
        case 308:
            return HTTPResponseCode::REDIRECTION_PERMANENT_REDIRECT;
        case 400:
            return HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST;
        case 403:
            return HTTPResponseCode::CLIENT_ERROR_FORBIDDEN;
        case 500:
            return HTTPResponseCode::SERVER_ERROR_INTERNAL;
    }
    logger::acsdkError(
        logger::LogEntry("HttpResponseCodes", __func__).d("code", code).m("Unknown HTTP response code."));
    return HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
}

/**
 * Converts a response code enum value to the code in int.
 *
 * @param responseCode The enum value.
 * @return The equivalent int value.
 */
inline int responseCodeToInt(HTTPResponseCode responseCode) {
    return static_cast<std::underlying_type<HTTPResponseCode>::type>(responseCode);
}

/**
 * Produces the string representation of a response code value.
 *
 * @param responseCode The enumeration value.
 * @return The string representation.
 */
inline std::string responseCodeToString(HTTPResponseCode responseCode) {
    switch (responseCode) {
        case HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED:
            return "HTTP_RESPONSE_CODE_UNDEFINED";
        case HTTPResponseCode::SUCCESS_OK:
            return "SUCCESS_OK";
        case HTTPResponseCode::SUCCESS_NO_CONTENT:
            return "SUCCESS_NO_CONTENT";
        case HTTPResponseCode::SUCCESS_END_CODE:
            return "SUCCESS_END_CODE";
        case HTTPResponseCode::REDIRECTION_MULTIPLE_CHOICES:
            return "REDIRECTION_MULTIPLE_CHOICES";
        case HTTPResponseCode::REDIRECTION_MOVED_PERMANENTLY:
            return "REDIRECTION_MOVED_PERMANENTLY";
        case HTTPResponseCode::REDIRECTION_FOUND:
            return "REDIRECTION_FOUND";
        case HTTPResponseCode::REDIRECTION_SEE_ANOTHER:
            return "REDIRECTION_SEE_ANOTHER";
        case HTTPResponseCode::REDIRECTION_TEMPORARY_REDIRECT:
            return "REDIRECTION_TEMPORARY_REDIRECT";
        case HTTPResponseCode::REDIRECTION_PERMANENT_REDIRECT:
            return "REDIRECTION_PERMANENT_REDIRECT";
        case HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST:
            return "CLIENT_ERROR_BAD_REQUEST";
        case HTTPResponseCode::CLIENT_ERROR_FORBIDDEN:
            return "CLIENT_ERROR_FORBIDDEN";
        case HTTPResponseCode::SERVER_ERROR_INTERNAL:
            return "SERVER_ERROR_INTERNAL";
    }
    logger::acsdkError(logger::LogEntry("HttpResponseCodes", __func__)
                           .d("longValue", static_cast<long>(responseCode))
                           .m("Unknown HTTP response code."));
    return "";
}

/**
 * Overwrites the << operator for @c HTTPResponseCode.
 *
 * @param os The output stream pointer.
 * @param code The HTTPResponseCode to write to the output stream.
 * @return The output stream pointer.
 */
inline std::ostream& operator<<(std::ostream& os, const HTTPResponseCode& code) {
    os << responseCodeToString(code);
    return os;
}

}  // namespace http
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP_HTTPRESPONSECODE_H_
