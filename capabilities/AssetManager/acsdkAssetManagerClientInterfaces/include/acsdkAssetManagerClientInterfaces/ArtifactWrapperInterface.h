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

#ifndef ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTWRAPPERINTERFACE_H_
#define ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTWRAPPERINTERFACE_H_

#include <acsdkAssetManagerClientInterfaces/ArtifactChangeObserver.h>
#include <acsdkAssetManagerClientInterfaces/ArtifactUpdateValidator.h>
#include <acsdkAssetsInterfaces/ArtifactRequest.h>
#include <acsdkAssetsInterfaces/Priority.h>

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace clientInterfaces {

/**
 * This interface provides a mechanism for controlling artifacts in asset manager through the Communication Interface.
 * This corresponds with a one to one mapping of ArtifactWrapperInterface to either a davs or url request.
 */
class ArtifactWrapperInterface {
public:
    virtual ~ArtifactWrapperInterface() = default;

    /**
     * @return unique name identifying this artifact wrapper.
     */
    virtual std::string name() const = 0;

    /**
     * Requests the download of the artifact referenced by this wrapper if not already downloaded or downloading.
     *
     * @return true if the request was submitted successfully, false otherwise.
     */
    virtual bool download() const = 0;

    /**
     * @return true if the artifact is already downloaded and ready.
     */
    virtual bool isAvailable() const = 0;

    /**
     * @return if the artifact is being created, requested, or downloading.
     */
    virtual bool isPending() const = 0;

    /**
     * @return the request used to identify this artifact.
     */
    virtual std::shared_ptr<commonInterfaces::ArtifactRequest> getRequest() const = 0;

    /**
     * @return the path where to find the artifact on disk using aipc.
     */
    virtual std::string getPath() const = 0;

    /**
     * @return gets the current artifact priority using aipc.
     */
    virtual commonInterfaces::Priority getPriority() const = 0;

    /**
     * Sets the priority accordingly.
     *
     * @return true if successful, false otherwise.
     */
    virtual bool setPriority(commonInterfaces::Priority priority) = 0;

    /**
     * Requests the removal and cleanup of the given artifact.
     */
    virtual void erase() = 0;

    /**
     * Add observer to the state changes of this artifact. This will hold a weak_ptr to the observer.
     *
     * @param observer to be added.
     */
    virtual void addWeakPtrObserver(const std::weak_ptr<ArtifactChangeObserver>& observer) = 0;

    /**
     * Remove the observer from this artifact.
     *
     * @param observer to be removed.
     */
    virtual void removeWeakPtrObserver(const std::weak_ptr<ArtifactChangeObserver>& observer) = 0;
};

}  // namespace clientInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTWRAPPERINTERFACE_H_
