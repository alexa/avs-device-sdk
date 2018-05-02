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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZERFACTORY_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZERFACTORY_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>

#include "ACL/Transport/PostConnectFactoryInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Factory for creating PostConnectSynchronizer instances.
 */
class PostConnectSynchronizerFactory : public PostConnectFactoryInterface {
public:
    /**
     * Create a PostConnectSynchronizerFactory instance.
     *
     * @param contextManager The @c ContextManager which will provide the context for @c SynchronizeState events.
     * @return A PostConnectSynchronizer instance.
     */
    static std::shared_ptr<PostConnectSynchronizerFactory> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// @name PostConnectFactoryInterface methods
    /// @{
    std::shared_ptr<PostConnectInterface> createPostConnect() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param contextManager The @c ContextManager which will provide the context for @c SynchronizeState events.
     */
    PostConnectSynchronizerFactory(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// The @c ContextManager which will provide the context for @c SynchronizeState events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZERFACTORY_H_
