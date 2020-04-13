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

#include <set>

#include "ACL/Transport/PostConnectSequencer.h"
#include "ACL/Transport/PostConnectSequencerFactory.h"

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;
/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectSequencerFactory");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PostConnectSequencerFactory> PostConnectSequencerFactory::create(
    const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& postConnectOperationProviders) {
    for (auto& provider : postConnectOperationProviders) {
        if (!provider) {
            ACSDK_ERROR(LX("createFailed").d("reason", "invalidProviderFound"));
            return nullptr;
        }
    }
    return std::shared_ptr<PostConnectSequencerFactory>(new PostConnectSequencerFactory(postConnectOperationProviders));
}

PostConnectSequencerFactory::PostConnectSequencerFactory(
    const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& postConnectOperationProviders) :
        m_postConnectOperationProviders{postConnectOperationProviders} {
}

std::shared_ptr<PostConnectInterface> PostConnectSequencerFactory::createPostConnect() {
    PostConnectSequencer::PostConnectOperationsSet postConnectOperationsSet;
    for (auto& provider : m_postConnectOperationProviders) {
        auto postConnectOperation = provider->createPostConnectOperation();
        if (postConnectOperation) {
            postConnectOperationsSet.insert(postConnectOperation);
        }
    }
    return PostConnectSequencer::create(postConnectOperationsSet);
}

}  // namespace acl
}  // namespace alexaClientSDK
