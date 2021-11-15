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

#ifndef ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTCHANGEOBSERVER_H_
#define ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTCHANGEOBSERVER_H_

#include "acsdkAssetsInterfaces/ArtifactRequest.h"
#include "acsdkAssetsInterfaces/State.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace clientInterfaces {

/**
 * Observer used for communicating artifact state changes.
 */
class ArtifactChangeObserver {
public:
    virtual void stateChanged(
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request,
            commonInterfaces::State newState) = 0;
    virtual ~ArtifactChangeObserver() = default;
};

}  // namespace clientInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENTINTERFACES_ARTIFACTCHANGEOBSERVER_H_
