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

#ifndef ACSDKASSETMANAGER_SRC_STORAGEMANAGER_H_
#define ACSDKASSETMANAGER_SRC_STORAGEMANAGER_H_

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include "Resource.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class AssetManager;

/**
 * This class manages a certain budget that Asset Manager should not go over. If at any point the amount of space used
 * by Asset Manager goes over the budget, this class will trigger a call to the Asset Manager to free up space by
 * deleting unused artifacts.
 */
class StorageManager : public std::enable_shared_from_this<StorageManager> {
public:
    static constexpr auto MAX_BUDGET_MB = 500;

    /**
     * A reservation token class will be used to reserve space before downloading any artifacts, this token will then
     * be used to utilize the reserved space when acquiring a resource. If this object is destroyed, the reserved space
     * allocated by it will automatically be freed.
     */
    struct ReservationToken {
    public:
        /**
         * Destructor to free up any reserved space.
         */
        ~ReservationToken() {
            if (m_reservedSize == 0) {
                return;
            }

            if (auto sm = m_storageManager.lock()) {
                sm->freeReservedSpace(m_reservedSize);
            }
        }

    private:
        /**
         * Create a reservation token (privately, only Storage Manager can create this)
         * @param storageManager the parent storage manager object used to call back in case we destruct
         * @param reservedSize amount of space reserved by this token
         */
        ReservationToken(std::weak_ptr<StorageManager> storageManager, size_t reservedSize) :
                m_storageManager(std::move(storageManager)),
                m_reservedSize(reservedSize) {
        }

        // parent storage manager class used to free up any reserved space
        std::weak_ptr<StorageManager> m_storageManager;
        // amount of space reserved
        size_t m_reservedSize;

        // it's important that the space reservation and freeing happens only by the Storage Manager.
        friend StorageManager;
    };

    /**
     * Create a Storage Manager that is responsible for maintaining a budget for the artifacts to remain under.
     *
     * @param workingDirectory REQUIRED, place to store budget configuration.
     * @param assetManager REQUIRED, used to request low priority artifacts cleanup to meet the budget.
     * @return NULLABLE, a smart pointer to Storage Manager if successful, null otherwise.
     */
    static std::shared_ptr<StorageManager> create(
            const std::string& workingDirectory,
            const std::shared_ptr<AssetManager>& assetManager);

    ~StorageManager() = default;

    /**
     * Post initialization step that goes through the map of resources and erases any that are unreferenced.
     */
    void purgeUnreferenced();

    /**
     * Registers a resource given a path to its content. If the operation succeeds, then it will be acquired as well.
     * If another resource is found with the same id, then this will delete the provided path and use the existing
     * resource. If there is no other resource with this id, then this will move the source path to the resources
     * directory.
     *
     * @param reservationToken REQUIRED, a unique token that was used to reserve space ahead of download
     * @param id REQUIRED, a unique identifier for this resource, preferably the sha2 checksum.
     * @param sourcePath REQUIRED, the path on disk where this resource is found.
     * @return NULLABLE, a smart pointer to the newly created and acquired resource, null if the operation failed.
     */
    std::shared_ptr<Resource> registerAndAcquireResource(
            std::unique_ptr<StorageManager::ReservationToken> reservationToken,
            const std::string& id,
            const std::string& sourcePath);

    /**
     * Given an id, attempt to acquire a resource which will increment its reference count and returns the resource
     * accordingly. If no resource is found with this id, then return a nullptr.
     *
     * @param id REQUIRED, a unique identifier for the requested resource.
     * @return NULLABLE, a smart pointer to an existing resource, null if not found.
     */
    std::shared_ptr<Resource> acquireResource(const std::string& id);

    /**
     * Given a resource, attempt to find it in the list and decrement its reference count. If the reference count is 0,
     * then erase the resource from the system and return the size of how much memory was freed. If there are others
     * referencing this resource, then return 0 and keep the resource on disk.
     *
     * @param resource REQUIRED, a valid resource that is registered by storage manager.
     * @return the size of the space that was cleared with this release, 0 if the resource was not deleted.
     */
    size_t releaseResource(const std::shared_ptr<Resource>& resource);

    /**
     * Reserve the requested amount of space and return a token that will be used to track the reserved space and is
     * needed for registering a new resource. If the token is destroyed, the space is automatically freed.
     *
     * @param requestedAmount space to reserve
     * @return a new unique token if space is successfully reserved, nullptr otherwise
     */
    std::unique_ptr<ReservationToken> reserveSpace(size_t requestedAmount);

    /**
     * @return the amount of bytes remaining that can be used for a new artifact.
     * This is calculated as the lowest of:
     * 1. Asset Manager Budget - reserved space - downloaded resource space
     * 2. Amount of space left on the device - 5MB buffer
     */
    size_t availableBudget();

    /**
     * @return Get the current budget in MB
     */
    size_t getBudget();

    /**
     * Set a new budget value.
     * @param value the new budget in MB.
     */
    void setBudget(size_t valueMB);

protected:
    explicit StorageManager(
            const std::string& workingDirectory,
            const std::shared_ptr<AssetManager>& assetManager,
            size_t budget);

    /**
     * Goes through the working directory and initializes all the available resources.
     *
     * @return true if the initialization succeeded, false otherwise.
     */
    bool init();

    /**
     * Asks the Asset Manager to free up a certain amount of space in a background task.
     *
     * @param requestedAmount amount in bytes to be cleared.
     */
    void requestGarbageCollection(size_t requestedAmount);

    /**
     * Forwards request to Asset Manager to free up space according to the given amount.
     *
     * @param requestedAmount amount in bytes to clear.
     * @return true if the requested space was cleared, false otherwise.
     */
    bool requestSpace(size_t requestedAmount);

    /**
     * Free up reserved space, to be used by reservation tokens
     */
    void freeReservedSpace(size_t size);

private:
    const std::string m_workingDirectory;
    const std::weak_ptr<AssetManager> m_assetManager;
    size_t m_budget;
    std::mutex m_allocationMutex;
    std::unordered_map<std::string, std::shared_ptr<Resource>> m_bank;

    size_t m_allocatedSize;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_STORAGEMANAGER_H_
