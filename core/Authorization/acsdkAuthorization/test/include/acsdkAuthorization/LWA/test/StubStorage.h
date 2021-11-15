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

#ifndef ACSDKAUTHORIZATION_LWA_TEST_STUBSTORAGE_H_
#define ACSDKAUTHORIZATION_LWA_TEST_STUBSTORAGE_H_

#include <mutex>
#include <string>

#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {
namespace test {

class StubStorage : public acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface {
public:
    bool openOrCreate() override;
    bool open() override;
    bool createDatabase() override;
    bool setRefreshToken(const std::string& refreshToken) override;
    bool clearRefreshToken() override;
    bool getRefreshToken(std::string* refreshToken) override;
    bool setUserId(const std::string& userId) override;
    bool getUserId(std::string* userId) override;
    bool clear() override;

private:
    std::mutex m_mutex;
    std::string m_refreshToken;
    std::string m_userId;
};

}  // namespace test
}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_LWA_TEST_STUBSTORAGE_H_
