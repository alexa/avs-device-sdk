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

#ifndef ACSDKASSETMANAGER_SRC_DAVSREQUESTER_H_
#define ACSDKASSETMANAGER_SRC_DAVSREQUESTER_H_

#include "Requester.h"
#include "acsdkAssetsInterfaces/Communication/AmdCommunicationInterface.h"
#include "acsdkDavsClientInterfaces/ArtifactHandlerInterface.h"
#include "acsdkDavsClientInterfaces/DavsCheckCallbackInterface.h"
#include "acsdkDavsClientInterfaces/DavsDownloadCallbackInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class DavsRequester
        : public Requester
        , public davsInterfaces::DavsCheckCallbackInterface
        , public davsInterfaces::DavsDownloadCallbackInterface {
public:
    /**
     * Deregister this artifact from DAVS Client.
     */
    ~DavsRequester() override {
        m_davsClient->deregisterArtifact(m_davsRequestId);
    };

    /// @name Requester Functions
    /// @{
    bool download() override;
    bool validateWriteRequest(const std::string& name, int newValue) override;
    /// @}

    /// @name DavsCheckCallbackInterface Functions
    /// @{
    bool checkIfOkToDownload(
            std::shared_ptr<commonInterfaces::VendableArtifact> availableArtifact,
            size_t freeSpaceNeeded) override;
    void onCheckFailure(commonInterfaces::ResultCode errorCode) override;
    /// @}

    /// @name DavsDownloadCallbackInterface Functions
    /// @{
    void onStart() override;
    void onArtifactDownloaded(
            std::shared_ptr<commonInterfaces::VendableArtifact> downloadedArtifact,
            const std::string& path) override;
    void onDownloadFailure(commonInterfaces::ResultCode errorCode) override;
    void onProgressUpdate(int progress) override;
    /// @}

private:
    DavsRequester(
            std::shared_ptr<StorageManager> storageManager,
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> communicationHandler,
            std::shared_ptr<RequesterMetadata> metadata,
            const std::string& metadataFilePath,
            std::shared_ptr<davsInterfaces::ArtifactHandlerInterface> davsClient) :
            Requester(
                    std::move(storageManager),
                    std::move(communicationHandler),
                    std::move(metadata),
                    std::move(metadataFilePath)),
            m_davsClient(std::move(davsClient)) {
    }
    /// @name Requester Functions
    /// @{
    size_t deleteAndCleanupLocked(std::unique_lock<std::mutex>& lock) override;
    /// @}

    /// Set auto update for the artifact based on the current priority.
    void adjustAutoUpdateBasedOnPriority(commonInterfaces::Priority newPriority);

private:
    // DAVS Client that will be used to download/update the artifacts.
    const std::shared_ptr<davsInterfaces::ArtifactHandlerInterface> m_davsClient;
    // Request ID from DAVS Client after registration.
    std::string m_davsRequestId;

    friend RequesterFactory;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_DAVSREQUESTER_H_
