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

#ifndef ACSDKASSETMANAGER_SRC_REQUESTERMETADATA_H_
#define ACSDKASSETMANAGER_SRC_REQUESTERMETADATA_H_

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>

#include <chrono>
#include <string>

#include "acsdkAssetsInterfaces/ArtifactRequest.h"
#include "acsdkAssetsInterfaces/Priority.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class RequesterMetadata {
public:
    /**
     * Creates a metadata file given a valid artifact request and storage metadata.
     *
     * @param request REQUIRED, full request containing all the data used to identify an artifact on davs.
     * @param resourceId OPTIONAL, resourceId which uniquely identifies the downloaded resource or content.
     * @param lastUsed OPTIONAL, last time this artifact was referenced.
     * @return NULLABLE, a valid pointer to Artifact Metadata, null otherwise.
     */
    static std::unique_ptr<RequesterMetadata> create(
            std::shared_ptr<commonInterfaces::ArtifactRequest> request,
            std::string resourceId = "",
            std::chrono::milliseconds lastUsed = std::chrono::milliseconds(0));

    /**
     * Read the metadata info from disk and construct ArtifactMetadata object.
     *
     * @param metadataFile REQUIRED, path of the metadata json file containing ArtifactMetadata info.
     * @return return ArtifactMetadata if read from file is success or return nullptr.
     */
    static std::unique_ptr<RequesterMetadata> createFromFile(const std::string& metadataFile);

    /**
     * Creates a metadata file which has artifact request info.
     *
     * @param metadataFile full path of the file to save the metadata to.
     * @return  returns true if successful
     */
    bool saveToFile(const std::string& metadataFile);

    inline const std::shared_ptr<commonInterfaces::ArtifactRequest>& getRequest() {
        return m_request;
    }

    inline const std::string& getResourceId() const {
        return m_resourceId;
    }

    inline std::chrono::milliseconds getLastUsed() const {
        return m_lastUsed;
    }

    inline void setResourceId(const std::string& value) {
        m_resourceId = value;
    }

    inline void setLastUsed(const std::chrono::milliseconds value) {
        m_lastUsed = value;
    }

    inline void clear(const std::string& metadataFile) {
        m_resourceId.clear();
        alexaClientSDK::avsCommon::utils::filesystem::removeAll(metadataFile);
    }

private:
    RequesterMetadata(
            std::shared_ptr<commonInterfaces::ArtifactRequest> request,
            std::string resourceId,
            std::chrono::milliseconds lastUsed);

private:
    const std::shared_ptr<commonInterfaces::ArtifactRequest> m_request;
    std::string m_resourceId;
    std::chrono::milliseconds m_lastUsed;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_REQUESTERMETADATA_H_
