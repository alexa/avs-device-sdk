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

#ifndef AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_CURLWRAPPERMOCK_H_
#define AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_CURLWRAPPERMOCK_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

class CurlWrapperMock {
public:
    static std::string root;
    static std::string capturedRequest;
    static std::string mockResponse;
    static bool getResult;
    static bool useDavsService;
    static bool downloadShallFail;
    static std::string header;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_CURLWRAPPERMOCK_H_
