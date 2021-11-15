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

#ifndef ACSDKASSETMANAGER_SRC_URLREQUESTER_H_
#define ACSDKASSETMANAGER_SRC_URLREQUESTER_H_

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/Power/PowerResource.h>

#include <atomic>
#include <future>

#include "Requester.h"
#include "acsdkAssetManager/UrlAllowListWrapper.h"
#include "acsdkAssetsCommon/CurlProgressCallbackInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

/**
 * This class implements the Requester class and extends it to allow for the handling of artifacts downloaded directly
 * from urls
 */
class UrlRequester : public Requester {
public:
    class CurlProgressCallback : public common::CurlProgressCallbackInterface {
    public:
        ~CurlProgressCallback() override = default;

        void enable(size_t budget) {
            availableBudget = budget;
        }

        void cancel() {
            availableBudget = 0;
        }

        bool onProgressUpdate(long dlTotal, long dlNow, long, long) override {
            return availableBudget >= static_cast<size_t>(dlNow);
        }

    private:
        std::atomic_size_t availableBudget{0};
    };

    ~UrlRequester() override;

    /// @name Requester Functions
    /// @{
    bool download() override;
    /// @}

private:
    UrlRequester(
            std::shared_ptr<StorageManager> storageManager,
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> communicationHandler,
            std::shared_ptr<RequesterMetadata> metadata,
            std::string metadataFilePath,
            std::string workingDirectory,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> powerResource,
            std::shared_ptr<UrlAllowListWrapper> allowUrlList);

    size_t deleteAndCleanupLocked(std::unique_lock<std::mutex>& lock) override;

    /**
     * This function checks to see if a download request is valid and if there is space for the downloaded asset, then
     * downloads the asset if appropriate.
     */
    void downloadWorker();

private:
    /// Directory where this class stores downloaded assets
    std::string m_workingDirectory;
    /// Condition variable used to block until no downloads are occurring
    std::condition_variable m_stateTrigger;
    /// Allows the class to monitor downloads (which are performed asynchronously)
    std::future<void> m_downloadFuture;
    /// Callback which curl calls repeatedly during a download which shares download progress
    std::shared_ptr<CurlProgressCallback> m_downloadProgressTrigger;
    /// AuthDelegate that curlWrapper will use to get the Authentication Token
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;
    /// PowerResource used to acquire/release the wakelock
    std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> m_powerResource;
    /// The list of urls that we can download an artifact from.
    std::shared_ptr<UrlAllowListWrapper> m_allowUrlList;

    friend RequesterFactory;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_URLREQUESTER_H_
