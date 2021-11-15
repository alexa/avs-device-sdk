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

#ifndef ACSDKASSETMANAGERCLIENT_ARTIFACTWRAPPERFACTORY_H_
#define ACSDKASSETMANAGERCLIENT_ARTIFACTWRAPPERFACTORY_H_

#include <acsdkAssetManagerClientInterfaces/ArtifactWrapperInterface.h>
#include <acsdkAssetsInterfaces/ArtifactRequest.h>
#include <acsdkAssetsInterfaces/Communication/AmdCommunicationInterface.h>

#include <memory>
#include <string>

#include "acsdkAssetManagerClientInterfaces/ArtifactWrapperFactoryInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

/**
 * This interface provides a mechanism for controlling artifacts in asset manager through the Communication Interface.
 * This corresponds with a one to one mapping of ArtifactWrapperInterface to either a davs or url request.
 */
class ArtifactWrapperFactory : public clientInterfaces::ArtifactWrapperFactoryInterface {
public:
    ~ArtifactWrapperFactory() override = default;

    static std::shared_ptr<ArtifactWrapperFactory> create(
            std::shared_ptr<commonInterfaces::AmdCommunicationInterface> amdComm);

    std::shared_ptr<clientInterfaces::ArtifactWrapperInterface> createArtifactWrapper(
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request,
            const std::shared_ptr<clientInterfaces::ArtifactUpdateValidator>& updateValidator = nullptr) override;

private:
    explicit ArtifactWrapperFactory(std::shared_ptr<commonInterfaces::AmdCommunicationInterface> amdComm);

private:
    std::shared_ptr<commonInterfaces::AmdCommunicationInterface> m_amdComm;
};

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENT_ARTIFACTWRAPPERFACTORY_H_
