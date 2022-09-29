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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIMEINTERFACES_INCLUDE_ACSDK_TEMPLATERUNTIMEINTERFACES_TEMPLATERUNTIMEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIMEINTERFACES_INCLUDE_ACSDK_TEMPLATERUNTIMEINTERFACES_TEMPLATERUNTIMEINTERFACE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>

#include "TemplateRuntimeObserverInterface.h"

namespace alexaClientSDK {
namespace templateRuntimeInterfaces {

/**
 * This class defines a contract that an implementation of TemplateRuntime capability must fulfill. Application
 * is expected to interact with TemplateRuntime by registering its observers and subscribe to GUI components that
 * may provide user activity for playerInfo card.
 */
class TemplateRuntimeInterface {
public:
    /**
     * Destructor
     */
    virtual ~TemplateRuntimeInterface() = default;

    /**
     * This function adds an observer to @c TemplateRuntime so that it will get notified for renderTemplateCard or
     * renderPlayerInfoCard.
     *
     * @param observer The @c TemplateRuntimeObserverInterface
     */
    virtual void addObserver(std::weak_ptr<TemplateRuntimeObserverInterface> observer) = 0;

    /**
     * This function removes an observer from @c TemplateRuntime so that it will no longer be notified of
     * renderTemplateCard or renderPlayerInfoCard callbacks.
     *
     * @param observer The @c TemplateRuntimeObserverInterface
     */
    virtual void removeObserver(std::weak_ptr<TemplateRuntimeObserverInterface> observer) = 0;

    /**
     * This function adds a @RenderPlayerInfoCardsProviderInterface for a client to subscribe @c TemplateRuntime as an
     * observer of changes for RenderPlayerInfoCards.
     *
     * @param cardsProvider The @c RenderPlayerInfoCardsProviderInterface
     */
    virtual void addRenderPlayerInfoCardsProvider(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderInterface> cardsProvider) = 0;
};

}  // namespace templateRuntimeInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIMEINTERFACES_INCLUDE_ACSDK_TEMPLATERUNTIMEINTERFACES_TEMPLATERUNTIMEINTERFACE_H_
