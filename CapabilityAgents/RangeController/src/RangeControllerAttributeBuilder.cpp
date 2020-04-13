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

#include "RangeController/RangeControllerAttributeBuilder.h"

#include <cmath>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace rangeController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::rangeController;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG{"RangeControllerAttributeBuilder"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<RangeControllerAttributeBuilder> RangeControllerAttributeBuilder::create() {
    return std::unique_ptr<RangeControllerAttributeBuilder>(new RangeControllerAttributeBuilder());
}

RangeControllerAttributeBuilder::RangeControllerAttributeBuilder() :
        m_invalidAttribute{false},
        m_unitOfMeasure{Optional<resources::AlexaUnitOfMeasure>()} {
}

RangeControllerAttributeBuilder& RangeControllerAttributeBuilder::withCapabilityResources(
    const CapabilityResources& capabilityResources) {
    ACSDK_DEBUG5(LX(__func__));
    if (!capabilityResources.isValid()) {
        ACSDK_ERROR(LX("withCapabilityResourcesFailed").d("reason", "invalidCapabilityResources"));
        m_invalidAttribute = true;
        return *this;
    }

    m_capabilityResources = capabilityResources;
    return *this;
}

RangeControllerAttributeBuilder& RangeControllerAttributeBuilder::withUnitOfMeasure(
    const resources::AlexaUnitOfMeasure& unitOfMeasure) {
    ACSDK_DEBUG5(LX(__func__));
    if (unitOfMeasure.empty()) {
        ACSDK_ERROR(LX("withUnitOfMeasureFailed").d("reason", "invalidUnitOfMeasure"));
        m_invalidAttribute = true;
        return *this;
    }

    m_unitOfMeasure = Optional<resources::AlexaUnitOfMeasure>(unitOfMeasure);
    return *this;
}

RangeControllerAttributeBuilder& RangeControllerAttributeBuilder::addPreset(
    const std::pair<double, PresetResources>& preset) {
    ACSDK_DEBUG5(LX(__func__));
    if (!preset.second.isValid()) {
        ACSDK_ERROR(LX("addPresetFailed").d("reason", "invalidPresetResources"));
        m_invalidAttribute = true;
        return *this;
    }

    ACSDK_DEBUG5(LX(__func__).sensitive("preset", preset.first).sensitive("presetResources", preset.second.toJson()));

    m_presets.push_back(preset);
    return *this;
}

avsCommon::utils::Optional<RangeControllerAttributes> RangeControllerAttributeBuilder::build() {
    ACSDK_DEBUG5(LX(__func__));
    avsCommon::utils::Optional<RangeControllerAttributes> controllerAttribute;
    if (m_invalidAttribute) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "invalidAttribute"));
        return controllerAttribute;
    }

    ACSDK_DEBUG5(LX(__func__).sensitive("capabilityResources", m_capabilityResources.toJson()));
    ACSDK_DEBUG5(LX(__func__).sensitive("unitOfMeasure", m_unitOfMeasure.valueOr("")));
    ACSDK_DEBUG5(LX(__func__).d("#presets", m_presets.size()));

    return avsCommon::utils::Optional<RangeControllerAttributes>({m_capabilityResources, m_unitOfMeasure, m_presets});
}

}  // namespace rangeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
