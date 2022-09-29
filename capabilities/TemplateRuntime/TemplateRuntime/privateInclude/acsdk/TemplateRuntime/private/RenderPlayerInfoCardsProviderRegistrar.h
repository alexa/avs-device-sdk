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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_PRIVATEINCLUDE_ACSDK_TEMPLATERUNTIME_PRIVATE_RENDERPLAYERINFOCARDSPROVIDERREGISTRAR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_PRIVATEINCLUDE_ACSDK_TEMPLATERUNTIME_PRIVATE_RENDERPLAYERINFOCARDSPROVIDERREGISTRAR_H_

#include <mutex>

#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>

namespace alexaClientSDK {
namespace templateRuntime {

/**
 * Class to accumulate the set of @c RenderPlayerInfoCardsProvider instances for creating the TemplateRuntime CA.
 */
class RenderPlayerInfoCardsProviderRegistrar
        : public avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface {
public:
    /// @name RenderPlayerInfoCardsProviderRegistrarInterface methods.
    /// @{
    bool registerProvider(
        const std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>& provider) override;

    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>> getProviders()
        override;
    /// @}

private:
    /// Serializes access to @c m_providers.
    std::mutex m_mutex;

    /// The @c RenderPlayerInfoCardsProviderInterface instances to use for the TemplateRuntime CA.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>> m_providers;
};

}  // namespace templateRuntime
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_PRIVATEINCLUDE_ACSDK_TEMPLATERUNTIME_PRIVATE_RENDERPLAYERINFOCARDSPROVIDERREGISTRAR_H_
