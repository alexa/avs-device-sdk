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

#include "acsdkAssetManagerClient/ArtifactWrapperFactory.h"
#include "acsdkAssetManagerClient/ArtifactWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

using namespace std;
using namespace commonInterfaces;
using namespace clientInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"ArtifactWrapperFactory"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<ArtifactWrapperFactory> ArtifactWrapperFactory::create(shared_ptr<AmdCommunicationInterface> amdComm) {
    if (amdComm == nullptr) {
        ACSDK_ERROR(LX("create").m("Null AmdCommunicationInterface"));
        return nullptr;
    }

    return shared_ptr<ArtifactWrapperFactory>(new ArtifactWrapperFactory(move(amdComm)));
}

ArtifactWrapperFactory::ArtifactWrapperFactory(shared_ptr<AmdCommunicationInterface> amdComm) :
        m_amdComm(move(amdComm)) {
}
shared_ptr<ArtifactWrapperInterface> ArtifactWrapperFactory::createArtifactWrapper(
        const shared_ptr<commonInterfaces::ArtifactRequest>& request,
        const shared_ptr<ArtifactUpdateValidator>& updateValidator) {
    return ArtifactWrapper::create(m_amdComm, request, updateValidator);
}

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK