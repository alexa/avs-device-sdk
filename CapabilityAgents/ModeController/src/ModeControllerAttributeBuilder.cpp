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

#include "ModeController/ModeControllerAttributeBuilder.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace modeController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::modeController;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG{"ModeControllerAttributeBuilder"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<ModeControllerAttributeBuilder> ModeControllerAttributeBuilder::create() {
    return std::unique_ptr<ModeControllerAttributeBuilder>(new ModeControllerAttributeBuilder());
}

ModeControllerAttributeBuilder::ModeControllerAttributeBuilder() : m_invalidAttribute{false}, m_ordered{false} {
}

ModeControllerAttributeBuilder& ModeControllerAttributeBuilder::withCapabilityResources(
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

ModeControllerAttributeBuilder& ModeControllerAttributeBuilder::addMode(
    const std::string& mode,
    const ModeResources& modeResources) {
    ACSDK_DEBUG5(LX(__func__));
    if (mode.empty()) {
        ACSDK_ERROR(LX("addModeFailed").d("reason", "emptyMode"));
        m_invalidAttribute = true;
        return *this;
    }
    if (!modeResources.isValid()) {
        ACSDK_ERROR(LX("addModeFailed").d("reason", "invalidModeResources"));
        m_invalidAttribute = true;
        return *this;
    }
    if (m_modes.find(mode) != m_modes.end()) {
        ACSDK_ERROR(LX("addModeFailed").d("reason", "modeAlreadyExists").sensitive("mode", mode));
        m_invalidAttribute = true;
        return *this;
    }

    ACSDK_DEBUG5(LX(__func__).sensitive("mode", mode).sensitive("modeResources", modeResources.toJson()));

    m_modes[mode] = modeResources;
    return *this;
}

ModeControllerAttributeBuilder& ModeControllerAttributeBuilder::setOrdered(bool ordered) {
    ACSDK_DEBUG5(LX(__func__));
    m_ordered = ordered;
    return *this;
}

avsCommon::utils::Optional<ModeControllerAttributes> ModeControllerAttributeBuilder::build() {
    ACSDK_DEBUG5(LX(__func__));
    auto controllerAttribute = avsCommon::utils::Optional<ModeControllerAttributes>();
    if (m_invalidAttribute) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "invalidAttribute"));
        return controllerAttribute;
    }
    if (m_modes.size() == 0) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "modesNotProvided"));
        return controllerAttribute;
    }

    ACSDK_DEBUG5(LX(__func__).sensitive("capabilityResources", m_capabilityResources.toJson()));
    ACSDK_DEBUG5(LX(__func__).d("#modes", m_modes.size()));

    return avsCommon::utils::Optional<ModeControllerAttributes>({m_capabilityResources, m_modes, m_ordered});
}

}  // namespace modeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
