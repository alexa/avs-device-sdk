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
#ifndef ACSDKDAVSCLIENTINTERFACES_ARTIFACTHANDLERINTERFACE_H_
#define ACSDKDAVSCLIENTINTERFACES_ARTIFACTHANDLERINTERFACE_H_

#include "DavsCheckCallbackInterface.h"
#include "DavsDownloadCallbackInterface.h"
#include "acsdkAssetsInterfaces/DavsRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davsInterfaces {

class ArtifactHandlerInterface {
public:
    virtual ~ArtifactHandlerInterface() = default;

    /**
     * Register an artifact to be checked, downloaded in requested, and maintained. This means that if an artifact is
     * registered, the Artifact Handler will perform regular checks when the expiry is reached to ensure that the
     * artifact is up to date.
     *
     * @param artifactRequest REQUIRED, a valid request containing information for the artifact to be downloaded.
     * @param downloadCallback REQUIRED, a manager listener that will handle what to do with the artifact when its
     * downloaded or failed.
     * @param checkCallback REQUIRED, a manager listener that will handle checking if the artifact should be downloaded.
     * @param downloadImmediately REQUIRED, tell the manager to download immediately or on the next update interval.
     * @return uuid key for the artifact from davs client based on the given request, EMPTY string if registration
     * failed.
     */
    virtual std::string registerArtifact(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            std::shared_ptr<DavsDownloadCallbackInterface> downloadCallback,
            std::shared_ptr<DavsCheckCallbackInterface> checkCallback,
            bool downloadImmediately) = 0;

    /**
     * Deregister an artifact, this cancels any download that's already been started and removes the request from the
     * registration list.
     *
     * @param requestUUID REQUIRED, uuid of the request to be deregistered.
     */
    virtual void deregisterArtifact(const std::string& requestUUID) = 0;

    /**
     * Issues a single check and a download (if requested) of a given artifact which is discarded afterwards.
     *
     * @param artifactRequest REQUIRED, a valid request containing information for the artifact to be downloaded.
     * @param downloadCallback REQUIRED, a listener that will handle what to do with the artifact when its downloaded or
     * failed.
     * @param checkCallback REQUIRED, a listener that will handle checking if the artifact should be downloaded.
     * @return uuid key for the artifact from davs client based on the given request, EMPTY string if request failed.
     */
    virtual std::string downloadOnce(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            std::shared_ptr<DavsDownloadCallbackInterface> downloadCallback,
            std::shared_ptr<DavsCheckCallbackInterface> checkCallback) = 0;
    /**
     * Can set a downloadOnce artifact to auto update (like registerArtifact) or prevent an artifact from updating (like
     * downloadOnce).
     *
     * @param requestUUID REQUIRED, uuid of the request to be deregistered.
     * @param enable weather to enable auto update or disable it (the difference between registerArtifact and
     * downloadOnce).
     */
    virtual void enableAutoUpdate(const std::string& requestUUID, bool enable) = 0;
};

}  // namespace davsInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKDAVSCLIENTINTERFACES_ARTIFACTHANDLERINTERFACE_H_
