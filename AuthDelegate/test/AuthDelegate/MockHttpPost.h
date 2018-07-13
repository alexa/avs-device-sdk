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

#ifndef ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_AUTHDELEGATE_MOCKHTTPPOST_H_
#define ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_AUTHDELEGATE_MOCKHTTPPOST_H_

#include <chrono>
#include <gmock/gmock.h>
#include <string>

#include "AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h"

namespace alexaClientSDK {
namespace authDelegate {
namespace test {

/// Mock HttpPostInterface class
class MockHttpPost : public avsCommon::utils::libcurlUtils::HttpPostInterface {
public:
    MOCK_METHOD1(addHTTPHeader, bool(const std::string& header));
    MOCK_METHOD4(
        doPost,
        long(const std::string& url, const std::string& data, std::chrono::seconds timeout, std::string& body));
};

}  // namespace test
}  // namespace authDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_AUTHDELEGATE_MOCKHTTPPOST_H_
