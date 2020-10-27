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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TOGGLECONTROLLER_INCLUDE_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTEBUILDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TOGGLECONTROLLER_INCLUDE_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTEBUILDER_H_

#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerAttributeBuilderInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace toggleController {

/**
 * This class implements @c ToggleControllerAttributeBuilderInterface to build
 * @c ToggleControllerAttributes.
 */
class ToggleControllerAttributeBuilder
        : public avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributeBuilderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ToggleControllerAttributeBuilder() = default;

    /**
     * Create an instance of @c ToggleControllerAttributeBuilder.
     *
     * @return Returns a new instance of @c ToggleControllerAttributeBuilder.
     */
    static std::unique_ptr<ToggleControllerAttributeBuilder> create();

    /// @name ToggleControllerAttributeBuilderInterface Functions
    /// @{
    ToggleControllerAttributeBuilder& withCapabilityResources(
        const avsCommon::avs::CapabilityResources& capabilityResources) override;
    ToggleControllerAttributeBuilder& withSemantics(
        const avsCommon::avs::capabilitySemantics::CapabilitySemantics& semantics) override;
    avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes> build() override;
    /// @}

private:
    /**
     * Constructor.
     */
    ToggleControllerAttributeBuilder();

    /// Flag used to indicate whether any unrecoverable error was found.
    bool m_invalidAttribute;

    /// The capability resources represented using @c CapabilityResources.
    avsCommon::avs::CapabilityResources m_capabilityResources;

    /// The semantics represented as an @c Optional @c CapabilitySemantics
    avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> m_semantics;
};

}  // namespace toggleController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_TOGGLECONTROLLER_INCLUDE_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTEBUILDER_H_
