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

#ifndef ACSDKDAVSCLIENTINTERFACES_DAVSCHECKCALLBACKINTERFACE_H_
#define ACSDKDAVSCLIENTINTERFACES_DAVSCHECKCALLBACKINTERFACE_H_

#include "acsdkAssetsInterfaces/ResultCode.h"
#include "acsdkAssetsInterfaces/VendableArtifact.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davsInterfaces {

class DavsCheckCallbackInterface {
public:
    virtual ~DavsCheckCallbackInterface() = default;

    /**
     * An event that is called after check to see if the manager would like to download the given artifact.
     * It is the manager's responsibility to check the exiting artifact against the one being checked.
     *    If it is the same then the requester should specify to not download.
     *    If it is different, then the requester should specify to download.
     *    If the insufficientSpace is true, then the requester should free the necessary space for the artifact.
     *
     * @param artifact ALWAYS VALID, information about the artifact including the original request.
     * @param freeSpaceNeeded amount of space needed to be freed to make room for this artifact, 0 if existing space is
     * sufficient.
     * @return should we download the artifact? return true to download, false to not download.
     */
    virtual bool checkIfOkToDownload(
            std::shared_ptr<commonInterfaces::VendableArtifact> artifact,
            size_t freeSpaceNeeded) = 0;

    /**
     * An event that is called when the check failed with a specific reason.
     *
     * @param errorCode reason for the failure.
     */
    virtual void onCheckFailure(commonInterfaces::ResultCode errorCode) = 0;
};

}  // namespace davsInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKDAVSCLIENTINTERFACES_DAVSCHECKCALLBACKINTERFACE_H_
