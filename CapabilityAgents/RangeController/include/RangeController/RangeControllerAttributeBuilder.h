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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_RANGECONTROLLER_INCLUDE_RANGECONTROLLER_RANGECONTROLLERATTRIBUTEBUILDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_RANGECONTROLLER_INCLUDE_RANGECONTROLLER_RANGECONTROLLERATTRIBUTEBUILDER_H_

#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerAttributeBuilderInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace rangeController {

/**
 * This class implements @c RangeControllerAttributeBuilderInterface to build
 * @c RangeControllerAttributes.
 */
class RangeControllerAttributeBuilder
        : public avsCommon::sdkInterfaces::rangeController::RangeControllerAttributeBuilderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~RangeControllerAttributeBuilder() = default;

    /**
     * Create an instance of @c RangeControllerAttributeBuilder.
     *
     * @return Returns a new instance of @c RangeControllerAttributeBuilder.
     */
    static std::unique_ptr<RangeControllerAttributeBuilder> create();

    /// @name RangeControllerAttributeBuilderInterface Functions
    /// @{
    RangeControllerAttributeBuilder& withCapabilityResources(
        const avsCommon::avs::CapabilityResources& capabilityResources) override;
    RangeControllerAttributeBuilder& withUnitOfMeasure(
        const avsCommon::avs::resources::AlexaUnitOfMeasure& unitOfMeasure) override;
    RangeControllerAttributeBuilder& addPreset(
        const std::pair<double, avsCommon::sdkInterfaces::rangeController::PresetResources>& preset) override;
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes> build() override;
    /// @}

private:
    /**
     * Constructor.
     */
    RangeControllerAttributeBuilder();

    /// Flag used to indicate whether any unrecoverable error was found.
    bool m_invalidAttribute;

    /// The capability resources represented using @c CapabilityResources.
    avsCommon::avs::CapabilityResources m_capabilityResources;

    /// The unit of measure of the range value.
    avsCommon::utils::Optional<avsCommon::avs::resources::AlexaUnitOfMeasure> m_unitOfMeasure;

    /**
     * A vector of a pair, that includes the value of a preset using @c double and its preset resources using @c
     * PresetResources.
     */
    std::vector<std::pair<double, avsCommon::sdkInterfaces::rangeController::PresetResources>> m_presets;
};

}  // namespace rangeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_RANGECONTROLLER_INCLUDE_RANGECONTROLLER_RANGECONTROLLERATTRIBUTEBUILDER_H_
