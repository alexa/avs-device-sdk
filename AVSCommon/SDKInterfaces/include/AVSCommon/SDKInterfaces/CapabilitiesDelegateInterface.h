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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_

#include <memory>
#include <vector>

#include <AVSCommon/AVS/AVSDiscoveryEndpointAttributes.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
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
        , public avsCommon::sdkInterfaces::AVSGatewayObserverInterface
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~CapabilitiesDelegateInterface() = default;

    /**
     * Updates an existing endpoint's capabilities or, if the endpoint does not already exist, registers a new endpoint.
     *
     * @param endpointAttributes The @c EndpointAttributes for the registering endpoint.
     * @param capabilities The array of @c CapabilityConfiguration the endpoint supports.
     * @return true if operation was successful, else false.
     *
     * @note This operation can fail at several stages before publishing the endpoint to AVS:
     * if the capabilities are empty; if the attributes or capability configurations are invalid;
     * if the endpoint is already pending deletion or registration. If this function returns true,
     * the endpoint will be published to AVS. Callers can be notified of published endpoints using
     * @c CapabilitiesDelegateObserverInterface.
     */
    virtual bool addOrUpdateEndpoint(
        const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
        const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) = 0;

    /**
     * Deletes an existing endpoint.
     *
     * @param endpointAttributes The @c EndpointAttributes for the deregistering endpoint.
     * @param capabilities The array of @c CapabilityConfiguration the endpoint supports.
     * @return true if operation was successful, else false.
     *
     * @note This operation can fail at several stages before publishing the endpoint to AVS:
     * if the endpoint is not registered; if the capabilities are empty; if the attributes or capability
     * configurations are invalid; if the endpoint is already pending deletion or registration. If this
     * function returns true, the endpoint will be deregistered from AVS. Callers can be notified of
     * deregistered endpoints using @c CapabilitiesDelegateObserverInterface.
     */
    virtual bool deleteEndpoint(
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
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateObserverInterface> observer) = 0;

    /**
     * Remove an observer.
     *
     * @param observer The observer to remove.
     */
    virtual void removeCapabilitiesObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateObserverInterface> observer) = 0;

    /**
     * Invalidates the capabilities reported to the AVS last. Capabilities information should be rebuilt and reported
     * to the AVS during the next synchronization.
     */
    virtual void invalidateCapabilities() = 0;

    /**
     * Set the message sender to use for sending Discovery events to AVS when connected.
     *
     * @param messageSender The message sender.
     */
    virtual void setMessageSender(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_
