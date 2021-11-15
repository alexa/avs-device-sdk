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

#ifndef AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_TESTUTIL_H_
#define AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_TESTUTIL_H_

#include <algorithm>
#include <chrono>
#include <functional>
#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace chrono;

/**
 * Keeps executing a given function every given amount, and will return true as soon as the function returns true.
 * Otherwise, it will return false after timeout time.
 */
bool waitUntil(const std::function<bool()>& validate, std::chrono::milliseconds timeout = std::chrono::seconds(5));

/**
 * Creates a temporary directory starting with <prefix>. Asserts false upon failure.
 * @note you are responsible for deleting this directory after usage.
 */
std::string createTmpDir(const std::string& prefix = "test");

struct PrintDescription {
    template <class TestParamInfoType>
    std::string operator()(const TestParamInfoType& info) const {
        auto s = info.param.description;
        std::transform(s.begin(), s.end(), s.begin(), [](char ch) { return isalnum(ch) ? ch : '_'; });
        return s;
    }
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // AVS_CAPABILITIES_DAVSCLIENT_ACSDKASSETSCOMMON_TEST_MOCKS_TESTUTIL_H_
