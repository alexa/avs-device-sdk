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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITYCONFIGURATIONINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITYCONFIGURATIONINTERFACE_H_

#include <memory>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityConfiguration.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface provides the CapabilitiesDelegate access to the version and configurations of the capabilities
 * being implemented by a capability agent.
 */
class CapabilityConfigurationInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CapabilityConfigurationInterface() = default;

    /**
     * Returns the configurations of the capability interfaces being implemented.
     *
     * @return A set of CapabilityConfigurations
     */
    virtual std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>>
    getCapabilityConfigurations() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITYCONFIGURATIONINTERFACE_H_
