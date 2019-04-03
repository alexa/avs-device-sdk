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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPRESPONSECODES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPRESPONSECODES_H_

/**
 * An enum to represent HTTP response codes.
 *
 * ATTENTION - This enum has been deprecated and only exists to provide backward compatibility with code that was
 * designed to work with past versions of the AVS C++ SDK.
 *
 * @deprecated Use the @c alexaClientSDK::avsCommon::utils::http::HTTPResponseCode instead.
 */
enum HTTPResponseCode {
    /// No HTTP response received.
    HTTP_RESPONSE_CODE_UNDEFINED = 0,
    /// HTTP Success with reponse payload.
    SUCCESS_OK = 200,
    /// HTTP Succcess with no response payload.
    SUCCESS_NO_CONTENT = 204,
    /// This is used to detect the first Redirection (3xx) code
    REDIRECTION_START_CODE = 300,
    /// This is used to detect the last Redirection (3xx) code
    REDIRECTION_END_CODE = 308,
    /// HTTP code for invalid request by user.
    BAD_REQUEST = 400,
    /// HTTP code for forbidden request by user.
    FORBIDDEN = 403,
    /// HTTP code for internal error by server which didn't fulfill the request.
    SERVER_INTERNAL_ERROR = 500
};

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPRESPONSECODES_H_
