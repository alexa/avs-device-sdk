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

#include <acsdkAuthorization/LWA/test/StubStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {
namespace test {

bool StubStorage::openOrCreate() {
    return true;
}

bool StubStorage::createDatabase() {
    return true;
}

bool StubStorage::open() {
    return true;
}

bool StubStorage::setRefreshToken(const std::string& refreshToken) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_refreshToken = refreshToken;
    return true;
}

bool StubStorage::clearRefreshToken() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_refreshToken.clear();
    return true;
}

bool StubStorage::getRefreshToken(std::string* refreshToken) {
    if (!refreshToken) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    *refreshToken = m_refreshToken;
    return !m_refreshToken.empty();
}

bool StubStorage::setUserId(const std::string& userId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_userId = userId;
    return true;
}

bool StubStorage::getUserId(std::string* userId) {
    if (!userId) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    *userId = m_userId;

    return !m_userId.empty();
}

bool StubStorage::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_userId.clear();
    m_refreshToken.clear();

    return true;
}

}  // namespace test
}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
