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

#include "acsdk/TemplateRuntime/TemplateRuntimeFactory.h"

#include "acsdk/TemplateRuntime/private/TemplateRuntime.h"

namespace alexaClientSDK {
namespace templateRuntime {

using namespace avsCommon::sdkInterfaces;

avsCommon::utils::Optional<TemplateRuntimeFactory::TemplateRuntimeAgentData> TemplateRuntimeFactory::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>&
        renderPlayerInfoCardsProviderRegistrar,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
    const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
        endpointCapabilitiesRegistrar) {
    auto templateRuntime =
        TemplateRuntime::create(renderPlayerInfoCardsProviderRegistrar->getProviders(), exceptionSender);
    endpointCapabilitiesRegistrar->withCapability(templateRuntime, templateRuntime);
    TemplateRuntimeFactory::TemplateRuntimeAgentData templateRuntimeAgentData;
    if (templateRuntime) {
        templateRuntimeAgentData.templateRuntime = templateRuntime;
        templateRuntimeAgentData.requiresShutdown = templateRuntime;

        return avsCommon::utils::Optional<TemplateRuntimeFactory::TemplateRuntimeAgentData>(templateRuntimeAgentData);
    }

    return avsCommon::utils::Optional<TemplateRuntimeFactory::TemplateRuntimeAgentData>();
}

}  // namespace templateRuntime
}  // namespace alexaClientSDK
