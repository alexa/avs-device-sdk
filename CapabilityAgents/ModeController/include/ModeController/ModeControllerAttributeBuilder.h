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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MODECONTROLLER_INCLUDE_MODECONTROLLER_MODECONTROLLERATTRIBUTEBUILDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MODECONTROLLER_INCLUDE_MODECONTROLLER_MODECONTROLLERATTRIBUTEBUILDER_H_

#include <unordered_map>

#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerAttributeBuilderInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace modeController {

/**
 * This class implements @c ModeControllerAttributeBuilderInterface to build
 * @c ModeControllerAttributes.
 */
class ModeControllerAttributeBuilder
        : public avsCommon::sdkInterfaces::modeController::ModeControllerAttributeBuilderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ModeControllerAttributeBuilder() = default;

    /**
     * Create an instance of @c ModeControllerAttributeBuilder.
     *
     * @return Returns a new instance of @c ModeControllerAttributeBuilder.
     */
    static std::unique_ptr<ModeControllerAttributeBuilder> create();

    /// @name ModeControllerAttributeBuilderInterface Functions
    /// @{
    ModeControllerAttributeBuilder& withCapabilityResources(
        const avsCommon::avs::CapabilityResources& capabilityResources) override;
    ModeControllerAttributeBuilder& addMode(
        const std::string& mode,
        const avsCommon::sdkInterfaces::modeController::ModeResources& modeResources) override;
    ModeControllerAttributeBuilder& setOrdered(bool ordered) override;
    ModeControllerAttributeBuilder& withSemantics(
        const avsCommon::avs::capabilitySemantics::CapabilitySemantics& semantics) override;
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes> build() override;
    /// @}

private:
    /**
     * Constructor.
     */
    ModeControllerAttributeBuilder();

    /// Flag used to indicate whether any unrecoverable error was found.
    bool m_invalidAttribute;

    /// The capability resources represented using @c CapabilityResources.
    avsCommon::avs::CapabilityResources m_capabilityResources;

    /// Pair of Mode and its mode resources represented using @c ModeResources.
    std::unordered_map<std::string, avsCommon::sdkInterfaces::modeController::ModeResources> m_modes;

    /// Indicates whether modes in the controller are ordered or not.
    bool m_ordered;

    /// The semantics represented as an @c Optional @c CapabilitySemantics
    avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> m_semantics;
};

}  // namespace modeController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MODECONTROLLER_INCLUDE_MODECONTROLLER_MODECONTROLLERATTRIBUTEBUILDER_H_
