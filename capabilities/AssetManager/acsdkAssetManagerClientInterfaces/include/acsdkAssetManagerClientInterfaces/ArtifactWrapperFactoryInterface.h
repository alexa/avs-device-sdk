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

#ifndef ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTWRAPPERFACTORYINTERFACE_H_
#define ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTWRAPPERFACTORYINTERFACE_H_

#include <acsdkAssetManagerClientInterfaces/ArtifactWrapperInterface.h>
#include <acsdkAssetsInterfaces/ArtifactRequest.h>

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace clientInterfaces {

/**
 * This class is used to house the mechanism for creating the impl instances of ArtifactWrapperInterface.
 */
class ArtifactWrapperFactoryInterface {
public:
    virtual ~ArtifactWrapperFactoryInterface() = default;

    /**
     * Create a pointer to an instance that implements ArtifactWrapperInterface.
     * @param request REQUIRED artifact request used for creating the artifact wrapper.
     * @param updateValidator OPTIONAL update validator used to check on new artifacts once they're downloaded.
     * @return a new artifact wrapper instance, null in case of error.
     */
    virtual std::shared_ptr<clientInterfaces::ArtifactWrapperInterface> createArtifactWrapper(
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request,
            const std::shared_ptr<ArtifactUpdateValidator>& updateValidator = nullptr) = 0;
};

}  // namespace clientInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTWRAPPERFACTORYINTERFACE_H_
