/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_

#include <memory>
#include <vector>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * CapabilitiesDelegateInterface is an interface with methods that provide clients a way to register endpoints and their
 * capabilities and publish them so that Alexa is aware of the device's capabilities.
 */
class CapabilitiesDelegateInterface
        : public avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface
        , public avsCommon::sdkInterfaces::AVSGatewayObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~CapabilitiesDelegateInterface() = default;

    /**
     * Registers endpoint capabilities
     *
     * @param endpointAttributes The @c EndpointAttributes for the registering endpoint.
     * @param capabilities The array of @c CapabilityConfiguration the endpoint supports.
     * @return true if registering was successful, else false.
     */
    virtual bool registerEndpoint(
        const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
        const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) = 0;

    /**
     * Specify an object to observe changes to the state of this CapabilitiesDelegate.
     * During the call to this setter the observers onCapabilitiesStateChange() method will be called
     * back with the current capabilities state.
     *
     * @param observer The object to observe the state of this CapabilitiesDelegate.
     */
    virtual void addCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) = 0;

    /**
     * Remove an observer
     *
     * @param observer The observer to remove.
     */
    virtual void removeCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface> observer) = 0;

    /**
     * Invalidates the capabilities reported to the AVS last. Capabilities information should be rebuilt and reported
     * to the AVS during the next synchronization.
     */
    virtual void invalidateCapabilities() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_
