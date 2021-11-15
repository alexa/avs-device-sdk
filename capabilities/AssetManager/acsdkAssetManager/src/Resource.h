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

#ifndef ACSDKASSETMANAGER_SRC_RESOURCE_H_
#define ACSDKASSETMANAGER_SRC_RESOURCE_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class StorageManager;

/**
 * This class will represent a resource as it is stored on the system. It will maintain an internal reference counter
 * which represents how many requests are referencing it.
 *
 * This class will only be created and managed by StorageManager to ensure that it does not get leaked or mishandled.
 */
class Resource {
public:
    /**
     * @return the id of this resource, most commonly the sha2 checksum.
     */
    inline const std::string& getId() const {
        return m_id;
    }

    /**
     * @return the full path of the resource itself, containing its content. ie /path/to/resource/file.txt
     */
    inline const std::string& getPath() const {
        return m_fullResourcePath;
    }

    /**
     * @return the size of the resource in bytes, if the resource has been deleted, then the size will be 0.
     */
    inline size_t getSizeBytes() const {
        return m_sizeBytes;
    }

    /**
     * @return weather the resource exists on the system or not using its size.
     */
    inline bool exists() const {
        return m_sizeBytes > 0;
    }

private:
    /**
     * Creates a resource given a source file or directory that will be represented by this resource and moves it to the
     * designated parent directory. If the operation succeeds, then this will also attempt to cache this information in
     * a metadata file.
     *
     * @param parentDirectory REQUIRED, directory that is used to store this resource.
     * @param id REQUIRED, id that will be used to keep track of this resource.
     * @param source REQUIRED, actual content (file or directory) that will be represented by this resource.
     * @return NULLABLE, a smart pointer to the newly created resource upon success, null otherwise.
     */
    static std::shared_ptr<Resource> create(
            const std::string& parentDirectory,
            const std::string& id,
            const std::string& source);

    /**
     * Creates a resource, given a directory, by analyzing the content of the directory. ie if /path/to/resource_id
     * Then this will parse out:
     *    resource_id as the ID of this resource.
     *    the single file inside it, ie file.txt as the name of the resource (fails if there are multiple files or none)
     *    size of this directory as the size of this resource.
     *
     * If the operation succeeds, then this will also attempt to cache this information in a metadata file.
     * @note this is used in case the metadata file is missing.
     *
     * @param resourceDirectory REQUIRED, path to a valid directory housing a single resource.
     * @return NULLABLE, a smart pointer to the newly created resource upon success, null otherwise.
     */
    static std::shared_ptr<Resource> createFromStorage(const std::string& resourceDirectory);

    /**
     * Creates a resource using a configuration file found inside the specified directory. If no metadata file is found
     * then call createFromStorage to create the file by analyzing this directory.
     *
     * @param resourceDirectory REQUIRED, path to a valid directory housing a single resource and its metadata file.
     * @return NULLABLE, a smart pointer to the newly created resource upon success, null otherwise.
     */
    static std::shared_ptr<Resource> createFromConfigFile(const std::string& resourceDirectory);

    Resource(
            const std::string& resourceDirectory,
            const std::string& resourceName,
            const std::string& id,
            size_t sizeBytes);

    /**
     * Cache the resource information to a metadata file inside resourceDirectory.
     *
     * @return true if successful, false otherwise.
     */
    bool saveMetadata();

    /**
     * Erases the entire resourceDirectory and resets the resource content.
     */
    void erase();

    inline int incrementRefCount() {
        return ++m_refCount;
    }
    inline int decrementRefCount() {
        return --m_refCount;
    }

private:
    // The parent directory where the resource is stored, like "/some/path/abc"
    const std::string m_resourceDirectory;
    // The name of the file or directory that is stored inside the resource directory, like "resource.txt"
    const std::string m_resourceName;
    // Id of the resource, usually the checksum, like "abc"
    const std::string m_id;
    // Size of the entire resource directory
    size_t m_sizeBytes;
    // The complete path of the resource including the name, like "/some/path/abc/resource.txt"
    std::string m_fullResourcePath;
    // Count of how many requesters reference this resource.
    int m_refCount;

    // it's important that the creation, ref counting, and deletion of resource happens only by the Storage Manager.
    friend StorageManager;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_RESOURCE_H_
