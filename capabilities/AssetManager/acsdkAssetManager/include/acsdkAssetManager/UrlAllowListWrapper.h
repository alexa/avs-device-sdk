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

#ifndef ACSDKASSETMANAGER_URLALLOWLISTWRAPPER_H_
#define ACSDKASSETMANAGER_URLALLOWLISTWRAPPER_H_
#include <memory>
#include <mutex>
#include <vector>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class UrlAllowListWrapper {
public:
    /**
     * Creates a UrlAllowListWrapper used to define which urls are allowed
     * @param newAllowList The list of allowed urls.
     * @param allowAllUrls, if we will allow all the urls to be downloaded from. defaults to false
     * @return Nullable, pointer to a new UrlAllowListWrapper, null if newAllowList is invalid.
     */
    static std::shared_ptr<UrlAllowListWrapper> create(
            std::vector<std::string> newAllowList,
            bool allowAllUrls = false);

    /**
     * Checks to see if the url is allowed to be downloaded from.
     * @param url, the url that we want to check if we can download from.
     * @return true, if url is in the allow list, false otherwise.
     */
    bool isUrlAllowed(const std::string& url);

    /**
     * Set the allow list to a new list.
     * @param newAllowList, the new list that we will allow downloads from.
     */
    void setUrlAllowList(std::vector<std::string> newAllowList);

    /**
     * Add a new url to the allow list.
     * @param url, the url that we want to add to the allow list.
     */
    void addUrlToAllowList(std::string url);

    /**
     * Set the bool flag to allow all urls
     * @param allow, if we are going to allow all urls or not.
     * @return true, if we are able to set allowAllUrls. False otherwise
     */
    bool allowAllUrls(bool allow);

    ~UrlAllowListWrapper() = default;

private:
    UrlAllowListWrapper(std::vector<std::string> newAllowList, bool allowAllUrls) :
            m_allowList(std::move(newAllowList)),
            m_allowAllUrls(allowAllUrls) {
    }

    /// The stored allow list of urls.
    std::vector<std::string> m_allowList;
    /// The mutex that protects reads and writes to the allow list.
    std::mutex m_allowListMutex;
    /// The flag that tells us if we will download all urls
    bool m_allowAllUrls;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_URLALLOWLISTWRAPPER_H_
