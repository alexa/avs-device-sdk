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

#ifndef ACSDKDAVSCLIENTINTERFACES_DAVSDOWNLOADCALLBACKINTERFACE_H_
#define ACSDKDAVSCLIENTINTERFACES_DAVSDOWNLOADCALLBACKINTERFACE_H_

#include "acsdkAssetsInterfaces/ResultCode.h"
#include "acsdkAssetsInterfaces/VendableArtifact.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davsInterfaces {

class DavsDownloadCallbackInterface {
public:
    virtual ~DavsDownloadCallbackInterface() = default;

    /**
     * An event that is called as soon as the download has started.
     */
    virtual void onStart() = 0;

    /**
     * An event that is called as soon as the artifact has been downloaded successfully. The manager will receive
     * metadata info about the artifact as well as the path of where to find the artifact on disk.
     *
     * It is the manager's responsibility to move the artifact from the specified location and maintain its lifecycle
     * thereafter. If the artifact is not moved, it will be DELETED.
     *
     * @param artifact ALWAYS VALID, information about the artifact including the original request.
     * @param path ALWAYS VALID, path of where to find the artifact on disk.
     */
    virtual void onArtifactDownloaded(
            std::shared_ptr<commonInterfaces::VendableArtifact> artifact,
            const std::string& path) = 0;

    /**
     * An event that is called when the download fails, providing a reason for failure.
     *
     * @param errorCode reason for the failure.
     */
    virtual void onDownloadFailure(commonInterfaces::ResultCode errorCode) = 0;

    /**
     * An event that is called periodically to denote the progress of the download.
     *
     * @param progress ALWAYS VALID, between 0 and 100
     */
    virtual void onProgressUpdate(int progress) = 0;
};

}  // namespace davsInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKDAVSCLIENTINTERFACES_DAVSDOWNLOADCALLBACKINTERFACE_H_
