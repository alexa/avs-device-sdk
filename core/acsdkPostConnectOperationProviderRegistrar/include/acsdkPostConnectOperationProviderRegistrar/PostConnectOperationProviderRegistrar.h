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

#ifndef ACSDKPOSTCONNECTOPERATIONPROVIDERREGISTRAR_POSTCONNECTOPERATIONPROVIDERREGISTRAR_H_
#define ACSDKPOSTCONNECTOPERATIONPROVIDERREGISTRAR_POSTCONNECTOPERATIONPROVIDERREGISTRAR_H_

#include <mutex>
#include <vector>

#include <acsdkPostConnectOperationProviderRegistrarInterfaces/PostConnectOperationProviderRegistrarInterface.h>
#include <acsdkStartupManagerInterfaces/RequiresStartupInterface.h>
#include <acsdkStartupManagerInterfaces/StartupNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>

namespace alexaClientSDK {
namespace acsdkPostConnectOperationProviderRegistrar {

/**
 * Class to accumulate the set of @c PostConnectOperationProviderInterface instances to be invoked
 * when creating a connection to AVS.
 */
class PostConnectOperationProviderRegistrar
        : public acsdkStartupManagerInterfaces::RequiresStartupInterface
        , public acsdkPostConnectOperationProviderRegistrarInterfaces::PostConnectOperationProviderRegistrarInterface {
public:
    /**
     * Create a new instance of @c PostConnectOperationProviderRegistrar.
     *
     * @param startupNotifier The object to register with the receive startup notifications.
     * @return A new instance of @c PostConnectOperationProviderRegistrar.
     */
    static std::shared_ptr<
        acsdkPostConnectOperationProviderRegistrarInterfaces::PostConnectOperationProviderRegistrarInterface>
    createPostConnectOperationProviderRegistrarInterface(
        const std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface>& startupNotifier);

    /// @name PostConnectOperationProviderRegistrarInterface methods.
    /// @{
    bool registerProvider(
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>& provider) override;
    avsCommon::utils::Optional<
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>>>
    getProviders() override;
    /// @}

    /// @name RequiresStartupInterface methods
    /// @{
    bool startup() override;
    /// @}

private:
    /**
     * Constructor.
     */
    PostConnectOperationProviderRegistrar();

    /// Serializes access to @c m_onStartupHasBeenCalled and @c m_providers.
    std::mutex m_mutex;

    /// Whether startup() has been called.
    bool m_onStartupHasBeenCalled;

    /// The @c PostConnectOperationProviderInterface instances to be invoked when creating a connection to AVS.
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> m_providers;
};

}  // namespace acsdkPostConnectOperationProviderRegistrar
}  // namespace alexaClientSDK

#endif  // ACSDKPOSTCONNECTOPERATIONPROVIDERREGISTRAR_POSTCONNECTOPERATIONPROVIDERREGISTRAR_H_
