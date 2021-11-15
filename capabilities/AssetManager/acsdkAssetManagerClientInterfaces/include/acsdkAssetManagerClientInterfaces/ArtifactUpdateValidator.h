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

#ifndef ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTUPDATEVALIDATOR_H_
#define ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTUPDATEVALIDATOR_H_

#include "acsdkAssetsInterfaces/ArtifactRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace clientInterfaces {

/**
 * Used to validate that the new path for the given artifact is good to keep.
 */
class ArtifactUpdateValidator {
public:
    virtual ~ArtifactUpdateValidator() = default;

    /**
     * Providing a new path, check to see if the content of the update is valid and usable.
     *
     * @param newPath of the potential updated artifact.
     * @return true if valid, false otherwise.
     */
    virtual bool validateUpdate(
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request,
            const std::string& newPath) = 0;
};

}  // namespace clientInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTUPDATEVALIDATOR_H_
