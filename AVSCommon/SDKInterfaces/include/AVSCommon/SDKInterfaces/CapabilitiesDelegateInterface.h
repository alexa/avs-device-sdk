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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CAPABILITIESDELEGATEINTERFACE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * CapabilitiesDelegateInterface is an interface with methods that provide clients a way to register capabilities
 * implemented by agents and publish them so that Alexa is aware of the device's capabilities.
 */
class CapabilitiesDelegateInterface {
public:
    enum class CapabilitiesPublishReturnCode {
        /// The Capabilities API message went through without issues
        SUCCESS,
        /// The message did not go through because of issues that need fixing
        FATAL_ERROR,
        /// The message did not go through, but you can retry to see if you succeed
        RETRIABLE_ERROR
    };

    /**
     * Destructor
     */
    virtual ~CapabilitiesDelegateInterface() = default;

    /**
     * Registers device capabilities that a component is implementing.
     * This only updates a local registry and does not actually send out a message to the Capabilities API endpoint.
     *
     * @param capability The capability configuration which houses the capabilities being implemented.
     * @return true if registering was successful, else false.
     */
    virtual bool registerCapability(
        const std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationInterface>& capability) = 0;

    /**
     * Publishes device capabilities that were registered.
     * This function actually sends out a message to the Capabilities API endpoint.
     *
     * @return A return code (CapabilitiesPublishReturnCode enum value) detailing the outcome of the publish message.
     */
    virtual CapabilitiesPublishReturnCode publishCapabilities() = 0;

    /**
     * Publishes capabilities asynchronously and will keep on retrying till it succeeds or there is a fatal error.
     */
    virtual void publishCapabilitiesAsyncWithRetries() = 0;

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
