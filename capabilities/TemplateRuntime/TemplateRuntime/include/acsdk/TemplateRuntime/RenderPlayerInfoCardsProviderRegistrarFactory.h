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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_INCLUDE_ACSDK_TEMPLATERUNTIME_RENDERPLAYERINFOCARDSPROVIDERREGISTRARFACTORY_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_INCLUDE_ACSDK_TEMPLATERUNTIME_RENDERPLAYERINFOCARDSPROVIDERREGISTRARFACTORY_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderRegistrarInterface.h>

namespace alexaClientSDK {
namespace templateRuntime {

/**
 * Create a registrar for components that provides an update for renderPlayerInfo card related changes.
 *
 * @return A new instance of @c RenderPlayerInfoCardsProviderRegistrarInterface.
 */
std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>
createRenderPlayerInfoCardsProviderRegistrarInterface();

}  // namespace templateRuntime
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TEMPLATERUNTIME_TEMPLATERUNTIME_INCLUDE_ACSDK_TEMPLATERUNTIME_RENDERPLAYERINFOCARDSPROVIDERREGISTRARFACTORY_H_
