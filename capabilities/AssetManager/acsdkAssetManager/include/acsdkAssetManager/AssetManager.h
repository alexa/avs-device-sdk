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

#ifndef ACSDKASSETMANAGER_ASSETMANAGER_H_
#define ACSDKASSETMANAGER_ASSETMANAGER_H_

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <atomic>
#include <memory>
#include <unordered_set>

#include "acsdkAssetManager/UrlAllowListWrapper.h"
#include "acsdkAssetsInterfaces/Communication/AmdCommunicationInterface.h"
#include "acsdkDavsClient/DavsClient.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class RequesterFactory;
class StorageManager;
class Requester;

enum class IdleState { ACTIVE = 0, IDLE = 1 };

class AssetManager
        : public std::enable_shared_from_this<AssetManager>
        , public acsdkCommunicationInterfaces::FunctionInvokerInterface<bool, std::string> {
public:
    /**
     * Creates a a new Asset Manager with a davs client handle and base directory to work off of.
     *
     * @param communicationHandler REQUIRED, used to communicate the values of properties and functions
     * @param davsClient REQUIRED, used to register artifact and communicate with DAVS.
     * @param artifactsDirectory REQUIRED, base directory where the artifacts are to be stored.
     * @param authDelegate REQUIRED, the Authentication Delegate to generate the authentication token.
     * @param allowList REQUIRED, URL allow list wrapper used for managing downloads from URLs.
     * @return NULLABLE, pointer to new AssetManager, null if failed.
     */
    static std::shared_ptr<AssetManager> create(
            const std::shared_ptr<commonInterfaces::AmdCommunicationInterface>& communicationHandler,
            const std::shared_ptr<davs::DavsClient>& davsClient,
            const std::string& artifactsDirectory,
            const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
            const std::shared_ptr<UrlAllowListWrapper>& allowList);

    ~AssetManager() override;

    /**
     * Requests a creation of a new requester if it does not already exists based off a json request.
     * Use an existing requester if it matches the json request.
     *
     * @param request REQUIRED, a request that represents an artifact in DAVS.
     * @return weather the operation was successful.
     */
    bool downloadArtifact(const std::shared_ptr<commonInterfaces::ArtifactRequest>& request);

    /** Queues up a call to downloadArtifact in an executor. Returns immediately. */
    inline void queueDownloadArtifact(const std::shared_ptr<commonInterfaces::ArtifactRequest>& request) {
        m_executor.execute([this, request] { downloadArtifact(request); });
    }

    /** Takes string and tries to make it a ArtifactRequest
     * Queues up a call to downloadArtifact in an executor.
     * Returns immediately.
     * @param requestString string that represents the ArtifactRequest
     * @return true, if successfully able to queue up call, false otherwise
     */
    bool queueDownloadArtifact(const std::string& requestString);

    /**
     * Requests the deletion and removal of an existing artifact by deleting its requester.
     *
     * @param summaryString REQUIRED, summary of an existing artifact.
     */
    void deleteArtifact(const std::string& summaryString);

    /** Queues up a call to deleteArtifact in an executor. Returns immediately. */
    inline void queueDeleteArtifact(const std::string& summaryString) {
        m_executor.execute([this, summaryString] { deleteArtifact(summaryString); });
    }

    /**
     * Handles the pending update for a specific requester given its summary string.
     *
     * @param summaryString for an artifact request that maps to a requester handling the update.
     * @param acceptUpdate weather to accept the update or reject it.
     */
    void handleUpdate(const std::string& summaryString, bool acceptUpdate);

    /** Queues up a call to handleUpdate in an executor. Returns immediately. */
    inline void queueHandleUpdate(const std::string& summaryString, bool acceptUpdate) {
        m_executor.execute([this, summaryString, acceptUpdate] { handleUpdate(summaryString, acceptUpdate); });
    }

    /**
     * Goes through the available requesters and deletes unused requesters and their artifacts based on used time and
     * priority.
     * @note this operation could take a while as it goes through and deletes artifacts until the requested amount is
     * satisfied.
     *
     * @param requestedAmount size in bytes that is needed to be cleared.
     * @return true if the requested amount was freed up, false otherwise.
     */
    bool freeUpSpace(size_t requestedAmount);

    /** Queues up a call to freeUpSpace in an executor. Returns immediately. */
    inline void queueFreeUpSpace(size_t requestedAmount) {
        m_executor.execute([this, requestedAmount] { freeUpSpace(requestedAmount); });
    }

    /**
     * Callback method when application changes idle state.
     * @param value, int value that will be parsed to a bool
     */
    void onIdleChanged(int value);

    /**
     * @return Get the current budget in MB
     */
    size_t getBudget();

    /**
     *  This method sets the budget for asset manager in megabytes.
     *  If the new budget is set to a number less than the current data stored
     *  asset manager will attempt to clear as many artifacts as possible to be
     *  within the threshold.
     * @param valueMB, int the number of MB for the new budget.
     */
    void setBudget(uint32_t valueMB);

    // override functionInvokerInterface for download and delete.
    bool functionToBeInvoked(const std::string& name, std::string arg) override;

private:
    explicit AssetManager(
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> communicationHandler,
            std::shared_ptr<davs::DavsClient> davsClient,
            const std::string& baseDirectory,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<UrlAllowListWrapper> allowUrlList);

    /**
     * Initializes the directory structure and initializes any downloaded requesters and artifacts.
     *
     * @return true if the operation succeeded and the directory structure was initialized.
     */
    bool init();

    /**
     * Searches an existing artifact list for an artifact handling a given request.
     *
     * @param request REQUIRED, request used to identify an artifact.
     * @return NULLABLE, an existing artifact if found, null otherwise.
     */
    std::shared_ptr<Requester> findRequesterLocked(
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request) const;

private:
    const std::shared_ptr<commonInterfaces::AmdCommunicationInterface> m_communicationHandler;
    const std::shared_ptr<davs::DavsClient> m_davsClient;
    const std::string m_resourcesDirectory;
    const std::string m_requestsDirectory;
    const std::string m_urlTmpDirectory;
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;
    const std::shared_ptr<UrlAllowListWrapper> m_urlAllowList;
    std::unique_ptr<RequesterFactory> m_requesterFactory;
    alexaClientSDK::avsCommon::utils::threading::Executor m_executor;
    std::mutex m_requestersMutex;
    std::unordered_set<std::shared_ptr<Requester>> m_requesters;
    std::shared_ptr<StorageManager> m_storageManager;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_ASSETMANAGER_H_
