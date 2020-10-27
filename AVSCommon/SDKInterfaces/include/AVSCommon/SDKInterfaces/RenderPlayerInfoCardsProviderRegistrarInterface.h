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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSPROVIDERREGISTRARINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSPROVIDERREGISTRARINTERFACE_H_

#include <memory>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class provides an interface to register providers of the @c RenderPlayerInfoCardsProviderInterface.
 */
class RenderPlayerInfoCardsProviderRegistrarInterface {
public:
    /**
     * Destructor
     */
    virtual ~RenderPlayerInfoCardsProviderRegistrarInterface() = default;

    /**
     * Add a new @c RenderPlayerInfoCardsProviderInterface instance to provide cards when creating a TemplateRuntime CA.
     *
     * @param provider The @c RenderPlayerInfoCardsProviderInterface instance to add.
     */
    virtual bool registerProvider(
        const std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>& provider) = 0;

    /**
     * Get the set of @c RenderPlayerInfoCardsProviderInterface instances to be invoked when creating a
     * TemplateRuntime CA.
     *
     * @return A vector of @c RenderPlayerInfoCardsProviderInterface instances.
     */
    virtual std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface>>
    getProviders() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSPROVIDERREGISTRARINTERFACE_H_
