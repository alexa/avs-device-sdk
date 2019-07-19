/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <memory>

#include "AVSCommon/Utils/Logger/Logger.h"

#include "ACL/Transport/PostConnectSynchronizer.h"
#include "ACL/Transport/PostConnectSynchronizerFactory.h"

namespace alexaClientSDK {
namespace acl {

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnectSynchronizerFactory");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PostConnectSynchronizerFactory> PostConnectSynchronizerFactory::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFactoryFailed").d("reason", "nullContextManager"));
        return nullptr;
    }

    return std::shared_ptr<PostConnectSynchronizerFactory>(new PostConnectSynchronizerFactory(contextManager));
}

std::shared_ptr<PostConnectInterface> PostConnectSynchronizerFactory::createPostConnect() {
    return PostConnectSynchronizer::create(m_contextManager);
}

PostConnectSynchronizerFactory::PostConnectSynchronizerFactory(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) :
        m_contextManager{contextManager} {
}

}  // namespace acl
}  // namespace alexaClientSDK
