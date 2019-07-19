/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCAPABILITYPROVIDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCAPABILITYPROVIDER_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class handles providing configuration for the System Capability agent, since a single class
 * does not handle all of the capability agent's functionings.
 */
class SystemCapabilityProvider : public avsCommon::sdkInterfaces::CapabilityConfigurationInterface {
public:
    /**
     * Create an instance of @c SystemCapabilityProvider.
     */
    static std::shared_ptr<SystemCapabilityProvider> create();

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor.
     */
    SystemCapabilityProvider();

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCAPABILITYPROVIDER_H_
