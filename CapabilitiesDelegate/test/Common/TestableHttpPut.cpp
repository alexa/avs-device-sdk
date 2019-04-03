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

#include "TestableHttpPut.h"

#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

using namespace avsCommon::utils::http;

TestHttpPut::TestHttpPut() : m_httpResponseCode{HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED} {
}

HTTPResponse TestHttpPut::doPut(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& data) {
    m_requestUrl = url;
    m_requestData = data;
    m_requestHeaders = headers;

    HTTPResponse httpresponse;
    httpresponse.code = m_httpResponseCode;
    httpresponse.body = m_httpResponseBody;
    return httpresponse;
}

void TestHttpPut::setResponseCode(long httpResponseCode) {
    m_httpResponseCode = httpResponseCode;
}

void TestHttpPut::setResponseBody(std::string httpResponseBody) {
    m_httpResponseBody = httpResponseBody;
}

std::string TestHttpPut::getRequestUrl() {
    return m_requestUrl;
}

std::string TestHttpPut::getRequestData() {
    return m_requestData;
}

std::vector<std::string> TestHttpPut::getRequestHeaders() {
    return m_requestHeaders;
}

void TestHttpPut::reset() {
    m_requestUrl = "";
    m_requestHeaders.clear();
    m_requestData = "";

    m_httpResponseCode = HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED;
    m_httpResponseBody = "";
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
