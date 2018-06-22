/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "ACL/Transport/PostConnectObject.h"
#include "ACL/Transport/PostConnectSynchronizer.h"
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;

/// Class static definition of context-manager.
std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> PostConnectObject::m_contextManager = nullptr;

/// String to identify log entries originating from this file.
static const std::string TAG("PostConnect");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/*
 * Static method to initialize the context manager.
 *
 * @param contextManager The contextManager instance to initialize with.
 */
void PostConnectObject::init(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager) {
    m_contextManager = contextManager;
}

/**
 * Method that creates post-connect object.
 */
std::shared_ptr<PostConnectObject> PostConnectObject::create() {
    if (!m_contextManager) {
        ACSDK_ERROR(LX("postCreateFailed").d("reason", "contextManagerNullData"));
        return nullptr;
    }

    return std::shared_ptr<PostConnectObject>(new PostConnectSynchronizer());
}

PostConnectObject::PostConnectObject() : RequiresShutdown{"PostConnectObject"} {
}

}  // namespace acl
}  // namespace alexaClientSDK
