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

#ifndef ACSDKPOSTCONNECTOPERATIONPROVIDERREGISTRARINTERFACES_POSTCONNECTOPERATIONPROVIDERREGISTRARINTERFACE_H_
#define ACSDKPOSTCONNECTOPERATIONPROVIDERREGISTRARINTERFACES_POSTCONNECTOPERATIONPROVIDERREGISTRARINTERFACE_H_

#include <memory>
#include <vector>

#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkPostConnectOperationProviderRegistrarInterfaces {

/**
 * Class to accumulate the set of @c PostConnectOperationProviderInterface instances to be invoked
 * when creating a connection to AVS.
 */
class PostConnectOperationProviderRegistrarInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PostConnectOperationProviderRegistrarInterface() = default;

    /**
     * Add a new @c PostConnectOperationProviderInterface instances to be invoked when creating a connection to AVS.
     *
     * @param provider The @c PostConnectOperationProviderInterface instance to add.
     * @return Whether the operation was successful.  This operation will fail if this is called after startup.
     */
    virtual bool registerProvider(
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>& provider) = 0;

    /**
     * Get the set of @c PostConnectOperationProviderInterface instances to be invoked when creating a
     * connection to AVS.
     *
     * @return An @c Optional vector of @c PostConnectOperationProviderInterface instances. If this method is
     * invoked before startup the @c Optional object returned will have not value.
     */
    virtual avsCommon::utils::Optional<
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>>>
    getProviders() = 0;
};

}  // namespace acsdkPostConnectOperationProviderRegistrarInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKPOSTCONNECTOPERATIONPROVIDERREGISTRARINTERFACES_POSTCONNECTOPERATIONPROVIDERREGISTRARINTERFACE_H_
