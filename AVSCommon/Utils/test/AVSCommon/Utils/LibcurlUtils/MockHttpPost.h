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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_LIBCURLUTILS_MOCKHTTPPOST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_LIBCURLUTILS_MOCKHTTPPOST_H_

#include <gmock/gmock.h>

#include <AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {
namespace test {

/// A mock object that implements the @c HttpPostInterface.
class MockHttpPost : public HttpPostInterface {
public:
    MOCK_METHOD4(
        doPost,
        long(const std::string& url, const std::string& data, std::chrono::seconds timeout, std::string& body));
    MOCK_METHOD4(
        doPost,
        HTTPResponse(
            const std::string& url,
            const std::vector<std::string> headerLines,
            const std::vector<std::pair<std::string, std::string>>& data,
            std::chrono::seconds timeout));
    MOCK_METHOD4(
        doPost,
        HTTPResponse(
            const std::string& url,
            const std::vector<std::string> headerLines,
            const std::string& data,
            std::chrono::seconds timeout));
};

}  // namespace test
}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_LIBCURLUTILS_MOCKHTTPPOST_H_
