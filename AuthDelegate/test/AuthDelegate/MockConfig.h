/*
 * MockConfig.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_AUTHDELEGATE_MOCK_CONFIG_H_
#define ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_AUTHDELEGATE_MOCK_CONFIG_H_

#include <gmock/gmock.h>
#include "AuthDelegate/Config.h"

namespace alexaClientSDK {
namespace authDelegate {

/// Mock Config class
class MockConfig : public Config {
public:
    MOCK_CONST_METHOD0(getLwaUrl, std::string());
    MOCK_CONST_METHOD0(getClientId, std::string());
    MOCK_CONST_METHOD0(getRefreshToken, std::string());
    MOCK_CONST_METHOD0(getClientSecret, std::string());
    MOCK_CONST_METHOD0(getAuthTokenRefreshHeadStart, std::chrono::seconds());
    MOCK_CONST_METHOD0(getMaxAuthTokenTtl, std::chrono::seconds());
    MOCK_CONST_METHOD0(getRequestTimeout, std::chrono::seconds());
};

}  // namespace authDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_AUTHDELEGATE_MOCK_CONFIG_H_
