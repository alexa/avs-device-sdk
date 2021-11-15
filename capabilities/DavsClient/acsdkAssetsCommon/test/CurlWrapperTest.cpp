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

#include <gtest/gtest.h>

#include "acsdkAssetsCommon/CurlWrapper.h"

using namespace std;
using namespace std::chrono;
using namespace ::testing;
using namespace alexaClientSDK::acsdkAssets::common;

class CurlWrapperTest : public Test {};

TEST_F(CurlWrapperTest, parsingValidHeaderTest) {
    auto header =
            "HTTP/2 200\r\n"
            "Content-Type:application/json\r\n"
            "Server: Server\r\n"
            "Date : Wed, 18 Aug 2021 22:55:02 GMT \r\n"
            "\n";

    ASSERT_EQ(CurlWrapper::getValueFromHeaders(header, "Content-Type"), "application/json");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders(header, "content-type"), "application/json");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders(header, "server"), "Server");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders(header, "DATE"), "Wed, 18 Aug 2021 22:55:02 GMT");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders(header, "ASDF"), "");
}

TEST_F(CurlWrapperTest, parsingInvalidHeaderTest) {
    ASSERT_EQ(CurlWrapper::getValueFromHeaders("Content-Type : ", "Content-Type"), "");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders("Content-Type :", "Content-Type"), "");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders("Content-Type", "Content-Type"), "");
    ASSERT_EQ(CurlWrapper::getValueFromHeaders("", "Content-Type"), "");
}