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

#include "ToggleController/ToggleControllerAttributeBuilder.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace toggleController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::toggleController;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
#define TAG "ToggleControllerAttributeBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<ToggleControllerAttributeBuilder> ToggleControllerAttributeBuilder::create() {
    return std::unique_ptr<ToggleControllerAttributeBuilder>(new ToggleControllerAttributeBuilder());
}

ToggleControllerAttributeBuilder::ToggleControllerAttributeBuilder() :
        m_invalidAttribute{false},
        m_semantics(Optional<capabilitySemantics::CapabilitySemantics>()) {
}

ToggleControllerAttributeBuilder& ToggleControllerAttributeBuilder::withCapabilityResources(
    const avsCommon::avs::CapabilityResources& capabilityResources) {
    ACSDK_DEBUG5(LX(__func__));
    if (!capabilityResources.isValid()) {
        ACSDK_ERROR(LX("withCapabilityResourcesFailed").d("reason", "invalidCapabilityResources"));
        m_invalidAttribute = true;
        return *this;
    }
    m_capabilityResources = capabilityResources;
    return *this;
}

ToggleControllerAttributeBuilder& ToggleControllerAttributeBuilder::withSemantics(
    const capabilitySemantics::CapabilitySemantics& semantics) {
    if (!semantics.isValid()) {
        ACSDK_ERROR(LX("withSemanticsFailed").d("reason", "invalidSemantics"));
        m_invalidAttribute = true;
        return *this;
    }
    m_semantics = avsCommon::utils::Optional<capabilitySemantics::CapabilitySemantics>(semantics);
    return *this;
}

avsCommon::utils::Optional<ToggleControllerAttributes> ToggleControllerAttributeBuilder::build() {
    ACSDK_DEBUG5(LX(__func__));
    if (m_invalidAttribute) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "invalidAttribute"));
        return avsCommon::utils::Optional<ToggleControllerAttributes>();
    }

    ACSDK_DEBUG5(LX(__func__).sensitive("capabilityResources", m_capabilityResources.toJson()));
    return avsCommon::utils::Optional<ToggleControllerAttributes>({m_capabilityResources, m_semantics});
}

}  // namespace toggleController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
