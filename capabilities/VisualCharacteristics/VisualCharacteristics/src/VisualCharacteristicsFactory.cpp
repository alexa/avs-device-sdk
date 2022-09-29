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

#include "acsdk/VisualCharacteristics/VisualCharacteristicsFactory.h"

#include "acsdk/VisualCharacteristics/private/VisualCharacteristics.h"

namespace alexaClientSDK {
namespace visualCharacteristics {
avsCommon::utils::Optional<VisualCharacteristicsFactory::VisualCharacteristicsExports> VisualCharacteristicsFactory::
    create(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&
            exceptionEncounteredSender) {
    auto visualCharacteristics = VisualCharacteristics::create(contextManager, exceptionEncounteredSender);
    if (!visualCharacteristics) {
        return avsCommon::utils::Optional<VisualCharacteristicsExports>();
    }

    VisualCharacteristicsExports exports = {
        visualCharacteristics, visualCharacteristics, visualCharacteristics, visualCharacteristics};
    return exports;
}
}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
