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

#ifndef ALEXA_CLIENT_SDK_SYNCHRONIZESTATESENDER_INCLUDE_SYNCHRONIZESTATESENDER_SYNCHRONIZESTATESENDERFACTORY_H_
#define ALEXA_CLIENT_SDK_SYNCHRONIZESTATESENDER_INCLUDE_SYNCHRONIZESTATESENDER_SYNCHRONIZESTATESENDERFACTORY_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>

namespace alexaClientSDK {
namespace synchronizeStateSender {

/**
 * Factory class to generate new instances of @c PostConnectSynchronizeStateSender.
 */
class SynchronizeStateSenderFactory : public avsCommon::sdkInterfaces::PostConnectOperationProviderInterface {
public:
    /**
     * Creates a new instance of the @c SynchronizeStateSenderFactory.
     *
     * @param contextManager The @c ContextManager used to construct the synchronize state sender.
     * @return a new instance of the @c SynchronizeStateSenderFactory.
     */
    static std::shared_ptr<SynchronizeStateSenderFactory> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// ContextRequesterInterface Methods.
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface> createPostConnectOperation() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param contextManager The @c ContextManager used to construct the synchronize state sender.
     */
    SynchronizeStateSenderFactory(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// The @c ContextManager used in the construction of the @c PostConnectSynchronizeStateSender.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;
};

}  // namespace synchronizeStateSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SYNCHRONIZESTATESENDER_INCLUDE_SYNCHRONIZESTATESENDER_SYNCHRONIZESTATESENDERFACTORY_H_
