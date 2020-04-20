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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSEQUENCERFACTORY_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSEQUENCERFACTORY_H_

#include <memory>
#include <vector>

#include "ACL/Transport/PostConnectFactoryInterface.h"
#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>

namespace alexaClientSDK {
namespace acl {

/**
 * Factory class to create a new instance of the @c PostConnectSequencer.
 */
class PostConnectSequencerFactory : public PostConnectFactoryInterface {
public:
    /// Alias for @c PostConnectOperationProviderInterface.
    using PostConnectOperationProviderInterface = avsCommon::sdkInterfaces::PostConnectOperationProviderInterface;

    /**
     * Creates a new instance of the @c PostConnectSequencer.
     *
     * @param postConnectOperationProviders The vector of @c PostConnectOperationProviders.
     * @return a new instance of the @c PostConnectSequencer.
     */
    static std::shared_ptr<PostConnectSequencerFactory> create(
        const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& postConnectOperationProviders);

    /// @name PostConnectFactoryInterface methods
    /// @{
    std::shared_ptr<PostConnectInterface> createPostConnect() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param postConnectOperationProviders The vector of @c PostConnectOperationProviders.
     */
    PostConnectSequencerFactory(
        const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& postConnectOperationProviders);

    /// The vector of @c PostConnectOperationProviders.
    std::vector<std::shared_ptr<PostConnectOperationProviderInterface>> m_postConnectOperationProviders;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSEQUENCERFACTORY_H_
