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

#include "acsdkAssetManager/UrlAllowListWrapper.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include <utility>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;

#define TAG "UrlAllowListWrapper"

/// String to identify log entries originating from this file.
static const std::string LOGGER_TAG{"UrlAllowListWrapper"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(LOGGER_TAG, event)

shared_ptr<UrlAllowListWrapper> UrlAllowListWrapper::create(vector<string> allowList, bool allowAllUrls) {
    if (allowList.empty()) {
        ACSDK_WARN(LX("Empty Allow List").m("No urls will be allowed"));
    }
    return shared_ptr<UrlAllowListWrapper>(new UrlAllowListWrapper(allowList, allowAllUrls));
}

bool UrlAllowListWrapper::isUrlAllowed(const std::string& url) {
    lock_guard<mutex> lock(m_allowListMutex);
    if (m_allowAllUrls) {
        return true;
    }
    for (auto const& prefix : m_allowList) {
        if (url.compare(0, prefix.length(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

void UrlAllowListWrapper::setUrlAllowList(std::vector<std::string> newAllowList) {
    lock_guard<mutex> lock(m_allowListMutex);
    m_allowList = move(newAllowList);
}

void UrlAllowListWrapper::addUrlToAllowList(std::string url) {
    lock_guard<mutex> lock(m_allowListMutex);
    m_allowList.emplace_back(move(url));
}

bool UrlAllowListWrapper::allowAllUrls(bool allow) {
    lock_guard<mutex> lock(m_allowListMutex);
    m_allowAllUrls = allow;
    return true;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK