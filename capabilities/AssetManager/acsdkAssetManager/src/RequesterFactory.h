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

#ifndef ACSDKASSETMANAGER_SRC_REQUESTERFACTORY_H_
#define ACSDKASSETMANAGER_SRC_REQUESTERFACTORY_H_

#include "DavsRequester.h"
#include "UrlRequester.h"
#include "acsdkAssetManager/UrlAllowListWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

/**
 * Requester factory responsible for creating requesters based on requests metadata.
 */
class RequesterFactory {
public:
    /**
     * Creates a requester factory used to identify which requester can handle a certain request.
     *
     * @param storageManager REQUIRED, a module that manages disk space and frees up artifacts accordingly.
     * @param communicationHandler REQUIRED, used to communicate the values of properties and invoke functions
     * @param davsClient REQUIRED, client used to fetch the artifact object.
     * @param urlTmpDirectory REQUIRED, folder for storing temporary url downloads.
     * @param authDelegate REQUIRED, the Authentication Delegate to generate the authentication token
     * @return NULLABLE, pointer to a new requester factory, null if inputs are invalid.
     */
    static std::unique_ptr<RequesterFactory> create(
            std::shared_ptr<StorageManager> storageManager,
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> communicationHandler,
            std::shared_ptr<davsInterfaces::ArtifactHandlerInterface> davsClient,
            std::string urlTmpDirectory,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<UrlAllowListWrapper> allowList);

    /**
     * Creates an artifact based off of the given storage path. Tries to open the metadata file inside the given
     * directory to parse out the artifact information, if found and parsed, then create the artifact object with LOADED
     * state, otherwise return nullptr.
     *
     * @param metadataFilePath REQUIRED, path to the artifact metadata file used to load this request.
     * @return NULLABLE, pointer to a new artifact based off of the storage.
     */
    std::shared_ptr<Requester> createFromStorage(const std::string& metadataFilePath);

    /**
     * Creates an artifact in an init state with the provided metadata information.
     *
     * @param metadata REQUIRED, used to form the DAVS Client request which describes the desired artifact file.
     * @param metadataFilePath REQUIRED, path to the artifact metadata file for this request.
     * @return NULLABLE, pointer to a new artifact based off of the DAVS Request.
     */
    std::shared_ptr<Requester> createFromMetadata(
            const std::shared_ptr<RequesterMetadata>& metadata,
            const std::string& metadataFilePath);

private:
    RequesterFactory(
            std::shared_ptr<StorageManager> storageManager,
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> communicationHandler,
            std::shared_ptr<davsInterfaces::ArtifactHandlerInterface> davsClient,
            std::string urlTmpDirectory,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<UrlAllowListWrapper> allowList);

private:
    const std::shared_ptr<StorageManager> m_storageManager;
    const std::shared_ptr<commonInterfaces::AmdCommunicationInterface> m_communicationHandler;
    const std::shared_ptr<davsInterfaces::ArtifactHandlerInterface> m_davsClient;
    /// Temporary directory used to store artifacts downloaded from a url
    const std::string m_urlTmpDirectory;
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> m_urlPowerResource;
    std::shared_ptr<UrlAllowListWrapper> m_allowedUrlList;
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_REQUESTERFACTORY_H_
